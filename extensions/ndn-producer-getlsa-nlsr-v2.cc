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

#include "ndn-producer-getlsa-nlsr-v2.h"
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

NS_LOG_COMPONENT_DEFINE ("ndn.ProducerGetLSANLSRV2");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ProducerGetLSANLSRV2);
   
TypeId
ProducerGetLSANLSRV2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ProducerGetLSANLSRV2")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<ProducerGetLSANLSRV2> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerGetLSANLSRV2::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerGetLSANLSRV2::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&ProducerGetLSANLSRV2::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0.05)),
                   MakeTimeAccessor (&ProducerGetLSANLSRV2::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ProducerGetLSANLSRV2::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&ProducerGetLSANLSRV2::m_keyLocator),
                   MakeNameChecker ())
    ;
  return tid;
}
  
  
ProducerGetLSANLSRV2::ProducerGetLSANLSRV2 ()
{
  // NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
ProducerGetLSANLSRV2::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
  m_prefix.append ("router");
  m_prefix.appendSeqNum (GetNode ()->GetId ());
  m_prefix.append ("getlsa");
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRV2> ())
    ->AddFibEntry (m_prefix, m_face, 0);
  RefreshFib();
}

void
ProducerGetLSANLSRV2::RefreshFib ()
{
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRV2> ())
    ->AddFibEntry (m_prefix, m_face, 0);
  Simulator::Schedule (Seconds (0.1), &ProducerGetLSANLSRV2::RefreshFib, this);
}

void
ProducerGetLSANLSRV2::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}


void
ProducerGetLSANLSRV2::OnInterest (Ptr<const Interest> interest)
{
//        /router/neighbourid/getlsa/lsanamesise/lsaname
//        /router/neighbourid/getlsa/lsanamesise/lsaname/npkts/pos/seq
  App::OnInterest (interest); // tracing inside
  if (!m_active) return;

  Ptr<Data> data = Create<Data> ();
  Ptr<Name> dataName = Create<Name> (interest->GetName ());
  uint32_t datanamesize = dataName->size();
  NS_LOG_DEBUG (dataName->toUri());
  dataName->append (m_postfix);
  data->SetName (dataName);
  data->SetFreshness (m_freshness);

  data->SetSignature (m_signature);
  if (m_keyLocator.size () > 0)
    {
      data->SetKeyLocator (Create<Name> (m_keyLocator));
    }

  uint32_t lsansize =  dataName->get(3).toNumber();
  Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
  Ptr<Name> payl = Create<Name> ();
  Name lsaname = dataName->getPrefix (dataName->get (3).toNumber(), 4);
  Name lsacontent = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRV2> ())->GetLSA (lsaname);

  // Mtu to change fragmentation
  uint32_t mtu = 1500;
  uint32_t bytes = 0;
  uint32_t dnbytes = 0;
  for (Name::iterator it = lsacontent.begin(); it != lsacontent.end(); it++)
    bytes = bytes + it->size() + 2;
  for (Name::iterator it = dataName->begin(); it != dataName->end(); it++)
    dnbytes = dnbytes + it->size() + 2;
  uint32_t npkts = bytes / (mtu - dnbytes);
  uint32_t nbytespkt = mtu - dnbytes;  
  
  if (lsacontent.size() == 0)
  {
    NS_LOG_WARN("LSA not found: " << lsaname);
    payl->append ("no");
  }
  else if (npkts > 1)
  {
    NS_LOG_DEBUG ("Get LSA MTU overflow! \t Packets: " << npkts);
    uint32_t count = 2;
    uint32_t pos = 0;
    if (datanamesize == lsansize + 4)
    {
      Name::iterator it = lsacontent.begin();
      count = count + it->size() + 2;
      while (count < nbytespkt && it != lsacontent.end())
      {
        payl->append (*it); 
        it++; 
        pos++;       
      }    
      dataName->appendSeqNum (npkts);
      dataName->appendSeqNum (pos);
      dataName->appendSeqNum (0);
    }
    else
    {
      pos = dataName->get(datanamesize-2).toNumber();
      count = count + lsacontent.get(pos).size() + 2;
      while (count < nbytespkt && pos < lsacontent.size())
      {
        payl->append (lsacontent.get(pos));        
        pos++;
      }
    }  
    payl->appendSeqNum (pos);  
  }
  else
    payl->append (lsacontent.begin(), lsacontent.end()); 
  NS_LOG_DEBUG ("Payload: " << payl->toUri() << ", size: " << payl->size());
  payheader->SetPayload (payl);
  Ptr<Packet> packt = Create<Packet> ();
  packt->AddHeader (*payheader);
  data->SetPayload (packt);

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
