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


#ifndef CROSNDN_TUNNEL_STRATEGY_H
#define CROSNDN_TUNNEL_STRATEGY_H

#include "crosndn-strategy.h"
#include "ns3/log.h"

namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup ndn
 * \brief Best route strategy
 */
class CRoSNDNTunnelStrategy :
    public CRoSNDNStrategy
{
private:
  typedef CRoSNDNStrategy super;

public:
  static TypeId
  GetTypeId ();

  /**
   * @brief Helper function to retrieve logging name for the forwarding strategy
   */
  static std::string
  GetLogName ();
  
  /**
   * @brief Default constructor
   */
  CRoSNDNTunnelStrategy ();
  
  virtual ~CRoSNDNTunnelStrategy ();
  
/*******************************************
//Tunnel specific methods
********************************************/

  void
  SetTunnelFace(Ptr<Face> face);

  Ptr<Face>
  GetTunnelFace();
  
  uint32_t
  GetTunnelDestination(Name prefix);
  
  void
  SetTunnelDestination(Name prefix, uint32_t nodeid);
  
  virtual void
  RouteInstall(Ptr<Name> prefix, Ptr<Face> face, uint32_t metric);
  
  // from super                               
  virtual void
  WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry);  

protected:
  static LogComponent g_log;
  std::map<Name, uint32_t> m_prefix_node_tunnel_map;
  Ptr<Face> m_tunnelface;
};
  
} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // CROSNDN_TUNNEL_STRATEGY_H