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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  0211110001307  USA
 *
 * Author: João Vitor Torres <jvitor@gta.ufrj.br>
 */
 
//This app receives all interest without a better fib match than "/".
//Then it asks the controller for a new route with the
//prefix: /controller0/controllerId/routeFrom/sourceRouterID/wantedprefix/seq
// 
//When the controller answer arrives, the app adds a new fib entry and it sends a new interest with the
//	prefix: /route/installRoute/sourcerouterId/nextRouterId/.../destinationRouterId/endRoute
//			/routePreference/x/toPrefix/wantedprefix/seq
//		/route/installRouteAndForward/sourcerouterId/nextRouterId/.../destinationRouterId
//			/endRoute/routePreference/x/toPrefix/wantedprefix/seq
//When an interest with the prefix above arrives, 
//the app also adds a new fib entry and it creates a new interest removing its nodeid from de sequence.

//This app also:
//- monitors node neighbors keepalives hello interests
//- updates the registration on controller upon neighboring changes
//- searchs the controller upon requests to controller expiration without response
//This app listen to interest /hello/consumernodeid/controllerid/seq
//If nodeid is a new neighbor, then it update the local state, it searchs the controller, and it registers the new neighbors.
//Controller searching prefix is /controller/seq 
//Register prefix is 
  //Interest /controller0/ControllerID/registerRouter/nodeID/flag/
  //			neighborlistsize/neighbour1/.../neighbourN/
  //			zonelistsize/ctl1/.../ctln/seqnumber  

#include "ns3/log.h"
#include "crosndn-tunnel-hello-producer.h"
#include "ndn-payload-header.h"
#include <boost/lexical_cast.hpp>
#include <ns3/ndnSIM/model/ndn-interest.h>
#include <ns3/ndnSIM/ndn.cxx/name-component.h>
#include <ns3/ndnSIM/helper/ndn-app-helper.h>
#include <ns3/ndnSIM/model/ndn-l3-protocol.h>
#include "ns3/ndnSIM/utils/ndn-rtt-mean-deviation.h"
#include "ns3/ndn-fib.h"

NS_LOG_COMPONENT_DEFINE ("ndn.CRoSNDNTunnelHelloProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CRoSNDNTunnelHelloProducer);
   
TypeId
CRoSNDNTunnelHelloProducer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::CRoSNDNTunnelHelloProducer")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<CRoSNDNTunnelHelloProducer> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/"),
                   MakeNameAccessor (&CRoSNDNTunnelHelloProducer::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&CRoSNDNTunnelHelloProducer::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&CRoSNDNTunnelHelloProducer::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&CRoSNDNTunnelHelloProducer::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CRoSNDNTunnelHelloProducer::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&CRoSNDNTunnelHelloProducer::m_keyLocator),
                   MakeNameChecker ())
    .AddAttribute ("Frequency", "Frequency of interest packets",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&CRoSNDNTunnelHelloProducer::m_frequency),
                   MakeDoubleChecker<double> ())
				   
    ;
  return tid;
}
  
  
CRoSNDNTunnelHelloProducer::CRoSNDNTunnelHelloProducer ()
: m_isRunning (false)//, l_face(m_face)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rtt = CreateObject<RttMeanDeviation> ();
  m_seqi = 0;
}

// inherited from Application base class.
void
CRoSNDNTunnelHelloProducer::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication (); 
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
  m_isRunning = true;
  m_seqreg = 0;
  m_seq = 0;
  m_interestLifeTime = Seconds (0.9);
  m_newneighbor = false;
  m_fw = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNTunnelStrategy> ());
  RefreshFib();
  SearchController ();
}


void
CRoSNDNTunnelHelloProducer::RefreshFib ()
{
	//m_prefix = Name("/hello");
	//m_prefix.appendSeqNum (GetNode()->GetId());
	Ptr<Name> prefix = Create<Name> (m_prefix);
	m_fw->AddFibEntry (prefix, m_face, 100);
	//m_prefix = Name("/router");
	//m_prefix.appendSeqNum (GetNode()->GetId());
	//(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())
	//	->AddFibEntry (m_prefix, m_face, 0);
		
  //Simulator::Schedule (Seconds (0.3), &CRoSNDNTunnelHelloProducer::RefreshFib, this);
  
}

void
CRoSNDNTunnelHelloProducer::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
  m_isRunning = false;
}

void
CRoSNDNTunnelHelloProducer::OnInterest (Ptr<const Interest> interest)
{

  App::OnInterest (interest); // tracing inside
  NS_LOG_FUNCTION (this << interest);
  if (!m_active) return;
  Name intname = interest->GetName ();
  FwHopCountTag hopCountTag;  

  // /hello/consumernodeid/controllerid/seq  
  if (intname.get(0).toUri() == "hello")
  {
  
	  uint32_t size = intname.size ();
	  uint64_t consid = intname.get(1).toNumber ();
	  uint64_t prodid = boost::lexical_cast<uint64_t> ((GetNode())->GetId ());
	  NS_LOG_INFO ("Received Interest Name! " << (intname.toUri ()));
	  if(prodid == consid)
	  {
		NS_LOG_INFO ("Do not answer my own interest! " << (intname.toUri ()));
		return;
	  }  
	  
	  /*
	  uint32_t localstate = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->GetState();
	  if (localstate < intname.get(3).toNumber())
		  (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->SetState(intname.get(3).toNumber());
	  */
	  
	  Ptr<Data> data = Create<Data> (Create<Packet> (m_virtualPayloadSize));
	  Ptr<Name> dataName = Create<Name> (interest->GetName ());
	  dataName->append (m_postfix);
	  data->SetName (dataName);
	  data->SetFreshness (m_freshness);
	  data->SetTimestamp (Simulator::Now());

	  data->SetSignature (m_signature);
	  if (m_keyLocator.size () > 0)
	  {
		data->SetKeyLocator (Create<Name> (m_keyLocator));
	  }

	  NS_LOG_INFO ("node("<< GetNode()->GetId() <<") respodning with Data: " << data->GetName ());

	  // Echo back FwHopCountTag if exists
	  FwHopCountTag hopCountTag;
	  if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
	  {
		data->GetPayload ()->AddPacketTag (hopCountTag);
	  }
	  m_face->ReceiveData (data);
	  m_transmittedDatas (data, this, m_face);  
	  std::set<uint32_t> neighbours = m_fw->GetNodeNeighbors ();
	  NS_LOG_INFO("Neighbors size: " << neighbours.size());
	  
	  Ptr<Name> ctlname =   m_fw->GetController ();
	  Name rctlname = intname.getSubName (2,1);
	  if (rctlname.toUri() != "-1" && (ctlname->size() <= 1 || rctlname != ctlname->getSubName(1,1)))
	  {
		m_neighborctl[consid] = rctlname;
	  }
	  
	  //bool newneighbor = false;
	  if (neighbours.find(consid) == neighbours.end())
	  {
		m_fw->AddNodeNeighbors (consid);
		//(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->IncrementState ();
		//(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->SetNewNeighbor (true);
		//SearchController ();
		m_newneighbor = true;
		NS_LOG_DEBUG ("New neighbor: " << consid);
	  }
	  Simulator::Cancel (m_events[consid]);
	  m_events[consid] = Simulator::Schedule ( Seconds((2.5 / m_frequency)),&CRoSNDNTunnelHelloProducer::OnTimeout, this, consid);
	  if ((!(m_fw->GetIsThereController ())))
		SearchController ();
	  else if (m_newneighbor)
		RegisterInController();  

  }
  else  //not hello interest
  {
	  UniformVariable rand (0,std::numeric_limits<uint32_t>::max ());
	  bool shouldAnswerNack  = AskRoute (interest);
	  
	  
	  if (shouldAnswerNack)
	  {
		  //NACK - moving fib entry "/" to a low preference in the pit entry for this specific name
		  NS_LOG_DEBUG("Send Nack to name: " << interest->GetName ());
		  Ptr<ndn::Interest> nack = Create<ndn::Interest> ();
		  nack->SetNack (Interest::NACK_GIVEUP_PIT);
		  nack->SetNonce (rand.GetValue ());
		  nack->SetName (interest->GetName ());
		  nack->SetInterestLifetime (Seconds (0.5));
		  // Create packet and add ndn::Interest
		  if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
			nack->GetPayload ()->AddPacketTag (hopCountTag);
		  // Forward packet to lower (network) layer
		  m_face->ReceiveInterest (nack);
		  // Call trace (for logging purposes)
		  m_transmittedInterests (nack, this, m_face);
	  }
	  	  
  }
  
  
}

void
CRoSNDNTunnelHelloProducer::OnTimeout (uint32_t nodeid)
{
  NS_LOG_DEBUG (GetNode()->GetId() <<  " Time out neighbour.");
  //(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->SetNewNeighbor (true);   
  //(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->IncrementState ();  
  m_fw->RemoveNodeNeighbors (nodeid);
  if (m_neighborctl.find(nodeid) !=  m_neighborctl.end())
	m_neighborctl.erase(m_neighborctl.find(nodeid));
  m_newneighbor = true;
  RegisterInController ();
}

void
CRoSNDNTunnelHelloProducer::SendInterest (Ptr<Name> prefixname)
{
  if (!m_isRunning) return;

  /////////////////////////////////////
  // Sending one Interest packet out //
  /////////////////////////////////////
  
  NS_LOG_FUNCTION_NOARGS ();
  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (prefixname);
  interest->SetInterestLifetime    (m_rtt->RetransmitTimeout ());

  NS_LOG_INFO ("Requesting Interest: \t" << *interest);
  NS_LOG_INFO ("> Interest for " << prefixname->toUri());

  FwHopCountTag hopCountTag;
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);
}

void
CRoSNDNTunnelHelloProducer::OnData (Ptr<const ndn::Data> contentObject)
{
  App::OnData (contentObject); // tracing inside 
  Name cname = contentObject->GetName();
  NS_LOG_FUNCTION (this << contentObject->GetName());
  const Name contentname = contentObject->GetName();
  uint32_t sz = contentname.size();
  
  if (cname.get(0).toUri() == "controller")
  {
    Simulator::Cancel (m_discoveryctl);
    Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
    Ptr<const Packet> payload = contentObject->GetPayload ();
    (payload->Copy())->PeekHeader (*payheader);	
    const Name nameaux = payheader->GetPayload ();
    Ptr<Name> contrx = Create<Name> ();
    contrx->append ("controller0");  
    contrx->append (nameaux.get(0));
	
	/*
    uint32_t localstate = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->GetState();
	if (localstate < nameaux.get(1).toNumber())
		(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->SetState(nameaux.get(1).toNumber());
	*/
	m_seq++;
   
    const Name temp = Name((m_fw->GetController ())->toUri());
      
        //rever com mais controladores
    ((m_fw->SetController (contrx)));
    NS_LOG_DEBUG ("Received Controller Content payload value: " << nameaux.get(0).toNumber ());
    NS_LOG_INFO ("Node controller in use: " << ((std::string) ((m_fw->GetController ())->toUri ())));
	//NS_LOG_UNCOND(GetNode()->GetId() << "\t Received controller response: \t" << nameaux.get(0).toNumber ());
    if (m_newneighbor)
		RegisterInController ();
  }
  else  if (cname.get(0).toUri() == "controller0" &&
    sz > 0 && cname.get(2).toUri() == "registerRouter")
  {
    Simulator::Cancel (m_regctl);
    m_seqreg++;
	m_newneighbor = false;
    NS_LOG_DEBUG ("Received Controller Register OK!");
  }
  
  //prefix: /controller0/controllerId/routeFrom/sourceRouterID/wantedcontent/seq
  else if (contentname.get(0).toUri() == "controller0" &&
    sz > 0 && contentname.get(2).toUri() == "routeFrom")
  {
	uint32_t seqn = contentname.get(sz-1).toNumber();
	Simulator::Cancel (m_askrouteTimeOut[seqn]);
	if (m_askrouteTimeOut.find(seqn) != m_askrouteTimeOut.end())
		m_askrouteTimeOut.erase(seqn);
	//Name pref = contentname.getPrefix(sz-6,4);
    //if (m_pendingroutes.find(pref) != m_pendingroutes.end())
    //{
    //    m_pendingroutes.erase(pref);
	//	NS_LOG_DEBUG ("Remove m_pendingroutes 1: " << pref.toUri ());
    //} 
	
    m_rtt->AckSeq (SequenceNumber32 (contentname.get(sz-1).toNumber()));
    //NS_LOG_DEBUG (GetNode()->GetId() << " ASK ROUTE ANSWER: " <<   m_rtt->RetransmitTimeout ());

    Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
    Ptr<const Packet> payload = contentObject->GetPayload();
    (payload->Copy ())->RemoveHeader (*payheader);	
    Ptr<const Name> nameaux = Create<const Name> ();
    nameaux = payheader->GetPayloadPtr ();
    NS_LOG_DEBUG ("Received Controller Content: " << (*nameaux));
	//NS_LOG_UNCOND ("Received Controller Content: " << (*nameaux));
    //Send route installation interest
    Ptr<ndn::Interest> newinterest= Create<ndn::Interest> ();
    Ptr<Name> nameroute = Create<Name> ();
    nameroute->append ("router");
    NS_LOG_DEBUG ("Test route size: " << nameaux->size ());
    //Name nexthop;
    std::list<std::string> nexthopstr;
    bool first = true;
    Ptr<Fib> fib = GetNode ()->GetObject<Fib> ();
    if (nameaux->size() >= 1 && nameaux->getPostfix(1,0).toUri () != "/")
    {
      for (Name::const_iterator it = nameaux->begin(); it != nameaux->end();
        it++)
      {
        {
          if (it->toUri() != "" && it->toNumber() != boost::lexical_cast<uint64_t>(GetNode ()->GetId ()))
          {
            nameroute->appendSeqNum (it->toNumber());
            //Identify next hop
            if (first)
            {
              //nexthop.append("router");
              //nexthop.appendSeqNum(it->toNumber());
              nameroute->append ("installRouteAndForward");
              nameroute->appendSeqNum (it->toNumber());
              first = false;
            }
          }
        }
      }    
	  uint32_t destrouter = nameroute->get(nameroute->size()-1).toNumber();
      nameroute->append("endRoute");
      nameroute->append("routePreference");
      nameroute->appendSeqNum(1);
      nameroute->append("toPrefix");
	  nameroute->append(FormatRouteString(contentname.getSubName(), destrouter));
      //nameroute->append (contentname.getPrefix (contentname.size()-4,4));
	  
	  uint32_t seqn = nameroute->get(nameroute->size()-1).toNumber();
	  m_routeInstallTimeOut[seqn] = Simulator::Schedule (m_rtt->RetransmitTimeout (), 
        &CRoSNDNTunnelHelloProducer::OnInstallRouteTimeout, this, contentname.getPrefix (sz-6,4), seqn, nameroute->get(1).toNumber());
      
      UniformVariable rand (0,std::numeric_limits<uint32_t>::max ());
      newinterest->SetNonce (rand.GetValue ());
      newinterest->SetName (nameroute);
      newinterest->SetInterestLifetime (Seconds (0.5));
      // Create packet and add ndn::Interest

      NS_LOG_DEBUG ("Sending Interest packet for " << *nameroute);
        // Echo back FwHopCountTag if exists
      FwHopCountTag hopCountTag;
      if (contentObject->GetPayload ()->PeekPacketTag (hopCountTag))
        newinterest->GetPayload ()->AddPacketTag (hopCountTag);
      // Forward packet to lower (network) layer
      m_face->ReceiveInterest (newinterest);
      // Call trace (for logging purposes)
      m_transmittedInterests (newinterest, this, m_face);         
    }else
	{
		Name pref = contentname.getPrefix(sz-6,4);
		if (m_pendingroutes.find(pref) != m_pendingroutes.end())
		{
			m_pendingroutes.erase(pref);
			NS_LOG_DEBUG ("Remove m_pendingroutes: " << pref.toUri ());
		}
		NS_LOG_DEBUG ("No route found for name route request: " << contentname.toUri ());
	}
  } 
  else if (contentname.get(0).toUri() == "router" &&
    sz > 6 && contentname.get(2).toUri() == "installRouteAndForward")
  {
	  uint32_t seqn = contentname.get(sz-1).toNumber();
	  Simulator::Cancel (m_routeInstallTimeOut[seqn]);
	  if (m_routeInstallTimeOut.find(seqn) != m_routeInstallTimeOut.end())
		m_routeInstallTimeOut.erase(seqn);
	  uint32_t pos = 0;
	  for(Name::const_iterator it = contentname.begin(); it != contentname.end(); it++)
	  {
		  pos++;
		  if (it->toUri() == "toPrefix" )
		  {
			  break;
		  }
	  }
	  Name pref = contentname.getPrefix(sz-pos-2,pos);
	  if (m_pendingroutes.find(pref) != m_pendingroutes.end())
	  {
		  m_pendingroutes.erase(pref);
		  NS_LOG_DEBUG ("Remove m_pendingroutes: " << pref.toUri ());
	  }
	  NS_LOG_DEBUG ("Next hop route install OK response: " << contentname.toUri ());
  }
  else
  {
    NS_LOG_DEBUG ("Received requested content: " << contentname.toUri ());
  }
     
}

void
CRoSNDNTunnelHelloProducer::SearchController ()
{
  NS_LOG_FUNCTION_NOARGS ();
// Interest: /controller/seq
  //Simulator::Cancel (m_discoveryctl);
  
  for (std::map<uint32_t,EventId>::iterator it = m_askrouteTimeOut.begin(); it != m_askrouteTimeOut.end(); it++)
  {
	  Simulator::Cancel (it->second);
  }
  NS_LOG_DEBUG ("Remove m_pendingroutes 3: all!");
  m_pendingroutes.clear();
  
  if (m_discoveryctl.IsRunning())
	  return;
  
  Ptr<Name> interestName = Create<Name> ("/controller");
  NS_LOG_INFO("Name: " << interestName->getSubName());
  Ptr<L3Protocol> ndn = GetNode()->GetObject<L3Protocol> ();
  for (u_int32_t faces = 0; faces < GetNode()->GetNDevices (); faces++)
  {
    m_fw->AddFibEntry (interestName->getSubName(), ndn->GetFaceByNetDevice(GetNode()->GetDevice (faces)), 0);	  
     NS_LOG_INFO("Add fib entries for controller discovery!");
  }
  
  if (m_fw->GetState() > m_seq)
  {
	m_seq = m_fw->GetState();  
  }
  //else
  //{
	//m_seq++;
	//(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->SetState(m_seq);
  //}
  //m_seq = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->GetState() + 1;
  
  interestName->appendSeqNum (m_seq);
  SendInterest (interestName);    
  //if ((!(m_fw->GetIsThereController ())))
  m_discoveryctl = Simulator::Schedule (m_rtt->RetransmitTimeout (),&CRoSNDNTunnelHelloProducer::OnNoController, this, interestName);
}

void
CRoSNDNTunnelHelloProducer::RegisterInController ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Simulator::Cancel (m_regctl);
  Ptr<Name> regname = Create<Name> ((m_fw->GetController ())->getSubName ());
  //Interest /controller0/ControllerID/registerRouter/nodeID/flag/
  //			neighborlistsize/neighbour1/.../neighbourN/
  //			zonelistsize/ctl1/.../ctln/seqnumber

  if (regname->size() > 0)
  {
    regname->append ("registerRouter");
    regname->appendSeqNum (GetNode()->GetId());
	if (m_newneighbor) //update list of neighbors.
		regname->appendSeqNum (0);   
	else
		regname->appendSeqNum (1);   
    std::set<uint32_t> neighborlist = m_fw->GetNodeNeighbors ();
    std::string output;
	regname->appendSeqNum (neighborlist.size());
    for (std::set<uint32_t>::iterator it = neighborlist.begin(); it != neighborlist.end(); it++)
    {
      output.append(boost::lexical_cast<std::string>(*it));
      output.append(", ");
      regname->appendSeqNum (boost::lexical_cast<uint64_t>(*it));
    }
    NS_LOG_DEBUG("Node neighbors: " << output << " for Node: " << GetNode()->GetId()); 
	
	std::set<Name> ctlset;
	Name ctlsetname = Name();
	for (std::map<uint32_t,Name>::iterator it = m_neighborctl.begin(); it != m_neighborctl.end(); it++)
	{
	  if (ctlset.find(it->second) == ctlset.end())
	  {
		  ctlset.insert(it->second);
		  ctlsetname.append (it->second);
	  }
	}
	regname->appendSeqNum (ctlset.size());
	regname->append (ctlsetname);
    regname->appendSeqNum (m_seqreg);
    NS_LOG_DEBUG ("Consumer Router Register sending prefix: " << regname->toUri());
	//NS_LOG_UNCOND ("Consumer Router Register sending prefix: " << regname->toUri());
//    m_appstate = (GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->GetState ();
    Ptr<Name> ctr = (m_fw->GetController ());    
    if (ctr->size() > 1 && ctr->get(1).toNumber () != GetNode()->GetId())
      m_regctl = Simulator::Schedule (m_rtt->RetransmitTimeout (),&CRoSNDNTunnelHelloProducer::OnNoRegisterResponse, this, regname);
    SendInterest (regname);
  }
}

void
CRoSNDNTunnelHelloProducer::OnNoController(Ptr<Name> interest)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG (GetNode()->GetId() << " Time out find controller.");
  //NS_LOG_UNCOND(GetNode()->GetId() << "\t DO NOT received controller response.");
  
  m_rtt->Reset();
  m_rtt->SetCurrentEstimate (Seconds(1.0)); 
  Simulator::Cancel (m_discoveryctl);
  //(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->SetNewNeighbor (true);
  //(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->IncrementState ();
  SendInterest (interest);
  if ((!(m_fw->GetIsThereController ())))
    m_discoveryctl = Simulator::Schedule (m_rtt->RetransmitTimeout (),&CRoSNDNTunnelHelloProducer::SearchController, this);//, interest);
}
  
void
CRoSNDNTunnelHelloProducer::OnNoRegisterResponse(Ptr<Name> interest)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG (GetNode()->GetId() << " Time out register in controller.");
  Simulator::Cancel (m_regctl);
  //(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->SetNewNeighbor (true);
  SearchController ();
}

bool
CRoSNDNTunnelHelloProducer::AskRoute (Ptr<const Interest> interest)
{
  Name namex = interest->GetName ();
  NS_LOG_DEBUG ("Node asking route for name: " << namex);
  if (!m_isRunning) 
	  return false;
  uint32_t sz = namex.size ();
  FwHopCountTag hopCountTag;
  Ptr<Name> contr = m_fw->GetController ();
  //if (contr->size () == 0)
  if (!m_fw->GetIsThereController ())
  {
    NS_LOG_DEBUG ("Node did not found a controller yet!");
	SearchController ();
    return false;
  }
  Ptr<Interest> newinterest = Create<Interest> ();  
  UniformVariable rand (0,std::numeric_limits<uint32_t>::max ());
  Ptr<Packet> packet = Create<Packet> ();
  Ptr<Name> nameprefix = Create<ndn::Name> ();
  Ptr<Name> wantedcontent = Create<ndn::Name> ();
  bool ishop = false;
  bool iswantedprefix = false;
  bool isnexthop = true;
  uint32_t hops = 0;
  //		/router/routerid/installRouteAndForward/sourcerouterId/nextRouterId/.../destinationRouterId
  //			/endRoute/routePreference/x/toPrefix/wantedcontent/seq
  if (namex.get(0).toUri() == "router" &&
	namex.get(1).toNumber() == GetNode()->GetId() &&
    namex.get(2).toUri() == "installRouteAndForward" &&
    contr->size () > 0)
  {
    NS_LOG_DEBUG("Route install propagation.");// <<
    for(Name::const_iterator it = namex.begin();
      it != namex.end(); it++)
    {
      if (!ishop || (ishop && it->toNumber() != boost::lexical_cast<uint64_t>(GetNode()->GetId())))
      {
        if (it->toUri() !="")
          nameprefix->append (*it);
      }
      if (it->toUri() == "installRouteAndForward" )
      {
        ishop = true;
        isnexthop = true;
        
      }
      if (it->toUri() == "endRoute" )
        ishop = false;
      if (ishop)
      {
        if (it->toUri() != "installRouteAndForward" 
          && it->toNumber() != boost::lexical_cast<uint64_t> (GetNode()->GetId()))
        {
          hops++;
          if (isnexthop)
          {
            isnexthop = false;
            (*nameprefix) [1] = *it;
          }
        }
      }
      if (it->toUri() == "toPrefix" )
        iswantedprefix = true;
      if (iswantedprefix && it->toUri() != "toPrefix" )
          wantedcontent->append (*it);
    }

	if (hops > 0)
    {
	
    //Let route installation propagate before ask new route.    
    //Name pref = Name (wantedcontent->begin(), wantedcontent->end());
	Name pref = wantedcontent->getSubName(0,wantedcontent->size()-2);
    //if (m_pendingroutes.find(pref) == m_pendingroutes.end())
    //{
      NS_LOG_DEBUG ("Add m_pendingroutes (route install request): " << pref.toUri ());
      m_pendingroutes.insert(pref);
	  uint32_t seqn = nameprefix->get(nameprefix->size()-1).toNumber();
      m_routeInstallTimeOut[seqn] = Simulator::Schedule (Seconds(1.0), 
        &CRoSNDNTunnelHelloProducer::OnInstallRouteTimeout, this, pref, seqn, nameprefix->get(1).toNumber());
      NS_LOG_DEBUG (GetNode()->GetId() << " INSTALL AND FORWARD: " <<   m_rtt->RetransmitTimeout ());
    //}
	
    

		newinterest->SetName (nameprefix);
		NS_LOG_DEBUG ("Sending Interest packet for prefix instalation " << *nameprefix);
    
	//retirado comentário 11dez2015
	/*
	}else
    {
		newinterest->SetName (wantedcontent->getSubName(0,wantedcontent->size()-1));
		NS_LOG_DEBUG ("Sending Interest packet for content " << *wantedcontent);
    }
	*/
	//retirado comentário 11dez2015
	
    // Create and configure ndn::Interest
		newinterest->SetNonce (rand.GetValue ());
		newinterest->SetInterestLifetime (m_rtt->RetransmitTimeout ());
		if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
		  newinterest->GetPayload ()->AddPacketTag (hopCountTag);
		// Forward packet to lower (network) layer
		m_face->ReceiveInterest (newinterest);
		// Call trace (for logging purposes)
		m_transmittedInterests (newinterest, this, m_face);
		
    }//comentário 11dez2015
    
	Ptr<Data> data = Create<Data> (Create<Packet> (m_virtualPayloadSize));
	Ptr<Name> dataName = Create<Name> (interest->GetName ());
	data->SetName (dataName);
	data->SetFreshness (Seconds (0));
	data->SetTimestamp (Simulator::Now());
	
	data->SetSignature (m_signature);
	if (m_keyLocator.size () > 0)
	{
		data->SetKeyLocator (Create<Name> (m_keyLocator));
	}
	// Echo back FwHopCountTag if exists
	//FwHopCountTag hopCountTag;
	if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
	{
	  data->GetPayload ()->AddPacketTag (hopCountTag);
	}

	m_face->ReceiveData (data);
	m_transmittedDatas (data, this, m_face);    
	return false;  
		
  } //Received a request for content /wantedcontent and now we ask a new route to controller
  else if ((namex.get(0).toUri() != "controller0") &&
    (namex.get(0).toUri() != "router") &&
    (namex.get(0).toUri() != "hello") &&
    (namex.get(0).toUri() != "controller") &&
    (contr->size () > 1))
  {
    if (m_regctl.IsRunning())
	  return false;
  
    Ptr<fib::Entry> fibEntry = GetNode ()->GetObject<Fib> ()->LongestPrefixMatch (*interest);
	if (fibEntry != 0 and fibEntry->GetPrefix().toUri() != "/")
		return true;
	
	if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
		if (hopCountTag.Get() > 0)
			return false; //teste 09dez2015 (evita pedidos de rota de nós entre consumidor e produtor)
  
    Name pref = namex.getPrefix(sz-1);
    if (m_pendingroutes.find(pref) == m_pendingroutes.end())
    {
	  NS_LOG_DEBUG ("Add m_pendingroutes (content request): " << pref.toUri ());
      m_pendingroutes.insert(pref);
      //m_sendEvent 
	  m_askrouteTimeOut[m_seqi] = Simulator::Schedule ((m_rtt->RetransmitTimeout ()),
        &CRoSNDNTunnelHelloProducer::OnAskRouteTimeout, this, pref,namex.get(sz-1).toNumber());
	  //m_askrouteTimeOut[m_seqi] = Simulator::Schedule (Seconds(m_rtt->RetransmitTimeout ().ToInteger(Time::S) * 2),
      //  &CRoSNDNTunnelHelloProducer::OnAskRouteTimeout, this, pref,namex.get(sz-1).toNumber(), true, 0);	
		//.ToInteger(Time::S) * 2
      NS_LOG_DEBUG (GetNode()->GetId() << " ASK ROUTE before: " <<   m_rtt->RetransmitTimeout ());
    }
    else
    {
      NS_LOG_DEBUG("Suppressing to brief repeated route ask.");
      return false; //era true 07dez2015
    }
    
    nameprefix = Create<ndn::Name> (contr->begin(), contr->end());
    nameprefix->append ("routeFrom");
    nameprefix->appendSeqNum (GetNode ()->GetId ());
//	if (namex.get(0).toUri() == "/NamedData")
//		nameprefix->append (namex.get(0));
//	else
		nameprefix->append (namex.begin(), namex.end());
    nameprefix->appendSeqNum (m_seqi);
    m_seqi++;
    newinterest->SetNonce (rand.GetValue ());
    newinterest->SetName (nameprefix);
    newinterest->SetInterestLifetime (m_rtt->RetransmitTimeout ());
    if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
      newinterest->GetPayload ()->AddPacketTag (hopCountTag);
    NS_LOG_DEBUG ("Sending Interest packet for " << *nameprefix);
    // Forward packet to lower (network) layer
    m_face->ReceiveInterest (newinterest);    
    // Call trace (for logging purposes)
    m_transmittedInterests (newinterest, this, m_face);
    
    m_rtt->SentSeq (SequenceNumber32 (namex.get(sz-1).toNumber()), 1);
	//NS_LOG_DEBUG (GetNode()->GetId() << " ASK ROUTE after: " <<   m_rtt->RetransmitTimeout ());
    return false;
    
  }else{
    NS_LOG_DEBUG ("Route request not followed for name  " << interest->GetName ());
    return false;  //era true 07dez2015
  }
}

void
CRoSNDNTunnelHelloProducer::OnAskRouteTimeout (Name &pref, uint32_t seqn)
{  
    NS_LOG_DEBUG ("Node " << GetNode()->GetId() << " Timeout prefix:" << pref.toUri() << " seqn: " << seqn);
	if (m_pendingroutes.find(pref) != m_pendingroutes.end())
	{
		m_pendingroutes.erase(pref);
		NS_LOG_DEBUG ("Remove m_pendingroutes: " << pref.toUri ());
	}
	m_rtt->Reset();
	m_rtt->SetCurrentEstimate (Seconds(1.0));
    //(GetNode()->GetObject<ns3::ndn::fw::CRoSNDNStrategy> ())->SetRegisteredRouter (false);
	SearchController ();
	if (m_askrouteTimeOut.find(seqn) != m_askrouteTimeOut.end())
		m_askrouteTimeOut.erase(seqn);
}

void
CRoSNDNTunnelHelloProducer::OnInstallRouteTimeout (Name &pref, uint32_t seqn, uint32_t nodeid)
{  
	NS_LOG_DEBUG ("Node: " << GetNode()->GetId() << ", Time out /router/nodeid. nodeid = " << nodeid);
	//NS_LOG_UNCOND ("Node: " << GetNode()->GetId() << ", Time out /router/nodeid. nodeid = " << nodeid);
	if (m_pendingroutes.find(pref) != m_pendingroutes.end())
	{
		m_pendingroutes.erase(pref);
		NS_LOG_DEBUG ("Remove m_pendingroutes: " << pref.toUri ());
	}
	m_fw->RemoveNodeNeighbors (nodeid);
	NS_LOG_DEBUG ("Node: " << GetNode()->GetId() << " Route Install Failure - Next hop: " << nodeid);
	if (m_neighborctl.find(nodeid) !=  m_neighborctl.end())
		m_neighborctl.erase(m_neighborctl.find(nodeid));
	if (!m_regctl.IsRunning())
	{
	  m_newneighbor = true; //17dez2015
	  RegisterInController ();
	}
    else if (m_regctl.IsRunning() and !m_newneighbor)
	{
	  m_newneighbor = true; //17dez2015
	  RegisterInController ();
	}
	if (m_routeInstallTimeOut.find(seqn) != m_routeInstallTimeOut.end())
		m_routeInstallTimeOut.erase(seqn);
}

Name
CRoSNDNTunnelHelloProducer::FormatRouteString (Name prefix, uint32_t nodeid)
{
	Name tunnelhop;
	tunnelhop.append("tunnel");	
	tunnelhop.appendSeqNum(nodeid);
	m_fw->SetTunnelDestination(prefix.getPrefix (prefix.size()-6,4), nodeid);
	m_fw->AddFibEntry (prefix.getPrefix (prefix.size()-6,4), m_fw->GetTunnelFace(), 0);
	tunnelhop.append(prefix.getPrefix (prefix.size()-4,4));
	return tunnelhop;
}
  

} // namespace ndn
} // namespace ns3
