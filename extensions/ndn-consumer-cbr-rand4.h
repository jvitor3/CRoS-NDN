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

#ifndef NDN_CONSUMER_CBR_RAND4_H
#define NDN_CONSUMER_CBR_RAND4_H

#include "ns3/ndnSIM/apps/ndn-consumer.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
 */
class ConsumerCbrRand4: public Consumer
{
public: 
  static TypeId GetTypeId ();
        
  /**
   * \brief Default constructor 
   * Sets up randomizer function and packet sequence number
   */
  ConsumerCbrRand4 ();
  virtual ~ConsumerCbrRand4 ();
  
  virtual void
  OnData (Ptr<const Data> data);
  
  void
  OnTimeout (uint32_t sequenceNumber); 
  
  uint32_t
  GetFaceId ();   
  
protected:
  virtual void
  StartApplication ();
  
  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN protocol
   */
  virtual void
  ScheduleNextPacket ();
  
  void
  SendPacket ();

  /**
   * @brief Set type of frequency randomization
   * @param value Either 'none', 'uniform', or 'exponential'
   */
  void
  SetRandomize (const std::string &value);

  /**
   * @brief Get type of frequency randomization
   * @returns either 'none', 'uniform', or 'exponential'
   */
  std::string
  GetRandomize () const;
   
protected:
  double              m_frequency; // Frequency of interest packets (in hertz)
  bool                m_firstTime;
  RandomVariable      *m_random;
  std::string         m_randomType;
  uint32_t m_nprefixes;
  UniformVariable m_randprefix;
  std::map<uint32_t,Name > m_pref;
  uint32_t m_newprefixes;
  uint32_t m_nnodes;
  uint32_t m_newpseq;
  uint32_t m_newpnode;
  std::map<uint32_t,uint32_t> m_seqcount;
};

} // namespace ndn
} // namespace ns3

#endif
