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

//CRoS-NDN strategy

#include "crosndn-strategy.h"
//#include "crosndn-route-consumer.h"
#include "crosndn-hello-producer.h"
#include <ns3/ndnSIM/model/ndn-data.h>
#include <ns3/ndnSIM/model/ndn-interest.h>
#include <ns3/ndnSIM/model/pit/ndn-pit.h>
#include <ns3/ndnSIM/model/pit/ndn-pit-entry.h>
#include <ns3/ndnSIM/model/pit/ndn-pit-entry-incoming-face.h>
#include <ns3/ndnSIM/model/cs/ndn-content-store.h>
#include <ns3/ndnSIM/model/fib/ndn-fib-entry.h>
#include <ns3/ndnSIM/model/fib/ndn-fib.h>
#include <ns3/ndnSIM/model/fib/ndn-fib-impl.h>
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

NS_OBJECT_ENSURE_REGISTERED (CRoSNDNStrategy);

LogComponent CRoSNDNStrategy::g_log = LogComponent (CRoSNDNStrategy::GetLogName ().c_str ());

std::string
CRoSNDNStrategy::GetLogName ()
{
  return super::GetLogName ()+".CRoSNDNStrategy";
}


TypeId
CRoSNDNStrategy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::CRoSNDNStrategy")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <ns3::ndn::fw::CRoSNDNStrategy> ()
    ;
  return tid;
}
    
CRoSNDNStrategy::CRoSNDNStrategy ()
{
  m_controller = Create<Name> ();
//  m_newNeighbor = true;
//  m_registeredRouter = false;
//  m_registeredContent = false;
  m_topologyChangeActive = false;
  super::m_nacksEnabled = true;
  m_state = 0;
  m_ctlseq = 0;
  m_nocontroller = true;
  boost::NdnControlRouterGraph m_graph = boost::NdnControlRouterGraph ();
}

CRoSNDNStrategy::~CRoSNDNStrategy ()
{
}

void
CRoSNDNStrategy::NotifyNewAggregate ()
{
  if (m_controller == 0)
    {
      m_controller = Create<Name> ();
    }
  super::NotifyNewAggregate ();
}

void
CRoSNDNStrategy::DoDispose ()
{

  m_controller = 0;

  super::DoDispose ();
}

/*******************************************
//Genereal Router node related methods
********************************************/

void
CRoSNDNStrategy::RouteInstall(Ptr<Name> prefix, Ptr<Face> face, uint32_t metric)
{
	//NS_LOG_UNCOND("TESTE SEM TUNEL");
	AddFibEntry  (prefix->getPrefix(prefix->size()-2), face, metric);
}	  

void
CRoSNDNStrategy::SetState(uint32_t state)
{
  m_state = state;
}
  
uint32_t
CRoSNDNStrategy::GetState()
{
  return m_state;
}

/*
void
CRoSNDNStrategy::AddFibEntryCtl (Ptr<Name> name, Ptr<Face> inFace, uint32_t metric)
{
  Ptr<Fib>  fib  = inFace->GetNode()->GetObject<Fib> ();
  Ptr<fib::Entry> fibentry = fib->Add (name, inFace , metric);
  if (fibentry != 0)
    fibentry->UpdateStatus (inFace, fib::FaceMetric::NDN_FIB_GREEN); 
}
*/


void CRoSNDNStrategy::SetController (Ptr<Name> controller)
{
   m_controller = controller;
}

Ptr<Name> CRoSNDNStrategy::GetController ()
{
   return m_controller;
}

void
CRoSNDNStrategy::RemoveNodeNeighbors(uint32_t id)
{
  if (m_nodeneighbors_set.find(id) != m_nodeneighbors_set.end())
    m_nodeneighbors_set.erase (m_nodeneighbors_set.find(id));
//  m_nodeneighbors_list.remove (id);
}


void
CRoSNDNStrategy::AddNodeNeighbors(uint32_t id)
{
  m_nodeneighbors_set.insert(id);
//  std::pair<std::set<uint32_t>::iterator,bool> exist;
//  exist = m_nodeneighbors_set.insert(id);
/*  if (exist.second)
  {
	m_nodeneighbors_list.push_back (id);
  }
*/  
}
  
CRoSNDNStrategy::NodeSet
CRoSNDNStrategy::GetNodeNeighbors ()
{
        return m_nodeneighbors_set;
}

/*
bool
CRoSNDNStrategy::GetNewNeighbor ()
{
  return m_newNeighbor;
}
  
void
CRoSNDNStrategy::SetNewNeighbor (bool flag)
{
  m_newNeighbor = flag;
}

bool
CRoSNDNStrategy::GetRegisteredRouter ()
{
  return m_registeredRouter;
}
  
void
CRoSNDNStrategy::SetRegisteredRouter (bool flag)
{
  m_registeredRouter = flag;
}


bool
CRoSNDNStrategy::GetRegisteredContent ()
{
  return m_registeredContent;
}

  
void
CRoSNDNStrategy::SetRegisteredContent (bool flag)
{
  m_registeredContent = flag;
}
*/

bool
CRoSNDNStrategy::GetIsThereController ()
{
  return !m_nocontroller;
}

void
CRoSNDNStrategy::DidExhaustForwardingOptions (Ptr<Face> inFace,
                               Ptr<const Interest> header,
                               Ptr<pit::Entry> pitEntry)
{
  
  NS_LOG_INFO ("Name: " << header->GetName()); 
  //(inFace->GetNode()->GetObject<ns3::ndn::CRoSNDNHelloProducer> ())->TestCall();
  super::DidExhaustForwardingOptions(inFace, header, pitEntry);
}

void
CRoSNDNStrategy::FailedToCreatePitEntry (Ptr<Face> inFace,
                          Ptr<const Interest> header)
{
  NS_LOG_INFO ("Name: "  << header->GetName());
  //if (!m_nocontroller)
//  (inFace->GetNode()->GetObject<ns3::ndn::CRoSNDNHelloProducer> ())->AskRouteCall(header, m_controller);
  //m_helloproducer->TestCall();
  //Simulator::ScheduleNow (&ns3::ndn::CRoSNDNHelloProducer::TestCall, m_helloproducer);
  //Simulator::ScheduleNow (&ns3::ndn::CRoSNDNHelloProducer::AskRouteCall, m_helloproducer , header, m_controller);
  //m_helloproducer->AskRouteCall(header, m_controller);
  super::FailedToCreatePitEntry(inFace, header);
}


void
CRoSNDNStrategy::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
  Name pname = pitEntry->GetFibEntry()->GetPrefix();
  size_t size = pname.size();
  std::string c0;
  c0 = "";
  if (size > 0)
    c0 = pname.get(0).toUri ();
  if ((c0 != "controller") &&
    (c0 != "hello") &&
	(c0 != "NamedData") &&
    (pname.toUri () != "/") &&
    (c0 != ""))
  {
	//super::WillEraseTimedOutPendingInterest (pitEntry);
	//NS_LOG_UNCOND ("Time: " << Simulator::Now ().ToDouble (Time::S) << "\t WillEraseTimedOutPendingInterest for " << pitEntry->GetPrefix ()
	//  << ", Fib Name removed: " << pname);
    NS_LOG_DEBUG ("Interest name" << pitEntry->GetPrefix ());
	//bool tryagain = false;
	for (pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();
		face != pitEntry->GetOutgoing ().end (); face ++)
	{
		// NS_LOG_DEBUG ("Face: " << face->m_face);
		if (pitEntry->GetFibEntry ()->m_faces.size() != 0)
		{
			fib::FaceMetricContainer::type::index<fib::i_face> ::type::iterator record = (pitEntry->GetFibEntry ()->m_faces.get<fib::i_face> ()).find (face->m_face);
			if (record != (pitEntry->GetFibEntry ()->m_faces.get<fib::i_face> ()).end())
			{
				if (record->GetStatus () == fib::FaceMetric::NDN_FIB_GREEN)
					pitEntry->GetFibEntry ()->UpdateStatus (face->m_face, fib::FaceMetric::NDN_FIB_YELLOW);
				else if (record->GetStatus () == fib::FaceMetric::NDN_FIB_YELLOW)
				{
					pitEntry->GetFibEntry ()->UpdateStatus (face->m_face, fib::FaceMetric::NDN_FIB_RED);
				}
				else if (record->GetStatus () == fib::FaceMetric::NDN_FIB_RED)
				{
					pitEntry->GetFibEntry ()->RemoveFace (face->m_face);
				//	tryagain = true;
				}
			}
		}

	}
	if (pitEntry->GetFibEntry ()->m_faces.size() == 0)
	{
		RemoveFibEntry (Create<Name> (pname));
		Ptr<pit::Entry > entr = m_pit->Begin();
		Ptr<pit::Entry > entr2;
		while (entr != m_pit->End())
		//for (Ptr<pit::Entry > entr = m_pit->Begin(); entr != m_pit->End(); entr = m_pit->Next(entr))
		//for (uint32_t ientry = 0; ientry < m_pit->GetSize(); ientry++;)
		{
			entr2 = entr;
			entr = m_pit->Next(entr);
			if (entr2->GetFibEntry()->GetPrefix() == pname)
			{
				//m_pit->MarkErased(entr2);
				//entr2->UpdateLifetime (Seconds (0.0));
				entr2->OffsetLifetime (Seconds (-2.0));
			}

		}
		NS_LOG_DEBUG ("Fib Name removed: " << pname << " \n Interest Name asked: " << pitEntry->GetPrefix());
	//	while (m_pit->Find(pname) != 0)
	//		m_pit->MarkErased(m_pit->Find(pname));
	}
	
	//else if (tryagain)
	//{
	//	DoPropagateInterest (pitEntry->GetIncoming().begin ()->m_face, pitEntry->GetInterest(), pitEntry);
	//}
	
    if (c0 == "controller0")
    {
      m_nocontroller = true;
    }
    
  }
  /*
  if (c0 == "controller")
  {
	  pit::Entry::out_container::iterator face = pitEntry->GetOutgoing ().begin ();	  
	  NS_LOG_UNCOND (face->m_face->GetNode()->GetId() << "\t Controller Discovery Failure!");
  }
  */
  //else
  m_timedOutInterests (pitEntry);
}

void
CRoSNDNStrategy::SatisfyPendingInterest (Ptr<Face> inFace,
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
      if (ctlseq >= m_ctlseq)
      {
		  m_ctlseq = ctlseq;
		  SetState(ctlseq);
	//      if (m_nocontroller)
	//        m_registeredRouter = false;
		  m_nocontroller = false;    
		  Ptr<Name> contrx = Create<Name> ();
		  contrx->append ("controller0");  
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
	//02dez2015
	/*
	if (nameWithSequence1->get(0).toUri () == "controller0" and m_nocontroller)
	{
		m_nocontroller = false;
		Ptr<Name> contrx = Create<Name> ();
		contrx->append (nameWithSequence1->getPrefix(2,0));		
		FwHopCountTag hopCountTag1;
		data->GetPayload ()->PeekPacketTag (hopCountTag1);
		m_fib->Remove (contrx);
		AddFibEntry (contrx, inFace, hopCountTag1.Get());		
	}
	*/
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
CRoSNDNStrategy::DoPropagateInterest (Ptr<Face> inFace,
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
// Interest /hello/nodeid/controllerid/seq  
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
		/* tentativa de múltiplos controladores - provocou erro, rever.
		if (nameWithSequence->get(2).toUri () != "-1")
		{
			Ptr<Name> contrx = Create<Name> ();
			contrx->append ("controller0"); 
			contrx->append (nameWithSequence->get(2));
			AddFibEntry (contrx, inFace, 0); //hot potato in distance metric
		}
		*/
  }
 
  // Try to work out with just faces
  int propagatedCount = 0;
  NS_LOG_INFO("Loop for faces starting.");
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    {
      NS_LOG_INFO ("Trying face: " << boost::cref(metricFace) << ", for name: " << interest->GetName() 
	    << " with FIB prefix: " << pitEntry->GetFibEntry ()->GetPrefix());
      if (metricFace.GetStatus () == fib::FaceMetric::NDN_FIB_RED) // all non-read faces are in front
      {
        NS_LOG_INFO("No more faces.");
        if (propagatedCount == 0 && nameWithSequence->get(0).toUri() == "controller0")
        {
          m_nocontroller = true;
          NS_LOG_UNCOND ("No Controller");
        }
		if (propagatedCount == 0 and pitEntry->GetFibEntry ()->GetPrefix().toUri() != "/")
			RemoveFibEntry (Create<Name> (pitEntry->GetFibEntry ()->GetPrefix()));
        break;
      }

      if (!TrySendOutInterest (inFace, metricFace.GetFace (), interest, pitEntry))
        {
          NS_LOG_INFO("Failure. Trying next face.");
          continue;
        }
      NS_LOG_INFO("Success. Interest propagated. " << nameWithSequence->toUri());
      propagatedCount++;
      //Stopping just in case it is not a hello neither a controller Interest.
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
  //Let the Hello Producer reply and modify the interest for route installation to the next hop.
  if ( namesize > 4 && (nameWithSequence->get (0).toUri () == "router"))
  {
//    if (nameWithSequence->get (2).toUri () == "installRouteAndForward" &&
//      nameWithSequence->get (1).toNumber () != inFace->GetNode()->GetId()) //next hop
	if (nameWithSequence->get (2).toUri () == "installRouteAndForward")
    {
      NS_LOG_INFO ("Route installation interest " << *interest);
      Ptr<Name> wantedprefix = Create<ndn::Name> ();
      Ptr<Name> wantedcontent = Create<ndn::Name> ();
      Name nexthop;
      bool ishop = false;
      bool iswantedprefix = false;
      bool isfromprevioushop = false;
      uint32_t hops = 0;
      
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
	  
  	  Ptr<Interest> interestctt = Create<Interest> ();
	  //UniformVariable rand (0,std::numeric_limits<uint32_t>::max ());
	  interestctt->SetName (wantedcontent->getPrefix(wantedcontent->size()-1));
	  //interestctt->SetNonce (rand.GetValue ());
	  interestctt->SetNonce (interest->GetNonce ());
	  interestctt->SetInterestLifetime (Seconds (1.0));
	  FwHopCountTag hopCountTag;
	  if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
		interestctt->GetPayload ()->AddPacketTag (hopCountTag);	  
	  Ptr<pit::Entry> pitEntryInsRoute = m_pit->Lookup (*interestctt);

	  if (nameWithSequence->get (1).toNumber () != inFace->GetNode()->GetId())
	  {
		  
		wantedprefix = Create<Name> (wantedcontent->getPrefix(wantedcontent->size()-2));
		if (hops > 0)
		{
			NS_LOG_DEBUG (" Searching next hop for name: " << nexthop.toUri());
			Ptr<fib::Entry> fibentry = m_fib->Find(nexthop);
			if (fibentry != 0)
			{
			  NS_LOG_DEBUG ("Next hop does exist!");
			  ndn::fib::FaceMetric facemetric = fibentry->FindBestCandidate(0);
			  Ptr<Face> facex = facemetric.GetFace();
			  //AddFibEntry  (wantedprefix, facex, hops);
			  //RouteInstall (wantedprefix, facex, hops);
			  RouteInstall (wantedcontent, facex, hops);
			  //AddFibEntry  (wantedprefix, facex, 0);
			  
			  if (pitEntryInsRoute == 0)
				  pitEntryInsRoute = m_pit->Create (interestctt);
			  //pitEntryInsRoute->AddIncoming (inFace);
			  pitEntryInsRoute->AddOutgoing (facex);	
			  if (!pitEntryInsRoute->IsNonceSeen (interestctt->GetNonce ()))
			  {
				pitEntryInsRoute->AddSeenNonce (interestctt->GetNonce ());
				pitEntryInsRoute->UpdateLifetime (Seconds (1.0));
			  }
			  
			}else
			{
			  //Send NACK on askroute app
			  NS_LOG_INFO ("Next hop does NOT exist!");
			}
		}
	  
	  }else
	  {
		  if (hops == 0)	
		  {
			  //retirado comentário 11dez2015
			  
			  //Add PIT entry for the request content in route installation interest
			  //pitEntryInsRoute->AddIncoming (inFace);
			  //pitEntryInsRoute->AddOutgoing (facex);
			  if (pitEntryInsRoute == 0)
				  pitEntryInsRoute = m_pit->Create (interestctt);
			  PropagateInterest (inFace, interestctt, pitEntryInsRoute);
			  //pitEntryInsRoute->AddSeenNonce (interestctt->GetNonce ());
			  //pitEntryInsRoute->AddIncoming (inFace);
			  //pitEntryInsRoute->UpdateLifetime (Seconds (1.0));
			  //pitEntryInsRoute->UpdateLifetime (Seconds (interestctt->GetInterestLifetime().ToInteger(Time::S) * 2));
			  
			  //retirado comentário 11dez2015
		  }else
		  {
			if (pitEntryInsRoute == 0)
			{
				pitEntryInsRoute = m_pit->Create (interestctt);
				pitEntryInsRoute->AddSeenNonce (interestctt->GetNonce ());
				pitEntryInsRoute->AddIncoming (inFace);
				pitEntryInsRoute->UpdateLifetime (Seconds (1.0));					  
			}
			else
			{
				if (pitEntryInsRoute->GetFibEntry()->GetPrefix().toUri() == "/")
				{
					  
					Ptr<pit::Entry> pitEntryInsRoute2 = m_pit->Create (interestctt);
					for ( pit::Entry::in_container::iterator it = (pitEntryInsRoute->GetIncoming()).begin(); 
						it != (pitEntryInsRoute->GetIncoming()).end(); it++ )
					{
						pitEntryInsRoute2->AddIncoming ((*it).m_face);
					}
					pitEntryInsRoute2->AddSeenNonce (interestctt->GetNonce ());
					pitEntryInsRoute2->UpdateLifetime (Seconds (1.0));
					//m_pit->MarkErased(pitEntryInsRoute);
				}
				else
				{
					pitEntryInsRoute->AddIncoming (inFace);
					if (!pitEntryInsRoute->IsNonceSeen (interestctt->GetNonce ()))
					{
						pitEntryInsRoute->AddSeenNonce (interestctt->GetNonce ());
						pitEntryInsRoute->UpdateLifetime (Seconds (1.0));					
					}
				}
			}  
		  }
	  }
    }
  } 
	
      
  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
  return propagatedCount > 0;
}

void
CRoSNDNStrategy::WillSatisfyPendingInterest  (Ptr<Face> inFace, Ptr<pit::Entry> pitEntry)
{
	if (inFace != NULL)
		super::WillSatisfyPendingInterest (inFace, pitEntry);
	//else
	//	m_dropData (data, inFace);
}

/*
void
CRoSNDNStrategy::WillSatisfyPendingInterest  (Ptr<Face> inFace, Ptr<pit::Entry> pitEntry)
{
	if ((pitEntry->GetPrefix ().get(0).toUri() != "hello") and (pitEntry->GetPrefix ().get(0).toUri() != "router"))
		super::WillSatisfyPendingInterest (inFace, pitEntry);
	else if ((pitEntry->GetPrefix ().get(0).toUri() == "hello") and (pitEntry->GetPrefix ().get(1).toNumber() != inFace->GetNode()->GetId()))
		super::WillSatisfyPendingInterest (inFace, pitEntry);
	else if ((pitEntry->GetPrefix ().get(0).toUri() == "router") and (pitEntry->GetPrefix ().get(1).toNumber() != inFace->GetNode()->GetId()))
		super::WillSatisfyPendingInterest (inFace, pitEntry);	
	else
	{
		pit::Entry::out_iterator out = pitEntry->GetOutgoing ().find (inFace);

		// If we have sent interest for this data via this face, then update stats.
	//	if (out != pitEntry->GetOutgoing ().end ())
	//	{
	//		pitEntry->GetFibEntry ()->UpdateFaceRtt (inFace, Simulator::Now () - out->m_sendTime);
	//	}
	
	//NS_LOG_UNCOND("Testing: " << pitEntry->GetPrefix ());
	
		m_satisfiedInterests (pitEntry);
	}
}
*/

/*******************************************
//Genereal Controller node related methods
********************************************/
 
void
CRoSNDNStrategy::AddRegisteredRouter (uint32_t id)
{
//cros  std::pair<std::set<uint32_t>::iterator,bool> exist;
  m_topologyChangeActive = true;
//cros  exist = m_registeredrouters_set.insert(id);
//cros  if (exist.second)
//cros  {
//cros	m_registeredrouters_list.push_back (id);
//cros  }
//cros  m_registeredrouters_set2.insert(id);
  m_registeredrouters_set.insert(id);
  m_topologyChangeActive = false;
  SetRouterNeighborOk (id, false);
}

void
CRoSNDNStrategy::RemoveRegisteredRouter(uint32_t id)
{
  if (m_registeredrouters_set.find(id) != m_registeredrouters_set.end())
  {
    m_topologyChangeActive = true;
//cros    m_registeredrouters_set2.erase(id);
    m_registeredrouters_set.erase(id);
    m_topologyChangeActive = false;
  }
}

CRoSNDNStrategy::NodeSet
CRoSNDNStrategy::GetRegisteredRouters ()
{
	return m_registeredrouters_set;
}

void
CRoSNDNStrategy::RemoveEdges (uint32_t level, uint32_t id)
{
  EdgeMap edgemap;
  if (m_zonemap.find(level) != m_zonemap.end())
	edgemap = m_zonemap[level];
  
//  if (m_registeredrouters_set.find (id) != m_registeredrouters_set.end () &&
//   m_edgemap.find(id) != m_edgemap.end())
  if (edgemap.find(id) !=  edgemap.end())
  {
    m_topologyChangeActive = true;
    edgemap.erase(edgemap.find(id));
	m_zonemap[level] = edgemap;
	
	//m_graph.RemoveEdges(id);
	m_isTopologyViewUpdated = false;
	
    m_topologyChangeActive = false;
  }
  
}

//Called only on controller to add adjacencies
void
CRoSNDNStrategy::AddGraphEdges (uint32_t level, uint32_t id1, uint32_t id2, uint32_t face)
{
  EdgeMap edgemap;
  if (m_zonemap.find(level) != m_zonemap.end())
	edgemap = m_zonemap[level];
  if (m_registeredrouters_set.find (id1) != m_registeredrouters_set.end ()
    && m_registeredrouters_set.find (id2) != m_registeredrouters_set.end ())
  {
     m_topologyChangeActive = true;
     if (edgemap.find(id1) != edgemap.end())
     {
       EdgeSet ed = (edgemap.find(id1))->second;
       ed.insert(Edge (id2, face));
       edgemap[id1] = ed;
     }
     else
     {
       EdgeSet temp;
       temp.insert(Edge (id2, face));
       edgemap[id1] = temp;
     }
	 m_zonemap[level] = edgemap;
	 
	 /*
	 if (edgemap.find(id2) != edgemap.end())
	 {
		 EdgeSet edgeset2 = (edgemap.find(id2))->second;
		 if (edgeset2.find(Edge (id1, face)) != edgeset2.end())  //there is a bidirectional edge
			m_graph.AddEdge(id1, id2, face); //teste 28nov2015
	 }
	 */
	 m_isTopologyViewUpdated = false;
	 
     m_topologyChangeActive = false;
  }
}


bool
CRoSNDNStrategy::GetRouterNeighborOk (uint32_t id)
{
  if (m_routerneighborok.find(id) != m_routerneighborok.end ())
    return m_routerneighborok.find(id)->second;
  else
    return false;
}
  
void
CRoSNDNStrategy::SetRouterNeighborOk (uint32_t id, bool flag)
{
  if (m_routerneighborok.find(id) != m_routerneighborok.end ())
    m_routerneighborok.find(id)->second = flag;
  else
    m_routerneighborok[id] = flag;
}

void
CRoSNDNStrategy::RegisterContent (const Name &name, uint32_t routerid, uint32_t replace)
{
  if (m_registered_content.find(name) != m_registered_content.end() and not replace)
	m_registered_content.find(name)->second.insert(routerid);
  else
  {
    NodeSet temp;
    temp.insert (routerid);
    m_registered_content[name] = temp;
  }
}
  
void
CRoSNDNStrategy::UnRegisterContent (const Name &name, uint32_t routerid)
{
  //Do nothing for now
}
  
CRoSNDNStrategy::NodeSet
CRoSNDNStrategy::GetRegisteredContentNodes (const Name &name)
{
  NodeSet test;
  Name tempname;
  uint32_t nsize = name.size();
  for (uint32_t i = 0; i < nsize; i++)
  {
	tempname = name.getPrefix(nsize-i);
	if (m_registered_content.find(tempname) != m_registered_content.end())
	{
		test = m_registered_content.find(tempname)->second;
		break;
	}
	
  }
  return test;
  /*
  if (m_registered_content.find(name) != m_registered_content.end())
    test = m_registered_content.find(name)->second;
  return test;
  */
}

Ptr<Name>
CRoSNDNStrategy::GetRoutes (uint32_t level, uint32_t sourceid, uint32_t targetid)
{
  NS_LOG_DEBUG ("\n GetRoutes - source, target: " << sourceid << ", " << targetid << "\n"); 
  typedef graph_traits < boost::NdnControlRouterGraph >::vertex_descriptor vertex_descriptor;
  Ptr<Name> pathname = Create<Name> ();
  NodeSet path;
  std::list<uint32_t> routerids;
  EdgeMap edgemap;
  if (m_zonemap.find(level) != m_zonemap.end())
	edgemap = m_zonemap[level];
  else
	return pathname;
  if (m_registeredrouters_set.find (sourceid) == m_registeredrouters_set.end ()
    || m_registeredrouters_set.find (targetid) == m_registeredrouters_set.end ()
    || edgemap.size() == 0//)
    || m_topologyChangeActive)
  {
    NS_LOG_DEBUG ("Path not found. Source or target not registered.");
    return pathname;
  }
  boost::NdnControlRouterGraph graph1 = boost::NdnControlRouterGraph ();
  for (NodeSet::const_iterator v = m_registeredrouters_set.begin();
    v != m_registeredrouters_set.end(); v++)
  {
    graph1.AddVertices(*v);
    NS_LOG_DEBUG ("Vertices. Node id: " << *v << ", Control id: " << graph1.GetControlId (*v));
  }
  for (EdgeMap::const_iterator e = edgemap.begin();
    e != edgemap.end(); e++)
  {
    for (EdgeSet::const_iterator inc = (e->second).begin();
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

Ptr<Name>
CRoSNDNStrategy::GetRoutes2 (uint32_t level, uint32_t sourceid, uint32_t targetid)
{
  NS_LOG_DEBUG ("\n GetRoutes - source, target: " << sourceid << ", " << targetid << "\n"); 
  typedef graph_traits < boost::NdnControlRouterGraph >::vertex_descriptor vertex_descriptor;
  Ptr<Name> pathname = Create<Name> ();
  std::set<uint32_t> path;
  std::list<uint32_t> routerids;
  if (m_registeredrouters_set.find (sourceid) == m_registeredrouters_set.end ()
    || m_registeredrouters_set.find (targetid) == m_registeredrouters_set.end ()
    || m_zonemap[level].size() == 0//)
    || m_topologyChangeActive)
  {
    NS_LOG_DEBUG ("Path not found. Source ou target not registered.");
    return pathname;
  }
  
  if (!m_isTopologyViewUpdated)
	UpdateTopologyView (level);  
   
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

void CRoSNDNStrategy::UpdateTopologyView (uint32_t level)
{
  m_isTopologyViewUpdated = true;
  boost::NdnControlRouterGraph graphtemp = boost::NdnControlRouterGraph ();
  for (NodeSet::const_iterator v = m_registeredrouters_set.begin();
    v != m_registeredrouters_set.end(); v++)
  {
    graphtemp.AddVertices(*v);
    NS_LOG_DEBUG ("Vertices. Node id: " << *v << ", Control id: " << graphtemp.GetControlId (*v));
  }
  
  EdgeMap edgemap;
  if (m_zonemap.find(level) != m_zonemap.end())
	edgemap = m_zonemap[level];

  for (EdgeMap::const_iterator e = edgemap.begin();
    e != edgemap.end(); e++)
  {
    for (EdgeSet::const_iterator inc = (e->second).begin();
      inc != (e->second).end(); inc++)
      {
        graphtemp.AddEdge(e->first, inc->first, inc->second);
		/*
		if ((edgemap.find(e->first) != edgemap.end()) and (edgemap.find(inc->first) != edgemap.end()))
		{
			EdgeSet edgeset1 = (edgemap.find(e->first))->second;
			EdgeSet edgeset2 = (edgemap.find(inc->first))->second;
			uint32_t face = 1;
			//there is a bidirectional edge
			if ((edgeset1.find(Edge (inc->first, face)) != edgeset1.end()) and
				(edgeset2.find(Edge (e->first, face)) != edgeset2.end()))
			{
				graphtemp.AddEdge(e->first, inc->first, face); //teste 28nov2015
				NS_LOG_DEBUG ("Edges. Router id1, id2 and face : " << e->first << ", " << inc->first << ", " << inc->second);
			}
		}
		*/
        NS_LOG_DEBUG ("Edges. Router id1, id2 and face : " << e->first << ", " 
          << inc->first << ", " << inc->second);
      }
  }
  m_graph = graphtemp;
}

} // namespace fw
} // namespace ndn
} // namespace ns3
