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

#ifndef NDNSIM_LIMITED_FIB_STRATEGY_H
#define NDNSIM_LIMITED_FIB_STRATEGY_H

#include <ns3/ndnSIM/model/fw/green-yellow-red.h>
#include <ns3/ndnSIM/ndn.cxx/name.h>
#include "ns3/log.h"

namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup ndn
 * \brief Best route strategy
 */
class LimitedFibStrategy :
    public GreenYellowRed
{
private:
  typedef GreenYellowRed super;

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
  LimitedFibStrategy ();

  void
  AddFibEntry (Ptr<Name> name, Ptr<Face> inFace, uint32_t metric);

  void
  AddFibEntry (Name name, Ptr<Face> inFace, uint32_t metric);
  
  void
  RemoveFibEntry (Ptr<Name> name);

protected:
  virtual void
  DidSendOutInterest (Ptr<Face> inFace, Ptr<Face> outFace, Ptr<const Interest> interest, Ptr<pit::Entry> pitEntry);
  
protected:
  static LogComponent g_log;
  std::list<Name> m_fibentries;
  uint32_t m_maxfibsize;
};

} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // NDNSIM_LIMITED_FIB_STRATEGY_H
