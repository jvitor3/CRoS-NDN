/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 GTA, UFRJ, RJ - Brasil
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
 
//This app send interests periodically with the
// prefix /floodingprefix/prefix/seq
// This app is used in conjunction with StackHelper2 and ns3::ndn::fw::FloodingPrefix (OSPFLike).


#include "ndn-consumer-floodingprefix.h"
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
#include <list>
#include <boost/algorithm/string.hpp>

NS_LOG_COMPONENT_DEFINE ("ndn.ConsumerFloodingPrefix");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ConsumerFloodingPrefix);

// register NS-3 type
TypeId
ConsumerFloodingPrefix::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::ConsumerFloodingPrefix")
    .SetGroupName ("Ndn")
    .SetParent<ndn::App> ()
    .AddConstructor<ConsumerFloodingPrefix> ()
    .AddAttribute ("Prefix", "Requested name root",
                   StringValue ("/"),
                   MakeStringAccessor (&ConsumerFloodingPrefix::m_name),
                   MakeStringChecker ())
    .AddAttribute ("NPrefixes", "Number of derived names to register",
                   IntegerValue (1),
                   MakeIntegerAccessor(&ConsumerFloodingPrefix::m_nprefixes),
                   MakeIntegerChecker<uint32_t>())
    .AddAttribute ("Frequency", "Frequency of interest packets",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&ConsumerFloodingPrefix::m_frequency),
                   MakeDoubleChecker<double> ())
    ;
  return tid;
}

ConsumerFloodingPrefix::ConsumerFloodingPrefix ()
  : m_isRunning (false)
  , m_random (0)
{
}

// Processing upon start of the application
void
ConsumerFloodingPrefix::StartApplication ()
{
   
  // initialize ndn::App
  ndn::App::StartApplication ();
  if (m_random)
    delete m_random;
  m_random = new UniformVariable (0.0, 1.0/m_frequency/2);
  m_isRunning = true;
  m_seq = 0;
  m_interestLifeTime = Seconds(1);
  for (uint32_t i = 0; i < m_nprefixes; i++)
    m_listnames[i] = false;
  Simulator::ScheduleNow (&ConsumerFloodingPrefix::BatchInterests, this);
}

// Processing when application is stopped
void
ConsumerFloodingPrefix::StopApplication ()
{

  m_isRunning = false;
  
   // cancel periodic packet generation
  Simulator::Cancel (m_sendEvent);
  
  // cleanup ndn::App
  ndn::App::StopApplication ();
}

void
ConsumerFloodingPrefix::BatchInterests ()
{
  //        /floodingprefix/prefix/seq
 uint32_t count = 0;
 std::list<std::string> nameprefixlist;
 for (uint32_t i = 0; i < m_nprefixes; i++)
 {
   Ptr<Name> nameprefix = Create<ndn::Name> ();
   nameprefix->append ("floodingprefix");
   nameprefix->append (m_name);
   nameprefix->appendSeqNum (i);
   nameprefix->appendSeqNum (m_seq);
   Simulator::Schedule ( Seconds(m_random->GetValue ()),
      &ConsumerFloodingPrefix::SendInterest, this, nameprefix);
   count++;
 }
 m_seq++;
 m_sendEvent = Simulator::Schedule (Seconds (1.0 / m_frequency), 
      &ConsumerFloodingPrefix::BatchInterests, this);
}


void
ConsumerFloodingPrefix::SendInterest (Ptr<Name> prefixname)
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

  NS_LOG_INFO ("> Interest for " << prefixname->toUri());

  FwHopCountTag hopCountTag;
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);
}

void
ConsumerFloodingPrefix::OnData (Ptr<const ndn::Data> contentObject)
{
  App::OnData (contentObject); // tracing inside 
  Name cname = contentObject->GetName ();
  uint32_t size = cname.size();
  NS_LOG_DEBUG ("Receiving ContentObject packet for " << cname.toUri());
  uint32_t name = cname.get(size-2).toNumber();
  if (m_listnames.find(name) != m_listnames.end())
     m_listnames.find(name)->second = true;
}


} // namespace ndn
} // namespace ns3
