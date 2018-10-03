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

//This app monitors connectivity with node neighbors.
//It also verifies if a neighbor database hash differs from local value.
//If true, then it asks neighbor lsa hashes.

#include "ns3/log.h"
#include "ndn-producer-nodeid-nlsrlike-v2.h"
#include <boost/lexical_cast.hpp>
#include <ns3/ndnSIM/model/ndn-interest.h>
#include <ns3/ndnSIM/ndn.cxx/name-component.h>
#include "best-route-nlsr-like.h"
#include <ns3/ndnSIM/helper/ndn-app-helper.h>
#include "ndn-payload-header.h"
#include "ns3/random-variable.h"

NS_LOG_COMPONENT_DEFINE ("ndn.ProducerNodeIDNLSRLikeV2");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (ProducerNodeIDNLSRLikeV2);
   
TypeId
ProducerNodeIDNLSRLikeV2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ProducerNodeIDNLSRLikeV2")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<ProducerNodeIDNLSRLikeV2> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/hello"),
                   MakeNameAccessor (&ProducerNodeIDNLSRLikeV2::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&ProducerNodeIDNLSRLikeV2::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&ProducerNodeIDNLSRLikeV2::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0.5)),
                   MakeTimeAccessor (&ProducerNodeIDNLSRLikeV2::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&ProducerNodeIDNLSRLikeV2::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&ProducerNodeIDNLSRLikeV2::m_keyLocator),
                   MakeNameChecker ())
    .AddAttribute ("Frequency", "Frequency of interest packets",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&ProducerNodeIDNLSRLikeV2::m_frequency),
                   MakeDoubleChecker<double> ())                   
    
    ;
  return tid;
}
  
  
ProducerNodeIDNLSRLikeV2::ProducerNodeIDNLSRLikeV2 ()
  : m_isRunning (false)
{
  NS_LOG_FUNCTION_NOARGS ();
}

// inherited from Application base class.
void
ProducerNodeIDNLSRLikeV2::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())
    ->AddFibEntry (m_prefix, m_face, 0);
  m_isRunning = true;
}

void
ProducerNodeIDNLSRLikeV2::RefreshFib ()
{
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())
    ->AddFibEntry (m_prefix, m_face, 0);
  Simulator::Schedule (Seconds (0.3), &ProducerNodeIDNLSRLikeV2::RefreshFib, this);
}

void
ProducerNodeIDNLSRLikeV2::StopApplication ()
{
  m_isRunning = false;
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}

void
ProducerNodeIDNLSRLikeV2::OnInterest (Ptr<const Interest> interest)
{
  NS_LOG_LOGIC (this << interest);
  if (!m_active) return;
  Name intname = interest->GetName ();
  uint32_t size = intname.size ();
  uint64_t consid = intname.get(1).toNumber ();
  uint64_t prodid = boost::lexical_cast<uint64_t> ((GetNode())->GetId ());
  if(prodid == consid)
  {
    NS_LOG_LOGIC ("Do not answer my own interest! " << (intname.toUri ()));
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
  NS_LOG_LOGIC ("node("<< GetNode()->GetId() <<") respodning with Data: " << data->GetName ());
  // Echo back FwHopCountTag if exists
  FwHopCountTag hopCountTag;
  if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
    {
      data->GetPayload ()->AddPacketTag (hopCountTag);
    }
  m_face->ReceiveData (data);
  m_transmittedDatas (data, this, m_face);
  
//  /hello/sourceid/roothash/seq
  std::set<uint32_t> neighbours = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())
    ->GetNodeNeighbors ();
  NS_LOG_INFO("Neighbors size: " << neighbours.size());
  std::map<uint32_t,uint32_t> localroothash = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->GetRootHashV2();

  if (neighbours.find(consid) == neighbours.end())
  {
    (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->AddNodeNeighbors (consid);
    (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->IncrementState ();
    NS_LOG_DEBUG ("New neighbor: " << consid);
    //name         /routerid/LSA.type1/version
    //content      /neighbour1/neighbour2/.../neighbourn  
    Name name = Name ();
    name.appendSeqNum (GetNode()->GetId());
    name.appendSeqNum (1);
    uint32_t state =   (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->GetState();
    name.appendSeqNum (state);
    Name content = Name ();
    std::set<uint32_t> neighborlist = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())
      ->GetNodeNeighbors ();
    for (std::set<uint32_t>::iterator it = neighborlist.begin(); it != neighborlist.end(); it++)
    {
      content.appendSeqNum (boost::lexical_cast<uint64_t>(*it));
    }      
    (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->AddLSA (name, content);
    NS_LOG_LOGIC ("Adding LSA name: " << name << " content: " << content << " on node: " << (GetNode())->GetId ());
  }
  
  if (intname.get(2).toNumber () != localroothash[0])
  {
    NS_LOG_DEBUG ("Topo Local hash: " << localroothash[0] << " on node: " << consid
       << ", remote asked hash: " << intname.get(2).toNumber()
       << " on node: " << prodid);
    // /router/neighbourid/gethashes/topoproducer/roothash
    Ptr<Name> gethashes = Create<Name> ("/router");
    gethashes->appendSeqNum (intname.get(1).toNumber());
    gethashes->append ("gethashes");
    gethashes->appendSeqNum (1);
    gethashes->append (intname.get(2));    
    SendInterest (gethashes);
    NS_LOG_INFO("Added prefix: " << gethashes);
  }

  
  if (intname.get(3).toNumber () != localroothash[1])
  {
    NS_LOG_DEBUG ("Producer Local hash: " << localroothash[1] << " on node: " << consid
      << ", remote asked hash: " << intname.get(3).toNumber()
      << " on node: " << prodid);
    // /router/neighbourid/gethashes/topoproducer/roothash
    Ptr<Name> gethashes = Create<Name> ("/router");
    gethashes->appendSeqNum (intname.get(1).toNumber());
    gethashes->append ("gethashes");
    gethashes->appendSeqNum (2);
    gethashes->append (intname.get(3));    
    SendInterest (gethashes);
    NS_LOG_INFO("Added prefix: " << gethashes);
  }


  Simulator::Cancel (m_events[consid]);
  m_events[consid] = Simulator::Schedule ( Seconds( 2.0 / m_frequency),
    &ProducerNodeIDNLSRLikeV2::OnTimeout, this, consid);
}

void
ProducerNodeIDNLSRLikeV2::OnTimeout (uint32_t nodeid)
{
  NS_LOG_DEBUG ("Timeout");
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->IncrementState ();  
  (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->RemoveNodeNeighbors (nodeid);
  NS_LOG_DEBUG ("Node removed: " << nodeid);
  //name         /routerid/LSA.type1/version
  //content      /neighbour1/neighbour2/.../neighbour 
  Name name = Name ();
  name.appendSeqNum (GetNode()->GetId());
  name.appendSeqNum (1);
  uint32_t state =   (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->GetState();
  name.appendSeqNum (state);
  Name content = Name ();
  std::set<uint32_t> neighbours = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())
    ->GetNodeNeighbors ();
  for (std::set<uint32_t>::iterator it = neighbours.begin(); it != neighbours.end(); it++)
  {
    content.appendSeqNum (boost::lexical_cast<uint64_t>(*it));
    NS_LOG_DEBUG (GetNode()->GetId() << " Nodes: " << *it);    
  }
  bool isnew = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->AddLSA (name, content);
  if (isnew)
    NS_LOG_DEBUG ("New LSA: " << name << ", " << content << " on node: " << (GetNode())->GetId ());
}

void
ProducerNodeIDNLSRLikeV2::SendInterest (Ptr<Name> prefixname)
{
  if (!m_isRunning) return;

  /////////////////////////////////////
  // Sending one Interest packet out //
  /////////////////////////////////////
  
  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (prefixname);
  interest->SetInterestLifetime    (m_interestLifeTime);

  NS_LOG_LOGIC ("Requesting Interest: \t" << *interest);
  FwHopCountTag hopCountTag;
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);
}


void
ProducerNodeIDNLSRLikeV2::OnData (Ptr<const ndn::Data> contentObject)
{
  App::OnData (contentObject); // tracing inside 
//        /router/routerid/gethashes/topoproducer/roothash or
//        /router/routerid/getlsa/type/lsahash
//        /router/routerid/getlsa/type/lsahash/npkts/pos/seq
  Name cname = contentObject->GetName ();
  uint32_t size = cname.size();
  NS_LOG_LOGIC (GetNode()->GetId() << " Receiving ContentObject packet for " << cname.toUri());
  Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
  Ptr<const Packet> payload = contentObject->GetPayload ();
  (payload->Copy())->PeekHeader (*payheader);	
  const Name nameaux = payheader->GetPayload (); 
  uint32_t psz = nameaux.size();
  NS_LOG_LOGIC (GetNode()->GetId() << " Receiving payload" << nameaux.toUri());  
  if (cname.get(2).toUri() == "gethashes" && psz > 1)
  {
    // name: /router/neighbourid/getlsa/type/lsahash
    // content: /size1/hash1/../hashn/size2/hash1/...
    uint32_t npr = 0;
    uint32_t hash = 0;
    uint32_t hashsize = 0;
    uint32_t localhashsize = 0;
    NS_LOG_DEBUG ("Node (" << GetNode()->GetId() <<  ") Received LSA hashes:" << nameaux);
    for (Name::const_iterator it = nameaux.begin(); it != nameaux.end(); it++)
    {
      hashsize = it->toNumber();
      it++;
      hash = it->toNumber();
      Name templsa = (GetNode()->
        GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->GetLSA (hash,cname.get(3).toNumber());
      localhashsize = (GetNode()->
        GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->GetLSASize (hash,cname.get(3).toNumber());
//This avoids lsa redundant replication, however it implies hash colisions do not synchronize        
      if (templsa.toUri() != "/" && localhashsize == hashsize) 
        continue;
//        
      Ptr<Name> getlsa = Create<Name> ("/router");
      getlsa->append (cname.get(1));
      getlsa->append ("getlsa");
      getlsa->append (cname.get(3));
      getlsa->append (*it);
      Simulator::Schedule ( Seconds(npr/m_frequency/2/psz),
          &ProducerNodeIDNLSRLikeV2::SendInterest, this, getlsa);
      npr++;
    }
    
    //Interest /router/routerid/gethashes/topoproducer/roothash 
    //Interest /router/routerid/gethashes/topoproducer/roothash/npkts/seq
    // flag for additional packets. hash set exceeds packet MTU
    if (cname.size() > 5 && cname.get(6).toNumber() == 0)
    {
      uint32_t ind = 1;
      while (ind <= cname.get(5).toNumber())
      {
        Ptr<Name> gethashesseq = Create<Name> ("/router");
        gethashesseq->append (cname.get(1));
        gethashesseq->append (cname.get(2));
        gethashesseq->append (cname.get(3));
        gethashesseq->append (cname.get(4));
        gethashesseq->append (cname.get(5));
        gethashesseq->appendSeqNum (ind);
        SendInterest (gethashesseq);
        NS_LOG_DEBUG ("MTU exceded. Fragmenting packtes.");        
        ind++;
      }
    }
  }
  if (cname.get(2).toUri() == "getlsa" && psz > 1 && nameaux.toUri() != "no")
  {
    Name name = Name();
    Name content = Name();
    uint32_t pos = 0;
    bool error = false;
    uint32_t clen = 0;
    NS_LOG_LOGIC("LSA: " << nameaux << " on node: " << (GetNode())->GetId ());
    while ( (size == 5 && pos + 4 < psz) || (size > 5 && pos + 4 < psz - 1))
    { 
      if (nameaux.get(pos+1).toNumber() == 1)
      {
      // content /neighbour1/neighbour2/...
        clen = nameaux.get(pos+3).toNumber();
        if (pos+4+clen > psz)
          break;
        name = nameaux.getPrefix (3,pos);          
        content = nameaux.getPrefix (clen,pos+4);
      }
      else if (nameaux.get(pos+1).toNumber() == 2)
      {
      // content /isvalid/prefix
        clen = nameaux.get(pos+4).toNumber();
        if (pos+5+clen > psz)
          break;
        name = nameaux.getPrefix (4,pos);
        content = nameaux.getPrefix (clen,pos+5);           
      }
      else
      {
        NS_LOG_ERROR("BUG: " 
          << "Time: " << Simulator::Now ().ToDouble (Time::S)
          << "\n content name: " << cname.toUri()
          << "\n payload:  " << nameaux.toUri()
          << "\n pos: " << pos << ", " << nameaux.get(pos).toUri()
          << "\n clen: " << clen
          << "\n payload size: " << psz
          << "\n pos in payload: " << nameaux.get(psz-1).toNumber());
        error = true;
        break;
      }
      bool isnew = (GetNode()->GetObject<ns3::ndn::fw::BestRouteNLSRLike> ())->AddLSA (name, content);    
      if (isnew)
      {
        NS_LOG_DEBUG ("Added LSA name: " << name << " content: " << content << " on node: " << (GetNode())->GetId ()); 
      }
      else
      {
        NS_LOG_DEBUG ("LSA not added! Name: " << name << " content: " << content << " on node: " << (GetNode())->GetId ()); 
      }
      pos = pos + name.size() + 1 + content.size(); 
    }
    // /router/routerid/getlsa/type/lsahash/npkts/pos/seq
    if (!error && cname.size() > 5 && cname.get(5).toNumber() > (cname.get(7).toNumber() + 1))
    {
      Ptr<Name> getlsaseq = Create<Name> ("/router");
      getlsaseq->append (cname.get(1));
      getlsaseq->append (cname.get(2));
      getlsaseq->append (cname.get(3));
      getlsaseq->append (cname.get(4));
      getlsaseq->append (cname.get(5));
        getlsaseq->appendSeqNum (nameaux.get(psz-1).toNumber() - psz + pos);
      getlsaseq->appendSeqNum (cname.get(7).toNumber() + 1);
      SendInterest (getlsaseq);   
    }
  }  
}


} // namespace ndn
} // namespace ns3
