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


#ifndef NDNSIM_BEST_ROUTE_NLSR_LIKE_H
#define NDNSIM_BEST_ROUTE_NLSR_LIKE_H

#include "limited-fib-strategy.h"
#include <ns3/ndnSIM/model/ndn-name-components.h>
#include "ns3/log.h"
#include "boost-graph-ndn-control-routing.h"
#include <set>
#include <list>
#include <iterator>
#include <boost/functional/hash.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace ns3 {
namespace ndn {
namespace fw {

/**
 * \ingroup ndn
 * \brief Best route strategy
 */
class BestRouteNLSRLike :
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
  BestRouteNLSRLike ();
  
  virtual ~BestRouteNLSRLike ();
  
  
/*******************************************
//Genereal router node related methods
********************************************/

  void
  IncrementState();
  
  uint32_t
  GetState();

  void
  AddFibEntryCtl (Ptr<Name> name, Ptr<Face> inFace, uint32_t metric);

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


  //from super
  virtual void
  DidExhaustForwardingOptions (Ptr<Face> inFace,
                               Ptr<const Interest> interest,
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
  RegisterContent (const Name &name, uint32_t routerid);
  
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
                       
  bool
  AddLSA (Name name, Name content);
  
  std::map<uint32_t,uint32_t>
  GetRootHashV2 ();
   
  std::set<uint32_t>
  GetHashSet ();
  
  Name
  GetLSA (uint32_t fullhash, uint32_t topoproducer);
  
  std::set<uint32_t>
  GetHashSetTopo (uint32_t hashkey);
  
  std::set<uint32_t>
  GetHashSetProducer (uint32_t hashkey);  
  
  uint32_t
  NameHash (Name name);
  
  uint32_t
  GetLSASize (uint32_t fullhash, uint32_t topoproducer);
  

protected:
  static LogComponent g_log;
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
  
  uint32_t m_state;
  
  // inherited from Object class
  virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object
  virtual void DoDispose (); ///< @brief Do cleanup

  
  struct LSA
  {
    LSA (uint32_t _hashfull, Name _name, Name _prefix, Name _content) :
      hashfull(_hashfull), name(_name), prefix(_prefix), content(_content)  { }
    
    uint32_t hashfull;
    Name name;
    Name prefix;
    Name content;
  };
  
  class i_hashfull { };
  class i_name { };
  class i_prefix { };

  struct LSAContainer :
    public boost::multi_index::multi_index_container<
    LSA,
    boost::multi_index::indexed_by<
      boost::multi_index::ordered_non_unique<
        boost::multi_index::tag<i_hashfull>,
        boost::multi_index::member<LSA, uint32_t, &LSA::hashfull>
        >,
      boost::multi_index::ordered_non_unique<
        boost::multi_index::tag<i_name>,
        boost::multi_index::member<LSA, Name, &LSA::name>
        >,        
      boost::multi_index::ordered_non_unique<
        boost::multi_index::tag<i_prefix>,
        boost::multi_index::member<LSA, Name, &LSA::prefix>
        >        
      >
    > { } ;

  LSAContainer m_LSAsTopo;
  LSAContainer m_LSAsProducer;  
  boost::hash<std::string> string_hash;
  std::map<uint32_t,uint32_t> m_roothashV2;

  
  //std::map<uint32_t,std::set<uint32_t> > m_topohashsetmap; 
  //std::map<uint32_t,std::set<uint32_t> > m_producerhashsetmap;
  std::set<uint32_t> m_topohashset;
  std::set<uint32_t> m_producerhashset;
};

} // namespace fw
} // namespace ndn
} // namespace ns3

#endif // NDNSIM_BEST_ROUTE_NLSR_LIKE_H
