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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  0211110001307  USA
 *
 * Author: João Vitor Torres <jvitor@gta.ufrj.br>
 */

#include "ns3/log.h"
#include "ndn-producer-nodeid-v3.h"
#include "ndn-payload-header.h"
#include "best-route-sdndn-controller.h"
#include <boost/lexical_cast.hpp>
#include <ns3/ndnSIM/model/ndn-interest.h>
#include <ns3/ndnSIM/ndn.cxx/name-component.h>
#include <ns3/ndnSIM/helper/ndn-app-helper.h>
#include <ns3/ndnSIM/model/ndn-l3-protocol.h>
#include "ns3/ndn-fib.h"

//This app listen to interest /hello/nodeid/seq
//If nodeid is a new neighbor, then it update the local state, it searchs the controller, and it registers the new neighbors.
//Controller searching prefix is /controller/seq 
//Register prefix is /controllerX/ControllerID/registerRouter/nodeID/flag/neighbour1/.../neighbourN/seqnumber


NS_LOG_COMPONENT_DEFINE ("ndn.ProducerNodeIDV3");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ProducerNodeIDV3);
   
TypeId
ProducerNodeIDV3::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ProducerNodeIDV3")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<ProducerNodeIDV3> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/hello"),
                   MakeNameAccessor (&ProducerNodeIDV3::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerNodeIDV3::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&ProducerNodeIDV3::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&ProducerNodeIDV3::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ProducerNodeIDV3::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&ProducerNodeIDV3::m_keyLocator),
                   MakeNameChecker ())
    .AddAttribute ("Frequency", "Frequency of interest packets",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&ProducerNodeIDV3::m_frequency),
                   MakeDoubleChecker<double> ())                        
    ;
  return tid;
}
  
  
ProducerNodeIDV3::ProducerNodeIDV3 ()
: m_isRunning (false)
{
  NS_LOG_FUNCTION_NOARGS ();
}

// inherited from Application base class.
void
ProducerNodeIDV3::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
    ->AddFibEntry (m_prefix, m_face, 0);  
  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());
  m_isRunning = true;
  m_seqreg = 0;
  m_seq = 0;
  m_interestLifeTime = Seconds (0.9);
}

void
ProducerNodeIDV3::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
  m_isRunning = false;
}

void
ProducerNodeIDV3::OnInterest (Ptr<const Interest> interest)
{
  NS_LOG_FUNCTION (this << interest);
  if (!m_active) return;
  Name intname = interest->GetName ();
  uint32_t size = intname.size ();
  uint64_t consid = intname.get(1).toNumber ();
  uint64_t prodid = boost::lexical_cast<uint64_t> ((GetNode())->GetId ());
  NS_LOG_INFO ("Received Interest Name! " << (intname.toUri ()));
  if(prodid == consid)
  {
    NS_LOG_INFO ("Do not answer my own interest! " << (intname.toUri ()));
    return;
  }  
  Ptr<Data> data = Create<Data> (Create<Packet> (m_virtualPayloadSize));
  Ptr<Name> dataName = Create<Name> (interest->GetName ());
  dataName->append (m_postfix);
  data->SetName (dataName);
  data->SetFreshness (m_freshness);

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
  //hello/sourceid/seq
  std::set<uint32_t> neighbours = (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
    ->GetNodeNeighbors ();
  NS_LOG_INFO("Neighbors size: " << neighbours.size());
  if (intname.get(2).toNumber () == 1)
  {
    RegisterInController ();
  }
  if (neighbours.find(consid) == neighbours.end())
  {
    (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->AddNodeNeighbors (consid);
    (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->IncrementState ();
    (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->SetNewNeighbor (true);
    SearchController ();
    NS_LOG_DEBUG ("New neighbor: " << consid);
  }
  Simulator::Cancel (m_events[consid]);
  m_events[consid] = Simulator::Schedule ( Seconds(2.0 / m_frequency),&ProducerNodeIDV3::OnTimeout, this, consid);
  if ((!((GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->GetIsThereController ())))
    SearchController ();
}

void
ProducerNodeIDV3::OnTimeout (uint32_t nodeid)
{
  NS_LOG_DEBUG (GetNode()->GetId() <<  " Time out neighbour.");
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->SetNewNeighbor (true);   
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->IncrementState ();  
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->RemoveNodeNeighbors (nodeid);
  RegisterInController ();
}

void
ProducerNodeIDV3::SendInterest (Ptr<Name> prefixname)
{
  if (!m_isRunning) return;

  /////////////////////////////////////
  // Sending one Interest packet out //
  /////////////////////////////////////
  
  NS_LOG_FUNCTION_NOARGS ();
  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (prefixname);
  interest->SetInterestLifetime    (m_interestLifeTime);

  NS_LOG_INFO ("Requesting Interest: \t" << *interest);
  NS_LOG_INFO ("> Interest for " << prefixname->toUri());

  FwHopCountTag hopCountTag;
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);
}

void
ProducerNodeIDV3::OnData (Ptr<const ndn::Data> contentObject)
{
  Name cname = contentObject->GetName();
  NS_LOG_FUNCTION (this << contentObject->GetName());
  
  if (cname.get(0).toUri() == "controller")
  {
    Simulator::Cancel (m_discoveryctl);
    Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
    Ptr<const Packet> payload = contentObject->GetPayload ();
    (payload->Copy())->PeekHeader (*payheader);	
    const Name nameaux = payheader->GetPayload ();
    Ptr<Name> contrx = Create<Name> ();
    contrx->append ("controllerX");  
    contrx->append (nameaux.get(0));
  
    m_seq = nameaux.get(1).toNumber() + 1;
    NS_LOG_DEBUG ("Discovery controller new and old seq number: " << m_seq 
      << ", " << nameaux.get(1).toNumber ());
  
    const Name temp = Name(((GetNode()
      ->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->GetController ())->toUri());
      
        //rever com mais controladores
    (((GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->SetController (contrx)));
    NS_LOG_DEBUG ("Received Controller Content payload value: " << nameaux.get(0).toNumber ());
    NS_LOG_INFO ("Node controller in use: " << ((std::string) (((GetNode()
      ->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->GetController ())->toUri ())));
    RegisterInController ();
  }
  else  if (cname.get(0).toUri() == "controllerX" && cname.get(2).toUri() == "registerRouter")
  {
    Simulator::Cancel (m_regctl);
    m_seqreg++;
    NS_LOG_DEBUG ("Received Controller Register OK!");
  }
  App::OnData (contentObject); // tracing inside    
}

void
ProducerNodeIDV3::SearchController ()
{
  NS_LOG_FUNCTION_NOARGS ();
// Interest: /controller/seq
  Simulator::Cancel (m_discoveryctl);
  Ptr<Name> interestName = Create<Name> ("/controller");
  NS_LOG_INFO("Name: " << interestName->getSubName());
  Ptr<L3Protocol> ndn = GetNode()->GetObject<L3Protocol> ();
  for (u_int32_t faces = 0; faces < GetNode()->GetNDevices (); faces++)
  {
    (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
      ->AddFibEntry (interestName->getSubName(), ndn->GetFaceByNetDevice(GetNode()->GetDevice (faces)), 0);	  
     NS_LOG_INFO("Add fib entries for controller discovery!");
  }
  interestName->appendSeqNum (m_seq);
  SendInterest (interestName);    
  if ((!((GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->GetIsThereController ())))
    m_discoveryctl = Simulator::Schedule ( Seconds(0.95),&ProducerNodeIDV3::OnNoController, this, interestName);
}

void
ProducerNodeIDV3::RegisterInController ()
{
  NS_LOG_FUNCTION_NOARGS ();
  Simulator::Cancel (m_regctl);
  Ptr<Name> regname = Create<Name> (((GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
    ->GetController ())->getSubName ());
  //Interest /controllerX/ControllerID/registerRouter/nodeID/flag/neighbour1/.../neighbourN/seqnumber

  if (regname->size() > 0)
  {
    regname->append ("registerRouter");
    regname->appendSeqNum (GetNode()->GetId());
    regname->appendSeqNum (0);   
    std::set<uint32_t> neighborlist = (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())
     ->GetNodeNeighbors ();
    std::string output;
    for (std::set<uint32_t>::iterator it = neighborlist.begin(); it != neighborlist.end(); it++)
    {
      output.append(boost::lexical_cast<std::string>(*it));
      output.append(", ");
      regname->appendSeqNum (boost::lexical_cast<uint64_t>(*it));
    }
    NS_LOG_DEBUG("Node neighbors: " << output << " for Node: " << GetNode()->GetId()); 
    regname->appendSeqNum (m_seqreg);
    NS_LOG_DEBUG ("Consumer Router Register sending prefix: " << regname->toUri());
    m_appstate = (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->GetState ();
    Ptr<Name> ctr = ((GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->GetController ());    
    if (ctr->size() > 1 && ctr->get(1).toNumber () != GetNode()->GetId())
      m_regctl = Simulator::Schedule ( Seconds(0.97),&ProducerNodeIDV3::OnNoRegisterResponse, this, regname);
    SendInterest (regname);
  }
}

void
ProducerNodeIDV3::OnNoController(Ptr<Name> interest)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG (GetNode()->GetId() << " Time out find controller.");
  Simulator::Cancel (m_discoveryctl);
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->SetNewNeighbor (true);
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->IncrementState ();
  SendInterest (interest);
  if ((!((GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->GetIsThereController ())))
    m_discoveryctl = Simulator::Schedule ( Seconds(0.94),&ProducerNodeIDV3::SearchController, this);//, interest);
}
  
void
ProducerNodeIDV3::OnNoRegisterResponse(Ptr<Name> interest)
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_LOG_DEBUG (GetNode()->GetId() << " Time out register in controller.");
  Simulator::Cancel (m_regctl);
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteSDNDNController> ())->SetNewNeighbor (true);
  SearchController ();
}

} // namespace ndn
} // namespace ns3
