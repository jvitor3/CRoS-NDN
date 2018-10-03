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

#ifndef NDN_CONTROL_ROUTER_H
#define NDN_CONTROL_ROUTER_H

#include "ns3/object.h"
#include "ns3/ptr.h"
#include <list>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>

namespace ns3 {

class Channel;

namespace ndn {

class Name;

typedef Name NameComponents;

//Parameters
//Face Id, metric, delay
////typedef boost::reference_wrapper<boost::tuple< uint32_t, uint32_t, double > > FaceInfo;


/**
 * @brief Class representing control router interface for ndnSIM
 */
class ControlRouter : public Object
{
public:	
  
  /**
   * @brief Graph edge
   */
  typedef boost::tuple< Ptr< ControlRouter >, uint32_t, Ptr< ControlRouter > > Incidency;
  
  /**
   * @brief List of graph edges
   */
  typedef std::list< Incidency > IncidencyList;
  
 
  /**
   * \brief Interface ID
   *
   * \return interface ID
   */
  static TypeId
  GetTypeId ();

  /**
   * @brief Default constructor
   */
  ControlRouter (uint32_t id);

  /**
   * @brief Get numeric ID of the node (internally assigned)
   */
  uint32_t
  GetId () const;


  /**
   * @brief Add edge to the node
   * @param face Face of the edge
   * @param ndn ControlRouter of another node
   */
  void
  AddIncidency (uint32_t face, Ptr< ControlRouter > cr);
  
  void
  RemoveIncidency (uint32_t face, Ptr< ControlRouter > cr);

  /**
   * @brief Get list of edges that are connected to this node
   */
  IncidencyList &
  GetIncidencies ();

 
private:
  uint32_t m_id;
  
  IncidencyList m_incidencies;
};

inline bool
operator == (const ControlRouter::Incidency &a,
             const ControlRouter::Incidency &b)
{
  return a.get<0> () == b.get<0> () &&
    a.get<1> () == b.get<1> () &&
    a.get<2> () == b.get<2> ();
}

inline bool
operator != (const ControlRouter::Incidency &a,
             const ControlRouter::Incidency &b)
{
  return ! (a == b);
}

} // namespace ndn
} // namespace ns3

#endif // NDN_CONTROL_ROUTER_H
