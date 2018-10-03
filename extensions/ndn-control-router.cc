/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013 UFRJ/PEE/GTA
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
 * Author:  João Vitor Torres <jvitor@gta.ufrj.br>
 */

//This class is used to model the topology view and calculate routes.

#include "ndn-control-router.h"
#include "ns3/ndn-l3-protocol.h"
#include "ns3/ndn-face.h"
#include "ns3/ndn-name.h"
#include "ns3/channel.h"

using namespace boost;

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ControlRouter);

TypeId 
ControlRouter::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::ControlRouter")
    .SetGroupName ("Ndn")
    .SetParent<Object> ()
  ;
  return tid;
}

ControlRouter::ControlRouter (uint32_t id)
{
  m_id = id;
}

uint32_t
ControlRouter::GetId () const
{
  return m_id;
}

void
ControlRouter::AddIncidency (uint32_t face, Ptr< ControlRouter > cr)
{
  m_incidencies.push_back (make_tuple (this, face, cr));
  m_incidencies.sort ();
  m_incidencies.unique ();
}


void
ControlRouter::RemoveIncidency (uint32_t face, Ptr< ControlRouter > cr)
{
  m_incidencies.remove (make_tuple (this, face, cr));
}

ControlRouter::IncidencyList &
ControlRouter::GetIncidencies ()
{
  return m_incidencies;
}


} // namespace ndn
} // namespace ns3
