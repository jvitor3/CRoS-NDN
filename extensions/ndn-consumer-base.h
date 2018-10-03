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

#ifndef NDN_CONSUMER_BASE_H
#define NDN_CONSUMER_BASE_H

#include "ns3/ndnSIM/apps/ndn-consumer-cbr.h"

namespace ns3 {
namespace ndn {

class ConsumerBase: public ConsumerCbr
{
public:
  static TypeId GetTypeId ();

  void
  SendPacket ();
  
protected:

  virtual void
  ScheduleNextPacket ();


};
} // namespace ndn
} // namespace ns3

#endif
