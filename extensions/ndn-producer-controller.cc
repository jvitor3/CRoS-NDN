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

//This app responds interests with the prefix /controller/seq
//The answer contains the nodeid informing the controller id.

#include "ndn-producer-controller.h"
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
#include "best-route-sdndn-controller.h"
#include "ns3/ndn-fib.h"
#include <ns3/ndnSIM/model/ndn-l3-protocol.h>
#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.ProducerController");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ProducerController);
   
TypeId
ProducerController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ProducerController")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<ProducerController> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerController::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerController::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&ProducerController::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0.5)),
                   MakeTimeAccessor (&ProducerController::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ProducerController::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&ProducerController::m_keyLocator),
                   MakeNameChecker ())
    ;
  return tid;
}
  
  
ProducerController::ProducerController ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_seq = 0;
}


// inherited from Application base class.
void
ProducerController::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  m_prefix.append ("controller");
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
    ->AddFibEntry (m_prefix, m_face, 0);    
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
}

void
ProducerController::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}


void
ProducerController::OnInterest (Ptr<const Interest> interest)
{
  App::OnInterest (interest); // tracing inside

  NS_LOG_FUNCTION (this << interest);

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
  
  Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
  Ptr<Name> payl = Create<Name> ();
  payl->appendSeqNum (boost::lexical_cast<uint64_t> (GetNode()->GetId()));  
  
  if (dataName->get(1).toNumber() > m_seq)
    m_seq = dataName->get(1).toNumber();
  payl->appendSeqNum(m_seq);
  
  
  payheader->SetPayload (payl);
  Ptr<Packet> packt = Create<Packet> ();
  packt->AddHeader (*payheader);
  data->SetPayload (packt);

  NS_LOG_DEBUG ("node("<< GetNode()->GetId() <<") respodning with Data: " << data->GetName ());
  NS_LOG_DEBUG ("Payload = "<< payl->get(0).toNumber() << ", " << payl->get(1).toNumber());

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
