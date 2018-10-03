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

//OSPFLike strategy
//It floods producer prefix announcement interests.

#include "flooding-prefix.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>

namespace ll = boost::lambda;

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (FloodingPrefix);

LogComponent FloodingPrefix::g_log = LogComponent (FloodingPrefix::GetLogName ().c_str ());

std::string
FloodingPrefix::GetLogName ()
{
  return super::GetLogName ()+".FloodingPrefix";
}


TypeId
FloodingPrefix::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::FloodingPrefix")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <FloodingPrefix> ()
    ;
  return tid;
}

FloodingPrefix::FloodingPrefix ()
{
}

void
FloodingPrefix::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
  
  Name pname = pitEntry->GetFibEntry()->GetPrefix();
  size_t size = pname.size();
  std::string c0;
  c0 = pname.get(0).toUri ();
  if (c0 != "floodingprefix")
  {
    m_fib->Remove (Create<Name> (pname));   
  }
  super::WillEraseTimedOutPendingInterest (pitEntry);
}

bool
FloodingPrefix::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> header,
                                Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << header->GetName ());
  Ptr<Name> nameWithSequence = Create<Name> (header->GetName ());
  uint32_t namesize = nameWithSequence->size ();
  uint32_t nodeidint = (inFace->GetNode())->GetId ();
  std::string nodeidstr;
  nodeidstr = boost::lexical_cast<std::string>(nodeidint);
  Ptr<Fib>  fib  = inFace->GetNode()->GetObject<Fib> ();

//Add Fib entries to prefix origins
//based on Interests from app flooding prefixes strating with floodingprefix
//filtering Interests with prefix "floodingprefix" not coming from the local node.
// Interest /floodingprefix/prefix
  if ((namesize > 1) &&
        (nameWithSequence->get(0).toUri() == "floodingprefix"))
  {
    NS_LOG_DEBUG ("Prefix Flooding Node: " << nodeidstr);
    Ptr<Name> neighbor = Create<Name> ();
    neighbor->append (nameWithSequence->getPrefix (namesize-2, 1));
    FwHopCountTag hopCountTag;
    header->GetPayload ()->PeekPacketTag (hopCountTag);
    
    if (hopCountTag.Get() > 0)
    {
      Ptr<fib::Entry> fibentneighbor = fib->Add (neighbor, inFace, hopCountTag.Get());
      if (fibentneighbor != 0)
        fibentneighbor->UpdateStatus (inFace, fib::FaceMetric::NDN_FIB_GREEN);
      NS_LOG_DEBUG ("Adding entry for prefix" << neighbor->toUri() << ", face:" << inFace->GetId ());
    }
  }

  int propagatedCount = 0;
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    {
      NS_LOG_DEBUG ("Trying " << boost::cref(metricFace));
      if (metricFace.GetStatus () == fib::FaceMetric::NDN_FIB_RED) // all non-read faces are in front
        break;

      if (!TrySendOutInterest (inFace, metricFace.GetFace (), header, pitEntry))
        {
          continue;
        }
      propagatedCount++;
      if(nameWithSequence->get(0).toUri() == "floodingprefix")
      {
        continue;
      }
      else
        break;
    }
  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
  return propagatedCount > 0;
}

} // namespace fw
} // namespace ndn
} // namespace ns3
