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

// This consumer sends interests with prefix /nodeid/contentid/seq
// nodeid is cyclically incremented up to NumberOfNodes
// contentid is cyclically incremented up to NumberOfPrefixes
// nodeid/contentid changes with probability PercentOfNewPrefix
// seq is sequentially incremented


#include "ndn-consumer-cbr-rand2.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

NS_LOG_COMPONENT_DEFINE ("ndn.ConsumerCbrRand2");

namespace ns3 {
namespace ndn {
    
NS_OBJECT_ENSURE_REGISTERED (ConsumerCbrRand2);
    
TypeId
ConsumerCbrRand2::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::ConsumerCbrRand2")
    .SetGroupName ("Ndn")
    .SetParent<Consumer> ()
    .AddConstructor<ConsumerCbrRand2> ()

    .AddAttribute ("Frequency", "Frequency of interest packets",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&ConsumerCbrRand2::m_frequency),
                   MakeDoubleChecker<double> ())
    
    .AddAttribute ("Randomize", "Type of send time randomization: none (default), uniform, exponential",
                   StringValue ("none"),
                   MakeStringAccessor (&ConsumerCbrRand2::SetRandomize, &ConsumerCbrRand2::GetRandomize),
                   MakeStringChecker ())

    .AddAttribute ("MaxSeq",
                   "Maximum sequence number to request",
                   IntegerValue (std::numeric_limits<uint32_t>::max ()),
                   MakeIntegerAccessor (&ConsumerCbrRand2::m_seqMax),
                   MakeIntegerChecker<uint32_t> ())

    .AddAttribute ("NumberOfPrefixes",
                   "Number of distinct prefixes",
                   StringValue ("1"),
                   MakeIntegerAccessor (&ConsumerCbrRand2::m_nprefixes),
                   MakeIntegerChecker<uint32_t> ())

    .AddAttribute ("NumberOfNodes",
                   "Number of nodes",
                   StringValue ("1"),
                   MakeIntegerAccessor (&ConsumerCbrRand2::m_nnodes),
                   MakeIntegerChecker<uint32_t> ())

    .AddAttribute ("PercentOfNewPrefix",
                   "Fraction of interests for new prefix",
                   StringValue ("100"),
                   MakeIntegerAccessor (&ConsumerCbrRand2::m_newprefixes),
                   MakeIntegerChecker<uint32_t> ())
                   
    ;

  return tid;
}
    
ConsumerCbrRand2::ConsumerCbrRand2 ()
  : m_frequency (1.0)
  , m_firstTime (true)
  , m_random (0)
  , m_newpseq (0)
  , m_newpnode (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_seqMax = std::numeric_limits<uint32_t>::max ();
  m_randprefix = UniformVariable (0, m_nprefixes);
}

ConsumerCbrRand2::~ConsumerCbrRand2 ()
{
  if (m_random)
    delete m_random;
}

void
ConsumerCbrRand2::StartApplication ()
{
  Consumer::StartApplication ();
  NS_LOG_DEBUG ("Consumer face: " << m_face->GetId());
}

void
ConsumerCbrRand2::ScheduleNextPacket ()
{
  if (m_firstTime)
    {
      m_sendEvent = Simulator::Schedule (Seconds (0.0),
                                         &ConsumerCbrRand2::SendPacket, this);
      m_firstTime = false;
    }
  else if (!m_sendEvent.IsRunning ())
    m_sendEvent = Simulator::Schedule (
                                       (m_random == 0) ?
                                         Seconds(1.0 / m_frequency)
                                       :
                                         Seconds(m_random->GetValue ()),
                                       &ConsumerCbrRand2::SendPacket, this);
}

void
ConsumerCbrRand2::SendPacket ()
{
  if (!m_active) return;

  uint32_t seq=std::numeric_limits<uint32_t>::max (); //invalid
  Name pref = Name ();
  uint32_t newseq = 0;
  
  uint32_t limit = m_newprefixes*m_nprefixes;

  while (m_retxSeqs.size ())
    {
      seq = *m_retxSeqs.begin ();
      m_retxSeqs.erase (m_retxSeqs.begin ());
      pref = m_pref[seq];
      break;
    }

  if (seq == std::numeric_limits<uint32_t>::max ())
    {
      if (m_seqMax != std::numeric_limits<uint32_t>::max ())
        {
          if (m_seq >= m_seqMax)
            {
              return; // we are totally done
              NS_LOG_WARN ("Consumer cbr rand: seq >= m_seqMax");
            }
        }

      seq = m_seq++;
      NS_LOG_DEBUG ("Test rand: " << newseq << ", " << limit);
      newseq = m_randprefix.GetInteger (0,100);
      if (newseq < m_newprefixes)
      {
        if (m_newpnode == 46) //error 47 dec = %00/ uri
          m_newpnode++;
        if (m_newpnode < (m_nnodes -1))
        {
          m_newpnode++;         
        }
        else
        {
          m_newpnode = 0;
          if (m_newpseq == 46) //error 47 dec = %00/ uri
            m_newpseq++;
          if (m_newpseq < (m_nprefixes-1))
            m_newpseq++;
          else
            m_newpseq = 0;
        }
      }
      ndn::Name aux = ndn::Name("/");
      aux.appendSeqNum(m_newpnode);
      aux.appendSeqNum(m_newpseq);     
      m_pref[seq] = aux;
      pref = m_pref[seq];
    }

  Ptr<Name> nameWithSequence = Create<Name> (pref.toUri ());
  nameWithSequence->appendSeqNum (seq);

  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (nameWithSequence);
  interest->SetInterestLifetime    (m_interestLifeTime);

  NS_LOG_INFO ("Requesting Interest: \n" << nameWithSequence->toUri());
  NS_LOG_INFO ("> Interest for " << seq);

  WillSendOutInterest (seq);  

  FwHopCountTag hopCountTag;
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);

  ScheduleNextPacket ();
}


void
ConsumerCbrRand2::SetRandomize (const std::string &value)
{
  if (m_random)
    delete m_random;

  if (value == "uniform")
    {
      m_random = new UniformVariable (0.0, 2 * 1.0 / m_frequency);
    }
  else if (value == "exponential")
    {
      m_random = new ExponentialVariable (1.0 / m_frequency, 50 * 1.0 / m_frequency);
    }
  else
    m_random = 0;

  m_randomType = value;  
}

std::string
ConsumerCbrRand2::GetRandomize () const
{
  return m_randomType;
}


///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
ConsumerCbrRand2::OnData (Ptr<const Data> data)
{
   Consumer::OnData (data); // tracing inside
   NS_LOG_DEBUG("Data received: " << data->GetName().toUri());
   uint32_t seq = data->GetName ().get (-1).toSeqNum ();
   if (m_pref.find(seq) != m_pref.end())
     m_pref.erase(m_pref.find(seq));
}

void
ConsumerCbrRand2::OnTimeout (uint32_t sequenceNumber)
{
  NS_LOG_DEBUG("Seq timeout! " << sequenceNumber
    << ", name prefix: " << m_pref[sequenceNumber]);
  Consumer::OnTimeout (sequenceNumber);
}

uint32_t
ConsumerCbrRand2::GetFaceId ()
{
  return m_face->GetId ();
}

} // namespace ndn
} // namespace ns3
