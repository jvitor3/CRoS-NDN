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

#ifndef NDN_PRODUCER_NODEID_NLSR_V2_H
#define NDN_PRODUCER_NODEID_NLSR_V2_H


#include <ns3/ndnSIM/apps/ndn-app.h>
#include "ns3/ptr.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/random-variable.h"
#include <set>

namespace ns3 {
namespace ndn {

class ProducerNodeIDNLSRV2 : public App
{
public: 
  static TypeId
  GetTypeId (void);
        
  ProducerNodeIDNLSRV2 ();

  // inherited from NdnApp
  void
  OnInterest (Ptr<const Interest> interest);
  
  void
  OnTimeout (uint32_t nodeid);
  
  void RefreshFib();
  
  void
  SendInterest (Ptr<Name> prefixname);
  
  virtual void
  OnData (Ptr<const ndn::Data> contentObject);
  
  void
  OnDataTimeout(Ptr<Name> interest);
  
protected:
  // inherited from Application base class.
  virtual void
  StartApplication ();    // Called at time specified by Start

  virtual void
  StopApplication ();     // Called at time specified by Stop

private:
  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;

  uint32_t m_signature;
  Name m_keyLocator;
  
  std::map<uint32_t,Time> m_neighbours;
  std::map<uint32_t,EventId> m_neighborevents;
  double              m_frequency;
  
  UniformVariable m_rand;
  Time m_interestLifeTime;
  bool m_isRunning;
  std::map<Name,EventId> m_events;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_PRODUCER_NODEID_NLSR_V2_H
