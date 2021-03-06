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

#ifndef ARPLIKE_STRATEGY
#define ARPLIKE_STRATEGY

#include "limited-fib-strategy.h"
#include "ns3/log.h"

namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup ndn
 * \brief Best route strategy
 */
class ARPLikeStrategy :
    public LimitedFibStrategy
{
private:
  typedef LimitedFibStrategy super;

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
  ARPLikeStrategy ();
  
  void
  WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry);
        
  // from super
  virtual bool
  DoPropagateInterest (Ptr<Face> incomingFace,
                       Ptr<const Interest> header,
                       Ptr<pit::Entry> pitEntry);
                       
  virtual void
  SatisfyPendingInterest (Ptr<Face> inFace,
                                            Ptr<const Data> data,
                                            Ptr<pit::Entry> pitEntry);
                                                                   
protected:
  static LogComponent g_log;
};

} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // ARPLIKE_STRATEGY
