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
 
//This app receives all interest without a better fib match than "/".
//Then it asks the controller for a new route with the
//prefix: /controller0/controllerId/routeFrom/sourceRouterID/wantedprefix/seq
// 
//When the controller answer arrives, the app adds a new fib entry and it sends a new interest with the
//	prefix: /route/installRoute/sourcerouterId/nextRouterId/.../destinationRouterId/endRoute
//			/routePreference/x/toPrefix/wantedprefix/seq
//		/route/installRouteAndForward/sourcerouterId/nextRouterId/.../destinationRouterId
//			/endRoute/routePreference/x/toPrefix/wantedprefix/seq
//When an interest with the prefix above arrives, 
//the app also adds a new fib entry and it creates a new interest removing its nodeid from de sequence.

#include "crosndn-route-consumer.h"
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
#include "crosndn-strategy.h"
#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.CRoSNDNRouteConsumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CRoSNDNRouteConsumer);
    
TypeId
CRoSNDNRouteConsumer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::CRoSNDNRouteConsumer")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<CRoSNDNRouteConsumer> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&CRoSNDNRouteConsumer::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor(&CRoSNDNRouteConsumer::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t>())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&CRoSNDNRouteConsumer::m_freshness),
                   MakeTimeChecker ())
    ;
  return tid;
}
    
CRoSNDNRouteConsumer::CRoSNDNRouteConsumer ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rtt = CreateObject<RttMeanDeviation> ();
  m_seqi = 0;
}


// inherited from Application base class.
void
CRoSNDNRouteConsumer::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId () << " prefix: " << m_prefix);
  RefreshFib();
}


void
CRoSNDNRouteConsumer::RefreshFib ()
{
  (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())
    ->AddFibEntry (m_prefix, m_face, 10);
  Simulator::Schedule (Seconds (0.3), &CRoSNDNRouteConsumer::RefreshFib, this);
}

void
CRoSNDNRouteConsumer::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}

void
CRoSNDNRouteConsumer::OnTimeout (Name &pref, uint32_t sequenceNumber, bool isAskRoute)
{
  if (m_pendingroutes.find(pref) != m_pendingroutes.end())
  {
    NS_LOG_DEBUG (GetNode()->GetId() << " Timeout " << pref.toUri());
    m_pendingroutes.erase(pref);
    if (isAskRoute)
    {
      NS_LOG_DEBUG (GetNode()->GetId() << " Timeout " << sequenceNumber);
      m_rtt->Reset();
      m_rtt->SetCurrentEstimate (Seconds(1.0));
      //(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->SetRegisteredRouter (false);
    }
  }
}

bool
CRoSNDNRouteConsumer::AskRoute (Ptr<const Interest> interest)
{
  Name namex = interest->GetName ();
  NS_LOG_DEBUG ("Node asking route for name: " << namex);
  uint32_t sz = namex.size ();
  FwHopCountTag hopCountTag;
  Ptr<Name> contr = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->GetController ();
  if (contr->size () == 0)
  {
    NS_LOG_DEBUG ("Node did not found a controller yet!");
    return true;
  }
  Ptr<Interest> newinterest = Create<Interest> ();  
  UniformVariable rand (0,std::numeric_limits<uint32_t>::max ());
  Ptr<Packet> packet = Create<Packet> ();
  Ptr<Name> nameprefix = Create<ndn::Name> ();
  Ptr<Name> wantedcontent = Create<ndn::Name> ();
  bool ishop = false;
  bool iswantedprefix = false;
  bool isnexthop = true;
  uint32_t hops = 0;
  if (namex.get(0).toUri() == "router" &&
    namex.get(2).toUri() == "installRouteAndForward" &&
    contr->size () > 0)
  {
    NS_LOG_DEBUG("Route install propagation.");// <<
    for(Name::const_iterator it = namex.begin();
      it != namex.end(); it++)
    {
      if (!ishop || (ishop && it->toNumber() != boost::lexical_cast<uint64_t>(GetNode()->GetId())))
      {
        if (it->toUri() !="")
          nameprefix->append (*it);
      }
      if (it->toUri() == "installRouteAndForward" )
      {
        ishop = true;
        isnexthop = true;
        
      }
      if (it->toUri() == "endRoute" )
        ishop = false;
      if (ishop)
      {
        if (it->toUri() != "route" && it->toUri() != "installRouteAndForward" 
          && it->toNumber() != boost::lexical_cast<uint64_t> (GetNode()->GetId()))
        {
          hops++;
          if (isnexthop)
          {
            isnexthop = false;
            (*nameprefix) [1] = *it;
          }
        }
      }
      if (it->toUri() == "toPrefix" )
        iswantedprefix = true;
      if (iswantedprefix && it->toUri() != "toPrefix" )
          wantedcontent->append (*it);
    }

    //Let route installation propagate before ask new route.    
    Name pref = Name (wantedcontent->begin(), wantedcontent->end());
    if (m_pendingroutes.find(pref) == m_pendingroutes.end())
    {
      m_pendingroutes.insert(pref);
      Simulator::Schedule (Seconds(1.0), 
        &CRoSNDNRouteConsumer::OnTimeout, this, pref, namex.get(sz-1).toNumber(), false);
      NS_LOG_DEBUG (GetNode()->GetId() << " INSTALL AND FORWARD: " <<   m_rtt->RetransmitTimeout ());
    }
    
    if (hops > 0)
    {
      newinterest->SetName (nameprefix);
      NS_LOG_DEBUG ("Sending Interest packet for prefix instalation " << *nameprefix);
    }else
    {
      newinterest->SetName (wantedcontent); 
      NS_LOG_DEBUG ("Sending Interest packet for content " << *wantedcontent);
    }
    // Create and configure ndn::Interest
    newinterest->SetNonce (rand.GetValue ());
    newinterest->SetInterestLifetime (Seconds (0.5));
    if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
      newinterest->GetPayload ()->AddPacketTag (hopCountTag);
    // Forward packet to lower (network) layer
    m_face->ReceiveInterest (newinterest);
    // Call trace (for logging purposes)
    m_transmittedInterests (newinterest, this, m_face);
    
    
	Ptr<Data> data = Create<Data> (Create<Packet> (m_virtualPayloadSize));
	Ptr<Name> dataName = Create<Name> (interest->GetName ());
	data->SetName (dataName);
	data->SetFreshness (Seconds (0));
	// Echo back FwHopCountTag if exists
	FwHopCountTag hopCountTag;
	if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
	{
	  data->GetPayload ()->AddPacketTag (hopCountTag);
	}
	m_face->ReceiveData (data);
	m_transmittedDatas (data, this, m_face);    
	return false;  
    
  }
  else if ((namex.get(0).toUri() != "controller0") &&
    (namex.get(0).toUri() != "router") &&
    (namex.get(0).toUri() != "hello") &&
    (namex.get(0).toUri() != "controller") &&
    (contr->size () > 1))
  {
	//mudança 11abril2016
	//Send Nack back in case of route request in core router.
	if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
	{
		if (hopCountTag.Get() > 1)
			return true;
	}
  
    Name pref = namex.getPrefix(sz-1);
    if (m_pendingroutes.find(pref) == m_pendingroutes.end())
    {
      m_pendingroutes.insert(pref);
      m_sendEvent = Simulator::Schedule ((m_rtt->RetransmitTimeout ()),
        &CRoSNDNRouteConsumer::OnTimeout, this, pref,namex.get(sz-1).toNumber(), true);
      NS_LOG_DEBUG (GetNode()->GetId() << " ASK ROUTE before: " <<   m_rtt->RetransmitTimeout ());
    }
    else
    {
      NS_LOG_DEBUG("Supressing to brief repeated route ask.");
      return false;
    }
    
    nameprefix = Create<ndn::Name> (contr->begin(), contr->end());
    nameprefix->append ("routeFrom");
    nameprefix->appendSeqNum (GetNode ()->GetId ());
//	if (namex.get(0).toUri() == "/NamedData")
//		nameprefix->append (namex.get(0));
//	else
		nameprefix->append (namex.begin(), namex.end());
    nameprefix->appendSeqNum (m_seqi);
    m_seqi++;
    newinterest->SetNonce (rand.GetValue ());
    newinterest->SetName (nameprefix);
    newinterest->SetInterestLifetime (Seconds (0.5));
    if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
      newinterest->GetPayload ()->AddPacketTag (hopCountTag);
    NS_LOG_DEBUG ("Sending Interest packet for " << *nameprefix);
    // Forward packet to lower (network) layer
    m_face->ReceiveInterest (newinterest);    
    // Call trace (for logging purposes)
    m_transmittedInterests (newinterest, this, m_face);
    
    m_rtt->SentSeq (SequenceNumber32 (namex.get(sz-1).toNumber()), 1);
	//mudança 11abril2016
	//return true;
    return false;
	//
    NS_LOG_DEBUG (GetNode()->GetId() << " ASK ROUTE after: " <<   m_rtt->RetransmitTimeout ());
  }else{
    NS_LOG_DEBUG ("Route request not followed for name  " << interest->GetName ());
    if (contr->size () == 0)
      NS_LOG_DEBUG ("Node did not found a controller yet!");
    return true;
  }
}

void
CRoSNDNRouteConsumer::OnInterest (Ptr<const Interest> interest)
{
  //	prefixo: /route/installRoute/sourcerouterId/nextRouterId/.../destinationRouterId/endRoute
  //			/routePreference/x/toPrefix/wantedprefix/seq
  //		/route/installRouteAndForward/sourcerouterId/nextRouterId/.../destinationRouterId
  //			/endRoute/routePreference/x/toPrefix/wantedprefix/seq

  App::OnInterest (interest); // tracing inside
  NS_LOG_FUNCTION (this << interest);
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
  }
}

void
CRoSNDNRouteConsumer::OnData (Ptr<const ndn::Data> contentObject)
{
  App::OnData (contentObject); // tracing inside 
  NS_LOG_DEBUG ("Receiving ContentObject packet for " << contentObject->GetName ());
  const Name contentname = contentObject->GetName ();
  uint32_t sz = contentname.size();
    
  //prefix: /controller0/controllerId/routeFrom/sourceRouterID/wantedprefix/seq
  if (contentname.get(0).toUri() == "controller0" &&
    contentname.size() > 0 && contentname.get(2).toUri() == "routeFrom")
  {
    m_rtt->AckSeq (SequenceNumber32 (contentname.get(sz-1).toNumber()));
    NS_LOG_DEBUG (GetNode()->GetId() << " ASK ROUTE ANSWER: " <<   m_rtt->RetransmitTimeout ());

    Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
    Ptr<const Packet> payload = contentObject->GetPayload();
    (payload->Copy ())->RemoveHeader (*payheader);	
    Ptr<const Name> nameaux = Create<const Name> ();
    nameaux = payheader->GetPayloadPtr ();
    NS_LOG_DEBUG ("Received Controller Content: " << (*nameaux));
    //Send route installation interest
    Ptr<ndn::Interest> newinterest= Create<ndn::Interest> ();
    Ptr<Name> nameroute = Create<Name> ();
    nameroute->append ("router");
    NS_LOG_DEBUG ("Test route size: " << nameaux->size ());
    Name nexthop;
    std::list<std::string> nexthopstr;
    bool first = true;
    Ptr<Fib> fib = GetNode ()->GetObject<Fib> ();
    if (nameaux->size() >= 1 && nameaux->getPostfix(1,0).toUri () != "/")
    {
      for (Name::const_iterator it = nameaux->begin(); it != nameaux->end();
        it++)
      {
        {
          if (it->toUri() != "" && it->toNumber() != boost::lexical_cast<uint64_t>(GetNode ()->GetId ()))
          {
            nameroute->appendSeqNum (it->toNumber());
            //Identify next hop
            if (first)
            {
              nexthop.append("router");
              nexthop.appendSeqNum(it->toNumber());
              nameroute->append ("installRouteAndForward");
              nameroute->appendSeqNum (it->toNumber());
              first = false;
            }
          }
        }
      }    
      nameroute->append("endRoute");
      nameroute->append("routePreference");
      nameroute->appendSeqNum(1);
      nameroute->append("toPrefix");
      nameroute->append (contentname.getPrefix (contentname.size()-5,4));
      
      UniformVariable rand (0,std::numeric_limits<uint32_t>::max ());
      newinterest->SetNonce (rand.GetValue ());
      newinterest->SetName (nameroute);
      newinterest->SetInterestLifetime (Seconds (0.5));
      // Create packet and add ndn::Interest

      NS_LOG_DEBUG ("Sending Interest packet for " << *nameroute);
        // Echo back FwHopCountTag if exists
      FwHopCountTag hopCountTag;
      if (contentObject->GetPayload ()->PeekPacketTag (hopCountTag))
        newinterest->GetPayload ()->AddPacketTag (hopCountTag);
      // Forward packet to lower (network) layer
      m_face->ReceiveInterest (newinterest);
      // Call trace (for logging purposes)
      m_transmittedInterests (newinterest, this, m_face);      
      Name pref = contentname.getPrefix(sz-5,4);
      NS_LOG_DEBUG ("Name " << pref.toUri());
      if (m_pendingroutes.find(pref) == m_pendingroutes.end())
      {
        m_pendingroutes.erase(pref);
      }    
    }else
      NS_LOG_DEBUG ("No route found for name route request: " << contentname.toUri ());
  } else
  {
    NS_LOG_DEBUG ("Received user requested content!");
  }
}

} // namespace ndn
} // namespace ns3
