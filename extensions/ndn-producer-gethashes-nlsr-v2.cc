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

#include "ndn-producer-gethashes-nlsr-v2.h"
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
#include "best-route-nlsr-v2.h"
#include "ndn-payload-header.h"
#include "ns3/ndn-fib.h"
#include <ns3/ndnSIM/model/ndn-l3-protocol.h>
#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.ProducerGetHashesNLSRV2");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ProducerGetHashesNLSRV2);
   
TypeId
ProducerGetHashesNLSRV2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ProducerGetHashesNLSRV2")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<ProducerGetHashesNLSRV2> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerGetHashesNLSRV2::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerGetHashesNLSRV2::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&ProducerGetHashesNLSRV2::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0.15)),
                   MakeTimeAccessor (&ProducerGetHashesNLSRV2::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ProducerGetHashesNLSRV2::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&ProducerGetHashesNLSRV2::m_keyLocator),
                   MakeNameChecker ())
    ;
  return tid;
}
  
  
ProducerGetHashesNLSRV2::ProducerGetHashesNLSRV2 ()
{
  // NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
ProducerGetHashesNLSRV2::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
  m_prefix.append ("router");
  m_prefix.appendSeqNum (GetNode ()->GetId ());
  m_prefix.append ("gethashes");
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRV2> ())
    ->AddFibEntry (m_prefix, m_face, 0);
  RefreshFib();
}

void
ProducerGetHashesNLSRV2::RefreshFib ()
{
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRV2> ())
    ->AddFibEntry (m_prefix, m_face, 0);
  Simulator::Schedule (Seconds (0.1), &ProducerGetHashesNLSRV2::RefreshFib, this);
}

void
ProducerGetHashesNLSRV2::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}


void
ProducerGetHashesNLSRV2::OnInterest (Ptr<const Interest> interest)
{
  //Interest /router/neighbourid/gethashes/prefixsize/prefix/seq
  
  //Interest /router/routerid/gethashes/prefixsize/prefix/seq/npkts/seq2
  // npkts is a flag for additional packets. hash set exceeds packet MTU
  App::OnInterest (interest); // tracing inside

  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") received interest: " << interest->GetName ());

  if (!m_active) return;

  Ptr<Data> data = Create<Data> ();
  Ptr<Name> dataName = Create<Name> (interest->GetName ());
  uint32_t dataNamesize = dataName->size();
  dataName->append (m_postfix);
  data->SetName (dataName);
  data->SetFreshness (m_freshness);

  data->SetSignature (m_signature);
  if (m_keyLocator.size () > 0)
  {
    data->SetKeyLocator (Create<Name> (m_keyLocator));
  }

  Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
  Ptr<Name> payl = Create<Name> ();
  
  Name prefix;
  if (dataName->get(3).toNumber() == 0)
    prefix = Name("/");
  else 
  {
    prefix = dataName->getPrefix (dataName->get(3).toNumber(), 4);
  }  
  Name contenthashes = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRV2> ())->GetBranchComponents (prefix);
  uint32_t bytes = 0;
  for (Name::iterator it = contenthashes.begin(); it != contenthashes.end(); it++)
    bytes = bytes + it->size() + 2;
  uint32_t mtu = 1500;
  uint32_t dnbytes = 0;
  for (Name::iterator it = dataName->begin(); it != dataName->end(); it++)
    dnbytes = dnbytes + it->size() + 2;
  uint32_t npkts = bytes / (mtu - dnbytes);  
  uint32_t nbytesperpkt = dnbytes / npkts;
  if (npkts > 1)
  {
    NS_LOG_DEBUG ("Get Hashes MTU overflow! \t Packets: " << npkts);
    payl = Create<Name> ();
    uint32_t countbytes = 0;
    if (dataNamesize == dataName->get(3).toNumber() + 5)
    {
      dataName->appendSeqNum (npkts);
      dataName->appendSeqNum (0);
      for (Name::iterator it = contenthashes.begin(); it != contenthashes.end(); it++)
      {
        countbytes = countbytes + it->size() + 2;
        if (countbytes <= nbytesperpkt)
        {
          payl->append(*it);
        }
        else
          break;
      }
    }
    else
    {
      for (Name::iterator it = contenthashes.begin(); it != contenthashes.end(); it++)
      {
        countbytes = countbytes + it->size() + 2;
        uint32_t pktseq = dataName->get(dataNamesize-1).toNumber();
        if ((countbytes/nbytesperpkt == pktseq ) && ( countbytes <= nbytesperpkt))
        {          
          payl->append(*it);
        }
        if (countbytes > nbytesperpkt)
          break;
      }
    }    
  }  
  else
  {
    payl->append (contenthashes);
  }
    
  Ptr<Packet> packt = Create<Packet> ();  
  payheader->SetPayload (payl);
  packt->AddHeader (*payheader);
  data->SetPayload (packt);
  
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
