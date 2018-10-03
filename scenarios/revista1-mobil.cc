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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include <boost/lexical_cast.hpp>
#include "ns3/ndn-link-control-helper.h"
#include "ndn-stack-helper-2.h"
#include <ns3/ndnSIM/apps/ndn-consumer-cbr.h>
#include "ndn-sdndn-giveroute.h"
#include "ndn-producer-controller.h"
#include "ndn-producer-registerrouter-v3.h"

using namespace ns3;

std::ofstream myfile;
std::ofstream confile;
std::ofstream lognodesfile;
ApplicationContainer groutecntnr;
ApplicationContainer producerctl;
ApplicationContainer producerrrt;

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
  producerctl = producerControllerHelper.Install (controllers); // last node

  // Producer
  ndn::AppHelper producerRegRouterHelper ("ns3::ndn::ProducerRegisterRouterV3");
  producerrrt = producerRegRouterHelper.Install (controllers); // last node

  //Producer
  ndn::AppHelper producerRegisterContentHelper ("ns3::ndn::ProducerRegisterContent");
  producerRegisterContentHelper.Install (controllers);

  //Asking for route in controller
  ndn::AppHelper askRoute ("ns3::ndn::SDNDNAskRoute");
  askRoute.SetAttribute("Prefix", StringValue("/"));
  askRoute.Install (nodes);

  //Producer
  ndn::AppHelper producerRoute ("ns3::ndn::SDNDNGiveRoute");
  groutecntnr = producerRoute.Install (controllers);
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
  controllers.Add (Names::Find<Node> (contrx));

  std::string filelognode = "./temp/"+suffix+"/lognodes.txt";
  lognodesfile.open ((char*)filelognode.c_str (), std::ios_base::app | std::ios_base::ate | std::ios_base::out);

  producers.Add (Names::Find<Node> (prodx));
  producers.Add (Names::Find<Node> ("Rtr4"));
  producers.Add (Names::Find<Node> ("Rtr6"));
  producers.Add (Names::Find<Node> ("Rtr2"));

  lognodesfile << "\n Producers: \n";
  for (NodeContainer::Iterator i = producers.Begin (); i != producers.End (); ++i)
  {
    lognodesfile << Names::FindName (*i) << "\n";
  }

  consumers.Add (Names::Find<Node> (consx));

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

  configureCRoSNDN (prefix, nprefix, nodes,
    hellorate, controllers, producers, consumers, regrate, fibsize);

  // Simulated traffic
  // Consumer
  ApplicationContainer app;
  ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute ("Frequency", StringValue (rate));
  consumerHelper.SetAttribute ("Randomize", StringValue ("exponential"));
  consumerHelper.SetAttribute ("RetxTimer", StringValue ("150ms"));
  consumerHelper.SetAttribute ("LifeTime", StringValue ("500ms"));
  //consumerHelper.SetAttribute ("Window", StringValue ("5"));

  uint32_t nprefixint =   boost::lexical_cast<uint32_t> (nprefix);
  for (uint32_t i = 0; i < nprefixint; i++)
  {
	ndn::Name aux1 = ndn::Name("/");
	//aux1.appendSeqNum((producers.Get (0))->GetId ());
	aux1.append ("prefix");
	aux1.appendSeqNum(boost::lexical_cast<uint64_t>(i));
	consumerHelper.SetAttribute("Prefix", StringValue(aux1.toUri ()));
	ApplicationContainer app1 = consumerHelper.Install (consumers);
	//app1.Start (Seconds (i * timesim / nprefixint));
	app.Add (app1);
  }	

  app.Start (Seconds (1));

  // Producer
  ndn::Name aux = ndn::Name("/");
  ApplicationContainer appP, appR;
  for (NodeContainer::Iterator i = producers.Begin (); i != producers.End (); ++i)
  {
    aux = ndn::Name("/");
    //aux.appendSeqNum((*i)->GetId ());
	aux.append ("prefix");
    ndn::AppHelper producerHelper ("ns3::ndn::Producer");
    producerHelper.SetAttribute("Prefix", StringValue(aux.toUri ()));
    producerHelper.SetAttribute ("PayloadSize", StringValue("1024"));
    appP.Add (producerHelper.Install (*i));

  //Register Content Name in controller  //Consumer	
    ndn::AppHelper consumerRegisterContentHelper ("ns3::ndn::ConsumerRegisterContent");
    consumerRegisterContentHelper.SetAttribute("Prefix", StringValue(aux.get (0).toBlob ()));
    consumerRegisterContentHelper.SetAttribute("NPrefixes",
      IntegerValue(boost::lexical_cast<uint32_t>(nprefix)));
    consumerRegisterContentHelper.SetAttribute("RegRate",
      IntegerValue(boost::lexical_cast<uint32_t>(regrate)));
    consumerRegisterContentHelper.SetAttribute ("Frequency", StringValue (hellorate));
    appR.Add (consumerRegisterContentHelper.Install (*i));	
  }
  
  uint32_t i = 0;
  uint32_t napp = appP.GetN();
  while (i < napp)
  {
    //while((i * constime) < timesim)
    {
	  appP.Get(i % napp)->SetStartTime (Seconds (constime * i + 1));
	  appR.Get(i % napp)->SetStartTime (Seconds (constime * i + 1));
	  if (i != napp - 1)
	  {
		  appP.Get(i % napp)->SetStopTime (Seconds (constime * (i + 1)));	
		  appR.Get(i % napp)->SetStopTime (Seconds (constime * (i + 1)));
	  }
	  i++;
    }
  }

  // The failure of the link
  if (fail == 1)
  {
    Simulator::Schedule (Seconds (1000), ndn::LinkControlHelper::FailLinkByName, "Rtr1", "Rtr7");
    Simulator::Schedule (Seconds (2000), ndn::LinkControlHelper::FailLinkByName, "Rtr2", "Rtr4");
  }
  else if (fail == 2)
  {
    Simulator::Schedule (Seconds (100), ndn::LinkControlHelper::FailLinkByName, "Rtr1", "Rtr7");
  }
  else if (fail == 3)
  {
	Simulator::Schedule (Seconds (1000), ndn::LinkControlHelper::FailLinkByName, "Rtr1", "Rtr7");
	Simulator::Schedule (Seconds (2000), ndn::LinkControlHelper::UpLinkByName, "Rtr1", "Rtr7");
  }
  

  Simulator::Stop (Seconds (timesim));

  // Traces
  std::string tracepostfix = strategy+topo;
  ndn::L3AggregateTracer::InstallAll ("./temp/"+suffix+"/aggregate-trace-"+tracepostfix, Seconds (samplei));
  L2RateTracer::InstallAll ("./temp/"+suffix+"/drop-trace.txt"+tracepostfix, Seconds (samplei));

  Simulator::Run ();

  //Generating r files for post processing traces. R files identify consumer faces.
  std::string filec = "./temp/"+suffix+"/cface-"+strategy+".r";
  confile.open ((char*)filec.c_str (), std::ios_base::trunc | std::ios_base::ate | std::ios_base::out);
  confile << "topo = '" + topo + "'\n";
  std::string cns = "consumer = c('";
  //confile << "producer = '" + (Names::FindName (nodes.Get (prod))) +"' \n";
  confile << "producer = '" + prodx +"' \n";
  confile << "faceid = c(";
  uint32_t nApplications = app.GetN ();
  bool fapp = true;
  for (uint32_t i = 0; i < nApplications; i++)
  {
    Ptr<ns3::Application> p = app.Get (i);
    Ptr<ns3::ndn::ConsumerCbr> pc = DynamicCast<ns3::ndn::ConsumerCbr> (p);
    if (pc->GetId () > 0 && fapp)
    {
      confile << pc->GetId ();
      fapp = false;
      cns = cns + Names::FindName (p->GetNode());
    }
    else if (pc->GetId () > 0)
    {
      confile << "," << pc->GetId ();
      cns = cns + "','" + Names::FindName (p->GetNode());
    }
  }

  confile << ")\n";
  cns = cns + "') \n";

  confile << cns; 
  if (groutecntnr.GetN () > 0)
  {
	  Ptr<ns3::ndn::SDNDNGiveRoute> groute = DynamicCast<ns3::ndn::SDNDNGiveRoute> (groutecntnr.Get(0));
	  if (groute->GetId () > 0)
	  {
		  confile << "controller = '";
		  confile << Names::FindName (groutecntnr.Get(0)->GetNode ());
		  confile << "'\n";
		  confile << "giverouteface = c(";
		  confile << groute->GetId ();
		  confile << ")\n";
	  }
  }  
  
  if (producerctl.GetN () > 0)
  {
	  Ptr<ns3::ndn::ProducerController> pctl = DynamicCast<ns3::ndn::ProducerController> (producerctl.Get(0));
	  if (pctl->GetId () > 0)
	  {
		  confile << "cdiscoveryface = c(";
		  confile << pctl->GetId ();
		  confile << ")\n";
	  }
  } 
  
  if (producerrrt.GetN () > 0)
  {
	  Ptr<ns3::ndn::ProducerRegisterRouterV3> prrt = DynamicCast<ns3::ndn::ProducerRegisterRouterV3> (producerrrt.Get(0));
	  if (prrt->GetId () > 0)
	  {
		  confile << "rrouterface = c(";
		  confile << prrt->GetId ();
		  confile << ")\n";
	  }
  }   
  confile.close ();

  Simulator::Destroy ();
  myfile.close();

  return 0;
}

