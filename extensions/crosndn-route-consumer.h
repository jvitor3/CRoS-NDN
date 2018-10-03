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

#ifndef CRoSNDN_ROUTE_CONSUMER_H
#define CRoSNDN_ROUTE_CONSUMER_H

#include <ns3/ndnSIM/apps/ndn-app.h>
#include "ns3/ptr.h"
#include <ns3/ndnSIM/model/ndn-name.h>
#include <ns3/ndnSIM/model/ndn-data.h>
#include "ns3/ndn-rtt-estimator.h"

namespace ns3 {
namespace ndn {

class CRoSNDNRouteConsumer : public App
{
public: 
  static TypeId
  GetTypeId (void);
        
  CRoSNDNRouteConsumer ();

  bool AskRoute  (Ptr<const Interest> interest);
  
  void OnTimeout (Name &pref, uint32_t sequenceNumber, bool isAskRoute);
  
  // inherited from NdnApp
  void OnInterest (Ptr<const Interest> interest);
  
  void OnData (Ptr<const ndn::Data> contentObject);
  
  void RefreshFib ();

protected:
  // inherited from Application base class.
  virtual void
  StartApplication ();    // Called at time specified by Start

  virtual void
  StopApplication ();     // Called at time specified by Stop

  Ptr<RttEstimator> m_rtt;
  
private:
  Name m_postfix;
  uint32_t m_signature;
  Name m_keyLocator;
  Name m_prefix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
  uint32_t m_seqi;
  
  EventId         m_sendEvent;
  std::set<Name> m_pendingroutes;

};

} // namespace ndn
} // namespace ns3

#endif // CRoSNDN_ROUTE_CONSUMER_H
