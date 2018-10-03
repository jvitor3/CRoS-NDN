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

#ifndef _NDN_PAYLOAD_HEADER_H_
#define _NDN_PAYLOAD_HEADER_H_

#include "ns3/integer.h"
#include "ns3/header.h"
#include "ns3/simple-ref-count.h"
#include "ns3/nstime.h"
#include <string>
#include <vector>
#include <list>
#include <ns3/ndnSIM/model/ndn-name.h>

namespace ns3 {

class Packet;

namespace ndn {


class PayloadHeader : public SimpleRefCount<PayloadHeader, Header>
{
public:
  /**
   * \brief Constructor
   *
   * Creates a null header
   **/
  PayloadHeader ();

  /**
   * @brief Copy constructor
   */
  PayloadHeader (const PayloadHeader &payload);

  void
  SetPayload (Ptr<Name> payload);

  const Name&
  GetPayload () const;

  Ptr<const Name>
  GetPayloadPtr () const;

  //////////////////////////////////////////////////////////////////

  static TypeId GetTypeId (void); ///< @brief Get TypeId of the class
  virtual TypeId GetInstanceTypeId (void) const; ///< @brief Get TypeId of the instance
  
  /**
   * \brief Print Interest packet 
   */
  virtual void Print (std::ostream &os) const;
  
  /**
   * \brief Get the size of Interest packet
   * Returns the Interest packet size after serialization
   */
  virtual uint32_t GetSerializedSize (void) const;
  
  /**
   * \brief Serialize Interest packet
   * Serializes Interest packet into Buffer::Iterator
   * @param[in] start buffer to contain serialized Interest packet
   */
  virtual void Serialize (Buffer::Iterator start) const;
  
  /**
   * \brief Deserialize Interest packet
   * Deserializes Buffer::Iterator into Interest packet
   * @param[in] start buffer that contains serialized Interest packet
   */ 
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * @brief Cheat for python bindings
   */
  static Ptr<PayloadHeader>
  GetPayloadHeader (Ptr<Packet> packet);
  
private:
  Ptr<Name> m_payload;
};

/**
 * @ingroup ndn-exceptions
 * @brief Class for Interest parsing exception 
 */
class PayloadHeaderException {};

} // namespace ndn
} // namespace ns3

#endif // _NDN_PAYLOAD_HEADER_H_
