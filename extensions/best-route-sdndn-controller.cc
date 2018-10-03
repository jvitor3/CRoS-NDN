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

//CRoS-NDN strategy

#include "best-route-sdndn-controller.h"
#include "ndn-sdndn-askroute.h"
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

NS_OBJECT_ENSURE_REGISTERED (BestRouteSDNDNController);

LogComponent BestRouteSDNDNController::g_log = LogComponent (BestRouteSDNDNController::GetLogName ().c_str ());

std::string
BestRouteSDNDNController::GetLogName ()
{
  return super::GetLogName ()+".BestRouteSDNDNController";
}


TypeId
BestRouteSDNDNController::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::BestRouteSDNDNController")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <ns3::ndn::fw::BestRouteSDNDNController> ()
    ;
  return tid;
}
    
BestRouteSDNDNController::BestRouteSDNDNController ()
{
  m_controller = Create<Name> ();
  m_newNeighbor = true;
  m_registeredRouter = false;
  m_registeredContent = false;
  m_topologyChangeActive = false;
  super::m_nacksEnabled = true;
  m_state = 0;
  m_ctlseq = 0;
  m_nocontroller = true;
}

BestRouteSDNDNController::~BestRouteSDNDNController ()
{
}

void
BestRouteSDNDNController::NotifyNewAggregate ()
{
  if (m_controller == 0)
    {
      m_controller = Create<Name> ();
    }

  super::NotifyNewAggregate ();
}

void
BestRouteSDNDNController::DoDispose ()
{

  m_controller = 0;

  super::DoDispose ();
}

/*******************************************
//Genereal Router node related methods
********************************************/

void
BestRouteSDNDNController::IncrementState()
{
  m_state++;
}
  
uint32_t
BestRouteSDNDNController::GetState()
{
  return m_state;
}

void
BestRouteSDNDNController::AddFibEntryCtl (Ptr<Name> name, Ptr<Face> inFace, uint32_t metric)
{
  Ptr<Fib>  fib  = inFace->GetNode()->GetObject<Fib> ();
  Ptr<fib::Entry> fibentry = fib->Add (name, inFace , metric);
  if (fibentry != 0)
    fibentry->UpdateStatus (inFace, fib::FaceMetric::NDN_FIB_GREEN); 
}


void BestRouteSDNDNController::SetController (Ptr<Name> controller)
{
   m_controller = controller;
}

Ptr<Name> BestRouteSDNDNController::GetController ()
{
   return m_controller;
}

void
BestRouteSDNDNController::RemoveNodeNeighbors(uint32_t id)
{
  if (m_nodeneighbors_set.find(id) != m_nodeneighbors_set.end())
    m_nodeneighbors_set.erase (m_nodeneighbors_set.find(id));
  m_nodeneighbors_list.remove (id);
}


void
BestRouteSDNDNController::AddNodeNeighbors(uint32_t id)
{
  std::pair<std::set<uint32_t>::iterator,bool> exist;
  exist = m_nodeneighbors_set.insert(id);
  if (exist.second)
  {
	m_nodeneighbors_list.push_back (id);
  }
}
  
std::set<uint32_t>
BestRouteSDNDNController::GetNodeNeighbors ()
{
        return m_nodeneighbors_set;
}


bool
BestRouteSDNDNController::GetNewNeighbor ()
{
  return m_newNeighbor;
}
  
void
BestRouteSDNDNController::SetNewNeighbor (bool flag)
{
  m_newNeighbor = flag;
}

bool
BestRouteSDNDNController::GetRegisteredRouter ()
{
  return m_registeredRouter;
}
  
void
BestRouteSDNDNController::SetRegisteredRouter (bool flag)
{
  m_registeredRouter = flag;
}

bool
BestRouteSDNDNController::GetRegisteredContent ()
{
  return m_registeredContent;
}
  
void
BestRouteSDNDNController::SetRegisteredContent (bool flag)
{
  m_registeredContent = flag;
}

bool
BestRouteSDNDNController::GetIsThereController ()
{
  return !m_nocontroller;
}

void
BestRouteSDNDNController::DidExhaustForwardingOptions (Ptr<Face> inFace,
                               Ptr<const Interest> header,
                               Ptr<pit::Entry> pitEntry)
{
  
  NS_LOG_INFO ("Name: " << header->GetName()); 
  super::DidExhaustForwardingOptions(inFace, header, pitEntry);
}

void
BestRouteSDNDNController::FailedToCreatePitEntry (Ptr<Face> inFace,
                          Ptr<const Interest> header)
{
  NS_LOG_INFO ("Name: "  << header->GetName());
  super::FailedToCreatePitEntry(inFace, header);
}


void
BestRouteSDNDNController::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
  Name pname = pitEntry->GetFibEntry()->GetPrefix();
  size_t size = pname.size();
  std::string c0;
  c0 = "";
  if (size > 0)
    c0 = pname.get(0).toUri ();
  if ((c0 != "controller") &&
    (c0 != "hello") &&
    (pname.toUri () != "/") &&
    (c0 != ""))
  {
	//super::WillEraseTimedOutPendingInterest (pitEntry);
    NS_LOG_DEBUG ("WillEraseTimedOutPendingInterest for " << pitEntry->GetPrefix ());
	for (pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();
		face != pitEntry->GetOutgoing ().end (); face ++)
	{
		// NS_LOG_DEBUG ("Face: " << face->m_face);
		pitEntry->GetFibEntry ()->UpdateStatus (face->m_face, fib::FaceMetric::NDN_FIB_RED);
	}
    RemoveFibEntry (Create<Name> (pitEntry->GetFibEntry()->GetPrefix()));
    if (c0 == "controllerX")
    {
      m_nocontroller = true;
    }  
    NS_LOG_DEBUG ("Fib Name removed: " << pname <<
      " \n Interest Name asked: " << pitEntry->GetPrefix());
  }
  //else
    m_timedOutInterests (pitEntry);
}

void
BestRouteSDNDNController::SatisfyPendingInterest (Ptr<Face> inFace,
                                            Ptr<const Data> data,
                                            Ptr<pit::Entry> pitEntry)
{

  NS_LOG_INFO ("Testing Satisfy. " << data->GetName ());
  //Add Fib entry pointing to the designated controller
  //based on monitoring controller responses
  // prefix /controller/seq
  //TODO avaliar alterações com múltiplos controladores
  //adicionar set de controladores
  //controlar membros do set
  //apenas um deve ser o ativo de forma consistente enquanto estiver respondendo.
  if (inFace != 0)
  {
    pitEntry->RemoveIncoming (inFace);
     
    Ptr<Name> nameWithSequence1 = Create<Name> (data->GetName ());
    if (nameWithSequence1->get(0).toUri () == "controller")
    {
      uint32_t ctlseq = nameWithSequence1->get(1).toNumber();
      if (ctlseq > m_ctlseq)
      {
      m_ctlseq = ctlseq;
      if (m_nocontroller)
        m_registeredRouter = false;
      m_nocontroller = false;    
      Ptr<Name> contrx = Create<Name> ();
      contrx->append ("controllerX");  
      Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
      Ptr<const Packet> payload;
      payload = data->GetPayload ();
      (payload->Copy ())->PeekHeader (*payheader);	
      Ptr<const Name> nameaux1 = Create<const Name> ();
      nameaux1 = payheader->GetPayloadPtr ();
      contrx->append (nameaux1->get(0));
      FwHopCountTag hopCountTag1;
      data->GetPayload ()->PeekPacketTag (hopCountTag1);
      m_fib->Remove (contrx);
      AddFibEntry (contrx, inFace, hopCountTag1.Get());
      }
    }
  }
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
BestRouteSDNDNController::DoPropagateInterest (Ptr<Face> inFace,
                       Ptr<const Interest> interest,
                       Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << interest->GetName ());
  Ptr<Name> nameWithSequence = Create<Name> (interest->GetName ());
  uint32_t namesize = nameWithSequence->size ();
  uint64_t nodeidint = boost::lexical_cast<uint64_t> ((inFace->GetNode())->GetId ());
  std::string nodeidstr;
  nodeidstr = boost::lexical_cast<std::string>(nodeidint);
  Ptr<Fib>  fib  = inFace->GetNode()->GetObject<Fib> ();
  FwHopCountTag hopCountTag1;

//Add Fib entries to one hop neighbors
//based on Interests from app consumer-nodeid (ConsumerNodeID)
//filtering Interests with prefix "hello" not coming from the local node.
// Interest /hello/nodeid/seq  
  if ((namesize > 2) && (interest->GetNack () <= 0) &&
        (nameWithSequence->get (0).toUri () == "hello"))
  {
        if (nameWithSequence->get (1).toNumber () != nodeidint)
        {
            NS_LOG_INFO ("SDNDN Node. Hello from: " << interest->GetName ());
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
        if (propagatedCount == 0 && nameWithSequence->get(0).toUri() == "controllerX")
        {
          m_nocontroller = true;
          NS_LOG_UNCOND ("No Controller");
        }
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
      if (nameWithSequence->get(0).toUri() != "hello" &&
        nameWithSequence->get(0).toUri() != "controller")
        {
           NS_LOG_INFO ("Not a hello nor a controller prefix.");
           break; // do only once
        }
      else if ( (nameWithSequence->get(0).toUri() == "hello") &&
        (pitEntry->GetFibEntry ()->GetPrefix ().size() > 1) &&
        (pitEntry->GetFibEntry ()->GetPrefix ().get(1).toNumber () != inFace->GetNode()->GetId()))
      {
        break;
      }        
    }

  //Installing route to next hop
  // I: /router/routerID/installRouterAndForward/hopids.../endRoute/routePreference/id/toPrefix/prefix/
  //Let AskRoute modify the interest for route installation in the next hop.
  if ( namesize > 4 && (nameWithSequence->get (0).toUri () == "router"))
  {
    if (nameWithSequence->get (2).toUri () == "installRouteAndForward" &&
      nameWithSequence->get (1).toNumber () != inFace->GetNode()->GetId())
    {
      NS_LOG_INFO ("Route installation interest " << *interest);
      Ptr<Name> wantedprefix = Create<ndn::Name> ();
      Ptr<Name> wantedcontent = Create<ndn::Name> ();
      Name nexthop;
      bool ishop = false;
      bool iswantedprefix = false;
      bool isfromprevioushop = false;
      uint32_t hops = 0;
      Ptr<pit::Entry> pitEntryInsRoute;
      for (Name::const_iterator it = nameWithSequence->begin ();
        it != nameWithSequence->end(); it++)
      {
        if (it->toUri () == "installRouteAndForward" )
          ishop = true;
        if (it->toUri () == "endRoute" )
          ishop = false;
        if (ishop)
        {
          if (it->toUri () != "router" && it->toUri () != "installRouteAndForward" 
            && it->toNumber () != boost::lexical_cast<uint64_t> (inFace->GetNode()->GetId()))
          {
            hops++;
            if (hops == 1)
            {
              nexthop.append ("router");
              nexthop.appendSeqNum (it->toNumber());               
            }
          }
        }
        if (it->toUri () == "toPrefix" )
          iswantedprefix = true;
        if (iswantedprefix && it->toUri () != "toPrefix" )
          wantedcontent->append(*it);
      }
      wantedprefix = Create<Name> (wantedcontent->getPrefix(wantedcontent->size()-1));
      if (hops > 0)
      {
        NS_LOG_DEBUG (" Searching next hop for name: " << nexthop.toUri());
        Ptr<fib::Entry> fibentry = m_fib->Find(nexthop);
        if (fibentry != 0)
        {
          NS_LOG_DEBUG ("Next hop does exist!");
          ndn::fib::FaceMetric facemetric = fibentry->FindBestCandidate(0);
          Ptr<Face> facex = facemetric.GetFace();
          AddFibEntry  (wantedprefix, facex, 0);
        }else
        {
          //Send NACK on askroute app
          NS_LOG_INFO ("Next hop does NOT exist!");
        }
      }
    }
  }       
  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
  return propagatedCount > 0;
}


/*******************************************
//Genereal Controller node related methods
********************************************/
 
void
BestRouteSDNDNController::AddRegisteredRouter (uint32_t id)
{
  std::pair<std::set<uint32_t>::iterator,bool> exist;
  m_topologyChangeActive = true;
  exist = m_registeredrouters_set.insert(id);
  if (exist.second)
  {
	m_registeredrouters_list.push_back (id);
  }
  m_registeredrouters_set2.insert(id);
  m_topologyChangeActive = false;
  SetRouterNeighborOk (id, false);
}

void
BestRouteSDNDNController::RemoveRegisteredRouter(uint32_t id)
{
  if (m_registeredrouters_set.find(id) != m_registeredrouters_set.end())
  {
    m_topologyChangeActive = true;
    m_registeredrouters_set2.erase(id);
    m_topologyChangeActive = false;
  }
}

std::set<uint32_t>
BestRouteSDNDNController::GetRegisteredRouters ()
{
	return m_registeredrouters_set2;
}

void
BestRouteSDNDNController::RemoveEdges (uint32_t id)
{
  if (m_registeredrouters_set.find (id) != m_registeredrouters_set.end () &&
   m_edgemap.find(id) != m_edgemap.end())
  {
    m_topologyChangeActive = true;
    m_edgemap.erase(m_edgemap.find(id));
    m_topologyChangeActive = false;
  }
}

//Called only on controller to add adjacencies
void
BestRouteSDNDNController::AddGraphEdges (uint32_t id1, uint32_t id2, uint32_t face)
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
     m_topologyChangeActive = false;
  }
}


bool
BestRouteSDNDNController::GetRouterNeighborOk (uint32_t id)
{
  if (m_routerneighborok.find(id) != m_routerneighborok.end ())
    return m_routerneighborok.find(id)->second;
  else
    return false;
}
  
void
BestRouteSDNDNController::SetRouterNeighborOk (uint32_t id, bool flag)
{
  if (m_routerneighborok.find(id) != m_routerneighborok.end ())
    m_routerneighborok.find(id)->second = flag;
  else
    m_routerneighborok[id] = flag;
}

void
BestRouteSDNDNController::RegisterContent (const Name &name, uint32_t routerid, uint32_t replace)
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
BestRouteSDNDNController::UnRegisterContent (const Name &name, uint32_t routerid)
{
  //Do nothing for now
}
  
std::set<uint32_t>
BestRouteSDNDNController::GetRegisteredContentNodes (const Name &name)
{
  std::set<uint32_t> test;
  if (m_registered_content.find(name) != m_registered_content.end())
    test = m_registered_content.find(name)->second;
  return test;
}

Ptr<Name>
BestRouteSDNDNController::GetRoutes (uint32_t sourceid, uint32_t targetid)
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
    if (source->GetId () == graph1.GetControlId (sourceid))
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


} // namespace fw
} // namespace ndn
} // namespace ns3
