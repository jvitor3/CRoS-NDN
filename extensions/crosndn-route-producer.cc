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

//This app receives interests /controller0/controllerId/routeFrom/sourceRouterID/wantedprefix/seq
//This app replies with content payload using a name format /node1/node2/.../noden
//Where, sourceRouterID=node1 and noden=producer

#include "crosndn-route-producer.h"
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
#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.CRoSNDNRouteProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CRoSNDNRouteProducer);
   
TypeId
CRoSNDNRouteProducer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::CRoSNDNRouteProducer")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<CRoSNDNRouteProducer> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/controller0"),
                   MakeNameAccessor (&CRoSNDNRouteProducer::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&CRoSNDNRouteProducer::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&CRoSNDNRouteProducer::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0.01)),
                   MakeTimeAccessor (&CRoSNDNRouteProducer::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CRoSNDNRouteProducer::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&CRoSNDNRouteProducer::m_keyLocator),
                   MakeNameChecker ())   
    .AddAttribute ("InternalNS", "0 indicates name service internal to controlller. 1 indicates external",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CRoSNDNRouteProducer::m_nameservice),
                   MakeUintegerChecker<uint32_t> ())			   
    ;
  return tid;
}
  
  
CRoSNDNRouteProducer::CRoSNDNRouteProducer ()
{
  NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
CRoSNDNRouteProducer::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  m_fw = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ());
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
  m_prefix.appendSeqNum (GetNode ()->GetId ());
  m_prefix.append ("routeFrom");
  NS_LOG_DEBUG ("Prefix: " << m_prefix.toUri());
  m_fw->AddFibEntry (m_prefix, m_face, 0);
}

void
CRoSNDNRouteProducer::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}

void
CRoSNDNRouteProducer::OnInterest (Ptr<const Interest> interest)
{
  //prefix: /controller0/controllerId/routeFrom/sourceRouterID/wantedcontent/seq	
  App::OnInterest (interest); // tracing inside
  NS_LOG_FUNCTION (this << interest);
  if (!m_active) return;
  Ptr<Name> dataName = Create<Name> (interest->GetName ());
  
  uint32_t size = dataName->size();
  const Name answer = dataName->getSubName (4, size-6);
  NS_LOG_DEBUG("Looking for name: " << answer);
  std::set<uint32_t> candidates;
  
  if (m_nameservice == 0)
  {
	candidates =  m_fw->GetRegisteredContentNodes (answer);
	GiveRoute (interest, candidates);
  }
  else 
  {
	if (answer.getSubName(0,1).toUri() == "/NamedData")
	  candidates =  m_fw->GetRegisteredContentNodes (answer.getSubName(0,1));  
    else
	   candidates =  m_fw->GetRegisteredContentNodes (answer);  
 
	if (candidates.size() > 0)
		GiveRoute (interest, candidates);
	else if (answer.getSubName(0,1).toUri() != "/NamedData")
	  {
		// /NamedData/locate/prefixo/seq
		Ptr<Name> requestlocal = Create<Name> ("/NamedData/locate");
		requestlocal->append (answer);
		requestlocal->append (dataName->get (size-1));
		m_interestmap[requestlocal->toUri()] = interest;
		SendInterest (requestlocal);
		//Simulator::Schedule (Seconds (0.001), &CRoSNDNRouteProducer::SendInterest, this, requestlocal);
	  }
  }
}
	
void
CRoSNDNRouteProducer::GiveRoute (Ptr<const Interest> interest, std::set<uint32_t> candidates)
{	
  Ptr<Data> data = Create<Data> (Create<Packet> (m_virtualPayloadSize));
  Ptr<Name> dataName = Create<Name> (interest->GetName ());
  dataName->append (m_postfix);
  data->SetName (dataName);
  data->SetFreshness (m_freshness);
  data->SetTimestamp (Simulator::Now());
  data->SetSignature (m_signature);
  if (m_keyLocator.size () > 0)
    {
      data->SetKeyLocator (Create<Name> (m_keyLocator));
    }
  NS_LOG_DEBUG ("Responding Interest seq:" << dataName->toUri ());
  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") respodning with Data: " << data->GetName ());
  
  uint32_t node1 = dataName->get(3).toNumber();
  Ptr<Name> path = Create<Name> ();
  for (std::set<uint32_t>::iterator it1 = candidates.begin();
    it1 != candidates.end(); it1++)
  {
    NS_LOG_INFO ("Candidate list item is: " << *it1);
    if (m_fw->GetRouterNeighborOk (*it1))
    {
      NS_LOG_INFO ("Trying candidate number: " << *(it1));
      //For now we try only the shortest path candidate
      Ptr<Name> temp = m_fw->GetRoutes2 (0, node1,*(it1));
      if (path->size () == 0)
        path = temp;
      else if (temp->size () > 0 && temp->size () < path->size ())
        path = temp;
    }    
  }
  
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

void
CRoSNDNRouteProducer::SendInterest (Ptr<Name> prefixname)
{
  if (!m_active) return;

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
CRoSNDNRouteProducer::OnData (Ptr<const ndn::Data> contentObject)
{
  App::OnData (contentObject); // tracing inside 
  Ptr<const Name> cname = contentObject->GetNamePtr ();
  Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
  Ptr<const Packet> payload = contentObject->GetPayload ();
  (payload->Copy())->PeekHeader (*payheader);	
  const Name nameaux = payheader->GetPayload ();
  NS_LOG_INFO ("controller("<< GetNode()->GetId() <<") received data name: " << cname->toUri());
  NS_LOG_INFO ("controller("<< GetNode()->GetId() <<") received data content: " << nameaux);
  std::set<uint32_t> candidates;
  for (Name::const_iterator it = nameaux.begin(); it != nameaux.end(); it++)
  {
	candidates.insert(it->toNumber());  //boost::lexical_cast<uint64_t>
  }
  std::string cnamestr = cname->toUri();
  if (m_interestmap.find(cnamestr) != m_interestmap.end())
  {  
    NS_LOG_INFO ("Pending route request: " << m_interestmap.find(cnamestr)->second);
	GiveRoute (m_interestmap.find(cnamestr)->second, candidates);
	m_interestmap.erase(cnamestr);
  }
  NS_LOG_INFO ("End OnData");
}

} // namespace ndn
} // namespace ns3
