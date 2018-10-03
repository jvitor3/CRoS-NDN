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

#include "ndn-producer-gethashes-nlsrlike.h"
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
#include "best-route-nlsr-like.h"
#include "ndn-payload-header.h"
#include "ns3/ndn-fib.h"
#include <ns3/ndnSIM/model/ndn-l3-protocol.h>
#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.ProducerGetHashesNLSRLike");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ProducerGetHashesNLSRLike);
   
TypeId
ProducerGetHashesNLSRLike::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ProducerGetHashesNLSRLike")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<ProducerGetHashesNLSRLike> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerGetHashesNLSRLike::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerGetHashesNLSRLike::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&ProducerGetHashesNLSRLike::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0.15)),
                   MakeTimeAccessor (&ProducerGetHashesNLSRLike::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ProducerGetHashesNLSRLike::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&ProducerGetHashesNLSRLike::m_keyLocator),
                   MakeNameChecker ())
    ;
  return tid;
}
  
  
ProducerGetHashesNLSRLike::ProducerGetHashesNLSRLike ()
{
  // NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
ProducerGetHashesNLSRLike::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
  m_prefix.append ("router");
  m_prefix.appendSeqNum (GetNode ()->GetId ());
  m_prefix.append ("gethashes");
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())
    ->AddFibEntry (m_prefix, m_face, 0);
  RefreshFib();
}

void
ProducerGetHashesNLSRLike::RefreshFib ()
{
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())
    ->AddFibEntry (m_prefix, m_face, 0);
  Simulator::Schedule (Seconds (0.1), &ProducerGetHashesNLSRLike::RefreshFib, this);
}

void
ProducerGetHashesNLSRLike::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}


void
ProducerGetHashesNLSRLike::OnInterest (Ptr<const Interest> interest)
{
  //Interest /router/neighbourid/gethashes/topoproducer/roothash
  
  //Interest /router/routerid/gethashes/topoproducer/roothash/npkts/seq
  // npkts is a flag for additional packets. hash set exceeds packet MTU
  App::OnInterest (interest); // tracing inside

  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") received interest: " << interest->GetName ());

  if (!m_active) return;

  Ptr<Data> data = Create<Data> ();
  Ptr<Name> dataName = Create<Name> (interest->GetName ());
  dataName->append (m_postfix);
  data->SetName (dataName);
  data->SetFreshness (m_freshness);

  data->SetSignature (m_signature);
  if (m_keyLocator.size () > 0)
    {
      data->SetKeyLocator (Create<Name> (m_keyLocator));
    }

  uint32_t roothash = dataName->get(4).toNumber();
  Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
  Ptr<Name> payl = Create<Name> ();
  std::set<uint32_t> hashset;
  if (dataName->get(3).toNumber() == 1)
    hashset = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->GetHashSetTopo (roothash);
  if (dataName->get(3).toNumber() == 2) 
    hashset = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->GetHashSetProducer (roothash);
  Ptr<Packet> packt = Create<Packet> ();
   

  //230 * 6 = 1380
  //100 * 12 = 1200
  uint32_t mtu = 1500;
  uint32_t bytes = 0;
  uint32_t dnbytes = 0;
  bytes = hashset.size() * 6 * 2 + 2; 
  for (Name::iterator it = dataName->begin(); it != dataName->end(); it++)
    dnbytes = dnbytes + it->size() + 2;
  uint32_t npkts = bytes / (mtu - dnbytes);
  uint32_t nhashesperpkt = hashset.size() / npkts;
  
  if (npkts > 1)
  {
    NS_LOG_DEBUG ("Get Hashes MTU overflow! \t Packets: " << npkts);
    payl = Create<Name> ();
    std::set<uint32_t>::iterator it = hashset.begin();
    uint32_t count = 0;
    uint32_t type =  dataName->get(3).toNumber();
    uint32_t hashsize = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())
        ->GetLSASize (*it, type);
    if (dataName->size() == 5)
    {
      dataName->appendSeqNum (npkts);
      dataName->appendSeqNum (0);
      while (count < nhashesperpkt && it != hashset.end())
      {
        count++;
        if (hashsize > 0)
        {
          payl->appendSeqNum (hashsize);
          payl->appendNumber (*it); 
        }
        it++;        
      }    
    }
    else
    {
      while (count/nhashesperpkt <= dataName->get(6).toNumber()
        && it != hashset.end())
      {
        if (count/nhashesperpkt == dataName->get(6).toNumber())
          if (hashsize > 0)
          {
            payl->appendSeqNum (hashsize);
            payl->appendNumber (*it); 
          }
        it++;          
        count++;
      }
    }
    
    payheader->SetPayload (payl);
    packt->AddHeader (*payheader);
    data->SetPayload (packt);
  }
  else
  {
    uint32_t type =  dataName->get(3).toNumber();
    for (std::set<uint32_t>::iterator it = hashset.begin(); it != hashset.end(); it++)
    {
      uint32_t hashsize = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())
        ->GetLSASize (*it, type);
      if (hashsize > 0)
      {
        payl->appendSeqNum (hashsize);
        payl->appendNumber (*it); 
      }
    }
    payheader->SetPayload (payl);
    packt->AddHeader (*payheader);
    data->SetPayload (packt);
  }
  NS_LOG_DEBUG ("node(" << GetNode()->GetId() << ") - Name: " << dataName->toUri());
  NS_LOG_DEBUG ("node(" << GetNode()->GetId() << ") - Payload: " << payl->toUri());

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
