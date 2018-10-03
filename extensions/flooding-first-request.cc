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

//ARPLike
//First interests are flooded in all faces.
//Based on received content, new fib entries are added.

#include "flooding-first-request.h"
#include <ns3/ndnSIM/model/ndn-content-object.h>
#include <ns3/ndnSIM/model/ndn-interest.h>
#include <ns3/ndnSIM/model/pit/ndn-pit.h>
#include <ns3/ndnSIM/model/pit/ndn-pit-entry.h>
#include "ns3/ndn-data.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h>

namespace ll = boost::lambda;

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (FloodingFirstRequest);

LogComponent FloodingFirstRequest::g_log = LogComponent (FloodingFirstRequest::GetLogName ().c_str ());

std::string
FloodingFirstRequest::GetLogName ()
{
  return super::GetLogName ()+".FloodingFirstRequest";
}


TypeId
FloodingFirstRequest::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::FloodingFirstRequest")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <FloodingFirstRequest> ()
    ;
  return tid;
}

FloodingFirstRequest::FloodingFirstRequest ()
{
}

void
FloodingFirstRequest::WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry)
{
  
  Name pname = pitEntry->GetFibEntry()->GetPrefix();
  if ((pname.toUri () != "/"))
  {
    super::WillEraseTimedOutPendingInterest (pitEntry);
    RemoveFibEntry (Create<Name> (pitEntry->GetFibEntry()->GetPrefix()));
  }
}

bool
FloodingFirstRequest::DoPropagateInterest (Ptr<Face> inFace,
                                Ptr<const Interest> header,                                
                                Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << header->GetName ());
  int propagatedCount = 0;

  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    {
      NS_LOG_DEBUG ("Trying " << boost::cref(metricFace));
      if (metricFace.GetStatus () == fib::FaceMetric::NDN_FIB_RED) // all non-read faces are in front
        break;
      
      if (!TrySendOutInterest (inFace, metricFace.GetFace (), header,  pitEntry))
        {
          continue;
        }

      propagatedCount++;
      if (pitEntry->GetFibEntry ()->GetPrefix () == Name("/"))
      {
        continue;
      }
      else
        break;
    }

  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
  return propagatedCount > 0;
}

void
FloodingFirstRequest::SatisfyPendingInterest (Ptr<Face> inFace,
                                            Ptr<const Data> data,
                                            Ptr<pit::Entry> pitEntry)
{
  if (pitEntry->GetFibEntry ()->GetPrefix () == Name("/"))
  {
    NS_LOG_DEBUG ("Adding specific fib entry.");
    const Name contentname = (data->GetName ());
    Ptr<Name> newprefix = Create<Name> ((contentname.getPrefix (contentname.size () - 1)));
    FwHopCountTag hopCountTag1;
    data->GetPayload ()->PeekPacketTag (hopCountTag1);
    AddFibEntry (newprefix, inFace, hopCountTag1.Get());
    NS_LOG_DEBUG ("Adding entry for prefix" << contentname << ", face:" << inFace->GetId ());
  }
  
  super::SatisfyPendingInterest (inFace, data, pitEntry);
}

} // namespace fw
} // namespace ndn
} // namespace ns3
