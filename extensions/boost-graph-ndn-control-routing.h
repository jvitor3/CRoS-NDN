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


#ifndef BOOST_GRAPH_NDN_CONTROL_ROUTING_HELPER_H
#define BOOST_GRAPH_NDN_CONTROL_ROUTING_HELPER_H

/// @cond include_hidden

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/ref.hpp>
#include "ns3/ndn-face.h"
#include "ns3/ndn-limits.h"
#include "ns3/node-list.h"
#include "ns3/channel-list.h"
#include "ns3/ptr.h"
#include "ndn-control-router.h"
#include <list>
#include <map>
#include <set>
#include <iterator>


namespace boost {

class NdnControlRouterGraph
{
public:
  typedef ns3::Ptr< ns3::ndn::ControlRouter > Vertice;
  typedef uint16_t edge_property_type;
  typedef uint32_t vertex_property_type;
  
  NdnControlRouterGraph ()
  {
    m_idCounter = 0;
  }

  const std::list< Vertice > &
  GetVertices () const
  {
    return m_vertices;
  }
  
  void 
  AddVertices (uint32_t id)
  {
    if (m_NodeToControl.find (id) == m_NodeToControl.end())
    {
      ns3::Ptr<ns3::ndn::ControlRouter> cr = ns3::CreateObject<ns3::ndn::ControlRouter> (m_idCounter);
//      NS_LOG_UNCOND ("m_idCounter= " << m_idCounter);  
      std::pair<std::set<Vertice>::iterator,bool> exist;
      exist = m_vertices_set.insert(cr);
      if (exist.second)
      {
	m_vertices.push_back (cr);
	
  	m_NodeToControl[id] = cr->GetId ();
   	m_ControlToNode[cr->GetId ()] = id;
     	m_idCounter++;
      }
    }
  }
  
  void
//  AddEdge (uint32_t id1, uint32_t id2, ns3::ndn::FaceInfo faceinfo)
  AddEdge (uint32_t id1or, uint32_t id2or, uint32_t face)
  {
    bool flagid1 = false;
    bool flagid2 = false;
    Vertice node1, node2;
    
    uint32_t id1, id2;
    id1 = m_NodeToControl[id1or];
    id2 = m_NodeToControl[id2or];    
    
    for (std::list<Vertice>::iterator it = m_vertices.begin(); it != m_vertices.end(); it++)
    {
      if ((*it)->GetId () == id1 && !flagid2)
      {
        flagid1 = true;
        node1 = (*it);
      }
      else if (((*it)->GetId()) == id2 && !flagid1)
      {
        flagid2 = true;
        node2 = (*it);

      }
      if (flagid1 && (*it)->GetId() == id2)
      {
          (*it)->AddIncidency (face, node1);
          node1->AddIncidency (face, (*it));
      }
      else if (flagid2 && (*it)->GetId() == id1)
      {
          (*it)->AddIncidency (face, node2);
          node2->AddIncidency (face, (*it));
      }
    }
  }
  
  void
  RemoveEdges (uint32_t NodeId)
  {
    Vertice node;
    
    uint32_t id;
    id = m_NodeToControl[NodeId];
	bool found = false;
    
    for (std::list<Vertice>::iterator it = m_vertices.begin(); it != m_vertices.end(); it++)
    {
      if ((*it)->GetId () == id)
      {
        node = (*it);
		found = true;
      }
    }
	if (found)
	{
		std::list< ns3::ndn::ControlRouter::Incidency > listx = node->GetIncidencies ();
		for (std::list< ns3::ndn::ControlRouter::Incidency >::iterator j = listx.begin(); j != listx.end(); j++)
		{
			RemoveEdge(*j);
		}
	}
  }
  
  void
  RemoveEdge (ns3::ndn::ControlRouter::Incidency inc)
  {
    (boost::get<0>(inc))->RemoveIncidency (boost::get<1>(inc), boost::get<2>(inc));
    (boost::get<2>(inc))->RemoveIncidency (boost::get<1>(inc), boost::get<0>(inc));
  }
 
  uint32_t
  GetSize ()
  {
    return m_vertices.size ();  
  }
  
  uint32_t
  GetEdgeSize ()
  {
    uint32_t s = 0;
    for (std::list<Vertice>::iterator it = m_vertices.begin(); it != m_vertices.end(); it++)
    {
      s = s + ((*it)->GetIncidencies ()).size();
    }
    return s;
  }
  
  uint32_t
  GetControlId (uint32_t NodeId)
  {
    return m_NodeToControl[NodeId];
  }
  
  uint32_t
  GetNodeId (uint32_t ControlId)
  {
    return m_ControlToNode[ControlId];
  }
  
public:
  std::list< Vertice > m_vertices;
  std::set< Vertice > m_vertices_set;
  std::map<uint32_t,uint32_t> m_NodeToControl;
  std::map<uint32_t,uint32_t> m_ControlToNode;
  uint32_t m_idCounter;
};


class ndn_control_router_graph_category :
    public virtual vertex_list_graph_tag,
    public virtual incidence_graph_tag
{
};


template<>
struct graph_traits< NdnControlRouterGraph >
{
  // Graph concept
  typedef NdnControlRouterGraph::Vertice vertex_descriptor;
  typedef ns3::ndn::ControlRouter::Incidency edge_descriptor;
  typedef directed_tag directed_category;
  typedef disallow_parallel_edge_tag edge_parallel_category;
  typedef ndn_control_router_graph_category traversal_category;

  // VertexList concept
  typedef std::list< vertex_descriptor >::const_iterator vertex_iterator;
  typedef size_t vertices_size_type;

  // AdjacencyGraph concept
  typedef ns3::ndn::ControlRouter::IncidencyList::iterator out_edge_iterator;
  typedef size_t degree_size_type;
};

} // namespace boost

namespace boost
{

inline
graph_traits< NdnControlRouterGraph >::vertex_descriptor
source(
       graph_traits< NdnControlRouterGraph >::edge_descriptor e,
       const NdnControlRouterGraph& g)
{
  return e.get<0> ();
}

inline
graph_traits< NdnControlRouterGraph >::vertex_descriptor
target(
       graph_traits< NdnControlRouterGraph >::edge_descriptor e,
       const NdnControlRouterGraph& g)
{
  return e.get<2> ();
}

inline
std::pair< graph_traits< NdnControlRouterGraph >::vertex_iterator,
	   graph_traits< NdnControlRouterGraph >::vertex_iterator >
vertices (const NdnControlRouterGraph&g)
{
  return make_pair (g.GetVertices ().begin (), g.GetVertices ().end ());
}

inline
graph_traits< NdnControlRouterGraph >::vertices_size_type
num_vertices(const NdnControlRouterGraph &g)
{
  return g.GetVertices ().size ();
}
  

inline
std::pair< graph_traits< NdnControlRouterGraph >::out_edge_iterator,
	   graph_traits< NdnControlRouterGraph >::out_edge_iterator >  
out_edges(
	  graph_traits< NdnControlRouterGraph >::vertex_descriptor u, 
	  const NdnControlRouterGraph& g)
{
  return std::make_pair(u->GetIncidencies ().begin (),
			u->GetIncidencies ().end ());
}

inline
graph_traits< NdnControlRouterGraph >::degree_size_type
out_degree(
	  graph_traits< NdnControlRouterGraph >::vertex_descriptor u, 
	  const NdnControlRouterGraph& g)
{
  return u->GetIncidencies ().size ();
}


//////////////////////////////////////////////////////////////
// Property maps

struct EdgeWeights
{
  EdgeWeights (const NdnControlRouterGraph &graph)
  : m_graph (graph)
  { 
  }

private:
  const NdnControlRouterGraph &m_graph;
};


struct VertexIds
{
  VertexIds (const NdnControlRouterGraph &graph)
  : m_graph (graph)
  { 
  }

private:
  const NdnControlRouterGraph &m_graph;
};

template<>
struct property_map< NdnControlRouterGraph, edge_weight_t >
{
  typedef const EdgeWeights const_type;
  typedef EdgeWeights type;
};

template<>
struct property_map< NdnControlRouterGraph, vertex_index_t >
{
  typedef const VertexIds const_type;
  typedef VertexIds type;
};


template<>
struct property_traits< EdgeWeights >
{
  // Metric property map
  //Face id, metric, delay
  typedef tuple<  uint32_t, uint16_t, double > value_type;
  typedef tuple<  uint32_t, uint16_t, double > reference;
  typedef ns3::ndn::ControlRouter::Incidency key_type;
  typedef readable_property_map_tag category;
};

const property_traits< EdgeWeights >::value_type WeightZero (0, 0, 0.0);
const property_traits< EdgeWeights >::value_type WeightInf (0, std::numeric_limits<uint16_t>::max (), 0.0);

struct WeightCompare :
    public std::binary_function<property_traits< EdgeWeights >::reference,
                                property_traits< EdgeWeights >::reference,
                                bool>
{
  bool
  operator () (property_traits< EdgeWeights >::reference a,
               property_traits< EdgeWeights >::reference b) const
  {
    return a.get<1> () < b.get<1> ();
  }

  bool
  operator () (property_traits< EdgeWeights >::reference a,
               uint32_t b) const
  {
    return a.get<1> () < b;
  }
  
  bool
  operator () (uint32_t a,
               uint32_t b) const
  {
    return a < b;
  }

};

struct WeightCombine :
    public std::binary_function<uint32_t,
                                property_traits< EdgeWeights >::reference,
                                uint32_t>
{
  uint32_t
  operator () (uint32_t a, property_traits< EdgeWeights >::reference b) const
  {
    return a + b.get<1> ();
  }

  tuple<  uint32_t, uint32_t, double >
  operator () (tuple<  uint32_t, uint32_t, double > a,

               property_traits< EdgeWeights >::reference b) const
               
  {
    if (a.get<0> () == 0)
      return make_tuple (b.get<0> (), a.get<1> () + b.get<1> (), a.get<2> () + b.get<2> ());
    else
      return make_tuple (a.get<0> (), a.get<1> () + b.get<1> (), a.get<2> () + b.get<2> ());
  }
};
  
template<>
struct property_traits< VertexIds >
{
  // Metric property map
  typedef uint32_t value_type;
  typedef uint32_t reference;
  typedef ns3::Ptr< ns3::ndn::ControlRouter > key_type;
  typedef readable_property_map_tag category;
};


inline EdgeWeights
get(edge_weight_t,
    const NdnControlRouterGraph &g)
{
  return EdgeWeights (g);
}


inline VertexIds
get(vertex_index_t,
    const NdnControlRouterGraph &g)
{
  return VertexIds (g);
}

template<class M, class K, class V>
inline void
put (reference_wrapper< M > mapp,
     K a, V p)
{
  mapp.get ()[a] = p;
}

inline uint32_t
get (const boost::VertexIds&, ns3::Ptr<ns3::ndn::ControlRouter> &gr)
{
  return gr->GetId ();
}

inline property_traits< EdgeWeights >::reference
get(const boost::EdgeWeights&, ns3::ndn::ControlRouter::Incidency &edge)
{
  if (edge.get<1> () == 0)
    return property_traits< EdgeWeights >::reference (0, 0, 0.0);
  else
    {
      return property_traits< EdgeWeights >::reference (edge.get<1> (), 1, 1.0);
    }
}

struct PredecessorsMap1 :
    public std::map< ns3::Ptr< ns3::ndn::ControlRouter >, ns3::Ptr< ns3::ndn::ControlRouter > >
{
};

template<>
struct property_traits< reference_wrapper<PredecessorsMap1> >
{
  // Metric property map
  typedef ns3::Ptr< ns3::ndn::ControlRouter > value_type;
  typedef ns3::Ptr< ns3::ndn::ControlRouter > reference;
  typedef ns3::Ptr< ns3::ndn::ControlRouter > key_type;
  typedef read_write_property_map_tag category;
};


struct DistancesMap1 :
  public std::map< ns3::Ptr< ns3::ndn::ControlRouter >, tuple< uint32_t, uint32_t, double > >
{
};

template<>
struct property_traits< reference_wrapper<DistancesMap1> >
{
  // Metric property map
  typedef tuple< uint32_t, uint32_t, double > value_type;
  typedef tuple< uint32_t, uint32_t, double > reference;
  typedef ns3::Ptr< ns3::ndn::ControlRouter > key_type;
  typedef read_write_property_map_tag category;
};

inline tuple< uint32_t, uint32_t, double >
get (DistancesMap1 &map, ns3::Ptr<ns3::ndn::ControlRouter> key)
{
  boost::DistancesMap1::iterator i = map.find (key);
  if (i == map.end ())
    return tuple< uint32_t, uint32_t, double > (0, std::numeric_limits<uint32_t>::max (), 0.0);
  else
    return i->second;
}

} // namespace boost

/// @endcond

#endif // BOOST_GRAPH_NDN_CONTROL_ROUTING_HELPER_H
