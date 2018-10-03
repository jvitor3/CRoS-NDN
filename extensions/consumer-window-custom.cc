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


#include "consumer-window-custom.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

NS_LOG_COMPONENT_DEFINE ("ndn.ConsumerWindowCustom");

namespace ns3 {
namespace ndn {
    
NS_OBJECT_ENSURE_REGISTERED (ConsumerWindowCustom);
    
TypeId
ConsumerWindowCustom::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ConsumerWindowCustom")
    .SetGroupName ("Ndn")
    .SetParent<ConsumerWindow> ()
    .AddConstructor<ConsumerWindowCustom> ()        
	
    ;
	
  return tid;
}

ConsumerWindowCustom::ConsumerWindowCustom ()
	: ConsumerWindow()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
ConsumerWindowCustom::StartApplication ()
{
  Consumer::StartApplication ();
  NS_LOG_DEBUG ("Consumer face: " << m_face->GetId());
}

void ConsumerWindowCustom::OnData (Ptr<const Data> data)
{
  ConsumerWindow::OnData (data); // tracing inside
  NS_LOG_DEBUG("Data received: " << data->GetName().toUri());	
}
  
void ConsumerWindowCustom::OnTimeout (uint32_t sequenceNumber)
{
  ConsumerWindow::OnTimeout (sequenceNumber);
  NS_LOG_DEBUG("Seq timeout! " << sequenceNumber);	
}
    
uint32_t
ConsumerWindowCustom::GetFaceId ()
{
  return m_face->GetId ();
}

} // namespace ndn
} // namespace ns3
