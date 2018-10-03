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
 
//This is a base strategy. It limits the max fib size
//Prefixes used for control messages are priorized.

#include "limited-fib-strategy.h"
#include <ns3/ndnSIM/model/ndn-content-object.h>
#include <ns3/ndnSIM/model/ndn-interest.h>
#include <ns3/ndnSIM/model/pit/ndn-pit.h>
#include <ns3/ndnSIM/model/pit/ndn-pit-entry.h>
#include "ns3/ndn-data.h"
#include "ns3/string.h"
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

NS_OBJECT_ENSURE_REGISTERED (LimitedFibStrategy);

LogComponent LimitedFibStrategy::g_log = LogComponent (LimitedFibStrategy::GetLogName ().c_str ());

std::string
LimitedFibStrategy::GetLogName ()
{
  return super::GetLogName ()+".LimitedFibStrategy";
}


TypeId
LimitedFibStrategy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::LimitedFibStrategy")
    .SetGroupName ("Ndn")
    .SetParent <super> ()
    .AddConstructor <LimitedFibStrategy> ()
    .AddAttribute ("MaxFibSize",
                   "Max number of fib entries",
                   StringValue ("100"),
                   MakeIntegerAccessor (&LimitedFibStrategy::m_maxfibsize),
                   MakeIntegerChecker<uint32_t> ())    
    
    ;
  return tid;
}

LimitedFibStrategy::LimitedFibStrategy ()
{
  m_fibentries.clear();
}

void
LimitedFibStrategy::RemoveFibEntry (Ptr<Name> name)
{

  Ptr<fib::Entry> fibentry = m_fib->Find (name->getSubName ());
  if (fibentry != 0)
  {
    m_fib->Remove (name);
    m_fibentries.remove (name->getSubName ());
	NS_LOG_INFO("Removed name from FIB: " << name->getSubName());
  }
}

bool equal_names (Name name1, Name name2)
{
  return name1 == name2;
}

void
LimitedFibStrategy::AddFibEntry (Ptr<Name> name, Ptr<Face> inFace, uint32_t metric)
{
  if (inFace == 0)
  {
	//NS_LOG_UNCOND("FALHA " << name->getSubName());
    return;
  }

  bool exist = false, notcontrol = false, toobig = false;
  std::string uri = "";
  //NS_LOG_UNCOND("TESTE " << name->getSubName());
  if (name->size() > 0)
    uri = name->get(0).toUri ();
  //NS_LOG_UNCOND("TESTE2 " << uri << " --- " << name->getSubName() << " --- " << inFace->GetId());
  
  //if ((name->toUri () != "/") &&
  if ((uri != "") &&
    (uri != "router") &&
    (uri != "controller") &&
    (uri != "controllerX") &&
	(uri != "controller0") &&
	(uri != "controller1") &&
	(uri != "controller2") && 
    (uri != "hello"))    
      notcontrol = true;
      
  //if (m_fib->GetSize () >= m_maxfibsize)
  if (m_fibentries.size () >= m_maxfibsize)
    toobig = true;
  
  if (notcontrol && (m_fib->Find (name->getSubName ()) != m_fib->End ()))
    exist = true;
  if (toobig && !(m_fibentries.empty()))
  {
    m_fib->Remove (Create<Name> (m_fibentries.back ()));
    m_fibentries.pop_back ();
	NS_LOG_INFO("Removed name from FIB e from control list: " << m_fibentries.back ());
  }
  
  
  Ptr<fib::Entry> fibentry = m_fib->Add (name, inFace , metric);
  if (fibentry != 0)
  {
	//if (name->toUri() != "/")
		fibentry->UpdateStatus (inFace, fib::FaceMetric::NDN_FIB_GREEN); 
  }

  if (notcontrol)
  {
    if (exist)
    {
      m_fibentries.remove (name->getSubName());
    }
    m_fibentries.push_front (name->getSubName());
	NS_LOG_INFO("Added name to control list: " << name->getSubName());
  }
}

void
LimitedFibStrategy::AddFibEntry (Name name, Ptr<Face> inFace, uint32_t metric)
{
  Ptr<Name> prefix = Create<Name> (name.getSubName ());
  AddFibEntry (prefix, inFace, metric);
}

void
LimitedFibStrategy::DidSendOutInterest (Ptr<Face> inFace, Ptr<Face> outFace, Ptr<const Interest> interest, Ptr<pit::Entry> pitEntry)
{
/* 	Name name = pitEntry->GetFibEntry()->GetPrefix();
	std::string uri = "";
	if (name.size() > 0)
		uri = name.get(0).toUri ();	
	if ((name.toUri () != "/") &&
		(uri != "") &&
		(uri != "controller") &&
		(uri != "controllerX") &&
		(uri != "controller0") &&
		(uri != "controller1") &&
		(uri != "controller2") &&
		(uri != "router") &&
		(uri != "registerNamedData") &&
		(uri != "hello"))
	{
	  m_fibentries.remove (name.getSubName());	  
	  m_fibentries.push_front (name.getSubName());
	} */ //from LRU to FIFO
	super::DidSendOutInterest (inFace, outFace, interest, pitEntry);
}

} // namespace fw
} // namespace ndn
} // namespace ns3
