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

#ifndef PRODUCER_PAUSE_H
#define PRODUCER_PAUSE_H

#include "ns3/ndnSIM/apps/ndn-producer.h"

namespace ns3 {
namespace ndn {

class ProducerPause: public Producer
{
public:
  static TypeId GetTypeId ();

  ProducerPause ();
  virtual ~ProducerPause ();
  
protected:

  Time GetPauseTime () const;

  void SetPauseTime (Time pauseTime);
  
  void Pause ();
  
  virtual void OnInterest (Ptr<const Interest> interest);
  
protected:
  Time m_pauseTime; 
  bool m_paused;

};
} // namespace ndn
} // namespace ns3

#endif
