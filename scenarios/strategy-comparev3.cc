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
//Temporal simulation
//It receives integer numbers to determine consumer and producer positions.
//Controller position can be given by interger number (randon option: CRoS-NDN-CRP) or node name (fixed option: CRoS-NDN-CFP).
//called by PacketxTime6.sh 

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include <boost/lexical_cast.hpp>
#include "ns3/ndn-link-control-helper.h"
#include "ndn-stack-helper-2.h"
#include "ndn-consumer-cbr-rand2.h"
#include "ndn-consumer-cbr-rand3.h"
#include "ndn-consumer-cbr-rand.h"

using namespace ns3;

std::ofstream myfile;
std::ofstream confile;
std::ofstream lognodesfile;

void
PrintFib (NodeContainer nodes, std::string strategy)
{
  for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
  {
    Ptr<ndn::Fib> fib = (*i)->GetObject<ndn::Fib> ();
    Ptr<ndn::Pit> pit = (*i)->GetObject<ndn::Pit> ();
    std::cout << "\n" << "Time: " << Simulator::Now ().ToDouble (Time::S) << "\t";
    std::cout << strategy << "\n";
    std::cout << "Node: " << (*i)->GetId() << "\n" <<
      "Fib Size: " << fib->GetSize() <<"\n" <<
      "Pit Size: " << pit->GetSize() << "\n";
    fib->Print (std::cout); 
    std::cout << "\n";
  }
}

void
PrintStatsNode (Ptr<Node> node, std::string strategy)
{
  Ptr<ndn::Pit> pit = node->GetObject<ndn::Pit> ();
  Ptr<ndn::Fib> fib = node->GetObject<ndn::Fib> ();
  myfile << Simulator::Now ().ToDouble (Time::S) << "\t"
            << node->GetId () << "\t"
            << Names::FindName (node) << "\t"
            << pit->GetSize () << "\t"
            << fib->GetSize () << "\t"
            << strategy << "\n";            
}

void
PrintStats (NodeContainer nodes, std::string strategy)
{
  for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
  {
    PrintStatsNode (*i, strategy);
  }
}


void
configureGlobalRouting (std::string prefix, std::string nprefix, NodeContainer &producers)
{
  ndn::StackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
  ndnHelper.InstallAll ();
  
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();
  
  ndn::Name aux = ndn::Name("/"+prefix);
  
  uint32_t total = boost::lexical_cast<uint32_t>(nprefix);

  // Add /prefix origins to ndn::GlobalRouter
  for (NodeContainer::Iterator i = producers.Begin (); i != producers.End (); ++i)
  {
    aux = ndn::Name();
    aux.appendSeqNum((*i)->GetId ());
    ndnGlobalRoutingHelper.AddOrigins (aux.toUri(), (*i));
  }

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes ();
}

void
configureGlobalRoutingAll (std::string prefix, std::string nprefix, NodeContainer &producers)
{
  ndn::StackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::SmartFlooding");
  ndnHelper.InstallAll ();
  
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll ();
  
  ndn::Name aux = ndn::Name("/"+prefix);
  uint32_t total = boost::lexical_cast<uint32_t>(nprefix);

  // Add /prefix origins to ndn::GlobalRouter
  for (NodeContainer::Iterator i = producers.Begin (); i != producers.End (); ++i)
  {
    aux = ndn::Name();
    aux.appendSeqNum((*i)->GetId ());
    ndnGlobalRoutingHelper.AddOrigins (aux.toUri(), (*i));
  }

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes ();
}

void
configureFlooding ()
{
  ndn::StackHelper ccnxHelper;
  ccnxHelper.SetDefaultRoutes (true);
  ccnxHelper.InstallAll ();
}

void
configureFloodingFirstRequest (std::string fibsize)
{
  ndn::StackHelper ccnxHelper;
  ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::FloodingFirstRequest", "MaxFibSize", fibsize);  
  ccnxHelper.SetDefaultRoutes (true);
  ccnxHelper.InstallAll ();
}


void
configurePrefixFlooding (std::string prefix, std::string nprefix, 
  std::string hellorate, NodeContainer &producers, NodeContainer &nodes)
{
  ns3::ndn::StackHelper2 ccnxHelper;
  ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::FloodingPrefix");
  ccnxHelper.SetDefaultRoutes(true);
  ccnxHelper.SetDefaultPrefix ("/floodingprefix");
  ccnxHelper.InstallAll ();
  
  //Flood prefixes 
  //Consumer
  ndn::Name aux = ndn::Name("/");
  for (NodeContainer::Iterator i = producers.Begin (); i != producers.End (); ++i)
  {
    aux = ndn::Name("/");
    aux.appendSeqNum((*i)->GetId ());
    ndn::AppHelper consumerRegisterContentHelper1 ("ns3::ndn::ConsumerFloodingPrefix");
    consumerRegisterContentHelper1.SetAttribute("Prefix", StringValue(aux.get (0).toBlob ()));
    consumerRegisterContentHelper1.SetAttribute("NPrefixes", 
      IntegerValue(boost::lexical_cast<uint32_t> (nprefix)));
    consumerRegisterContentHelper1.SetAttribute ("Frequency", StringValue (hellorate));
    ApplicationContainer appProd = consumerRegisterContentHelper1.Install (*i);  
  }  
}


void
configureNLSRLike (std::string prefix, std::string nprefix,
  NodeContainer &nodes,   std::string hellorate,
  NodeContainer &controllers, NodeContainer &producers, NodeContainer &consumers,
  std::string regrate, std::string fibsize)
{
  // Install CCNx stack on all nodes
  ndn::StackHelper ccnxHelper;
  ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRouteNLSRLike", "MaxFibSize", fibsize);
  ccnxHelper.InstallAll ();

  //Neighbor discovery
  // ConsumerNodeID
  ndn::AppHelper ConsumerNodeIDHelper ("ns3::ndn::ConsumerNodeIDNLSRLike");
  ConsumerNodeIDHelper.SetAttribute ("Frequency", StringValue (hellorate)); // 2 interests a second
  ApplicationContainer app0 = ConsumerNodeIDHelper.Install (nodes);

  // NodeIDReply
  ndn::AppHelper NodeIDReplyHelper ("ns3::ndn::ProducerNodeIDNLSRLikeV2");
  NodeIDReplyHelper.SetAttribute ("Prefix", StringValue("/hello"));  
  NodeIDReplyHelper.SetAttribute ("Frequency", StringValue (hellorate));
  NodeIDReplyHelper.Install (nodes);
  
  //Register Content Name in controller  
  //Consumer
  ndn::Name aux = ndn::Name("/");
  for (NodeContainer::Iterator i = producers.Begin (); i != producers.End (); ++i)
  {
    aux = ndn::Name("/");
    aux.appendSeqNum((*i)->GetId ());
    ndn::AppHelper consumerRegisterContentHelper ("ns3::ndn::ConsumerRegisterContentNLSRLike");
    consumerRegisterContentHelper.SetAttribute("Prefix", StringValue(aux.get (0).toBlob ()));
    consumerRegisterContentHelper.SetAttribute("NPrefixes", 
      IntegerValue(boost::lexical_cast<uint32_t>(nprefix)));
    consumerRegisterContentHelper.SetAttribute("RegRate", 
      IntegerValue(boost::lexical_cast<uint32_t>(regrate)));
    ApplicationContainer app5 = consumerRegisterContentHelper.Install (*i); 
  }
  
  //Producer
  ndn::AppHelper producerRegisterContentHelper ("ns3::ndn::ProducerRegisterContentNLSRLike");
  producerRegisterContentHelper.Install (nodes);
  
  ndn::AppHelper producergethashesHelper ("ns3::ndn::ProducerGetHashesNLSRLike");
  producergethashesHelper.Install (nodes);
  
  ndn::AppHelper producergetlsaHelper ("ns3::ndn::ProducerGetLSANLSRLike");
  producergetlsaHelper.Install (nodes);
  
  ndn::AppHelper askRoute ("ns3::ndn::NLSRLikeAskRoute");
  askRoute.SetAttribute("Prefix", StringValue("/"));
  askRoute.Install (nodes);  
}  

void
configureNLSRV2 (std::string prefix, std::string nprefix,
  NodeContainer &nodes,   std::string hellorate,
  NodeContainer &controllers, NodeContainer &producers, NodeContainer &consumers,
  std::string regrate, std::string fibsize)
{
  // Install CCNx stack on all nodes
  ndn::StackHelper ccnxHelper;
  ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRouteNLSRV2", "MaxFibSize", fibsize);
  ccnxHelper.InstallAll ();

  //Neighbor discovery
  // ConsumerNodeID
  ndn::AppHelper ConsumerNodeIDHelper ("ns3::ndn::ConsumerNodeIDNLSRV2");
  ConsumerNodeIDHelper.SetAttribute ("Frequency", StringValue (hellorate)); // 2 interests a second
  ApplicationContainer app0 = ConsumerNodeIDHelper.Install (nodes);

  // NodeIDReply
  ndn::AppHelper NodeIDReplyHelper ("ns3::ndn::ProducerNodeIDNLSRV2");
  NodeIDReplyHelper.SetAttribute ("Prefix", StringValue("/hello"));  
  NodeIDReplyHelper.SetAttribute ("Frequency", StringValue (hellorate));
  NodeIDReplyHelper.Install (nodes);
  
  //Register Content Name in controller  
  //Consumer
  ndn::Name aux = ndn::Name("/");
  for (NodeContainer::Iterator i = producers.Begin (); i != producers.End (); ++i)
  {
    aux = ndn::Name("/");
    aux.appendSeqNum((*i)->GetId ());
    ndn::AppHelper consumerRegisterContentHelper ("ns3::ndn::ConsumerRegisterContentNLSRLike");
    consumerRegisterContentHelper.SetAttribute("Prefix", StringValue(aux.get (0).toBlob ()));
    consumerRegisterContentHelper.SetAttribute("NPrefixes", 
      IntegerValue(boost::lexical_cast<uint32_t>(nprefix)));
    consumerRegisterContentHelper.SetAttribute("RegRate", 
      IntegerValue(boost::lexical_cast<uint32_t>(regrate)));
    ApplicationContainer app5 = consumerRegisterContentHelper.Install (*i); 
  }
  
  //Producer
  ndn::AppHelper producerRegisterContentHelper ("ns3::ndn::ProducerRegisterContentNLSRV2");
  producerRegisterContentHelper.Install (nodes);
  
  ndn::AppHelper producergethashesHelper ("ns3::ndn::ProducerGetHashesNLSRV2");
  producergethashesHelper.Install (nodes);
  
  ndn::AppHelper producergetlsaHelper ("ns3::ndn::ProducerGetLSANLSRV2");
  producergetlsaHelper.Install (nodes);
  
  ndn::AppHelper askRoute ("ns3::ndn::NLSRV2AskRoute");
  askRoute.SetAttribute("Prefix", StringValue("/"));
  askRoute.Install (nodes);  
}  


void
configureCRoSNDN (std::string prefix, std::string nprefix,
  NodeContainer &nodes,   std::string hellorate,
  NodeContainer &controllers, NodeContainer &producers, NodeContainer &consumers,
  std::string regrate, std::string fibsize)
{
  // Install CCNx stack on all nodes
  ndn::StackHelper ccnxHelper;
  ccnxHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRouteSDNDNController", "MaxFibSize", fibsize);
//  ccnxHelper.SetContentStore ("ns3::ndn::cs::Freshness::Fifo", "MaxSize", "1000");
  ccnxHelper.InstallAll ();

  //Neighbor discovery
  // ConsumerNodeID
  ndn::AppHelper ConsumerNodeIDHelper ("ns3::ndn::ConsumerNodeID");
  ConsumerNodeIDHelper.SetAttribute ("Frequency", StringValue (hellorate)); // 2 interests a second
  ApplicationContainer app0 = ConsumerNodeIDHelper.Install (nodes);

  // NodeIDReply
  ndn::AppHelper NodeIDReplyHelper ("ns3::ndn::ProducerNodeIDV3");
  NodeIDReplyHelper.SetAttribute ("Prefix", StringValue("/hello")); 
  NodeIDReplyHelper.SetAttribute ("Frequency", StringValue (hellorate)); 
  NodeIDReplyHelper.Install (nodes);
  
  // Producer
  ndn::AppHelper producerControllerHelper ("ns3::ndn::ProducerController");
  ApplicationContainer appx = producerControllerHelper.Install (controllers); // last node

  // Producer
  ndn::AppHelper producerRegRouterHelper ("ns3::ndn::ProducerRegisterRouterV3");
  producerRegRouterHelper.Install (controllers); // last node
 
  //Register Content Name in controller  
  //Consumer
  ndn::Name aux = ndn::Name("/");
  for (NodeContainer::Iterator i = producers.Begin (); i != producers.End (); ++i)
  {
    aux = ndn::Name("/");
    aux.appendSeqNum((*i)->GetId ());
    ndn::AppHelper consumerRegisterContentHelper ("ns3::ndn::ConsumerRegisterContent");
    consumerRegisterContentHelper.SetAttribute("Prefix", StringValue(aux.get (0).toBlob ()));
    consumerRegisterContentHelper.SetAttribute("NPrefixes", 
      IntegerValue(boost::lexical_cast<uint32_t>(nprefix)));
    consumerRegisterContentHelper.SetAttribute("RegRate", 
      IntegerValue(boost::lexical_cast<uint32_t>(regrate)));
    consumerRegisterContentHelper.SetAttribute ("Frequency", StringValue (hellorate));
    ApplicationContainer app5 = consumerRegisterContentHelper.Install (*i); 
  }
  
  //Producer
  ndn::AppHelper producerRegisterContentHelper ("ns3::ndn::ProducerRegisterContent");
  producerRegisterContentHelper.Install (controllers);
  
  //Asking for route in controller
  ndn::AppHelper askRoute ("ns3::ndn::SDNDNAskRoute");
  askRoute.SetAttribute("Prefix", StringValue("/"));
  askRoute.Install (nodes);
  
  //Producer
  ndn::AppHelper producerRoute ("ns3::ndn::SDNDNGiveRoute");
  producerRoute.Install (controllers);  
}


void
configureStrategy (std::string prefix, std::string nprefix, std::string strategy, 
   NodeContainer &nodes,   std::string hellorate,
   NodeContainer &controllers, NodeContainer &producers, NodeContainer &consumers,
   std::string regrate, std::string fibsize)
{
  if (strategy == "CRoS-NDN" || strategy == "CRoS-NDN-CRP" || strategy == "CRoS-NDN-CFP")
    configureCRoSNDN (prefix, nprefix, nodes, hellorate, controllers, producers, consumers, regrate, fibsize);
  else if (strategy == "NLSRLike")
    configureNLSRV2 (prefix, nprefix, nodes, hellorate, controllers, producers, consumers, regrate, fibsize);       
  else if (strategy == "OSPFLike")
    configurePrefixFlooding (prefix, nprefix, hellorate, producers, nodes);
  else if (strategy == "Flooding")
    configureFlooding ();
  else if (strategy == "Omniscient")
    configureGlobalRouting (prefix, nprefix, producers);
  else if (strategy == "Omniscientv2")
    configureGlobalRoutingAll (prefix, nprefix, producers);    
  else if (strategy == "ARPLike")
    configureFloodingFirstRequest (fibsize);
  else
    std::cout << "Error! Stratey is not valid.";
}


int 
main (int argc, char *argv[])
{
  std::string prefix = "myname";
  std::string nprefix = "1";
  std::string strategy = "CRoS-NDN";
  std::string hellorate = "0.01";
  std::string topo = "topo-3-way.txt";
  std::string consx = "Src";//"Src";
  uint32_t consxint = 1;
  std::string prodx = "Prd"; //"Prd";
  uint32_t prodxint = 1;
  std::string contrx = "Ctr1";
  std::string rate = "1";
  std::string consprefs = "1";
  std::string consperc = "10";
  uint32_t timesim = 200;
  uint32_t fail = 0;
  std::string failnode1 = "Rtr7";
  std::string failnode2 = "Rtr1";
  double constime = 0;  
  std::string regrate = "1";
  std::string fibsize = "100";
  uint32_t seed = 0;
  double samplei = 20;
  std::string suffix = "";
  
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("prefix", "Prefix", prefix);
  cmd.AddValue("nprefix", "Number of published prefixes", nprefix);
  cmd.AddValue("strategy", 
    "Type of strategy: CRoS-NDN, NLSRLike, OSPFLike, Flooding, ARPLike or GlobalRouting Omniscient", strategy);
  cmd.AddValue("topo", "Topology file in scenarios/topologies/", topo);
  cmd.AddValue("consumer", "Consumer Name", consx);
  cmd.AddValue("consxint", "Consumer number", consxint);
  cmd.AddValue("producer", "Producer", prodx);
  cmd.AddValue("prodxint", "Producer number", prodxint);
  cmd.AddValue("controller", "Controller", contrx);
  cmd.AddValue("rate", "Consumer interest rate", rate);
  cmd.AddValue("hellorate", "Control Interests rate", hellorate);
  cmd.AddValue("consprefs", "Number of consumed prefixes", consprefs);
  cmd.AddValue("consperc", "Fraction of new consumed prefixes", consperc);  
  cmd.AddValue("time", "Total simulation time", timesim);
  cmd.AddValue("failtime", "Failure time", fail);
  cmd.AddValue("fail1", "Fail edge node 1", failnode1);
  cmd.AddValue("fail2", "Fail edge node 2", failnode2);
  cmd.AddValue("constime", "Consumer start time", constime);
  cmd.AddValue("regrate", "Taxa de registro de conteÃƒÂºdos", regrate);
  cmd.AddValue("fibsize", "Fib size", fibsize);
  cmd.AddValue("seed", "Seed generator", seed);
  cmd.AddValue("samplei", "Sample interval", samplei);
  cmd.AddValue("suffix", "Directory name suffix", suffix);
  cmd.Parse (argc, argv);

  if ((boost::lexical_cast<uint32_t> (consprefs)) > (boost::lexical_cast<uint32_t> (nprefix)))
  {
    std::cout << "Error! consprefs should be smaller than nprefix\n" 
      << consprefs << " " << nprefix;
    return 0;
  }
  
  Config::SetDefault ("ns3::ndn::RttEstimator::MaxRTO", StringValue ("1s"));
  Config::SetGlobal ("RngRun", IntegerValue (seed));  
  
  AnnotatedTopologyReader topologyReader ("");
  topologyReader.SetFileName ("./scenarios/topologies/"+topo);
  topologyReader.Read ();

  // Creating nodes
  // NodeContainer nodes;
  NodeContainer controllers;
  NodeContainer producers;
  NodeContainer consumers;
  NodeContainer nodes =  topologyReader.GetNodes();
  
  std::cout << "\n Topology: " << topo << " \n" << "Number of nodes: " << nodes.GetN () << "\n";

  //Used in installing consumers and producers
  uint32_t cons = boost::lexical_cast<uint32_t> (consx) % nodes.GetN ();
  uint32_t prod = boost::lexical_cast<uint32_t> (prodx) % nodes.GetN ();

  uint32_t contr = boost::lexical_cast<uint32_t> (contrx) % nodes.GetN ();
  controllers.Add (nodes.Get (contr));
 
  std::string filelognode = "./temp/"+suffix+"/lognodes.txt";
  lognodesfile.open ((char*)filelognode.c_str (), std::ios_base::app | std::ios_base::ate | std::ios_base::out);
   
  UniformVariable m_randNode;
  if (prodxint == nodes.GetN () || prodxint == 0)
    producers.Add (nodes);
  else if (prodxint == 1)
    producers.Add (nodes.Get (prod));
  else
  {
    std::cout << "Error! prodxint should be 0 (all nodes) or 1 (one node)!\n";
    return 0;
  }
      
  lognodesfile << "\n Producers: \n";
  for (NodeContainer::Iterator i = producers.Begin (); i != producers.End (); ++i)
  {
    lognodesfile << Names::FindName (*i) << "\n";
  }
  
  if (consxint == 1)
  {
    if (prodxint == 1 and prod == cons)
      cons = (cons + 1)  % nodes.GetN ();    
    consumers.Add (nodes.Get (cons));
  }
  else 
    while (consxint > consumers.GetN ())
    {
      consumers.Add (nodes.Get ((cons + m_randNode.GetInteger (0,nodes.GetN())) % nodes.GetN ()));
    }

  lognodesfile << "\n Consumers: \n";
  for (NodeContainer::Iterator i = consumers.Begin (); i != consumers.End (); ++i)
  {
    lognodesfile << Names::FindName (*i) << "\n";
  }
  
  lognodesfile.close ();

  // Set up node stats
  std::string filex = "./temp/"+suffix+"/Pit-Fib-"+topo;
  myfile.open ((char*)filex.c_str (), std::ios_base::app | std::ios_base::ate | std::ios_base::out);
  myfile << "Time" << "\t"
            << "NodeId" << "\t"
            << "NodeName" << "\t"
            << "NumberOfPitEntries" << "\t"
            << "NumberOfFibEntries" << "\t"
            << "Strategy" << "\n";            
  Simulator::Schedule (Seconds (timesim-1), PrintStats, nodes, strategy);

  configureStrategy (prefix, nprefix, strategy, nodes, 
    hellorate, controllers, producers, consumers, regrate, fibsize);

  // Simulated traffic
  // Consumer
  ApplicationContainer app;
  if (prodxint == 1)
  {
    ndn::Name aux1 = ndn::Name("/");
    aux1.appendSeqNum((producers.Get (0))->GetId ());
    ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbrRand");
    consumerHelper.SetAttribute("Prefix", StringValue(aux1.toUri ()));
    consumerHelper.SetAttribute ("Frequency", StringValue (rate)); 
    consumerHelper.SetAttribute ("LifeTime", StringValue ("0.5"));
    consumerHelper.SetAttribute ("NumberOfPrefixes", StringValue (consprefs));
    consumerHelper.SetAttribute ("PercentOfNewPrefix", StringValue (consperc));
    app = consumerHelper.Install (consumers); // first node
    app.Start (Seconds (constime));
  }
  else
  {
    ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbrRand3");
    consumerHelper.SetAttribute("Prefix", StringValue("/"));
    consumerHelper.SetAttribute ("Frequency", StringValue (rate));
    consumerHelper.SetAttribute ("LifeTime", StringValue ("0.5"));
    consumerHelper.SetAttribute ("NumberOfPrefixes", StringValue (consprefs));
    consumerHelper.SetAttribute ("NumberOfNodes", StringValue (boost::lexical_cast<std::string> (nodes.size ())));
    consumerHelper.SetAttribute ("PercentOfNewPrefix", StringValue (consperc));
    app = consumerHelper.Install (consumers); // first node
    app.Start (Seconds (constime));
  }

  // Producer
  ndn::Name aux = ndn::Name("/");
  for (NodeContainer::Iterator i = producers.Begin (); i != producers.End (); ++i)
  {
    aux = ndn::Name("/");
    aux.appendSeqNum((*i)->GetId ());
    ndn::AppHelper producerHelper ("ns3::ndn::Producer");
    producerHelper.SetAttribute("Prefix", StringValue(aux.toUri ()));
    producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
    producerHelper.Install (*i);
  }

  // The failure of the link  
  if (fail > 0)
  {
    Simulator::Schedule (Seconds (fail), ndn::LinkControlHelper::FailLinkByName, failnode1, failnode2);
  } 
  
  Simulator::Stop (Seconds (timesim));
  
  // Traces
  std::string tracepostfix = strategy+topo;
  ndn::L3AggregateTracer::InstallAll ("./temp/"+suffix+"/aggregate-trace-"+tracepostfix, Seconds (samplei));
  
  Simulator::Run ();

  //Generating r files for post processing traces. R files identify consumer faces.
  std::string filec = "./temp/"+suffix+"/cface-"+strategy+".r";
  confile.open ((char*)filec.c_str (), std::ios_base::trunc | std::ios_base::ate | std::ios_base::out);
  confile << "topo = '" + topo + "'\n";
  std::string cns = "consumer = c('";
  confile << "producer = '" + (Names::FindName (nodes.Get (prod))) +"' \n";  
  confile << "faceid = c(";
  uint32_t nApplications = app.GetN ();
  bool fapp = true;
  for (uint32_t i = 0; i < nApplications; i++)
  {
    Ptr<ns3::Application> p = app.Get (i);
    if (prodxint == 1)
    {
      Ptr<ns3::ndn::ConsumerCbrRand> pc = DynamicCast<ns3::ndn::ConsumerCbrRand> (p);
      if (pc->GetFaceId () > 0 && fapp)
      {
        confile << pc->GetFaceId ();
        fapp = false;
        cns = cns + Names::FindName (p->GetNode());
      }
      else if (pc->GetFaceId () > 0)
      {
        confile << "," << pc->GetFaceId ();
        cns = cns + "','" + Names::FindName (p->GetNode());
      }   
    }
    else
    {
      Ptr<ns3::ndn::ConsumerCbrRand3> pc = DynamicCast<ns3::ndn::ConsumerCbrRand3> (p);
      if (pc->GetFaceId () > 0 && fapp)
      {
        confile << pc->GetFaceId ();
        fapp = false;
        cns = cns + Names::FindName (p->GetNode());
      }
      else if (pc->GetFaceId () > 0)
      {
        confile << "," << pc->GetFaceId ();
        cns = cns + "','" + Names::FindName (p->GetNode());
      }
    }
  }
  confile << ")\n";
  cns = cns + "') \n";
  confile << cns;
  confile.close (); 
  
  Simulator::Destroy ();
  myfile.close();

  return 0;
}
