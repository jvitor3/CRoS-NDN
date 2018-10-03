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
 
#include "tunnel-in.h"
#include "crosndn-tunnel-strategy.h"
#include <ns3/ndnSIM/model/ndn-interest.h>
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndn-fib.h" 


NS_LOG_COMPONENT_DEFINE ("ndn.TunnelIn");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (TunnelIn);
   
TypeId
TunnelIn::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::TunnelIn")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<TunnelIn> ()
    ;
  return tid;
}
  
  
TunnelIn::TunnelIn ()
{
  NS_LOG_FUNCTION_NOARGS ();
}


// inherited from Application base class.
void
TunnelIn::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  m_fw = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNTunnelStrategy> ());
  m_fw->SetTunnelFace(m_face);
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
}

void
TunnelIn::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}


void
TunnelIn::OnInterest (Ptr<const Interest> interest)
{
  App::OnInterest (interest); // tracing inside

  NS_LOG_FUNCTION (this << interest->GetName().toUri());

  if (!m_active) return;

  Name intName = Name(interest->GetName ());  
  uint32_t tundest = m_fw->GetTunnelDestination(intName.getPrefix(2,0)); 
  if (tundest > 10000) 
  {	  
	  NS_LOG_DEBUG("No route for interest: " << interest->GetName().toUri() );
	  return;
  }
  Ptr<Interest> interestout = Create<Interest> ();
  interestout->SetNonce(interest->GetNonce());
  Ptr<Name> intNameout = Create<Name> ("/tunnel");
  intNameout->appendSeqNum (tundest);
  intNameout->append(intName.getSubName());
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
TunnelIn::OnData (Ptr<const Data> contentObject)
{
  App::OnData (contentObject); // tracing inside
  Name cname = contentObject->GetName();
    
  Ptr<Data> data = Create<Data> ();
  Ptr<Name> dataName = Create<Name> (cname.getSubName(2,cname.size()-2));
  data->SetName (dataName);
  data->SetFreshness (contentObject->GetFreshness());
  data->SetTimestamp (contentObject->GetTimestamp());
  data->SetSignature (contentObject->GetSignature());
/*   if (contentObject->GetKeyLocator()->size () > 0)
    {
	  Ptr<Name> keyl = Create<Name> (contentObject->GetKeyLocator()->getSubName());
      data->SetKeyLocator (keyl);		
    } */
  data->SetPayload (contentObject->GetPayload()->Copy());

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
 