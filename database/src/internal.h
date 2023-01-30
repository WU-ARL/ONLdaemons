/*
 * Copyright (c) 2009-2013 Charlie Wiseman, Jyoti Parwatikar, John DeHart 
 * and Washington University in St. Louis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef _INTERNAL_H
#define _INTERNAL_H

namespace onl
{
  template <class T>
  inline std::string to_string(const T& t)
  {
    std::stringstream ss;
    ss << t;
    return ss.str();
  }

  struct _link_resource;

  typedef struct _node_resource
  {
    // these are filled in at creation time
    std::string type;
    unsigned int label;
    bool is_parent;
    boost::shared_ptr<struct _node_resource> parent;
    std::list< boost::shared_ptr<struct _node_resource> > node_children;
    std::list< boost::shared_ptr<struct _link_resource> > links;

    //JP added for new scheduling for vgige nodes: list of the original experiment vgiges this vgige represents, 
    //for a merged node this will be more than one
    std::list< boost::shared_ptr<struct _node_resource> > user_nodes;

    // this will be filled in if the resource becomes part of an actual experiment
    bool has_vport;
    bool fixed;
    std::string node; // initialized to ""
    std::string acl; // initialized to "unused"
    std::string cp; // initialized to "unused"

    // these are filled in later and are used for auxiliary purposes by the implementation
    //std::string type_type; // initialized to ""
    bool marked; // initialized to false
    unsigned int level; // initialized to 0
    unsigned int priority;  //initialized to 0
    unsigned int in;  //initialized to 0
    unsigned int cost; // initialized to 0. JP changed for new scheduling
    boost::shared_ptr<struct _node_resource> mapped_node;
    bool is_split; //initialize to false

    //virtual component support
    //bool has_vmsupport;//initialize to false
    std::string hosted_type; // initialized to ""
    std::string dev_type; // initialized to ""
    unsigned int core_capacity;//For user nodes, this is the requested capacity. For base nodes, this is the current available cap
    unsigned int mem_capacity;//same as core_capacity
    std::map< int,int > port_capacities;//port id, capacity

    //used for auxillary purposes to keep track during intermediate steps as to how a mapping would affect core and memory
    //allocation
    unsigned int potential_corecap;//initialized to 0 
    unsigned int potential_memcap;//initialized to 0

    //used for reconstructing user topology on commit and assigning ids at reservation time
    unsigned int vmid;
    std::string ext_tag; //external tag used to designate the specific device this should be set at reservation time
    std::string ext_ip; //filled in by looking up ext_tag in database for user. filled in at commit time
    //for setting access list for node. if root only doesn't put user in access list
    bool root_only;
  } node_resource;

  typedef boost::shared_ptr<node_resource> node_resource_ptr;

  typedef struct _link_resource
  {
    // these are filled in at creation time
    unsigned int label;
    unsigned int capacity;
    node_resource_ptr node1;
    unsigned int node1_port;
    int node1_rport;//real physical port this is may differ from node1_port for virtual ports
    std::string node1_mac;
    unsigned int node1_capacity;
    node_resource_ptr node2;
    unsigned int node2_port;
    unsigned int node2_capacity;
    int node2_rport;//real physical port this is may differ from node2_port for virtual ports
    std::string node2_mac;

    // these will be filled in if the resource becomes part of an actual experiment
    std::list<int> conns;

    // these are filled in later and are used for auxiliary purposes by the implementation
    bool marked; // initialized to false
    unsigned int level; // initialized to 0
    unsigned int in; // initialized to 0
    unsigned int linkid; // initialized to 0
    bool added;//initialized to false

    // JP added for new scheduling
    unsigned int rload;
    unsigned int lload;
    int cost;
    unsigned int potential_rcap;//used in computation of mapping cost of a potential path
    unsigned int potential_lcap;//used in computation of mapping cost of a potential path
    boost::shared_ptr<struct _link_resource> user_link; //only used for processing vgiges
    std::list<boost::shared_ptr<struct _link_resource> > mapped_path;//only used when computing schedule at reservation time
    std::list<int> mp_linkdir; //gives the direction 1 is right 0 is left of the link in the mapped_path in the context of this link
                                 //this allows us to write the rload and lload correctly to the database connschedule. 
                                 //There is a one to one coorespondence to the mapped_path links. 
    std::list<boost::shared_ptr<struct _link_resource> > added_vgige_links;//links that will tag along with this link in the database 
                                                                             //since they weren't in the initial user topology  

  } link_resource;

  typedef boost::shared_ptr<link_resource> link_resource_ptr;

  //added by JP to list subnets for new scheduling
  typedef struct _subnet_info
  {
    std::list<node_resource_ptr> nodes;
    std::list<link_resource_ptr> links;
  } subnet_info;
  
  typedef boost::shared_ptr<subnet_info> subnet_info_ptr;

  typedef struct _assign_info
  {
    std::string type;
    std::string dev_type;
    bool marked;
    std::list<node_resource_ptr> user_nodes;
    std::list<node_resource_ptr> testbed_nodes;
  } assign_info;

  typedef boost::shared_ptr<assign_info> assign_info_ptr;

  //JP added for computing scheduling, an intermediate structure
  typedef struct _link_path
  {
    std::list<link_resource_ptr> path;
    node_resource_ptr sink;
    int cost;
    int sink_port;
  } link_path;

  typedef boost::shared_ptr<link_path> link_path_ptr;

  //structures for doing vgige splits
  typedef struct _node_load_resource
  {
    // these are filled in at creation time
    boost::shared_ptr<struct _node_resource> node;
    int load; // initialized to 0
  } node_load_resource;

  typedef boost::shared_ptr<node_load_resource> node_load_ptr;
  typedef struct _mapping_cluster_resource
  {
    // these are filled in at creation time
    boost::shared_ptr<struct _node_resource> cluster;
    std::list< boost::shared_ptr<struct _node_load_resource> > nodes_used;
    std::list< boost::shared_ptr<struct _node_resource> > rnodes_used;
    std::list< boost::shared_ptr<struct _node_resource> > nodes;
    int load; // initialized to 0
    bool used; //initialized to false
    bool mapped; //initialized to false
  } mapping_cluster_resource;

  typedef boost::shared_ptr<mapping_cluster_resource> mapping_cluster_ptr;
};


#endif // _INTERNAL_H
