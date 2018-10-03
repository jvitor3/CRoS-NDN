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

#include "nlsr-strategy.h"
#include <ns3/ndnSIM/model/ndn-data.h>
#include <ns3/ndnSIM/model/ndn-interest.h>
#include <ns3/ndnSIM/model/pit/ndn-pit.h>
#include <ns3/ndnSIM/model/pit/ndn-pit-entry.h>
#include <ns3/ndnSIM/model/cs/ndn-content-store.h>
#include <ns3/ndnSIM/ndn.cxx/name-component.h>
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/random-variable.h"
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/concept/assert.hpp>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include "ndn-payload-header.h"
#include <ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h>
#include <set>


using namespace boost;
using namespace std;

namespace ll = boost::lambda;

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (NLSRStrategy);

LogComponent NLSRStrategy::g_log = LogComponent (NLSRStrategy::GetLogName ().c_str ());

std::string
NLSRStrategy::GetLogName ()
{
  return super::GetLogName ()+".NLSRStrategy";
}


TypeId
NLSRStrategy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::NLSRStrategy")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <ns3::ndn::fw::NLSRStrategy> ()
    ;
  return tid;
}
    
NLSRStrategy::NLSRStrategy ()
{
  m_newNeighbor = true;
  m_registeredRouter = false;
  m_registeredContent = false;
  m_topologyChangeActive = false;
  super::m_nacksEnabled = true;
  m_state = 0;
  boost::NdnControlRouterGraph m_graph = boost::NdnControlRouterGraph ();
}

NLSRStrategy::~NLSRStrategy ()
{
}

void
NLSRStrategy::NotifyNewAggregate ()
{
  super::NotifyNewAggregate ();
}

void
NLSRStrategy::DoDispose ()
{

  super::DoDispose ();
}

/*******************************************
//Genereal Router node related methods
********************************************/

void
NLSRStrategy::IncrementState()
{
  m_state++;
}
  
uint32_t
NLSRStrategy::GetState()
{
  return m_state;
}

void
NLSRStrategy::AddFibEntryCtl (Ptr<Name> name, Ptr<Face> inFace, uint32_t metric)
{
  Ptr<fib::Entry> fibentry = m_fib->Add (name, inFace , metric);
  if (fibentry != 0)
    fibentry->UpdateStatus (inFace, fib::FaceMetric::NDN_FIB_GREEN); 
}

void
NLSRStrategy::RemoveNodeNeighbors(uint32_t id)
{
  if (m_nodeneighbors_set.find(id) != m_nodeneighbors_set.end())
    m_nodeneighbors_set.erase (m_nodeneighbors_set.find(id));
  m_nodeneighbors_list.remove (id);
}


void
NLSRStrategy::AddNodeNeighbors(uint32_t id)
{
  std::pair<std::set<uint32_t>::iterator,bool> exist;
  exist = m_nodeneighbors_set.insert(id);
  if (exist.second)
  {
    m_nodeneighbors_list.push_back (id);
  }
}
  
std::set<uint32_t>
NLSRStrategy::GetNodeNeighbors ()
{
  return m_nodeneighbors_set;
}


bool
NLSRStrategy::GetNewNeighbor ()
{
  return m_newNeighbor;
}
  
void
NLSRStrategy::SetNewNeighbor (bool flag)
{
  m_newNeighbor = flag;
}

bool
NLSRStrategy::GetRegisteredRouter ()
{
  return m_registeredRouter;
}
  
void
NLSRStrategy::SetRegisteredRouter (bool flag)
{
  m_registeredRouter = flag;
}

bool
NLSRStrategy::GetRegisteredContent ()
{
  return m_registeredContent;
}
  
void
NLSRStrategy::SetRegisteredContent (bool flag)
{
  m_registeredContent = flag;
}


void
NLSRStrategy::DidExhaustForwardingOptions (Ptr<Face> inFace,
                               Ptr<const Interest> interest,
                               Ptr<pit::Entry> pitEntry)
{
  
  NS_LOG_INFO ("Name: " << interest);  
  super::DidExhaustForwardingOptions(inFace, interest, pitEntry);
}

void
NLSRStrategy::FailedToCreatePitEntry (Ptr<Face> inFace,
                          Ptr<const Interest> header)
{
  NS_LOG_INFO ("Name: "  << header->GetName());
  bool success = InstallRoute(inFace, header);
  if (success)
  {
	  Ptr<pit::Entry> pitEntry = m_pit->Lookup (*header);
	  if (pitEntry == 0)
		  pitEntry = m_pit->Create (header);
	  else
		  super::FailedToCreatePitEntry(inFace, header);
	  if (pitEntry != 0)
		  PropagateInterest (inFace, header, pitEntry);
  }
  else
	  super::FailedToCreatePitEntry(inFace, header);
}


void
NLSRStrategy::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
  Name pname = pitEntry->GetFibEntry()->GetPrefix();
  size_t size = pname.size();
  std::string c0, c2;
  c2 = "";
  c0 = "";
  if (size > 0)
    c0 = pname.get(0).toUri ();
  if ((c0 != "hello") && 
      (pname.toUri () != "/") &&
      (c0 != "")) 
  {
	  
    NS_LOG_DEBUG ("WillEraseTimedOutPendingInterest for " << pitEntry->GetPrefix ());
	for (pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();
		face != pitEntry->GetOutgoing ().end (); face ++)
	{
		// NS_LOG_DEBUG ("Face: " << face->m_face);
		pitEntry->GetFibEntry ()->UpdateStatus (face->m_face, fib::FaceMetric::NDN_FIB_YELLOW);
	}
    RemoveFibEntry (Create<Name> (pname));	  
	NS_LOG_DEBUG ("Fib Name removed: " << pname << " \n Interest Name asked: " << pitEntry->GetPrefix());  
	  
    //super::WillEraseTimedOutPendingInterest (pitEntry);
    //RemoveFibEntry (Create<Name> (pname)); 
    //NS_LOG_DEBUG ("Fib Name removed: " << pname <<
    //  " \n Interest Name asked: " << pitEntry->GetPrefix());
  }
  //else
    m_timedOutInterests (pitEntry);
}

void
NLSRStrategy::SatisfyPendingInterest (Ptr<Face> inFace,
                                            Ptr<const Data> data,
                                            Ptr<pit::Entry> pitEntry)
{

  NS_LOG_INFO ("Testing Satisfy. " << data->GetName ());
  //satisfy all pending incoming Interests
  BOOST_FOREACH (const pit::IncomingFace &incoming, pitEntry->GetIncoming ())
  {
      bool ok = incoming.m_face->SendData (data);
      if (ok)
        {
          DidSendOutData (inFace, incoming.m_face, data, pitEntry);
          NS_LOG_INFO ("Satisfy " << *incoming.m_face);
        }
      else
        {
          m_dropData (data, incoming.m_face);
          NS_LOG_INFO ("Cannot satisfy data to " << *incoming.m_face);
        }
  }
  // All incoming interests are satisfied. Remove them
  pitEntry->ClearIncoming ();
  // Remove all outgoing faces
  pitEntry->ClearOutgoing ();
  // Set pruning timout on PIT entry (instead of deleting the record)
  m_pit->MarkErased (pitEntry);
}

      
bool
NLSRStrategy::DoPropagateInterest (Ptr<Face> inFace,
                       Ptr<const Interest> interest,
                       Ptr<pit::Entry> pitEntry)
{
  NS_LOG_INFO (this << interest->GetName ());
  Ptr<Name> nameWithSequence = Create<Name> (interest->GetName ());
  uint32_t namesize = nameWithSequence->size ();
  uint64_t nodeidint = boost::lexical_cast<uint64_t> ((inFace->GetNode())->GetId ());
  std::string nodeidstr;
  nodeidstr = boost::lexical_cast<std::string>(nodeidint);
  FwHopCountTag hopCountTag1;

//Add Fib entries to one hop neighbors
//based on Interests from app consumer-nodeid (ConsumerNodeID)
//filtering Interests with prefix "hello" not coming from the local node.
// Interest /hello/nodeid/seq  
  if ((namesize > 2) &&
        (nameWithSequence->get (0).toUri () == "hello"))
  {
        if (nameWithSequence->get (1).toNumber () != nodeidint)
        {
            NS_LOG_INFO ("Hello from: " << interest->GetName ());
            Ptr<Name> neighbor = Create<Name> ();
            NS_LOG_INFO ("Adding neighbor to neighbor list, face:" << inFace->GetId ());
            Ptr<Name> neigh = Create<Name> ();
            neigh->append ("router");
            neigh->append (nameWithSequence->get (1));
            interest->GetPayload ()->PeekPacketTag (hopCountTag1);
            AddFibEntry (neigh, inFace, hopCountTag1.Get());
            NS_LOG_INFO ("Adding entry for neighbor " 
              << nameWithSequence->get (1).toNumber () << ", face:" << inFace->GetId ());
        }
  }
 

  // Try to work out with just faces
  int propagatedCount = 0;
  NS_LOG_INFO("Loop for faces starting.");
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    { 
      NS_LOG_INFO ("Trying face: " << boost::cref(metricFace) << ", for name: " << interest->GetName());
      if (metricFace.GetStatus () == fib::FaceMetric::NDN_FIB_RED) // all non-read faces are in front
      {
        NS_LOG_INFO("No more faces.");
        break;
      }

      if (!TrySendOutInterest (inFace, metricFace.GetFace (), interest, pitEntry))
        {
          NS_LOG_INFO("Failure. Trying next face.");
          continue;
        }
        NS_LOG_INFO("Success. Interest propagated. " << nameWithSequence->get(0).toUri());
      propagatedCount++;
      //Stopping just in case it is not a Hello Interest.
      if (nameWithSequence->get(0).toUri() != "hello")
        {
           NS_LOG_INFO ("Not a hello.");
           break; // do only once
        }
    }
   NS_LOG_INFO ("Propagated to " << propagatedCount << " faces" << interest->GetName().toUri());
  return propagatedCount > 0;
}

void
NLSRStrategy::WillSatisfyPendingInterest  (Ptr<Face> inFace, Ptr<pit::Entry> pitEntry)
{
	if (inFace != NULL)
		super::WillSatisfyPendingInterest (inFace, pitEntry);
	//else
	//	m_dropData (data, inFace);
}

/*******************************************
//Genereal Controller node related methods
********************************************/
 
void
NLSRStrategy::AddRegisteredRouter (uint32_t id)
{
  std::pair<std::set<uint32_t>::iterator,bool> exist;
  m_topologyChangeActive = true;
  exist = m_registeredrouters_set.insert(id);
  if (exist.second)
  {
	m_registeredrouters_list.push_back (id);
  }
  m_registeredrouters_set2.insert(id);
  
  m_graph.AddVertices(id);  //teste 28nov2015
  
  m_topologyChangeActive = false;
  SetRouterNeighborOk (id, false);
}

void
NLSRStrategy::RemoveRegisteredRouter(uint32_t id)
{
  if (m_registeredrouters_set.find(id) != m_registeredrouters_set.end())
  {
    m_topologyChangeActive = true;
    m_registeredrouters_set2.erase(id);
    m_topologyChangeActive = false;
  }
}

std::set<uint32_t>
NLSRStrategy::GetRegisteredRouters ()
{
  return m_registeredrouters_set2;
}



void
NLSRStrategy::RemoveEdges (uint32_t id)
{
  if (m_registeredrouters_set.find (id) != m_registeredrouters_set.end () &&
   m_edgemap.find(id) != m_edgemap.end())
  {
    m_topologyChangeActive = true;
    m_edgemap.erase(m_edgemap.find(id));
	
	m_graph.RemoveEdges(id);
	
    m_topologyChangeActive = false;
  }
}

//Called only on controller to add adjacencies
void
NLSRStrategy::AddGraphEdges (uint32_t id1, uint32_t id2, uint32_t face)
{
  if (m_registeredrouters_set.find (id1) != m_registeredrouters_set.end ()
    && m_registeredrouters_set.find (id2) != m_registeredrouters_set.end ())
  {
     m_topologyChangeActive = true;
     if (m_edgemap.find(id1) != m_edgemap.end())
     {
       std::set<std::pair<uint32_t, uint32_t> > ed = (m_edgemap.find(id1))->second;
       ed.insert(std::pair<uint32_t,uint32_t> (id2, face));
       m_edgemap[id1] = ed;
     }
     else
     {
       std::set<std::pair<uint32_t,uint32_t> > temp;
       temp.insert(std::pair<uint32_t,uint32_t> (id2, face));
       m_edgemap[id1] = temp;
     }
	 
	 m_graph.AddEdge(id1, id2, face); //teste 28nov2015
	 
     m_topologyChangeActive = false;
  }
}


bool
NLSRStrategy::GetRouterNeighborOk (uint32_t id)
{
  if (m_routerneighborok.find(id) != m_routerneighborok.end ())
    return m_routerneighborok.find(id)->second;
  else
    return false;
}
  
void
NLSRStrategy::SetRouterNeighborOk (uint32_t id, bool flag)
{
  if (m_routerneighborok.find(id) != m_routerneighborok.end ())
    m_routerneighborok.find(id)->second = flag;
  else
    m_routerneighborok[id] = flag;

}

void
NLSRStrategy::RegisterContent (const Name &name, uint32_t routerid, uint32_t replace)
{
  if (m_registered_content.find(name) != m_registered_content.end() and not replace)
    m_registered_content.find(name)->second.insert(routerid);
  else
  {
    std::set<uint32_t> temp;
    temp.insert (routerid);
    m_registered_content[name] = temp;
  }
}
  
void
NLSRStrategy::UnRegisterContent (const Name &name, uint32_t routerid)
{
  std::map<const Name, std::set<uint32_t> >::iterator regcont = m_registered_content.find(name);
  if (regcont != m_registered_content.end() and regcont->second.find(routerid) != regcont->second.end())
    m_registered_content.find(name)->second.erase(routerid);
}

void
NLSRStrategy::UnRegisterContent (const Name &name)
{
  std::map<const Name, std::set<uint32_t> >::iterator regcont = m_registered_content.find(name);
  if (regcont != m_registered_content.end())
    m_registered_content.erase(name);
}
  
std::set<uint32_t>
NLSRStrategy::GetRegisteredContentNodes (const Name &name)
{
  std::set<uint32_t> test;
  if (m_registered_content.find(name) != m_registered_content.end())
    test = m_registered_content.find(name)->second;
  return test;
}

Ptr<Name>
NLSRStrategy::GetRoutes (uint32_t sourceid, uint32_t targetid)
{
  NS_LOG_DEBUG ("\n GetRoutes - source, target: " << sourceid << ", " << targetid << "\n"); 
  typedef graph_traits < boost::NdnControlRouterGraph >::vertex_descriptor vertex_descriptor;
  Ptr<Name> pathname = Create<Name> ();
  std::set<uint32_t> path;
  std::list<uint32_t> routerids;
  if (m_registeredrouters_set.find (sourceid) == m_registeredrouters_set.end ()
    || m_registeredrouters_set.find (targetid) == m_registeredrouters_set.end ()
    || m_edgemap.size() == 0//)
    || m_topologyChangeActive)
  {
    NS_LOG_DEBUG ("Path not found. Source ou target not registered.");
    return pathname;
  }

  
  boost::NdnControlRouterGraph graph1 = boost::NdnControlRouterGraph ();
  for (std::set<uint32_t>::const_iterator v = m_registeredrouters_set.begin();
    v != m_registeredrouters_set.end(); v++)
  {
    graph1.AddVertices(*v);
    NS_LOG_DEBUG ("Vertices. Node id: " << *v << ", Control id: " << graph1.GetControlId (*v));
  }
  
  for (std::map<uint32_t, std::set<std::pair<uint32_t, uint32_t> > >::const_iterator e = m_edgemap.begin();
    e != m_edgemap.end(); e++)
  {
    for (std::set<std::pair<uint32_t,uint32_t> >::const_iterator inc = (e->second).begin();
      inc != (e->second).end(); inc++)
      {
        graph1.AddEdge(e->first, inc->first, inc->second);
        NS_LOG_DEBUG ("Edges. Router id1, id2 and face : " << e->first << ", " 
          << inc->first << ", " << inc->second);
      }
  }
  
  // For now we doing Dijkstra for every node.  Can be replaced with Bellman-Ford or Floyd-Warshall.
  // Other algorithms should be faster, but they need additional EdgeListGraph concept provided by the graph,  
  //which is not obviously how implement in an efficient manner
  for (std::list<ns3::Ptr< ns3::ndn::ControlRouter > >::const_iterator node = graph1.GetVertices ().begin ();
        node != graph1.GetVertices ().end ();
        node++)        
  {	
    Ptr<ControlRouter> source = *node;
    if (source->GetId () == graph1.GetControlId (sourceid)) //sourceid)
    {
      boost::DistancesMap1    distances;
      boost::PredecessorsMap1  predecessors;

      NS_LOG_DEBUG ("Source id, graph size, edge size: " << source->GetId () 
        << ", " << graph1.GetSize() << ", " << graph1.GetEdgeSize());
      boost::dijkstra_shortest_paths (graph1, source,
      predecessor_map (boost::ref(predecessors))
      .
      distance_map (boost::ref(distances))
      .
      distance_inf (boost::WeightInf)
      .
      distance_zero (boost::WeightZero)
      .
      distance_compare (boost::WeightCompare ())
      .
      distance_combine (boost::WeightCombine ())
      );


      uint32_t nexthop = graph1.GetControlId (targetid);
      NS_LOG_DEBUG ("Node ids: source node: " << sourceid << " to target node: " << targetid);
      NS_LOG_DEBUG ("Control ids: source node: " << source->GetId () << " to target node: " << nexthop);
      NS_LOG_DEBUG ("Predecessors size: " << predecessors.size () 
        <<  ", Distances size: " << distances.size () 
        << ", number of nodes: "  << graph1.GetSize ());

      boost::PredecessorsMap1::iterator j = predecessors.begin ();
      bool foundpath = true;
      uint32_t s, i = 0, k = 0;
      s = predecessors.size ();
      path.clear();
      routerids.clear();
      std::pair<std::set<uint32_t>::iterator,bool> exist;
      while (foundpath)
      { 
           NS_LOG_DEBUG ("Test j: " << j->first->GetId () << " " << j->second->GetId ());
        if (j->first->GetId () == nexthop && (j->first->GetId () != j->second->GetId()))
        {
          exist = path.insert(nexthop);
          i++;
          NS_LOG_DEBUG ("Test nexthop: " << nexthop);
          if (exist.second)
          {
            NS_LOG_DEBUG ("Old next hop in reverse order: " <<  nexthop);
            routerids.push_back(nexthop);
            nexthop = j->second->GetId ();
            NS_LOG_DEBUG ("New next hop in reverse order: " <<  nexthop);
          }            
        }
        
        if (nexthop == graph1.GetControlId (sourceid)) //sourceid)
        {
          NS_LOG_DEBUG ("Next hop in reverse order: " <<  nexthop);
          routerids.push_back(nexthop);
          path.insert(nexthop);
          NS_LOG_DEBUG ("Path complete!");
          foundpath = false;
        }
        j++;
        k++;
        if ((k > (s * s)) or (i > s ))
        {
          NS_LOG_DEBUG ("Path not found.");
          break;
        }
        if (j == predecessors.end())
        {
          j = predecessors.begin ();
        }
      }
      break;    
    }
  }
  routerids.reverse();
  for (std::list<uint32_t>::iterator it = routerids.begin(); it != routerids.end(); ++it)
  {
    pathname->appendSeqNum (graph1.GetNodeId(*it));
  }
  return pathname;
}

Ptr<Name>
NLSRStrategy::GetRoutes2 (uint32_t sourceid, uint32_t targetid)
{
  NS_LOG_DEBUG ("\n GetRoutes - source, target: " << sourceid << ", " << targetid << "\n"); 
  typedef graph_traits < boost::NdnControlRouterGraph >::vertex_descriptor vertex_descriptor;
  Ptr<Name> pathname = Create<Name> ();
  std::set<uint32_t> path;
  std::list<uint32_t> routerids;
  if (m_registeredrouters_set.find (sourceid) == m_registeredrouters_set.end ()
    || m_registeredrouters_set.find (targetid) == m_registeredrouters_set.end ()
    || m_edgemap.size() == 0//)
    || m_topologyChangeActive)
  {
    NS_LOG_DEBUG ("Path not found. Source ou target not registered.");
    return pathname;
  }
   
  // For now we doing Dijkstra for every node.  Can be replaced with Bellman-Ford or Floyd-Warshall.
  // Other algorithms should be faster, but they need additional EdgeListGraph concept provided by the graph,  
  //which is not obviously how implement in an efficient manner
  for (std::list<ns3::Ptr< ns3::ndn::ControlRouter > >::const_iterator node = m_graph.GetVertices ().begin ();
        node != m_graph.GetVertices ().end ();
        node++)        
  {	
    Ptr<ControlRouter> source = *node;
    if (source->GetId () == m_graph.GetControlId (sourceid)) //sourceid)
    {
      boost::DistancesMap1    distances;
      boost::PredecessorsMap1  predecessors;

      NS_LOG_DEBUG ("Source id, graph size, edge size: " << source->GetId () 
        << ", " << m_graph.GetSize() << ", " << m_graph.GetEdgeSize());
      boost::dijkstra_shortest_paths (m_graph, source,
      predecessor_map (boost::ref(predecessors))
      .
      distance_map (boost::ref(distances))
      .
      distance_inf (boost::WeightInf)
      .
      distance_zero (boost::WeightZero)
      .
      distance_compare (boost::WeightCompare ())
      .
      distance_combine (boost::WeightCombine ())
      );


      uint32_t nexthop = m_graph.GetControlId (targetid);
      NS_LOG_DEBUG ("Node ids: source node: " << sourceid << " to target node: " << targetid);
      NS_LOG_DEBUG ("Control ids: source node: " << source->GetId () << " to target node: " << nexthop);
      NS_LOG_DEBUG ("Predecessors size: " << predecessors.size () 
        <<  ", Distances size: " << distances.size () 
        << ", number of nodes: "  << m_graph.GetSize ());

      boost::PredecessorsMap1::iterator j = predecessors.begin ();
      bool foundpath = true;
      uint32_t s, i = 0, k = 0;
      s = predecessors.size ();
      path.clear();
      routerids.clear();
      std::pair<std::set<uint32_t>::iterator,bool> exist;
      while (foundpath)
      { 
           NS_LOG_DEBUG ("Test j: " << j->first->GetId () << " " << j->second->GetId ());
        if (j->first->GetId () == nexthop && (j->first->GetId () != j->second->GetId()))
        {
          exist = path.insert(nexthop);
          i++;
          NS_LOG_DEBUG ("Test nexthop: " << nexthop);
          if (exist.second)
          {
            NS_LOG_DEBUG ("Old next hop in reverse order: " <<  nexthop);
            routerids.push_back(nexthop);
            nexthop = j->second->GetId ();
            NS_LOG_DEBUG ("New next hop in reverse order: " <<  nexthop);
          }            
        }
        
        if (nexthop == m_graph.GetControlId (sourceid)) //sourceid)
        {
          NS_LOG_DEBUG ("Next hop in reverse order: " <<  nexthop);
          routerids.push_back(nexthop);
          path.insert(nexthop);
          NS_LOG_DEBUG ("Path complete!");
          foundpath = false;
        }
        j++;
        k++;
        if ((k > (s * s)) or (i > s ))
        {
          NS_LOG_DEBUG ("Path not found.");
          break;
        }
        if (j == predecessors.end())
        {
          j = predecessors.begin ();
        }
      }
      break;    
    }
  }
  routerids.reverse();
  for (std::list<uint32_t>::iterator it = routerids.begin(); it != routerids.end(); ++it)
  {
    pathname->appendSeqNum (m_graph.GetNodeId(*it));
  }
  return pathname;
}


// NLSRStrategy added methods

bool
NLSRStrategy::AddLSA (Name name, Name content)
{
//name  /routerid/LSA.type1/version
  //content      /neighbour1/neighbour2/.../neighbourn

//name  /routerid/LSA.type2/LSA.id/version
  //content
//      /isvalid/nameprefix

  bool isnew = false;
  uint32_t sz = name.size();
  uint32_t csz = content.size();
  Name prefix = name.getPrefix (sz - 1);
  LSAContainer::index<i_prefix>::type::iterator LSAentry = m_LSAs.get<i_prefix> ().find (prefix);
  uint32_t fullid = NameHash (name.toUri()); 
  if (LSAentry != m_LSAs.get<i_prefix> ().end()) 
  {
    if (name.get(1).toNumber() == 1)
    {
      if (name.get(sz-1).toNumber() > LSAentry->name.get(sz-1).toNumber())
      {
        isnew = true;
        //Hash tree update
        EraseFromHashtree (LSAentry->name);
        //
        m_LSAs.get<i_prefix> ().erase (LSAentry);
      }
      else
      {
        NS_LOG_LOGIC ("LSA already in database!: " << name << " content: " << content
          << "\nLSA local: " << LSAentry->name << " content: " << LSAentry->content);
      }
    }
    else if (name.get(1).toNumber() == 2)
    {
      if (name.get(sz-1).toNumber() > LSAentry->name.get(sz-1).toNumber()
        && content.getPrefix(csz-1,1).toUri() == LSAentry->content.getPrefix(csz-1,1).toUri())
      {
        isnew = true;
        //Hash tree update
        EraseFromHashtree (LSAentry->name);
        //        
        m_LSAs.get<i_prefix> ().erase (LSAentry);
      }
      else
      { 
        //LSA type 2 content hash colision case
        if (content.getPrefix(csz-1,1).toUri() != LSAentry->content.getPrefix(csz-1,1).toUri())
        {
          NS_LOG_DEBUG ("Same prefix, different LSA content. Name: " << name.toUri() << ", " 
            << LSAentry->name.toUri() << ", content: " << content.toUri() << ", " << LSAentry->content.toUri());
          isnew = true;
        }
        /*
        while (LSAentry != m_LSAs.get<i_prefix> ().end())
        {
          if (LSAentry->prefix.toUri() == prefix.toUri()
            && content.getPrefix(csz-1,1).toUri() == LSAentry->content.getPrefix(csz-1,1).toUri())
          {
            isnew = false;  //LSA already in LSDB          
            break;
            
          }
          //else
          //  NS_LOG_UNCOND ("Really new LSA: " << name.toUri() << ", " << LSAentry->name.toUri());
          LSAentry++;
        }
        */
      }
    }
  }
  else
  {
    isnew = true;
  }
  if (isnew)
  {
    m_LSAs.insert (LSA (name, prefix, content));
    if (name.get(1).toNumber() == 1)
    {
      NS_LOG_DEBUG ("New topo LSA name: " << name.toUri() << ", content: " << content.toUri());
      uint32_t node1 = name.get(0).toNumber();
      AddRegisteredRouter (node1);
      uint32_t node2;
      RemoveEdges (node1);
      for (Name::iterator it = content.begin(); it != content.end(); it++)
      {
          node2 = boost::lexical_cast<uint64_t>(it->toNumber());
          AddRegisteredRouter (node2);        
          AddGraphEdges (node1, node2, 1);
          NS_LOG_LOGIC (" New edges: " << node1 << ", " << node2);
      }     
    }
    if (name.get(1).toNumber() == 2)
    {
      NS_LOG_DEBUG (" New procuder LSA name: " << name.toUri() << ", content: " << content.toUri());
      uint32_t node1 = name.get(0).toNumber();
      Name regname = content.getPrefix(content.size()-1, 1);
      if (content.get(0).toNumber() == 1)
      {
        RegisterContent (regname, node1, 0);
        NS_LOG_DEBUG ("New content: " << node1 << ", " << regname);
      }
      else if (content.get(0).toNumber() == 2)
      {
		RegisterContent (regname, node1, 1);
        NS_LOG_DEBUG ("New content: " << node1 << ", " << regname);
      }	  
    }
    //Hash tree update
    AddToHashTree (name);
    //
  } 
  return isnew;
} 

void
NLSRStrategy::EraseFromHashtree (Name name)
{
  uint32_t sz = name.size();
  uint32_t fullid = NameHash (name.toUri()); 
  TreeContainer::index<i_prefix>::type::iterator entry = m_hashtree.get<i_prefix> ().find (name);
  if ( entry != m_hashtree.get<i_prefix> ().end () )
  {
    m_hashtree.get<i_prefix> ().erase (name);
    for (uint32_t i = 1; i <= sz; i++)
    {
      name::Component component = name.get (sz-i);
      std::map<name::Component, uint32_t> comphash;
      Name prefix;
      if (i != sz)
        prefix = name.getPrefix (sz-i);
      else
        prefix = Name("/");   
      entry = m_hashtree.get<i_prefix> ().find (prefix);     
      if (entry != m_hashtree.get<i_prefix> ().end ())
      {
        TreeEntry treeentry = *entry; 
        //comphash = treeentry.compset;
        Name branchname = Name(prefix);
        branchname.append(component);
        TreeContainer::index<i_prefix>::type::iterator branch = m_hashtree.get<i_prefix> ().find (branchname);
        bool erasedbranch = false;
        if (branch != m_hashtree.get<i_prefix> ().end ())
          treeentry.compset[component] = branch->hash;
        else if (treeentry.compset.find(component)  != treeentry.compset.end())
        {
          treeentry.compset.erase(component);
          if (treeentry.compset.size () == 0)
          {
            m_hashtree.get<i_prefix> ().erase (prefix);
            erasedbranch = true;
          }
            
          //erasedcomp = true;
        }
        //if ((!erasedcomp and entry->hash - fullid > 0) or (erasedcomp and entry->hash - fullid - NameHash(branchname) > 0))
        if (!erasedbranch)
        {
          if (treeentry.hash - fullid > 0)
          {
          //if (erasedcomp)
          //  treeentry.hash = treeentry.hash - fullid - NameHash(branchname);
          //else
            treeentry.hash = treeentry.hash - fullid;
          }
          else
          {
            uint32_t hashsum = 0;
            for (std::map<name::Component, uint32_t>::const_iterator it = treeentry.compset.begin(); 
              it != treeentry.compset.end(); it++)
            {
              hashsum = hashsum + it->second;
            }
          //hashsum = hashsum + NameHash(prefix);
            treeentry.hash = hashsum;
          }
        //treeentry.compset = comphash;
          m_hashtree.replace (entry,treeentry);
        }
      }
    }        
  }
}

void
NLSRStrategy::AddToHashTree (Name name)
{
  uint32_t sz = name.size();
  uint32_t fullid = NameHash (name.toUri());
  TreeContainer::index<i_prefix>::type::iterator entry = m_hashtree.get<i_prefix> ().find (name);
  std::map<name::Component, uint32_t> comphash;
  if ( entry == m_hashtree.get<i_prefix> ().end () )
  {
    NS_LOG_DEBUG ("New name: " << name.toUri());
    comphash[name::Component("empty")] = fullid;
    m_hashtree.insert(TreeEntry (name, fullid, comphash)); 
    for (uint32_t i = 1; i <= sz; i++)
    {
      name::Component component = name.get (sz-i);
      comphash.clear();
      Name prefix;
      if (i != sz)
        prefix = name.getPrefix (sz-i);
      else
        prefix = Name("/");   
      entry = m_hashtree.get<i_prefix> ().find (prefix);     
      if (entry != m_hashtree.get<i_prefix> ().end ())
      {
        NS_LOG_DEBUG ("Update prefix: " << prefix.toUri());
        TreeEntry treeentry = *entry; 
        treeentry.hash = treeentry.hash + fullid;
        comphash = treeentry.compset;
        if (comphash.find(component) != comphash.end())
          comphash[component] = comphash[component] + fullid;
        else
          comphash[component] = fullid;
        treeentry.compset = comphash;
        NS_LOG_DEBUG ("Before update: " << prefix.toUri() << ", " <<
          entry->hash << ", " << entry->compset[component]);
        m_hashtree.replace (entry,treeentry);
        NS_LOG_DEBUG ("After update: " << entry->hash << ", " << entry->compset[component]);
      }
      else
      {
        NS_LOG_DEBUG ("New prefix: " << prefix.toUri());
        Name branchname = Name(prefix);
        branchname.append(component);
        TreeContainer::index<i_prefix>::type::iterator branch = m_hashtree.get<i_prefix> ().find (branchname);
        uint32_t hashsum = 0;
        if (branch != m_hashtree.get<i_prefix> ().end ())
        {
          hashsum = branch->hash;
          comphash[component] = branch->hash;
        }
        else
        {
          hashsum = fullid;
          comphash[component] = fullid;
        }
        m_hashtree.insert(TreeEntry (prefix, hashsum, comphash));
      }
    }
  }
}


uint32_t
NLSRStrategy::GetRootHash ()
{
  Name name = Name("/");
  TreeContainer::index<i_prefix>::type::iterator entry = m_hashtree.get<i_prefix> ().find (name);
  if (entry != m_hashtree.get<i_prefix> ().end())
    return entry->hash;
  else
    return 0;
}

Name
NLSRStrategy::GetBranchComponents (Name name)
{
  TreeContainer::index<i_prefix>::type::iterator entry = m_hashtree.get<i_prefix> ().find (name);
  Name content = Name("/");
  if (entry != m_hashtree.get<i_prefix> ().end())
    for (std::map<name::Component, uint32_t>::const_iterator it = entry->compset.begin();
      it != entry->compset.end(); it++)
    {
      content.append(it->first);
      content.appendNumber(it->second);
    }
  return content;
}

std::map<name::Component, uint32_t>
NLSRStrategy::GetBranchHashes (Name name)
{
  TreeContainer::index<i_prefix>::type::iterator entry = m_hashtree.get<i_prefix> ().find (name);
  if (entry != m_hashtree.get<i_prefix> ().end())
    return entry->compset;
  else 
  {
    std::map<name::Component, uint32_t> temp;
    return temp;
  }
}

Name
NLSRStrategy::GetLSA (Name name)
{
  LSAContainer::index<i_name>::type::iterator entry = m_LSAs.get<i_name> ().find (name);
  if (entry != m_LSAs.get<i_name> ().end())
    return entry->content;
  else
  {
    Name temp = Name("/");
    return temp;
  }
}

int32_t
NLSRStrategy::GetLSAVersion (Name prefix)
{
  LSAContainer::index<i_prefix>::type::iterator entry = m_LSAs.get<i_prefix> ().find (prefix);
  if (entry != m_LSAs.get<i_prefix> ().end())
    return entry->name.get(prefix.size()).toNumber();
  else
  {
    return -1;
  }
}

uint32_t
NLSRStrategy::NameHash (Name name)
{
  uint32_t ans = 0;
  ans = string_hash(name.toUri());
  return ans;
}

bool
NLSRStrategy::InstallRoute (Ptr<Face> inFace, Ptr<const Interest> interest)
{
  Name namex = interest->GetName ();
  NS_LOG_DEBUG ("Node asking route for name: " << namex);
  uint32_t sz = namex.size (); 
  uint32_t node1 = inFace->GetNode()->GetId();
  Ptr<Name> path = Create<Name> ();
  const Name answer = namex.getPrefix (sz-1);
  std::set<uint32_t> candidates = GetRegisteredContentNodes (answer);
  for (std::set<uint32_t>::iterator it1 = candidates.begin();
    it1 != candidates.end(); it1++)
  {
    NS_LOG_DEBUG ("Trying candidate number: " << *(it1));
        //For now we try only the shortest path candidate
    Ptr<Name> temp = GetRoutes2 (node1,*(it1)); //teste 28nov2015
    if (path->size () == 0)
      path = temp;
    else if (temp->size () > 0 && temp->size () < path->size ())
      path = temp;
  }    
        
  if (path->size() > 1)
  {
    Ptr<Name> wantedprefix = Create<Name> (answer);
    Name nexthop = Name ("/router");
    nexthop.append (path->get(1));
    Ptr<fib::Entry> fibentry = m_fib->Find(nexthop);
    if (fibentry != 0)
    {
      NS_LOG_DEBUG ("Next hop does exist!");
      ndn::fib::FaceMetric facemetric = fibentry->FindBestCandidate(0);
      Ptr<Face> facex = facemetric.GetFace();
      AddFibEntry (wantedprefix, facex, 0);
      return true;
    }
  }
  else
    NS_LOG_DEBUG ("No route for name: " << answer);
  return false;      
}
  
} // namespace fw
} // namespace ndn
} // namespace ns3
