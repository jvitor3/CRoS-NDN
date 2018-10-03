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

#ifndef NDN_CROSNDN_GIVEROUTE_H
#define NDN_CROSNDN_GIVEROUTE_H

#include <ns3/ndnSIM/apps/ndn-app.h>

#include "ns3/ptr.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-data.h"
#include <set>
#include <list>
#include <iterator>
#include <map>

namespace ns3 {
namespace ndn {

/**
 * @brief A simple Interest-sink applia simple Interest-sink application
 *
 * A simple Interest-sink applia simple Interest-sink application,
 * which replying every incoming Interest with Data packet with a specified
 * size and name same as in Interest.cation, which replying every incoming Interest
 * with Data packet with a specified size and name same as in Interest.
 */
class CROSNDNGiveRoute : public App
{
public: 
  static TypeId
  GetTypeId (void);
        
  CROSNDNGiveRoute ();

  // inherited from NdnApp
  void OnInterest (Ptr<const Interest> interest);

protected:
  // inherited from Application base class.
  virtual void
  StartApplication ();    // Called at time specified by Start

  virtual void
  StopApplication ();     // Called at time specified by Stop

private:
  Name m_postfix;
  uint32_t m_signature;
  Name m_keyLocator;
  Name m_prefix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
  std::map<const Name,std::set<uint32_t> > m_askedNames;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_CROSNDN_GIVEROUTE_H
