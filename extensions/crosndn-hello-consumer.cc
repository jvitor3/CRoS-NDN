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

//This app sends interests /hello/consumernodeid/controllerid/seq

#include "ns3/log.h"
#include "crosndn-hello-consumer.h"
#include <ns3/ndnSIM/model/fib/ndn-fib.h>
#include <ns3/ndnSIM/model/ndn-l3-protocol.h>
#include <ns3/ndnSIM/model/ndn-data.h>
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndn-interest.h"

NS_LOG_COMPONENT_DEFINE ("ndn.CRoSNDNHelloConsumer");

namespace ns3 {
namespace ndn {
    
NS_OBJECT_ENSURE_REGISTERED (CRoSNDNHelloConsumer);
 
   
TypeId
CRoSNDNHelloConsumer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::CRoSNDNHelloConsumer")
    .SetGroupName ("Ndn")
    .SetParent<ConsumerBase> ()
    .AddConstructor<CRoSNDNHelloConsumer> ()
    ;

  return tid;
}

// Application Methods
void
CRoSNDNHelloConsumer::StartApplication () // Called at time specified by Start
{
// Interest : /hello/consumernodeid/controllerid/seq
  NS_LOG_FUNCTION_NOARGS ();
  m_interestName = Name ("/hello");
  m_interestName.appendSeqNum (GetNode()->GetId());
  m_interestLifeTime = Seconds (1.0 / m_frequency);  
  NS_LOG_INFO("Name: " << m_interestName);
  m_fw = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ());
  ConsumerBase::StartApplication ();
}

void
CRoSNDNHelloConsumer::ScheduleNextPacket ()
{
  NS_LOG_INFO ("Schedule Interest");
  //Ptr<Name> interestName = Create<Name> ("/controller"); 
  m_interestName = Name ("/hello");
  m_interestName.appendSeqNum (GetNode()->GetId());   
  Ptr<L3Protocol>     ndn = GetNode()->GetObject<L3Protocol> ();
  Ptr<Face> lface;
  for (u_int32_t faces = 0; faces < GetNode()->GetNDevices (); faces++)
  {
    //Refresh fib entries to check neighbors connectivity and to flood controller search interests
	lface = ndn->GetFaceByNetDevice(GetNode()->GetDevice (faces));
	if (lface->GetId() != m_face->GetId())
	{
		m_fw->AddFibEntry (m_interestName.getPrefix(2), lface, 0);      
		//m_fw->AddFibEntry (interestName, lface, 0);
	}
  }
  Ptr<Name> ctlname =   m_fw->GetController ();
  //m_interestName = Name ("/hello");
  //m_interestName.appendSeqNum (GetNode()->GetId());
  if (ctlname->size() > 1)
  {	  
    m_interestName.append (ctlname->getSubName(1,1));
  }
  else
	m_interestName.append ("-1");
  ConsumerBase::ScheduleNextPacket();
}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
CRoSNDNHelloConsumer::OnData (Ptr<const Data> contentObject)
{
  NS_LOG_DEBUG("Received Hello content " << contentObject->GetName() );
  NS_LOG_DEBUG("Node: " << GetNode()->GetId());
  ConsumerBase::OnData (contentObject); // tracing inside  
  ScheduleNextPacket ();
}

void
CRoSNDNHelloConsumer::OnTimeout (uint32_t sequenceNumber)
{
  NS_LOG_DEBUG("Seq timeout! " << sequenceNumber);
  ConsumerBase::OnTimeout (sequenceNumber);
}

//teste
void
CRoSNDNHelloConsumer::OnInterest (Ptr<const Interest> interest)
{

  //App::OnInterest (interest); // tracing inside
  NS_LOG_FUNCTION ("Test: " << interest->GetName ());
} 
//teste 
 
} // namespace ndn
} // namespace ns3
