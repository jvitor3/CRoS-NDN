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
 
 //This app receives interests 
 //		/NamedData/register/routerId/prefixo/seq
 //		/NamedData/locate/prefixo/seq
 //Is updates the content locations database
 //This database is a map of the type nodeid - prefix

#include "crosndn-datalocation.h"
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
#include "crosndn-strategy.h"
#include "ndn-payload-header.h"
#include "ns3/ndn-fib.h"
#include <ns3/ndnSIM/model/ndn-l3-protocol.h>
#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.CRoSNDNDataLocation");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CRoSNDNDataLocation);
   
TypeId
CRoSNDNDataLocation::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::CRoSNDNDataLocation")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<CRoSNDNDataLocation> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&CRoSNDNDataLocation::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&CRoSNDNDataLocation::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&CRoSNDNDataLocation::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&CRoSNDNDataLocation::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CRoSNDNDataLocation::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&CRoSNDNDataLocation::m_keyLocator),
                   MakeNameChecker ())
    ;
  return tid;
}
  
  
CRoSNDNDataLocation::CRoSNDNDataLocation ()
{
  // NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
CRoSNDNDataLocation::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
//  m_prefix.append ("controller0");
//  m_prefix.appendSeqNum (GetNode ()->GetId ());
  m_prefix.append ("NamedData");
  (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())
    ->AddFibEntry (m_prefix, m_face, 0);  
}

void
CRoSNDNDataLocation::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}


void
CRoSNDNDataLocation::OnInterest (Ptr<const Interest> interest)
{
  //Interest 
  //		/NamedData/register/routerId/prefixo/seq
  //		/NamedData/locate/prefixo/seq
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

  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") responding with Data: " << data->GetName ());

  if (dataName->get(1).toUri() == "register")
  {
    uint64_t nodex = dataName->get(2).toNumber();
    uint32_t size = dataName->size();
    const Name nameaux = dataName->getSubName (3, size-4);
    (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->RegisterContent (nameaux, nodex, 1);
    NS_LOG_INFO ("Registering name: " << nameaux << " on node: " << nodex);
  }	  
  else if (dataName->get(1).toUri() == "locate")
  {
    uint32_t size = dataName->size();
    const Name nameaux = dataName->getSubName (2, size-3);
	std::set<uint32_t> candidates =  (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())
    ->GetRegisteredContentNodes (nameaux);
	
    Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
    Ptr<Name> payl = Create<Name> ();
	for (std::set<uint32_t>::iterator it = candidates.begin(); it != candidates.end(); it++)
	{
	  payl->appendSeqNum (boost::lexical_cast<uint32_t> (*it));  
	}
    payheader->SetPayload (payl);
    Ptr<Packet> packt = Create<Packet> ();
    packt->AddHeader (*payheader);
    data->SetPayload (packt);
	
    NS_LOG_INFO ("Locating name: " << nameaux << " on node: " << payl->toUri());
  }	

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
