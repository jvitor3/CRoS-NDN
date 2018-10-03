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

#include "nlsr-register-content-producer.h"
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
#include "ns3/ndn-fib.h"
#include <ns3/ndnSIM/model/ndn-l3-protocol.h>
#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.NLSRRegisterContentProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (NLSRRegisterContentProducer);
   
TypeId
NLSRRegisterContentProducer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::NLSRRegisterContentProducer")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<NLSRRegisterContentProducer> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&NLSRRegisterContentProducer::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&NLSRRegisterContentProducer::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&NLSRRegisterContentProducer::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&NLSRRegisterContentProducer::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&NLSRRegisterContentProducer::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&NLSRRegisterContentProducer::m_keyLocator),
                   MakeNameChecker ())
    ;
  return tid;
}
  
  
NLSRRegisterContentProducer::NLSRRegisterContentProducer ()
{
  // NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
NLSRRegisterContentProducer::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  m_fw = (GetNode()->GetObject<ns3::ndn::fw::NLSRStrategy> ());
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
  m_prefix.append ("registerNamedData");
  m_fw->AddFibEntry (m_prefix, m_face, 0);
  RefreshFib();
}

void
NLSRRegisterContentProducer::RefreshFib ()
{
  Ptr<Fib> fib = GetNode ()->GetObject<Fib> ();
  m_fw->AddFibEntry (m_prefix, m_face, 0);
  Simulator::Schedule (Seconds (0.3), &NLSRRegisterContentProducer::RefreshFib, this);
}


void
NLSRRegisterContentProducer::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}


void
NLSRRegisterContentProducer::OnInterest (Ptr<const Interest> interest)
{
  //Interest /registerNamedData/routerId/newlocation/prefixo(/seq)
  App::OnInterest (interest); // tracing inside

  NS_LOG_FUNCTION (this << interest);

  if (!m_active) return;

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

  uint64_t nodex = dataName->get(1).toNumber();
  uint32_t size = dataName->size();
  const Name nameaux = dataName->getSubName (3, size-4);
  
  //name         /routerid/LSA.type2/LSA.id/version
  //content      /isvalid/nameprefix
  Name name = Name();
  name.appendSeqNum (nodex);
  name.appendSeqNum (2);
  uint32_t nh = m_fw->NameHash (nameaux);
  name.appendSeqNum (nh);
  name.append (dataName->get(size-1));
  Name content = Name ();
  if (dataName->get(2).toNumber() == 0)
    content.appendSeqNum (1);  //isvalid
  else if (dataName->get(2).toNumber() == 1)
    content.appendSeqNum (2);  //isvalid reset producer location to only one node
  content.append (nameaux.begin(), nameaux.end());
  m_fw->AddLSA (name, content);
  NS_LOG_DEBUG ("Registering name: " << nameaux << " on node: " << nodex);

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
