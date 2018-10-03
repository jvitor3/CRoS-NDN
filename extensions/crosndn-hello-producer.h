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

#ifndef CRoSNDN_HELLO_PRODUCER_H
#define CRoSNDN_HELLO_PRODUCER_H


#include <ns3/ndnSIM/apps/ndn-app.h>
#include "ns3/ptr.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/random-variable.h"
#include "ns3/ndn-rtt-estimator.h"
#include <set>
#include "crosndn-strategy.h"


namespace ns3 {
namespace ndn {

class CRoSNDNHelloProducer : public App
{
public: 
  static TypeId
  GetTypeId (void);
        
  CRoSNDNHelloProducer ();

  // inherited from NdnApp
  void
  OnInterest (Ptr<const Interest> interest);
  
  void
  OnTimeout (uint32_t nodeid);
  
  void
  SendInterest (Ptr<Name> prefixname);
  
  void
  SearchController ();
  
  void
  RegisterInController ();
  
  virtual void
  OnData (Ptr<const ndn::Data> contentObject);
  
  void
  OnNoController(Ptr<Name> interest);
  
  void
  OnNoRegisterResponse(Ptr<Name> interest);
  
  std::set<uint32_t> m_neighbors;   
  
  void
  RefreshFib ();
  
  bool
  AskRoute  (Ptr<const Interest> interest);
  
  void
  OnAskRouteTimeout (Name &pref, uint32_t seqn);

  void
  OnInstallRouteTimeout (Name &pref, uint32_t seqn, uint32_t nodeid);
  
  Name
  FormatRouteString (Name prefix, uint32_t nodeid);
  
protected:
  // inherited from Application base class.
  virtual void
  StartApplication ();    // Called at time specified by Start

  virtual void
  StopApplication ();     // Called at time specified by Stop
  
  Ptr<RttEstimator> m_rtt;
  
private:
  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;

  uint32_t m_signature;
  Name m_keyLocator;
  
//  std::map<uint32_t,Time> m_neighbours;
  std::map<uint32_t,EventId> m_events;  //neighbor router, hello event timer
  std::map<uint32_t,Name> m_neighborctl; //neighbor router, neighbor controller
  
  UniformVariable m_rand;
  Time m_interestLifeTime;
  bool m_isRunning;
  uint32_t m_seq;  //sequence number for search controller interests
  
  uint32_t m_seqreg;  //sequence number for register in controller interests
//  uint32_t m_appstate;
  
  EventId m_discoveryctl;  //timeout pending controller discovery response event
  EventId m_regctl;  //timeout pending register response event
//  bool m_regintflag;
  double m_frequency;
  
  uint32_t m_seqi;  //sequence number for route request
  
  //EventId         m_sendEvent;  //for route request
  std::map<uint32_t,EventId> m_askrouteTimeOut;
  std::set<Name> m_pendingroutes;
  std::map<uint32_t,EventId> m_routeInstallTimeOut;
  
  Ptr<ns3::ndn::fw::CRoSNDNStrategy> m_fw;
  bool m_newneighbor;
};

} // namespace ndn
} // namespace ns3

#endif // CRoSNDN_HELLO_PRODUCER_H
