/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 GTA, UFRJ, RJ - Brasil
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

#include "crosndn-tunnel-strategy.h"
#include <ns3/ndnSIM/model/pit/ndn-pit-entry.h>
#include "ns3/log.h"
#include <ns3/ndnSIM/model/ndn-interest.h>
#include <ns3/ndnSIM/model/pit/ndn-pit.h>
#include "ns3/random-variable.h"

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (CRoSNDNTunnelStrategy);

LogComponent CRoSNDNTunnelStrategy::g_log = LogComponent (CRoSNDNTunnelStrategy::GetLogName ().c_str ());

std::string
CRoSNDNTunnelStrategy::GetLogName ()
{
  return super::GetLogName ()+".CRoSNDNTunnelStrategy";
}

TypeId
CRoSNDNTunnelStrategy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::CRoSNDNTunnelStrategy")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <ns3::ndn::fw::CRoSNDNTunnelStrategy> ()
    ;
  return tid;
}
    
CRoSNDNTunnelStrategy::CRoSNDNTunnelStrategy ()
{
  m_tunnelface = NULL;
}

CRoSNDNTunnelStrategy::~CRoSNDNTunnelStrategy ()
{
}

/*******************************************
//Tunnel specific methods
********************************************/

void
CRoSNDNTunnelStrategy::SetTunnelFace(Ptr<Face> face)
{
	m_tunnelface = face;
}

Ptr<Face>
CRoSNDNTunnelStrategy::GetTunnelFace()
{
	return m_tunnelface;
}
  
uint32_t
CRoSNDNTunnelStrategy::GetTunnelDestination(Name prefix)
{
	if (m_prefix_node_tunnel_map.find(prefix) != m_prefix_node_tunnel_map.end())
		return m_prefix_node_tunnel_map[prefix];
	else
		return 10001;
}
  
void
CRoSNDNTunnelStrategy::SetTunnelDestination(Name prefix, uint32_t nodeid)
{
	m_prefix_node_tunnel_map[prefix] = nodeid;
}

void
CRoSNDNTunnelStrategy::RouteInstall(Ptr<Name> prefix, Ptr<Face> face, uint32_t metric)
{
	Ptr<Name> newprefix = Create<Name> (prefix->getPrefix(2,0));
	//NS_LOG_UNCOND("RouteInstall TESTE AQUI");
	//AddFibEntry  (newprefix, face, metric);
	super::RouteInstall(newprefix, face, metric);
	if (GetTunnelDestination(prefix->getPrefix(prefix->size()-4,2)) < 10000)
	{
	  Ptr<Interest> interestctt = Create<Interest> ();
	  UniformVariable rand (0,std::numeric_limits<uint32_t>::max ());
	  interestctt->SetName (prefix->getPrefix(prefix->size()-1));
	  interestctt->SetNonce (rand.GetValue ());
	  //interestctt->SetNonce (interest->GetNonce ());
	  interestctt->SetInterestLifetime (Seconds (1.0));
	  //FwHopCountTag hopCountTag;
	  //if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
		//interestctt->GetPayload ()->AddPacketTag (hopCountTag);	  
	  Ptr<pit::Entry> pitEntryTunnelIn = m_pit->Lookup (*interestctt);
	  if (pitEntryTunnelIn == 0)
		pitEntryTunnelIn = m_pit->Create (interestctt);
	  pitEntryTunnelIn->AddIncoming (GetTunnelFace());
	  //pitEntryTunnelIn->AddOutgoing (facex);	
	  if (!pitEntryTunnelIn->IsNonceSeen (interestctt->GetNonce ()))
	  {
		pitEntryTunnelIn->AddSeenNonce (interestctt->GetNonce ());
		pitEntryTunnelIn->UpdateLifetime (Seconds (1.0));
	  }
	}
}
  
// from super                               
void
CRoSNDNTunnelStrategy::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
	//NS_LOG_UNCOND("WillEraseTimedOutPendingInterest TESTE AQUI");
	Name pname = pitEntry->GetFibEntry()->GetPrefix();
	if (m_prefix_node_tunnel_map.find(pname) != m_prefix_node_tunnel_map.end())
		m_prefix_node_tunnel_map.erase(pname);
	super::WillEraseTimedOutPendingInterest (pitEntry);
}

} // namespace fw
} // namespace ndn
} // namespace ns3
