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
 
#ifndef TUNNEL_IN_H_
#define TUNNEL_IN_H_

#include <ns3/ndnSIM/apps/ndn-app.h>
#include "ns3/ptr.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-data.h"
#include "crosndn-tunnel-strategy.h"

namespace ns3 {
namespace ndn {

class TunnelIn : public App
{
public: 
  static TypeId
  GetTypeId (void);
        
  TunnelIn ();

  // inherited from NdnApp
  void OnInterest (Ptr<const Interest> interest);
  
  virtual void
  OnData (Ptr<const Data> contentObject);  

protected:
  // inherited from Application base class.
  virtual void
  StartApplication ();    // Called at time specified by Start

  virtual void
  StopApplication ();     // Called at time specified by Stop

private:

  Ptr<ns3::ndn::fw::CRoSNDNTunnelStrategy> m_fw;
};

} // namespace ndn
} // namespace ns3

#endif // TUNNEL_IN_H