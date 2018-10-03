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

#ifndef CRoSNDN_REGISTER_ROUTER_H
#define CRoSNDN_REGISTER_ROUTER_H

#include <ns3/ndnSIM/apps/ndn-app.h>
#include "ns3/ptr.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-data.h"
#include "ns3/random-variable.h"
#include "crosndn-strategy.h"
#include <map>

namespace ns3 {
namespace ndn {

class CRoSNDNRegisterRouter : public App
{
public: 
  static TypeId
  GetTypeId (void);
        
  CRoSNDNRegisterRouter ();

  // inherited from NdnApp
  void OnInterest (Ptr<const Interest> interest);
 
protected:
  // inherited from Application base class.
  virtual void
  StartApplication ();    // Called at time specified by Start

  virtual void
  StopApplication ();     // Called at time specified by Stop
  
private:
  Ptr<ns3::ndn::fw::CRoSNDNStrategy> m_fw;
  Name m_postfix;
  uint32_t m_signature;
  Name m_keyLocator;
  Name m_prefix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
  typedef std::set<uint32_t> NodeSet;
  NodeSet m_routers;
  
  std::map<uint32_t,EventId> m_events;
  UniformVariable m_rand;
  Time m_interestLifeTime;
  bool m_isRunning;
  uint32_t m_seq;  
};

} // namespace ndn
} // namespace ns3

#endif // CRoSNDN_REGISTER_ROUTER_H
