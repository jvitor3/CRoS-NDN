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

//This app monitors connectivity with node neighbors.
//It also verifies if a neighbor database hash differs from local value.
//If true, then it asks neighbor lsa hashes.

#include "ns3/log.h"
#include "nlsr-hello-producer.h"
#include <boost/lexical_cast.hpp>
#include <ns3/ndnSIM/model/ndn-interest.h>
#include <ns3/ndnSIM/ndn.cxx/name-component.h>
#include <ns3/ndnSIM/helper/ndn-app-helper.h>
#include "ndn-payload-header.h"
#include "ns3/random-variable.h"
#include "ns3/ndnSIM/utils/ndn-rtt-mean-deviation.h"

NS_LOG_COMPONENT_DEFINE ("ndn.NLSRHelloProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (NLSRHelloProducer);
   
TypeId
NLSRHelloProducer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::NLSRHelloProducer")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<NLSRHelloProducer> ()
    .AddAttribute ("Prefix","Prefix, for which producer has the data",
                   StringValue ("/hello"),
                   MakeNameAccessor (&NLSRHelloProducer::m_prefix),
                   MakeNameChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&NLSRHelloProducer::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&NLSRHelloProducer::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0.5)),
                   MakeTimeAccessor (&NLSRHelloProducer::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&NLSRHelloProducer::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&NLSRHelloProducer::m_keyLocator),
                   MakeNameChecker ())
    .AddAttribute ("Frequency", "Frequency of interest packets",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&NLSRHelloProducer::m_frequency),
                   MakeDoubleChecker<double> ())                   
    
    ;
  return tid;
}
  
  
NLSRHelloProducer::NLSRHelloProducer ()
  : m_isRunning (false)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rtt = CreateObject<RttMeanDeviation> ();
}

// inherited from Application base class.
void
NLSRHelloProducer::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StartApplication ();
  m_fw = (GetNode()->GetObject<ns3::ndn::fw::NLSRStrategy> ());
  m_fw->AddFibEntry (m_prefix, m_face, 0);
  m_isRunning = true;
}

void
NLSRHelloProducer::RefreshFib ()
{
  m_fw->AddFibEntry (m_prefix, m_face, 0);
  Simulator::Schedule (Seconds (0.3), &NLSRHelloProducer::RefreshFib, this);
}

void
NLSRHelloProducer::StopApplication ()
{
  m_isRunning = false;
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);
  App::StopApplication ();
}

void
NLSRHelloProducer::OnInterest (Ptr<const Interest> interest)
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
  std::set<uint32_t> neighbours = m_fw->GetNodeNeighbors ();
  NS_LOG_INFO("Neighbors size: " << neighbours.size());
  uint32_t localroothash = m_fw->GetRootHash();

  if (neighbours.find(consid) == neighbours.end())
  {
    m_fw->AddNodeNeighbors (consid);
    m_fw->IncrementState ();
    NS_LOG_DEBUG ("New neighbor: " << consid);
    //name         /routerid/LSA.type1/version
    //content      /neighbour1/neighbour2/.../neighbourn  
    Name name = Name ();
    name.appendSeqNum (GetNode()->GetId());
    name.appendSeqNum (1);
    uint32_t state =   m_fw->GetState();
    name.appendSeqNum (state);
    Name content = Name ();
    std::set<uint32_t> neighborlist = m_fw->GetNodeNeighbors ();
    for (std::set<uint32_t>::iterator it = neighborlist.begin(); it != neighborlist.end(); it++)
    {
      content.appendSeqNum (boost::lexical_cast<uint64_t>(*it));
    }      
    m_fw->AddLSA (name, content);
    NS_LOG_LOGIC ("Adding LSA name: " << name << " content: " << content << " on node: " << (GetNode())->GetId ());
  }
  
  if (intname.get(2).toNumber () != localroothash)
  {
    NS_LOG_DEBUG ("Local hash: " << localroothash << " on node: " << consid
       << ", remote asked hash: " << intname.get(2).toNumber()
       << " on node: " << prodid);
    // /router/neighbourid/gethashes/prefixsize
    Ptr<Name> gethashes = Create<Name> ("/router");
    gethashes->appendSeqNum (intname.get(1).toNumber());
    gethashes->append ("gethashes");
    gethashes->appendSeqNum (0);
    gethashes->append (intname.get(size-1));
    m_events[gethashes->getSubName()] = Simulator::Schedule ( m_rtt->RetransmitTimeout (),
      &NLSRHelloProducer::OnDataTimeout, this, gethashes);
    SendInterest (gethashes);
    NS_LOG_INFO("Asking prefix hashes: " << gethashes->toUri());
  }
  Simulator::Cancel (m_neighborevents[consid]);
  m_neighborevents[consid] = Simulator::Schedule ( Seconds( 2.5 / m_frequency),
    &NLSRHelloProducer::OnTimeout, this, consid);
}

void
NLSRHelloProducer::OnTimeout (uint32_t nodeid)
{
  NS_LOG_DEBUG ("Timeout");
  m_fw->IncrementState ();  
  m_fw->RemoveNodeNeighbors (nodeid);
  NS_LOG_DEBUG ("Node removed: " << nodeid);
  //name         /routerid/LSA.type1/version
  //content      /neighbour1/neighbour2/.../neighbour 
  Name name = Name ();
  name.appendSeqNum (GetNode()->GetId());
  name.appendSeqNum (1);
  uint32_t state =   m_fw->GetState();
  name.appendSeqNum (state);
  Name content = Name ();
  std::set<uint32_t> neighbours = m_fw->GetNodeNeighbors ();
  for (std::set<uint32_t>::iterator it = neighbours.begin(); it != neighbours.end(); it++)
  {
    content.appendSeqNum (boost::lexical_cast<uint64_t>(*it));
    NS_LOG_DEBUG (GetNode()->GetId() << " Nodes: " << *it);    
  }
  m_fw->AddLSA (name, content);
}

void
NLSRHelloProducer::SendInterest (Ptr<Name> prefixname)
{
  if (!m_isRunning) return;

  /////////////////////////////////////
  // Sending one Interest packet out //
  /////////////////////////////////////
  
  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (prefixname);
  interest->SetInterestLifetime    (m_rtt->RetransmitTimeout ());

  NS_LOG_LOGIC ("Requesting Interest: \t" << *interest);
  FwHopCountTag hopCountTag;
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);
  m_rtt->SentSeq (SequenceNumber32 (prefixname->get(prefixname->size()-1).toNumber()), 1);
}


void
NLSRHelloProducer::OnData (Ptr<const ndn::Data> contentObject)
{
  App::OnData (contentObject); // tracing inside 
  //name  
  //        /router/routerid/gethashes/prefixsize/prefix or
  //        /router/routerid/gethashes/prefixsize/prefix/npkts/seq
  //content
  //        /comp1/hash1/.../compn/hashn
  //name
  //        /router/neighbourid/getlsa/lsanamesize/lsaname
  //        /router/neighbourid/getlsa/lsanamesize/lsaname/npkts/pos/seq
  //content
  //        //lsacontent
  Name cname = contentObject->GetName ();
  Simulator::Cancel (m_events[cname]);
  uint32_t size = cname.size();
  NS_LOG_DEBUG (GetNode()->GetId() << " Receiving ContentObject packet for " << cname.toUri());
  m_rtt->AckSeq (SequenceNumber32 (cname.get(size-1).toNumber()));
  Ptr<PayloadHeader> payheader = Create<PayloadHeader> ();
  Ptr<const Packet> payload = contentObject->GetPayload ();
  (payload->Copy())->PeekHeader (*payheader);	
  const Name payloadname = payheader->GetPayload (); 
  uint32_t psz = payloadname.size();
  NS_LOG_LOGIC (GetNode()->GetId() << " Receiving payload: " << payloadname.toUri());  
  if (cname.get(2).toUri() == "gethashes")
  {
    uint32_t localhashsize = 0;
    Name prefix;
    uint32_t prefixsize = cname.get(3).toNumber();
    name::Component localcomp;
    uint32_t localcomphash = 0;
    NS_LOG_DEBUG ("Node (" << GetNode()->GetId() <<  ") Received LSA hashes:" << payloadname.toUri());
    if (prefixsize == 0)    
      prefix = Name("/");
    else
      prefix = cname.getPrefix (prefixsize,4);
    //if (payloadname.size() < 2)  //if there is more branches, it must be at least /component/branchhash
    if (payloadname.size() > 0 and payloadname.get(0).toUri() == "empty")
    {
      if (prefix.size() > 2)  //LSA name can have 3 or 4 components
      {
        //Name locallsaname = m_fw->GetLSA (prefix);
        int32_t remotelsaversion = prefix.get (prefixsize-1).toNumber();
        int32_t locallsaversion = m_fw->GetLSAVersion (prefix.getPrefix(prefixsize-1));
        NS_LOG_DEBUG ("LSA prefix: " << prefix.toUri() //<< ", " << locallsaname.toUri()
          << ", remote and local versions: " << remotelsaversion << ", " << locallsaversion);  
        if (remotelsaversion > locallsaversion)
        {
        
          // name: /router/neighbourid/getlsa/lsanamesize/lsaname
          Ptr<Name> getlsa = Create<Name> ("/router");
          getlsa->append (cname.get(1));
          getlsa->append ("getlsa");
          getlsa->appendSeqNum (prefix.size());
          getlsa->append (prefix);
          m_events[getlsa->getSubName()] = Simulator::Schedule ( m_rtt->RetransmitTimeout (),
            &NLSRHelloProducer::OnDataTimeout, this, getlsa);
          SendInterest (getlsa);
          NS_LOG_DEBUG ("Node (" << GetNode()->GetId() <<  ") asking lsa: " << getlsa->toUri());
        
        }
      }
    }
    else
    {
      std::map<name::Component, uint32_t> localhashes = m_fw->GetBranchHashes (prefix);
      for (uint32_t i = 0; i+1 < payloadname.size(); i=i+2)
      {
        std::map<name::Component, uint32_t>::iterator localhashesit = localhashes.find (payloadname.get(i));
        if (localhashesit != localhashes.end())
        {
          localcomp = localhashesit->first;
          localcomphash = localhashesit->second;
        }
        NS_LOG_DEBUG ("Remote and local comp hashes: " << payloadname.get(i+1).toNumber()
          << ", " << localcomphash);
        if (localhashesit == localhashes.end() or localcomphash != payloadname.get(i+1).toNumber())
        {
          Name newprefix = Name(prefix);
          newprefix.append(payloadname.get(i));
          //Interest /router/routerid/gethashes/prefixsize/prefix or
          Ptr<Name> getbrhashes = Create<Name> ();
          getbrhashes->append (cname.get(0));
          getbrhashes->append (cname.get(1));
          getbrhashes->append (cname.get(2));
          getbrhashes->appendSeqNum (newprefix.size());
          getbrhashes->append (newprefix);
          getbrhashes->append (cname.get(size-1));
          NS_LOG_DEBUG ("Node (" << GetNode()->GetId() <<  ") asking branch hash: " << getbrhashes->toUri());
          m_events[getbrhashes->getSubName()] = Simulator::Schedule ( m_rtt->RetransmitTimeout (),
            &NLSRHelloProducer::OnDataTimeout, this, getbrhashes);
          SendInterest (getbrhashes);
        }
      }
    }    
    //Interest /router/routerid/gethashes/prefixsize/prefix/seq/npkts/seq2
    // flag for additional packets. hash set exceeds packet MTU
    if (size > cname.get(3).toNumber() + 5)
    {
      uint32_t ind = 1;
      while (ind <= cname.get(size-1).toNumber())
      {
        Ptr<Name> gethashesseq = Create<Name> ();
        gethashesseq->append (cname.getPrefix(size-1));
        gethashesseq->appendSeqNum (ind);
        m_events[gethashesseq->getSubName()] = Simulator::Schedule ( m_rtt->RetransmitTimeout (),
          &NLSRHelloProducer::OnDataTimeout, this, gethashesseq);
        SendInterest (gethashesseq);
        NS_LOG_DEBUG ("MTU exceded. Fragmenting packtes.");
        NS_LOG_DEBUG ("Node (" << GetNode()->GetId() <<  ") asking hash seq: " << gethashesseq->toUri());                  
        ind++;
      }
    }
  }
  if (cname.get(2).toUri() == "getlsa" && payloadname.toUri() != "/no")
  {
    //name
    //        /router/neighbourid/getlsa/lsanamesize/lsaname
    //        /router/neighbourid/getlsa/lsanamesize/lsaname/npkts/pos/seq
    uint32_t lsanamesize = cname.get(3).toNumber();
    Name lsaname = Name(cname.getPrefix(lsanamesize, 4));
    Name lsacontent = Name (payloadname);
    uint32_t pos = 0;
    bool error = false;
    NS_LOG_DEBUG("Received LSA: " << payloadname << " on node: " << (GetNode())->GetId ());
    if (lsacontent.size() > 0)
      m_fw->AddLSA (lsaname, lsacontent);    
    else
      error = true;
    if (!error and size > (cname.get(3).toNumber() + 4) and 
      cname.get(size-3).toNumber() > (cname.get(-1).toNumber() + 1))
    {
      Ptr<Name> getlsaseq = Create<Name> ();  
      getlsaseq->append (cname.getPrefix (size-2));
      getlsaseq->appendSeqNum (payloadname.get(psz-1).toNumber() - psz + pos);
      getlsaseq->appendSeqNum (cname.get(size-1).toNumber() + 1);
      m_events[getlsaseq->getSubName()] = Simulator::Schedule ( m_rtt->RetransmitTimeout (),
        &NLSRHelloProducer::OnDataTimeout, this, getlsaseq);
      SendInterest (getlsaseq); 
      NS_LOG_UNCOND ("Node (" << GetNode()->GetId() <<  ") asking lsa seq: " << getlsaseq->toUri());                    
    }
  }  
}


void
NLSRHelloProducer::OnDataTimeout(Ptr<Name> interest)
{
  NS_LOG_LOGIC ("Data timeout: " << interest->toUri());
  m_rtt->Reset();
  m_rtt->SetCurrentEstimate (Seconds(1.0));
  m_events[interest->getSubName()] = Simulator::Schedule ( m_rtt->RetransmitTimeout (),
    &NLSRHelloProducer::OnDataTimeout, this, interest);
  SendInterest (interest);
}

} // namespace ndn
} // namespace ns3
