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
 
 //This app receives interests /controller0/controllerId/registerNamedData/routerId/prefixo/seq
 //Is updates the controller content locations database
 //This database is a map of the type nodeid - prefix

#include "crosndn-registercontent-producer.h"
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

NS_LOG_COMPONENT_DEFINE ("ndn.CRoSNDNRegisterContentProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CRoSNDNRegisterContentProducer);
   
TypeId
CRoSNDNRegisterContentProducer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::CRoSNDNRegisterContentProducer")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<CRoSNDNRegisterContentProducer> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&CRoSNDNRegisterContentProducer::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&CRoSNDNRegisterContentProducer::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&CRoSNDNRegisterContentProducer::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&CRoSNDNRegisterContentProducer::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CRoSNDNRegisterContentProducer::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&CRoSNDNRegisterContentProducer::m_keyLocator),
                   MakeNameChecker ())
    ;
  return tid;
}
  
  
CRoSNDNRegisterContentProducer::CRoSNDNRegisterContentProducer ()
{
  // NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
CRoSNDNRegisterContentProducer::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  m_fw = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ());
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
  m_prefix.append ("controller0");
  m_prefix.appendSeqNum (GetNode ()->GetId ());
  m_prefix.append ("registerNamedData");
  m_fw->AddFibEntry (m_prefix, m_face, 0);  
}

void
CRoSNDNRegisterContentProducer::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}


void
CRoSNDNRegisterContentProducer::OnInterest (Ptr<const Interest> interest)
{
  //Interest /controller0/controllerId/registerNamedData/routerId/newlocation/prefixo(/seq)
  //or
  //Interest /controller0/controllerId/registerNamedData/routerId/newlocation/set/prefixo(/seq)
  App::OnInterest (interest); // tracing inside

  NS_LOG_FUNCTION (this << interest);

  if (!m_active) return;

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

  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") respodning with Data: " << data->GetName ());

  uint64_t nodex = dataName->get(3).toNumber();
  uint32_t size = dataName->size();
  uint32_t newlocation = dataName->get(4).toNumber();
  Name nameaux;
  if (dataName->get(5).toUri() == "set")
  {
	uint64_t nprefix = dataName->get(size-1).toNumber();
	uint64_t start = dataName->get(size-2).toNumber();
	
	for (uint64_t i = start; i < nprefix; i++)
	{
		Name nameaux = dataName->getSubName (6, size-8);
		nameaux.appendSeqNum(i);
		m_fw->RegisterContent (nameaux, nodex, newlocation);
		NS_LOG_INFO("Testing reg name: " <<  nameaux);
	}
  }else
  {
    nameaux = dataName->getSubName (5, size-6);
	m_fw->RegisterContent (nameaux, nodex, newlocation);
	
  }
  NS_LOG_INFO ("Registering name: " << nameaux << " on node: " << nodex);

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
