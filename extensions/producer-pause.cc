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

 
#include "producer-pause.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/ndn-fib.h"
#include "ns3/ndn-interest.h"

NS_LOG_COMPONENT_DEFINE ("ndn.ProducerPause");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ProducerPause);

TypeId
ProducerPause::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ProducerPause")
    .SetGroupName ("Ndn")
    .SetParent<Producer> ()
    .AddConstructor<ProducerPause> ()    
	
	.AddAttribute ("PauseTime",
                   "Time defining the consumer pause sending interest, but keeps the app interface up",
                   StringValue ("20s"),
                   MakeTimeAccessor (&ProducerPause::GetPauseTime, &ProducerPause::SetPauseTime),
                   MakeTimeChecker ())
    ;

  return tid;
}

ProducerPause::ProducerPause ()
  :m_paused(false)
{
}

ProducerPause::~ProducerPause ()
{
}

Time
ProducerPause::GetPauseTime () const
{
	return m_pauseTime;
}  
  

void
ProducerPause::SetPauseTime (Time pauseTime)
{
	m_pauseTime = ProducerPause::m_startTime + pauseTime;
	Simulator::Schedule (m_pauseTime, &ProducerPause::Pause, this);
}

void
ProducerPause::Pause ()
{
  m_paused = true;
}  

void 
ProducerPause::OnInterest (Ptr<const Interest> interest)
{
	if (!m_paused)
		Producer::OnInterest (interest);
	else
	{
		Ptr<Fib> fib = GetNode ()->GetObject<Fib> ();
		fib->RemoveFromAll (m_face);
	}
}
  
} // namespace ndn
} // namespace ns3
