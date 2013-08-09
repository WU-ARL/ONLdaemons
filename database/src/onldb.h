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
 *
 */

#ifndef _ONLDB_H
#define _ONLDB_H

namespace onl
{
  class onldb
  {
    private:
      mysqlpp::Connection *onl;
      int db_count; //for performance tracking
      float make_res_time; //for performance tracking
      //void print_diff(const char* lbl, clock_t end, clock_t start); //for performance tracking
      //void print_diff(const char* lbl, clock_t dtime); //for performance tracking


      bool lock(std::string l) throw();
      void unlock(std::string l) throw();

      // returned vals are based on times in current local TZ
      std::string time_unix2db(time_t unix_time) throw();
      int str2int(std::string s) throw();
      time_t time_db2unix(std::string db_time) throw();

      time_t get_start_of_week(time_t time) throw();
      time_t discretize_time(time_t time, unsigned int hour_divisor) throw();
      time_t add_time(time_t time, unsigned int seconds) throw();
      time_t sub_time(time_t time, unsigned int seconds) throw();

      std::string get_type_type(std::string type) throw();
      onldb_resp is_infrastructure(std::string node) throw();
      onldb_resp verify_clusters(topology *t) throw();

      onldb_resp handle_special_state(std::string state, std::string node, unsigned int len, bool extend) throw();
      onldb_resp clear_special_state(std::string state, std::string new_state, std::string node) throw();


      //bool add_link(topology* t, int rid, unsigned int cur_link, unsigned int linkid, unsigned int cur_cap, unsigned int node1_label, unsigned int node1_port, unsigned int node2_label, unsigned int node2_port, unsigned int rload, unsigned int lload) throw();
      bool add_link(topology* t, int rid, unsigned int cur_link, unsigned int linkid, unsigned int cur_cap, unsigned int node1_label, unsigned int node1_port, unsigned int node1_rport, unsigned int node2_label, unsigned int node2_port, unsigned int node2_rport, unsigned int rload, unsigned int lload) throw();
      onldb_resp get_topology(topology *t, int rid) throw();
      void build_assign_list(node_resource_ptr hw, std::list<assign_info_ptr> *l) throw();
      onldb_resp fill_in_topology(topology *t, int rid) throw();
      bool subset_assign(std::list<assign_info_ptr> rl, std::list< std::list<assign_info_ptr>* >::iterator ali, std::list< std::list<assign_info_ptr>* >::iterator end, unsigned int level) throw();
      bool find_mapping(node_resource_ptr abs_node, node_resource_ptr res_node, unsigned int level) throw();


      void merge_vswitches(topology* req) throw();
      onldb_resp get_base_topology(topology *t, std::string begin, std::string end) throw();    
      onldb_resp add_special_node(topology *t, std::string begin, std::string end, node_resource_ptr node) throw();
      //onldb_resp try_reservation(topology *t, std::string user, std::string begin, std::string end) throw();//JP changed 3/29/12
      onldb_resp try_reservation(topology *t, std::string user, std::string begin, std::string end, std::string state = "pending") throw();
      bool find_embedding(topology* req, topology* base, std::list<node_resource_ptr> al) throw();
      bool embed(node_resource_ptr user, node_resource_ptr testbed) throw();
      //onldb_resp add_reservation(topology *t, std::string user, std::string begin, std::string end) throw();//JP changed 3/29/12
      onldb_resp add_reservation(topology *t, std::string user, std::string begin, std::string end, std::string state = "pending") throw();
      //bool has_reservation(std::string user, std::string begin, std::string end) throw();
 
      onldb_resp check_interswitch_bandwidth(topology* t, std::string begin, std::string end) throw();

      //new scheduling methods
      void calculate_node_costs(topology* req) throw();
      void calculate_edge_loads(topology* req) throw();
      void add_edge_load(node_resource_ptr node, int port, int load, std::list<link_resource_ptr>& links_seen) throw();
      node_resource_ptr find_feasible_cluster(node_resource_ptr node, std::list<node_resource_ptr> cl, topology* req, topology* base) throw();
      node_resource_ptr find_available_node(node_resource_ptr cluster, std::string ntype) throw();
      node_resource_ptr find_available_node(node_resource_ptr cluster, std::string ntype, std::list<node_resource_ptr> nodes_used) throw();
      void get_subnet(node_resource_ptr vgige, subnet_info_ptr subnet) throw();
      bool is_cluster_mapped(node_resource_ptr cluster) throw();
      void initialize_base_potential_loads(topology* base);
      int compute_mapping_cost(node_resource_ptr cluster, node_resource_ptr node, topology* req, topology* base) throw();
      int compute_path_costs(node_resource_ptr node, node_resource_ptr n) throw();
      int find_cheapest_path(link_resource_ptr ulink, link_resource_ptr potential_path) throw();
      node_resource_ptr map_node(node_resource_ptr node, topology* req, node_resource_ptr cluster, topology* base) throw();
      node_resource_ptr get_new_vswitch(topology* req) throw();
      void map_edges(node_resource_ptr unode, node_resource_ptr rnode, topology* base) throw();
      void get_mapped_edges(node_resource_ptr node, std::list<link_resource_ptr>& mapped_edges);
      void get_unmapped_lneighbors(node_resource_ptr node, std::list<node_load_ptr>& lneighbors);
      void get_lneighbors(node_resource_ptr node, std::list<node_load_ptr>& lneighbors);
      void map_children(node_resource_ptr unode, node_resource_ptr rnode);
      void report_metrics(topology* topo, std::string username, time_t res_start, time_t res_end, time_t comp_start, int success);
      bool split_vgige(std::list<mapping_cluster_ptr>& clusters, std::list<node_load_ptr>& unmapped_nodes, node_resource_ptr root_vgige, node_resource_ptr root_rnode, topology* base);
      int find_neighbor_mapping(mapping_cluster_ptr cluster, std::list<node_load_ptr>& unmapped_nodes, node_resource_ptr root_node);
      int find_neighbor_mapping(mapping_cluster_ptr cluster, std::list<node_load_ptr>& unmapped_nodes, node_resource_ptr root_node, std::list<node_load_ptr>& neighbors);
      //added to support virtual ports
      onldb_resp get_link_vport(unsigned int linkid, unsigned int rid, int port);

    public:
      onldb() throw();
      ~onldb() throw();

      onldb_resp print_types() throw();

      onldb_resp clear_all_experiments() throw();
      onldb_resp get_switch_list(switch_info_list& list) throw();
      onldb_resp get_base_node_list(node_info_list& list) throw();
      onldb_resp get_node_info(std::string node, bool is_cluster, node_info& info) throw();

      onldb_resp get_state(std::string node, bool is_cluster) throw();
      onldb_resp set_state(std::string node, std::string state, unsigned int len=0) throw();
      onldb_resp put_in_testing(std::string node, unsigned int len=0) throw();
      onldb_resp remove_from_testing(std::string node) throw();
      onldb_resp extend_repair(std::string node, unsigned int len) throw();
      onldb_resp extend_testing(std::string node, unsigned int len) throw();

      onldb_resp get_type(std::string node, bool is_cluster) throw();
      onldb_resp get_node_from_cp(std::string cp) throw();

      onldb_resp authenticate_user(std::string username, std::string password_hash) throw();
      onldb_resp is_admin(std::string username) throw();

      // begin1,begin2 are strings in YYYYMMDDHHMMSS form, and should be in CST6CDT timezone
      // len is in minutes
      onldb_resp make_reservation(std::string username, std::string begin1, std::string begin2, unsigned int len, topology *t) throw();
      onldb_resp reserve_all(std::string begin, unsigned int len) throw();

      onldb_resp fix_component(topology *t, unsigned int label, std::string node) throw();
      onldb_resp cancel_current_reservation(std::string username) throw();
      onldb_resp extend_current_reservation(std::string username, int len) throw();
      onldb_resp has_reservation(std::string username) throw();

      onldb_resp assign_resources(std::string username, topology *t) throw();
      onldb_resp return_resources(std::string username, topology *t) throw();
      
      onldb_resp get_expired_sessions(std::list<std::string>& users) throw();
      onldb_resp get_capacity(std::string type, unsigned int port) throw();
      onldb_resp get_switch_ports(unsigned int cid, switch_port_info& info1, switch_port_info& info2) throw();

      onldb_resp has_virtual_port(std::string type) throw();
  };
};
#define MAX_INTERCLUSTER_CAPACITY 10
#endif // _ONLDB_H
