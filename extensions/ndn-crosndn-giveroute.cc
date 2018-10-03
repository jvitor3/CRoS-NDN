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

//This app receives interests /controllerX/controllerId/routeFrom/sourceRouterID/wantedprefix/seq
//This app replies with content payload using a name format /node1/node2/.../noden
//Where, sourceRouterID=node1 and noden=producer
//This is a copy of ndn-sdndn-giveroute for future use, routing and caching coordination.

#include "ndn-crosndn-giveroute.h"
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
#include "ns3/ndn-fib.h"
#include <ns3/ndnSIM/model/ndn-l3-protocol.h>
#include "best-route-sdndn-controller.h"
#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.CROSNDNGiveRoute");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CROSNDNGiveRoute);
   
TypeId
CROSNDNGiveRoute::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::CROSNDNGiveRoute")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<CROSNDNGiveRoute> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/controllerX"),
                   MakeNameAccessor (&CROSNDNGiveRoute::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&CROSNDNGiveRoute::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&CROSNDNGiveRoute::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0.01)),
                   MakeTimeAccessor (&CROSNDNGiveRoute::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CROSNDNGiveRoute::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&CROSNDNGiveRoute::m_keyLocator),
                   MakeNameChecker ())   
    ;
  return tid;
}
  
  
CROSNDNGiveRoute::CROSNDNGiveRoute ()
{
  NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
CROSNDNGiveRoute::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
  m_prefix.appendSeqNum (GetNode ()->GetId ());
  m_prefix.append ("routeFrom");
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
    ->AddFibEntry (m_prefix, m_face, 0);
}

void
CROSNDNGiveRoute::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}


void
CROSNDNGiveRoute::OnInterest (Ptr<const Interest> interest)
{
  App::OnInterest (interest); // tracing inside
  NS_LOG_FUNCTION (this << interest);
  if (!m_active) return;
  //prefix: /controllerX/controllerId/routeFrom/sourceRouterID/wantedprefix/seq
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
  NS_LOG_DEBUG ("Received Controller Interest seq:" << dataName->toUri ());
  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") respodning with Data: " << data->GetName ());
  
  uint32_t node1 = dataName->get(3).toNumber();
  uint32_t size = dataName->size();
  Ptr<Name> path = Create<Name> ();
//  const Name answer = dataName->getSubName (4, size-5);  
  const Name answer = dataName->getSubName (4, size-6);
  NS_LOG_DEBUG("Looking for name: " << answer);
  std::set<uint32_t> candidates =  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
    ->GetRegisteredContentNodes (answer);
  for (std::set<uint32_t>::iterator it1 = candidates.begin();
    it1 != candidates.end(); it1++)
  {
    NS_LOG_INFO ("Candidate list item is: " << *it1);
    if ((GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
      ->GetRouterNeighborOk (*it1))
    {
      NS_LOG_INFO ("Trying candidate number: " << *(it1));
      //For now we try only the shortest path candidate
      Ptr<Name> temp = (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
        ->GetRoutes (node1,*(it1));
      if (path->size () == 0)
        path = temp;
      else if (temp->size () > 0 && temp->size () < path->size ())
        path = temp;
    }    
  }

   
  if (m_askedNames.find (answer) != m_askedNames.end())
  {
     std::set<uint32_t> cachecandidates = m_askedNames[answer];
     for (std::set<uint32_t>::iterator it1 = cachecandidates.begin();
        it1 != cachecandidates.end(); it1++)
     {
        NS_LOG_INFO ("Cache Candidate list item is: " << *it1);
        if ( (node1 != *it1) && (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
          ->GetRouterNeighborOk (*it1))
        {
          NS_LOG_INFO ("Trying candidate number: " << *(it1));
          //For now we try only the shortest path candidate
          Ptr<Name> temp = (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
            ->GetRoutes (node1,*(it1));
          if (path->size () == 0)
            path = temp;
          else if (temp->size () > 0 && temp->size () < path->size ())
            path = temp;
        }    
     }
  }
  else
  {
    std::set<uint32_t> newset;
    newset.insert (node1);
    m_askedNames[answer] = newset;
  }
  
  // Echo back FwHopCountTag if exists
  FwHopCountTag hopCountTag;
  NS_LOG_DEBUG ("Path: " << path->toUri ());
  Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
  payheader->SetPayload (path);
  Ptr<Packet> packet = Create<Packet> (m_virtualPayloadSize);
  packet->AddHeader (*payheader);
  data->SetPayload(packet);
  if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
  {
    data->GetPayload ()->AddPacketTag (hopCountTag);
  }
  m_face->ReceiveData (data);
  m_transmittedDatas (data, this, m_face);

}
} // namespace ndn
} // namespace ns3
