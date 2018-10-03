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


#ifndef NDNSIM_BEST_ROUTE_SDNDN_CONTROLLER_H
#define NDNSIM_BEST_ROUTE_SDNDN_CONTROLLER_H

#include "limited-fib-strategy.h"
#include <ns3/ndnSIM/model/ndn-name-components.h>
#include "ns3/log.h"
#include "boost-graph-ndn-control-routing.h"
#include <set>
#include <list>
#include <iterator>

namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup ndn
 * \brief Best route strategy
 */
class BestRouteSDNDNController :
    public LimitedFibStrategy
{
private:
  typedef LimitedFibStrategy super;

public:
  static TypeId
  GetTypeId ();

  /**
   * @brief Helper function to retrieve logging name for the forwarding strategy
   */
  static std::string
  GetLogName ();
  
  /**
   * @brief Default constructor
   */
  BestRouteSDNDNController ();
  
  virtual ~BestRouteSDNDNController ();
  
  
/*******************************************
//Genereal router node related methods
********************************************/

  void
  IncrementState();
  
  uint32_t
  GetState();

  void
  AddFibEntryCtl (Ptr<Name> name, Ptr<Face> inFace, uint32_t metric);

  //Called on each node router when receiving controller response for discovery
  void
  SetController (Ptr<Name> controller);

  //Called on each node router when registering in the controller
  Ptr<Name> 
  GetController ();

  //Called on each router note by NodeId app
  void
  AddNodeNeighbors(uint32_t id);
  
  void
  RemoveNodeNeighbors(uint32_t id);
  
  //Called on each router note by controller-get-router-neighbor
  std::set<uint32_t>
  GetNodeNeighbors ();

  bool
  GetNewNeighbor ();
  
  void
  SetNewNeighbor (bool flag);
  
  bool
  GetRegisteredRouter ();
  
  void
  SetRegisteredRouter (bool flag);
  
  bool
  GetRegisteredContent ();
  
  void
  SetRegisteredContent (bool flag);  
  
  bool
  GetIsThereController (); 


  //from super
  virtual void
  DidExhaustForwardingOptions (Ptr<Face> inFace,
                               Ptr<const Interest> header,
                               Ptr<pit::Entry> pitEntry);
  
  // from super                               
  virtual void
  FailedToCreatePitEntry (Ptr<Face> inFace,
                          Ptr<const Interest> header);    

  // from super                               
  virtual void
  WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry);

  // from forwarding-strategy
  virtual void
  SatisfyPendingInterest (Ptr<Face> inFace, // 0 allowed (from cache)
                          Ptr<const Data> header,
                          Ptr<pit::Entry> pitEntry);

  // from super
  virtual bool
  DoPropagateInterest (Ptr<Face> inFace,
                       Ptr<const Interest> interest,
                       Ptr<pit::Entry> pitEntry);                   

 /*******************************************
//Genereal Controller node related methods
********************************************/

  //Called only on controllers to store registered routers
  void
  AddRegisteredRouter(uint32_t id);
  
  void
  RemoveRegisteredRouter(uint32_t id);
  
  //Called only on controller to retrieve registered routers
  std::set<uint32_t>
  GetRegisteredRouters ();
  
  void
  RemoveEdges (uint32_t id);
  
  //Called only on controller to add adjacencies
  void
//  AddGraphEdges (uint32_t id1, uint32_t id2, FaceInfo faceinfo);
  AddGraphEdges (uint32_t id1, uint32_t id2, uint32_t face);
    
  void
  RegisterContent (const Name &name, uint32_t routerid, uint32_t replace);
  
  void
  UnRegisterContent (const Name &name, uint32_t routerid);
  
  std::set<uint32_t>
  GetRegisteredContentNodes (const Name &name);
  
  bool
  GetRouterNeighborOk (uint32_t id);
  
  void
  SetRouterNeighborOk (uint32_t id, bool flag);
  
  Ptr<Name>
  GetRoutes (uint32_t sourceid, uint32_t targetid);
                       


protected:
  static LogComponent g_log;
  Ptr<Name> m_controller;
  std::set<uint32_t> m_registeredrouters_set;
  std::set<uint32_t> m_registeredrouters_set2;
  std::list<uint32_t> m_registeredrouters_list;
  std::list<uint32_t> m_nodeneighbors_list;
  std::set<uint32_t> m_nodeneighbors_set;
  std::map<const Name, std::set<uint32_t> > m_registered_content;
  std::map<const uint32_t, bool> m_routerneighborok;
  std::map<uint32_t, std::set<std::pair<uint32_t, uint32_t> > > m_edgemap;
  
  bool m_newNeighbor;
  bool m_registeredRouter;
  bool m_registeredContent;
  bool m_topologyChangeActive;
  
  bool m_nocontroller;
  uint32_t m_state;
  uint32_t m_ctlseq;
  
  // inherited from Object class
  virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object
  virtual void DoDispose (); ///< @brief Do cleanup

};

} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // NDNSIM_BEST_ROUTE_SDNDN_CONTROLLER_H
