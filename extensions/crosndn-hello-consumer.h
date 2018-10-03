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

#ifndef CRoSNDN_HELLO_CONSUMER
#define CRoSNDN_HELLO_CONSUMER

#include "ndn-consumer-base.h"
#include "crosndn-strategy.h"

namespace ns3 {
namespace ndn {

class CRoSNDNHelloConsumer: public ConsumerBase
{
public: 
  static TypeId GetTypeId ();

  virtual void
  OnData (Ptr<const Data> contentObject);
  
  virtual void
  OnTimeout (uint32_t sequenceNumber);
  
  //teste
  void
  OnInterest (Ptr<const Interest> interest);
  //teste
  
protected:
  virtual void
  StartApplication ();

  virtual void
  ScheduleNextPacket ();

private:
  Ptr<ns3::ndn::fw::CRoSNDNStrategy> m_fw;
  
};
} // namespace ndn
} // namespace ns3

#endif
