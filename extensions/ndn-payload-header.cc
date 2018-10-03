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
 
//This defines a new header.
//The header contains a name prefix. 
//The name is used in different scenarios to carry information.
//This was a shortcut to avoid coding serialize and deserialize methods for a more general payload.
 

#include "ndn-payload-header.h"
#include "ns3/log.h"
#include "ns3/unused.h"
#include "ns3/packet.h"
#include <ns3/ndnSIM/model/ndn-name.h>

NS_LOG_COMPONENT_DEFINE ("ndn.PayloadHeader");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (PayloadHeader);

TypeId
PayloadHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::PayloadHeader")
    .SetGroupName ("Ndn")
    .SetParent<Header> ()
    .AddConstructor<PayloadHeader> ()
    ;
  return tid;
}
  

PayloadHeader::PayloadHeader ()
  : m_payload (Create<Name> ())
{
}

PayloadHeader::PayloadHeader (const PayloadHeader &payload)
  : m_payload                (Create<Name> (payload.GetPayload ()))
{
}

Ptr<PayloadHeader>
PayloadHeader::GetPayloadHeader (Ptr<Packet> packet)
{
  Ptr<PayloadHeader> payload = Create<PayloadHeader> ();
  packet->RemoveHeader (*payload);
  return payload;
}


void
PayloadHeader::SetPayload (Ptr<Name> payload)
{
  m_payload = payload;
}

const Name&
PayloadHeader::GetPayload () const
{
  if (m_payload==0) throw PayloadHeaderException();
  return *m_payload;
}

Ptr<const Name>
PayloadHeader::GetPayloadPtr () const
{
  return m_payload;
}

uint32_t
PayloadHeader::GetSerializedSize (void) const
{
  size_t nameSerializedSize = 2;
  for (Name::const_iterator i = m_payload->begin ();
       i != m_payload->end ();
       i++)
    {
      nameSerializedSize += 2 + i->size ();
    }
  NS_LOG_INFO ("Serialize size = " << nameSerializedSize);
  return nameSerializedSize;
}
    
void
PayloadHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;
  i.WriteU16 (static_cast<uint16_t> (this->GetSerializedSize ()-2));
  for (Name::const_iterator it = m_payload->begin();
    it != m_payload->end(); it++)
  {
      i.WriteU16 (static_cast<uint16_t> (it->size ()));
      i.Write (reinterpret_cast<const uint8_t*> (it->buf ()), it->size ());
    }
}
 


uint32_t
PayloadHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint16_t nameLength = i.ReadU16 ();
  NS_LOG_INFO ("teste 1 " << nameLength);
  while (nameLength > 0)
    {
      uint16_t length = i.ReadU16 ();
      NS_LOG_INFO ("teste 2 " << nameLength);
      NS_LOG_INFO ("teste 3 " << length);
      nameLength = nameLength - 2 - length;

      uint8_t tmp[length];
      i.Read (tmp, length);

      m_payload->append (tmp, length);
      NS_LOG_INFO ("teste 4 " << nameLength);
    }
  NS_LOG_INFO ("teste 5 " << nameLength);
  return i.GetDistanceFrom (start);
}

TypeId
PayloadHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
  
void
PayloadHeader::Print (std::ostream &os) const
{
  os << GetPayload ();
}


} // namespace ndn
} // namespace ns3

