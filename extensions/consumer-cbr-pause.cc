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
 
 //ConsumerBase modifies ConsumerCbr.

 
#include "consumer-cbr-pause.h"
#include "ns3/log.h"
#include "ns3/string.h"

NS_LOG_COMPONENT_DEFINE ("ndn.ConsumerCbrPause");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ConsumerCbrPause);

TypeId
ConsumerCbrPause::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ConsumerCbrPause")
    .SetGroupName ("Ndn")
    .SetParent<ConsumerCbr> ()
    .AddConstructor<ConsumerCbrPause> ()    
	
	.AddAttribute ("PauseTime",
                   "Time defining the consumer pause sending interest, but keeps the app interface up",
                   StringValue ("20s"),
                   MakeTimeAccessor (&ConsumerCbrPause::GetPauseTime, &ConsumerCbrPause::SetPauseTime),
                   MakeTimeChecker ())
    ;

  return tid;
}

ConsumerCbrPause::ConsumerCbrPause ()
  :m_paused(false)
{
}

ConsumerCbrPause::~ConsumerCbrPause ()
{
}

Time
ConsumerCbrPause::GetPauseTime () const
{
	return m_pauseTime;
}  
  

void
ConsumerCbrPause::SetPauseTime (Time pauseTime)
{
	m_pauseTime = ConsumerCbr::m_startTime + pauseTime;
	Simulator::Schedule (m_pauseTime, &ConsumerCbrPause::Pause, this);
}

void
ConsumerCbrPause::Pause ()
{
  m_paused = true;
}  

void 
ConsumerCbrPause::ScheduleNextPacket ()
{
	if (!m_paused)
		ConsumerCbr::ScheduleNextPacket ();
}
  
} // namespace ndn
} // namespace ns3
