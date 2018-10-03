/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 GTA, UFRJ, RJ - Brasil
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: João Vitor Torres <jvitor@gta.ufrj.br>
 */

//It is used to register a prefix
//It send interests with the form /controller0/controllerId/registerNamedData/nodeid/prefix/seq
//Prefix parameter sets the content prefix
//NPrefix sets the number of prefixes with the prefix above.


#include "crosndn-registerservice.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndn-fib.h"
#include <ns3/ndnSIM/model/ndn-l3-protocol.h>
#include "ns3/random-variable.h"
#include "crosndn-strategy.h"
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

NS_LOG_COMPONENT_DEFINE ("ndn.CRoSNDNRegisterService");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CRoSNDNRegisterService);

// register NS-3 type
TypeId
CRoSNDNRegisterService::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::CRoSNDNRegisterService")
    .SetGroupName ("Ndn")
    .SetParent<ndn::App> ()
    .AddConstructor<CRoSNDNRegisterService> ()
    .AddAttribute ("Prefix", "Requested name root",
                   StringValue ("/registeringprefix"),
                   MakeStringAccessor (&CRoSNDNRegisterService::m_name),
                   MakeStringChecker ())

    ;
  return tid;
}

CRoSNDNRegisterService::CRoSNDNRegisterService ()
  : m_isRunning (false)
{
}

// Processing upon start of the application
void
CRoSNDNRegisterService::StartApplication ()
{
  // initialize ndn::App
  ndn::App::StartApplication ();
  m_isRunning = true;
  m_seq = 0;
  m_interestLifeTime = Seconds (0.2);
  Simulator::ScheduleNow (&CRoSNDNRegisterService::BatchInterests, this);
}

// Processing when application is stopped
void
CRoSNDNRegisterService::StopApplication ()
{
  m_isRunning = false;
  // cleanup ndn::App
  ndn::App::StopApplication ();
}

void
CRoSNDNRegisterService::BatchInterests ()
{
  //        /controller0/controllerId/registerNamedData/routerId/prefixo/seq
  Ptr<Name> controllerx = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->GetController ();
  if (controllerx->size() > 0)
  {
    Ptr<Name> nameprefix = Create<ndn::Name> ();
    nameprefix->append (controllerx->begin(), controllerx->end());
    nameprefix->append ("registerNamedData");
    nameprefix->appendSeqNum (GetNode ()->GetId ());
    nameprefix->append (m_name);
    nameprefix->appendSeqNum (m_seq);
    Name namesent = nameprefix->getSubName ();
    m_events[namesent] = Simulator::Schedule ( Seconds(0.20),
      &CRoSNDNRegisterService::OnTimeout, this, nameprefix);    
    SendInterest (nameprefix);     
    m_seq++;
  }
  else
    Simulator::Schedule (Seconds (1.0), &CRoSNDNRegisterService::BatchInterests, this);
}


void
CRoSNDNRegisterService::SendInterest (Ptr<Name> prefixname)
{
  if (!m_isRunning) return;

  /////////////////////////////////////
  // Sending one Interest packet out //
  /////////////////////////////////////
  
  NS_LOG_FUNCTION_NOARGS ();
  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (prefixname);
  interest->SetInterestLifetime    (m_interestLifeTime);

  NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO ("> Interest for " << prefixname->toUri());

  FwHopCountTag hopCountTag;
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);
}

void
CRoSNDNRegisterService::OnData (Ptr<const ndn::Data> contentObject)
{
  App::OnData (contentObject); // tracing inside 

//        /controller0/controllerId/registerNamedData/routerId/prefixo/seq
  Name cname = contentObject->GetName ();
  Simulator::Cancel (m_events[cname]);
  NS_LOG_DEBUG ("Registered prefixes: " << m_seq << cname.toUri());
}

void
CRoSNDNRegisterService::OnTimeout(Ptr<Name> interest)
{
  NS_LOG_LOGIC ("Register timeout: " << interest->toUri());
  m_events[interest->getSubName()] = Simulator::Schedule ( Seconds(0.20),
    &CRoSNDNRegisterService::OnTimeout, this, interest);
  SendInterest (interest);
}

} // namespace ndn
} // namespace ns3
