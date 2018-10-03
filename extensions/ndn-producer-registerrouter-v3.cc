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

//This app receives interest /controllerX/ControllerID/registerRouter/nodeID/flag/neighbour1/.../neighbourN/seqnumber
//It updates the controller topology view.

#include "ndn-producer-registerrouter-v3.h"
#include "ns3/log.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/ndn-app-face.h"
#include "ns3/ndn-fib.h"
#include "ns3/buffer.h"
#include "ndn-payload-header.h"
#include "best-route-sdndn-controller.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndnSIM/model/ndn-l3-protocol.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndnSIM/helper/ndn-app-helper.h"
#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.ProducerRegisterRouterV3");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ProducerRegisterRouterV3);
   
TypeId
ProducerRegisterRouterV3::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ProducerRegisterRouterV3")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<ProducerRegisterRouterV3> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerRegisterRouterV3::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerRegisterRouterV3::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&ProducerRegisterRouterV3::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&ProducerRegisterRouterV3::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ProducerRegisterRouterV3::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&ProducerRegisterRouterV3::m_keyLocator),
                   MakeNameChecker ())    
    ;
  return tid;
}
  
  
ProducerRegisterRouterV3::ProducerRegisterRouterV3 ()
:   m_isRunning (false)
{
  NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
ProducerRegisterRouterV3::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  uint32_t node1 = GetNode ()->GetId ();
  NS_LOG_DEBUG ("NodeID: " << node1);
  m_prefix.append ("controllerX");
  m_prefix.appendSeqNum (node1);
  m_prefix.append ("registerRouter");
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
    ->AddFibEntry (m_prefix, m_face, 0);
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->SetRouterNeighborOk (node1, false);
  m_seq = 0; 
  m_interestLifeTime = Seconds(0.3);
  m_isRunning = true;
}

void
ProducerRegisterRouterV3::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
  m_isRunning = false;
}

void
ProducerRegisterRouterV3::OnInterest (Ptr<const Interest> interest)
{
  NS_LOG_FUNCTION (this << interest);
  if (!m_active) return;
  //Add router to the list of registered routers
  Ptr<Name> nameWithSequence = Create<Name> (interest->GetName ());
  uint32_t ns = nameWithSequence->size (); 
  NS_LOG_INFO (nameWithSequence->toUri());
  //Interest /controllerX/ControllerID/registerRouter/nodeID/flag/neighbour1/.../neighbourN/seqnumber
  if (ns > 4)
  {
    uint32_t consid = nameWithSequence->get(3).toNumber();
    if (nameWithSequence->get(4).toNumber() == 0)
    {      
      std::set<uint32_t> routers = (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
        ->GetRegisteredRouters ();
      NS_LOG_DEBUG ("Number of registered routers: " << routers.size ());
       
      if (routers.find (consid) == routers.end())
      {
        NS_LOG_DEBUG ("Adding router: " << consid);
        (GetNode ()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
        ->AddRegisteredRouter (consid);
      }     
      uint64_t node2;
      (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->RemoveEdges (consid);
      Name neighbours = nameWithSequence->getSubName (5, ns-6);
      for (Name::const_iterator it = neighbours.begin(); it != neighbours.end(); it++)
      {
        node2 = boost::lexical_cast<uint64_t>(it->toNumber());
        if (consid != node2)
        {
          (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->AddGraphEdges (consid, node2, 1);
        }
      }
      (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->SetRouterNeighborOk (consid, true);    
    }else
    {
      NS_LOG_DEBUG("Keep alive message");
    }
  }

  App::OnInterest (interest); // tracing inside

  Ptr<Data> data = Create<Data> (Create<Packet> (m_virtualPayloadSize));
  Ptr<Name> dataName = Create<Name> (interest->GetName ());
  dataName->append (m_postfix);
  data->SetName (dataName);
  data->SetFreshness (m_freshness);

  data->SetSignature (m_signature);
  if (m_keyLocator.size () > 0)
  {
    data->SetKeyLocator (Create<Name> (m_keyLocator));
  }
  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") respodning with Data: " << data->GetName ());

  // Echo back FwHopCountTag if exists
  FwHopCountTag hopCountTag;
  if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
  {
    data->GetPayload ()->AddPacketTag (hopCountTag);
  }

  m_face->ReceiveData (data);
  m_transmittedDatas (data, this, m_face);

}

} // namespace ndn
} // namespace ns3
