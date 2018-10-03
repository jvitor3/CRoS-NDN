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
//It send interests with the form /registerNamedData/nodeid/newlocation/prefix/seq
//Prefix parameter sets the content prefix
//NPrefix sets the number of prefixes with the prefix above.

#include "nlsr-register-content-consumer.h"
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
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include "ns3/ndnSIM/utils/ndn-rtt-mean-deviation.h"

NS_LOG_COMPONENT_DEFINE ("ndn.NLSRRegisterContentConsumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (NLSRRegisterContentConsumer);

// register NS-3 type
TypeId
NLSRRegisterContentConsumer::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::NLSRRegisterContentConsumer")
    .SetGroupName ("Ndn")
    .SetParent<ndn::App> ()
    .AddConstructor<NLSRRegisterContentConsumer> ()
    .AddAttribute ("Prefix", "Requested name root",
                   StringValue ("/registeringprefix"),
                   MakeStringAccessor (&NLSRRegisterContentConsumer::m_name),
                   MakeStringChecker ())
    .AddAttribute ("NPrefixes", "Number of derived names to register",
                   IntegerValue (1),
                   MakeIntegerAccessor(&NLSRRegisterContentConsumer::m_nprefixes),
                   MakeIntegerChecker<uint32_t>())
    .AddAttribute ("RegRate", "Prefix register rate",
                   IntegerValue (1),
                   MakeIntegerAccessor(&NLSRRegisterContentConsumer::m_regrate),
                   MakeIntegerChecker<uint32_t>())
    .AddAttribute ("NewLocation", "0 indicates parallel producers, 1 indicates one producer changing its location",
                   IntegerValue (0),
                   MakeIntegerAccessor(&NLSRRegisterContentConsumer::m_newlocation),
                   MakeIntegerChecker<uint32_t>())                   
    ;
  return tid;
}

NLSRRegisterContentConsumer::NLSRRegisterContentConsumer ()
  : m_isRunning (false)
{
	m_rtt = CreateObject<RttMeanDeviation> ();
}

// Processing upon start of the application
void
NLSRRegisterContentConsumer::StartApplication ()
{
  // initialize ndn::App
  ndn::App::StartApplication ();
  m_fw = (GetNode()->GetObject<ns3::ndn::fw::NLSRStrategy> ());
  m_isRunning = true;
  m_seq = 0;
  m_count = 0;
  m_interestLifeTime = Seconds (0.2);
  Simulator::ScheduleNow (&NLSRRegisterContentConsumer::BatchInterests, this);
}

// Processing when application is stopped
void
NLSRRegisterContentConsumer::StopApplication ()
{
  m_isRunning = false;
  // cleanup ndn::App
  ndn::App::StopApplication ();
}

void
NLSRRegisterContentConsumer::BatchInterests ()
{

  uint32_t count = 0;
  while (m_nprefixes >= m_seq && count < m_regrate && m_count < m_regrate)
  {
    Ptr<Name> nameprefix = Create<ndn::Name> ();
    nameprefix->append ("registerNamedData");
    nameprefix->appendSeqNum (GetNode ()->GetId ());
	nameprefix->appendSeqNum (m_newlocation);
    nameprefix->append (m_name);
    nameprefix->appendSeqNum (m_seq);
    nameprefix->appendSeqNum (m_seq);
    Name namesent = nameprefix->getSubName ();
    m_events[namesent] = Simulator::Schedule ( m_rtt->RetransmitTimeout (),
      &NLSRRegisterContentConsumer::OnTimeout, this, nameprefix);
    SendInterest (nameprefix);      
    count++;    
    m_count++;
    m_seq++;
  }
  Simulator::Schedule (Seconds (1.0), &NLSRRegisterContentConsumer::BatchInterests, this);
}


void
NLSRRegisterContentConsumer::SendInterest (Ptr<Name> prefixname)
{
  if (!m_isRunning) return;

  /////////////////////////////////////
  // Sending one Interest packet out //
  /////////////////////////////////////
  
  NS_LOG_FUNCTION_NOARGS ();
  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (prefixname);
  interest->SetInterestLifetime    (m_rtt->RetransmitTimeout ());

  NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO ("> Interest for " << prefixname->toUri());

  FwHopCountTag hopCountTag;
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);
  m_rtt->SentSeq (SequenceNumber32 (prefixname->get(prefixname->size()-1).toNumber()),1);
}

void
NLSRRegisterContentConsumer::OnData (Ptr<const ndn::Data> contentObject)
{
  App::OnData (contentObject); // tracing inside 
  m_count--;
  Name cname = contentObject->GetName ();
  m_rtt->AckSeq (SequenceNumber32 (cname.get(cname.size()-1).toNumber()));
  Simulator::Cancel (m_events[cname]);  
  uint32_t size = cname.size();
  NS_LOG_DEBUG ("Receiving ContentObject packet for " << cname.toUri());
}

void
NLSRRegisterContentConsumer::OnTimeout(Ptr<Name> interest)
{
  m_rtt->Reset();
  m_rtt->SetCurrentEstimate (Seconds(1.0));	
  m_events[interest->getSubName()] = Simulator::Schedule ( m_rtt->RetransmitTimeout (),
    &NLSRRegisterContentConsumer::OnTimeout, this, interest);
  SendInterest (interest);
}

} // namespace ndn
} // namespace ns3
