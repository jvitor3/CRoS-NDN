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

#ifndef CRoSNDN_REGISTER_CONTENTSET_CONSUMER_H_
#define CRoSNDN_REGISTER_CONTENTSET_CONSUMER_H_

#include <ns3/ndnSIM/apps/ndn-app.h>
#include "ns3/random-variable.h"
#include "ns3/ndn-name.h"
#include <map>
#include "ns3/ndn-rtt-estimator.h"
#include "crosndn-strategy.h"

namespace ns3 {
namespace ndn {

class CRoSNDNRegisterContentSetConsumer : public ndn::App
{
public:
  // register NS-3 type "DumbRequester"
  static TypeId
  GetTypeId ();

  CRoSNDNRegisterContentSetConsumer ();
  
  // (overridden from ndn::App) Processing upon start of the application
  virtual void
  StartApplication ();

  // (overridden from ndn::App) Processing when application is stopped
  virtual void
  StopApplication ();

  // (overridden from ndn::App) Callback that will be called when Data arrives
  virtual void
  OnData (Ptr<const ndn::Data> contentObject);
  
  void
  OnTimeout(Ptr<Name> interest);
  
private:
 
  void
  SendInterest (Ptr<Name> prefixname);
  
  void
  BatchInterests ();

protected:
    Ptr<RttEstimator> m_rtt;

private:
  bool m_isRunning;
  std::string m_name;
  uint32_t m_seq;
  uint32_t m_nprefixes;
  double   m_frequency; // Frequency of interest packets (in hertz)
  UniformVariable m_rand;
  Time               m_interestLifeTime;
  
  std::map<Name,EventId> m_events;
  uint32_t m_regrate;
  uint32_t m_count;
  uint32_t m_newlocation;
  
  Ptr<ns3::ndn::fw::CRoSNDNStrategy> m_fw;
};

}// namespace ndn
} // namespace ns3
#endif
