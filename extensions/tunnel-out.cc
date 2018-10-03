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
 
#include "tunnel-out.h"
#include "crosndn-tunnel-strategy.h"
#include <ns3/ndnSIM/model/ndn-interest.h>
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndn-fib.h"
 
NS_LOG_COMPONENT_DEFINE ("ndn.TunnelOut");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (TunnelOut);
   
TypeId
TunnelOut::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::TunnelOut")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<TunnelOut> ()
    ;
  return tid;
}
  
  
TunnelOut::TunnelOut ()
{
  NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
TunnelOut::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  Name prefix = Name("/tunnel");
  prefix.appendSeqNum (GetNode ()->GetId ());
  m_fw = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNTunnelStrategy> ());
  m_fw->AddFibEntry (prefix, m_face, 0);    
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
}

void
TunnelOut::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}


void
TunnelOut::OnInterest (Ptr<const Interest> interest)
{
  App::OnInterest (interest); // tracing inside

  NS_LOG_FUNCTION (this << interest->GetName().toUri());

  if (!m_active) return;

  Ptr<Name> intName = Create<Name> (interest->GetName ()); 
  if (intName->get(0).toUri() != "tunnel" or intName->get(1).toNumber () != GetNode()->GetId())
  {	  
	  NS_LOG_DEBUG("Should not receive this interest: " << interest->GetName().toUri() );
	  return;
  }
  Ptr<Interest> interestout = Create<Interest> ();
  interestout->SetNonce(interest->GetNonce());
  Ptr<Name> intNameout = Create<Name>(intName->getSubName(2,intName->size()-2));
  interestout->SetName(intNameout);
  interestout->SetInterestLifetime(interest->GetInterestLifetime ());

  NS_LOG_INFO ("Received Interest: \t" << *interest);
  NS_LOG_INFO ("Sending Interest for " << intNameout->toUri());

  FwHopCountTag hopCountTag;
  if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
  {
    interestout->GetPayload ()->AddPacketTag (hopCountTag);
  }

  m_transmittedInterests (interestout, this, m_face);
  m_face->ReceiveInterest (interestout);
}


void
TunnelOut::OnData (Ptr<const Data> contentObject)
{
  App::OnData (contentObject); // tracing inside
  Name cname = contentObject->GetName();
    
  Ptr<Data> data = Create<Data> ();
  Ptr<Name> dataName = Create<Name> ("/tunnel");
  dataName->appendSeqNum (GetNode ()->GetId ());
  dataName->append(cname);
  data->SetName (dataName);
  data->SetFreshness (contentObject->GetFreshness());
  data->SetTimestamp (contentObject->GetTimestamp());
  data->SetSignature (contentObject->GetSignature());
/*   if (contentObject->GetKeyLocator()->size () > 0)
    {
	  Ptr<Name> keyl = Create<Name> (contentObject->GetKeyLocator()->getSubName());
      data->SetKeyLocator (keyl);
    } */
  Ptr<Packet> pckt = contentObject->GetPayload()->Copy();
  data->SetPayload (pckt);

  NS_LOG_DEBUG ("node("<< GetNode()->GetId() <<") received Data: " << contentObject->GetName ());
  NS_LOG_DEBUG ("node("<< GetNode()->GetId() <<") sending Data: " << data->GetName ());

  // Echo back FwHopCountTag if exists
  FwHopCountTag hopCountTag;
  if (contentObject->GetPayload ()->PeekPacketTag (hopCountTag))
    {
      data->GetPayload ()->AddPacketTag (hopCountTag);
    }
  m_face->ReceiveData (data);
  m_transmittedDatas (data, this, m_face);
}
  
} // namespace ndn
} // namespace ns3
 