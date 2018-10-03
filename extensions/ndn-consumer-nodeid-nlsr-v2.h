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

#ifndef NDN_CONSUMER_NODEID_NLSRLIKE_V2
#define NDN_CONSUMER_NODEID_NLSRLIKE_V2

#include "ndn-consumer-base.h"

namespace ns3 {
namespace ndn {

class ConsumerNodeIDNLSRV2: public ConsumerBase
{
public: 
  static TypeId GetTypeId ();

  virtual void
  OnData (Ptr<const Data> contentObject);
  
  virtual void
  OnTimeout (uint32_t sequenceNumber);
  
protected:
  virtual void
  StartApplication ();

  virtual void
  ScheduleNextPacket ();
  
  virtual void
  UpdateInterest ();

};
} // namespace ndn
} // namespace ns3

#endif
