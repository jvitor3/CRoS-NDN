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

//This app sends interests /hello/consumernodeid/topologyChangeflag/seq

#include "ns3/log.h"
#include "ndn-consumer-nodeid.h"
#include "best-route-sdndn-controller.h"
#include <ns3/ndnSIM/model/fib/ndn-fib.h>
#include <ns3/ndnSIM/model/ndn-l3-protocol.h>
#include <ns3/ndnSIM/model/ndn-data.h>
#include "best-route-sdndn-controller.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndn-interest.h"

NS_LOG_COMPONENT_DEFINE ("ndn.ConsumerNodeID");

namespace ns3 {
namespace ndn {
    
NS_OBJECT_ENSURE_REGISTERED (ConsumerNodeID);
 
   
TypeId
ConsumerNodeID::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ConsumerNodeID")
    .SetGroupName ("Ndn")
    .SetParent<ConsumerBase> ()
    .AddConstructor<ConsumerNodeID> ()
    ;

  return tid;
}

// Application Methods
void
ConsumerNodeID::StartApplication () // Called at time specified by Start
{
// Interest : /hello/consumernodeid/topologyChangeflag/seq
  NS_LOG_FUNCTION_NOARGS ();
  m_interestName = Name ("/hello");
  m_interestName.appendSeqNum (GetNode()->GetId());
  m_interestLifeTime = Seconds (1.0 / m_frequency);  
  NS_LOG_INFO("Name: " << m_interestName);
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->SetNewNeighbor (true);
  ConsumerBase::StartApplication ();
}

void
ConsumerNodeID::ScheduleNextPacket ()
{
  NS_LOG_INFO ("Schedule Interest");
  Ptr<Name> interestName = Create<Name> ("/controller"); 
  m_interestName = Name ("/hello");
  m_interestName.appendSeqNum (GetNode()->GetId());   
  Ptr<L3Protocol>     ndn = GetNode()->GetObject<L3Protocol> ();
  for (u_int32_t faces = 0; faces < GetNode()->GetNDevices (); faces++)
  {
    //Refresh fib entries to check neighbors connectivity and to flood controller search interests
    (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
       ->AddFibEntry (m_interestName.getPrefix(2), ndn->GetFaceByNetDevice(GetNode()->GetDevice (faces)), 0);      
    (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
       ->AddFibEntry (interestName, ndn->GetFaceByNetDevice(GetNode()->GetDevice (faces)), 0);
  }
  m_appstate =   (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->GetState (); 
  if ((GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->GetNewNeighbor ())
  {
    m_interestName = Name ("/hello");
    m_interestName.appendSeqNum (GetNode()->GetId());
    m_interestName.appendSeqNum (1);
    NS_LOG_DEBUG ("New neighbor pending.");
  }
  else
  {
    m_interestName = Name ("/hello");
    m_interestName.appendSeqNum (GetNode()->GetId());
    m_interestName.appendSeqNum (0);
  }
  ConsumerBase::ScheduleNextPacket();
}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
ConsumerNodeID::OnData (Ptr<const Data> contentObject)
{
  NS_LOG_DEBUG("Received Hello content " << contentObject->GetName() );
  NS_LOG_DEBUG("Node: " << GetNode()->GetId());
  uint32_t flag =  (contentObject->GetName()).get(2).toNumber();
  uint32_t state =   (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->GetState (); 
  if (flag && state == m_appstate)
    (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->SetNewNeighbor (false);
  ConsumerBase::OnData (contentObject); // tracing inside  
  ScheduleNextPacket ();
}

void
ConsumerNodeID::OnTimeout (uint32_t sequenceNumber)
{
  NS_LOG_DEBUG("Seq timeout! " << sequenceNumber);
  ConsumerBase::OnTimeout (sequenceNumber);
}


} // namespace ndn
} // namespace ns3
