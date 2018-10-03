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

//This app receives all interest without a better fib match than "/".
//Then the app locally evaluates the route and install a new fib entry, if available.

#include "ndn-nlsrlike-askroute.h"
#include "ns3/log.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ndn-payload-header.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/ndn-app-face.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndnSIM/utils/ndn-rtt-mean-deviation.h"
#include "ns3/random-variable.h"
#include "best-route-nlsr-like.h"
#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.NLSRLikeAskRoute");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (NLSRLikeAskRoute);
    
TypeId
NLSRLikeAskRoute::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::NLSRLikeAskRoute")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<NLSRLikeAskRoute> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&NLSRLikeAskRoute::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor(&NLSRLikeAskRoute::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t>())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&NLSRLikeAskRoute::m_freshness),
                   MakeTimeChecker ())
    ;
  return tid;
}
    
NLSRLikeAskRoute::NLSRLikeAskRoute ()
{
  NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
NLSRLikeAskRoute::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId () << " prefix: " << m_prefix);
  RefreshFib();
}

void
NLSRLikeAskRoute::RefreshFib ()
{
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())
    ->AddFibEntry (m_prefix, m_face, 10);  
  Simulator::Schedule (Seconds (0.3), &NLSRLikeAskRoute::RefreshFib, this);
}

void
NLSRLikeAskRoute::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}


bool
NLSRLikeAskRoute::AskRoute (Ptr<const Interest> interest)
{
  Name namex = interest->GetName ();
  NS_LOG_DEBUG ("Node asking route for name: " << namex);
  uint32_t sz = namex.size (); 
  uint32_t node1 = GetNode()->GetId();
  Ptr<Name> path = Create<Name> ();
  const Name answer = namex.getPrefix (sz-1);
  std::set<uint32_t> candidates = (GetNode()->
    GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->GetRegisteredContentNodes (answer);
  for (std::set<uint32_t>::iterator it1 = candidates.begin();
    it1 != candidates.end(); it1++)
  {
    NS_LOG_DEBUG ("Trying candidate number: " << *(it1));
        //For now we try only the shortest path candidate
    Ptr<Name> temp = (GetNode()->
      GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->GetRoutes (node1,*(it1));
    if (path->size () == 0)
      path = temp;
    else if (temp->size () > 0 && temp->size () < path->size ())
      path = temp;
  }    
        
  if (path->size() > 0)
  {
    Ptr<Name> wantedprefix = Create<Name> (answer);
    Name nexthop = Name ("/router");
    nexthop.append (path->get(1));
    Ptr<Fib>  fib  = GetNode()->GetObject<Fib> ();
    Ptr<fib::Entry> fibentry = fib->Find(nexthop);
    if (fibentry != 0)
    {
      NS_LOG_DEBUG ("Next hop does exist!");
      ndn::fib::FaceMetric facemetric = fibentry->FindBestCandidate(0);
      Ptr<Face> facex = facemetric.GetFace();
      (GetNode()->
      GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->AddFibEntry (wantedprefix, facex, 0);
      return true;
    }
  }
  else
    NS_LOG_DEBUG ("No route for name: " << answer);
  return false;      
}

void
NLSRLikeAskRoute::OnInterest (Ptr<const Interest> interest)
{
  App::OnInterest (interest);
  FwHopCountTag hopCountTag;
  UniformVariable rand (0,std::numeric_limits<uint32_t>::max ());
  
  bool shouldAnswerNack  = AskRoute (interest);
  
  if (shouldAnswerNack)
  {
    //NACK - moving fib entry "/" to a low preference in the pit entry for this specific name
    Ptr<ndn::Interest> nack = Create<ndn::Interest> ();
    nack->SetNack (Interest::NACK_GIVEUP_PIT);
    nack->SetNonce (rand.GetValue ());
    nack->SetName (interest->GetName ());
    nack->SetInterestLifetime (Seconds (0.5));
    // Create packet and add ndn::Interest
    if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
      nack->GetPayload ()->AddPacketTag (hopCountTag);
    // Forward packet to lower (network) layer
    m_face->ReceiveInterest (nack);
    // Call trace (for logging purposes)
    m_transmittedInterests (nack, this, m_face);
    Ptr<ndn::Interest> newint = Create<ndn::Interest> ();
    newint->SetNonce (rand.GetValue ());
    newint->SetName (interest->GetName ());
    newint->SetInterestLifetime (interest->GetInterestLifetime ());
    // Create packet and add ndn::Interest
    if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
      newint->GetPayload ()->AddPacketTag (hopCountTag);
    m_face->ReceiveInterest (newint);
    // Call trace (for logging purposes)
    m_transmittedInterests (newint, this, m_face);
    }
  else
    NS_LOG_DEBUG ("Path not found for name: " << interest->GetName().toUri());
}

void
NLSRLikeAskRoute::OnData (Ptr<const ndn::Data> contentObject)
{
  App::OnData (contentObject); // tracing inside 
  NS_LOG_DEBUG ("Receiving ContentObject packet for " << contentObject->GetName ());
  const Name contentname = contentObject->GetName ();
  uint32_t sz = contentname.size();
  

}

} // namespace ndn
} // namespace ns3
