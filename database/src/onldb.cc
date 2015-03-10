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

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <list>
#include <map>
#include "time.h"
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>
#include <boost/shared_ptr.hpp>

//#include <gurobi_c++.h>

#include "internal.h"
#include "onldb_resp.h"
#include "topology.h"
#include "onldb_internal.h"
#include "onldb.h"
#include "onldb_types.h"


using namespace std;
using namespace onl;

// fill this in
#define ONLDB     ""
#define ONLDBHOST ""
#define ONLDBUSER ""
#define ONLDBPASS ""

//#define MAX_INTERCLUSTER_CAPACITY 10
#define UNUSED_CLUSTER_COST 50
#define CANT_SPLIT_VGIGE_COST 20 //penalty for not splitting is multiply MAX_INTERCLUSTER_CAPACITY * each unmapped node
#define USER_UNUSED_CLUSTER_COST 20
#define VM_COST 10 //penalty for putting more than one vm of the same experiment on the same machine
int fnd_path;

bool in_list(node_resource_ptr node, std::list<node_resource_ptr> nodes)
{
  std::list<node_resource_ptr>::iterator nit;
  for (nit = nodes.begin(); nit != nodes.end(); ++nit)
    {
      if ((*nit) == node) return true;
    }
  return false;
}

int num_in_subnet(subnet_info_ptr subnet, std::list<node_resource_ptr>& nodes)
{
  int rtn = 0;
  std::list<node_resource_ptr>::iterator nit;
  for (nit = nodes.begin(); nit != nodes.end(); ++nit)
    {
      if (in_list((*nit), subnet->nodes)) ++rtn;
    }
  return rtn;
}

void fill_in_cluster(mapping_cluster_ptr cluster)
{
  std::list<link_resource_ptr>::iterator clusterlit;
  for (clusterlit = cluster->cluster->links.begin(); clusterlit != cluster->cluster->links.end(); ++clusterlit)
    {
      node_resource_ptr n;
      if ((*clusterlit)->node1 == cluster->cluster) n = (*clusterlit)->node2;
      else n = (*clusterlit)->node1;
      if (n->type_type == "infrastructure") continue;//don't add infrastructure nodes
      if (!in_list(n, cluster->nodes))
	{
	  n->potential_corecap = n->core_capacity;
	  n->potential_memcap = n->mem_capacity;
	  cluster->nodes.push_back(n);
	  if (n->marked) cluster->mapped = true;
	}
    }
  cluster->used = false;
}

void reset_cluster(mapping_cluster_ptr cluster)
{
  std::list<node_resource_ptr>::iterator nit;
  cluster->used = false;
  cluster->rnodes_used.clear();
  cluster->nodes_used.clear();
  for (nit = cluster->nodes.begin(); nit != cluster->nodes.end(); ++nit)
    {
      (*nit)->potential_corecap = (*nit)->core_capacity;
      (*nit)->potential_memcap = (*nit)->mem_capacity;
      if ((*nit)->marked) cluster->mapped = true;
      if ((*nit)->parent)
	{
	  (*nit)->parent->potential_corecap = (*nit)->parent->core_capacity;
	  (*nit)->parent->potential_memcap = (*nit)->parent->mem_capacity;
	}
    }
}

void copy_cluster(mapping_cluster_ptr mcluster, mapping_cluster_ptr copy_to)
{
  copy_to->cluster = mcluster->cluster;
  copy_to->nodes.assign(mcluster->nodes.begin(), mcluster->nodes.end());
  reset_cluster(copy_to);
}

bool req_sort_comp(assign_info_ptr i, assign_info_ptr j)
{
  return(i->user_nodes.size() < j->user_nodes.size());
}

bool base_sort_comp(node_resource_ptr i, node_resource_ptr j)
{
  return(i->priority < j->priority);
}


node_resource_ptr one_node_subnet(subnet_info_ptr subnet, mapping_cluster_ptr mcluster)
{
  node_resource_ptr rtn_node;
  node_resource_ptr null_node;
  std::list<node_resource_ptr>::iterator snit;
  std::list<node_load_ptr>::iterator user_nit;
  std::list<node_resource_ptr>::iterator real_nit;
  for (snit = subnet->nodes.begin(); snit != subnet->nodes.end(); ++snit)
    {
      if ((*snit)->type != "vgige")
	{
	  node_resource_ptr mapped_node = (*snit)->mapped_node;
	  if (!(*snit)->marked)
	    {
	      //lookup to see if it was allocated in the cluster structure
	      real_nit = mcluster->rnodes_used.begin();
	      for (user_nit = mcluster->nodes_used.begin(); user_nit != mcluster->nodes_used.end(); ++user_nit)
		{
		  if ((*user_nit)->node == (*snit)) 
		    {
		      mapped_node = (*real_nit);
		      break;
		    }
		  ++real_nit;
		}
	    }
	  if (mapped_node && !rtn_node) rtn_node = mapped_node;
	  if (!mapped_node || mapped_node != rtn_node) //if we the node hasn't been mapped yet or the one of the subnet nodes is mapped to a different node
	    //return a null ptr
	    return null_node;
	}
    }
  return rtn_node;
}


//int
//get_available_cluster_size(node_resource_ptr cluster)
//{
//  std::list<link_resource_ptr>::iterator clusterlit;
//  int rtn = 0;
//  node_resource_ptr other;
//  for (clusterlit = cluster->links.begin(); clusterlit != cluster->links.end(); ++clusterlit)
//    {
//      other = (*clusterlit)->node1;
//      if ((*clusterlit)->node2 != cluster) other = (*clusterlit)->node2;
//      if (other->type_type != "infrastructure" && !other->marked) ++rtn;
//    }
//  return rtn;
//}

int calculate_edge_cost(int rload, int lload)
{
  int rtraffic = rload;
  int ltraffic = lload;
  int rtn = -1;
  if (rtraffic > MAX_INTERCLUSTER_CAPACITY)
    rtraffic = MAX_INTERCLUSTER_CAPACITY;
  if (ltraffic > MAX_INTERCLUSTER_CAPACITY)
    ltraffic = MAX_INTERCLUSTER_CAPACITY;
  //divide by 1000 to account for the change from loads in Gbps to Mbps
  rtraffic = rtraffic/1000;
  ltraffic = ltraffic/1000;
  rtn = rtraffic + ltraffic + (fabs(rtraffic - ltraffic)/2);
  return rtn;
}

node_load_ptr get_element(node_resource_ptr node, std::list<node_load_ptr> nodes)
{
  std::list<node_load_ptr>::iterator nit;
  node_load_ptr nullnode;
  for (nit = nodes.begin(); nit != nodes.end(); ++nit)
    {
      if ((*nit)->node == node) return (*nit);
    }
  return nullnode;
}

bool in_list(link_resource_ptr link, std::list<link_resource_ptr> links)
{
  std::list<link_resource_ptr>::iterator lit;
  for (lit = links.begin(); lit != links.end(); ++lit)
    {
      if ((*lit) == link) return true;
    }
  return false;
}

bool in_list(node_resource_ptr node, std::list<node_load_ptr> nodes)
{
  if (get_element(node, nodes)) return true;
  else
    return false;
}


void print_diff(const char* lbl,  struct timeval& stime)
{
  struct timeval etime;
  gettimeofday(&etime, NULL);

  double secs = (double)(etime.tv_sec - stime.tv_sec);
  double usecs = (double)(etime.tv_usec -stime.tv_usec);
  if (etime.tv_usec < stime.tv_usec)
    {
      usecs += 1000000;
      secs -= 1;
    }
    
  cout << lbl << " computation time: " << secs << " seconds " << usecs << " useconds " << endl;
}

void fill_node_info(node_resource_ptr node, vector<typeinfo>& typenfo)
{
  vector<typeinfo>::iterator it;
  for (it = typenfo.begin(); it != typenfo.end(); ++it)
    {
      if ((*it).tid == node->type)
	{
	  if ((int)(*it).hasvport == 0) 
	    node->has_vport = false;
	  else
	    node->has_vport = true;
	  if ((int)(*it).vmsupport == 0) 
	    node->has_vmsupport = false;
	  else
	    {
	      node->has_vmsupport = true;
	      node->has_vport = true;
	    }
	  node->core_capacity = (int)(*it).corecapacity;
	  node->mem_capacity = (int)(*it).memcapacity;
	  int max = (int)(*it).numinterfaces;
	  int bw = (int)(*it).interfacebw;
	  for (int i = 0; i < max; ++i)
	    {
	      node->port_capacities[i] = bw;
	    }
	  break;
	}
    }
}

bool has_vport(std::string tid, vector<typeinfo>& typenfo)
{
  vector<typeinfo>::iterator it;
  for (it = typenfo.begin(); it != typenfo.end(); ++it)
    {
      if ((*it).tid == tid)
	{
	  if ((int)(*it).hasvport == 0) return false;
	  else return true;
	}
    }
  return false;
}

bool 
onldb::subnet_mapped(subnet_info_ptr subnet, unsigned int cin)
{
  node_resource_ptr nptr;
  std::list<link_resource_ptr> potential_mappings;
  return (subnet_mapped(subnet, cin, potential_mappings, nptr));
}

bool 
onldb::subnet_mapped(subnet_info_ptr subnet, unsigned int cin, std::list<link_resource_ptr>& potential_mappings )
{
  node_resource_ptr nptr;
  return (subnet_mapped(subnet, cin, potential_mappings, nptr));
}

bool 
onldb::subnet_mapped(subnet_info_ptr subnet, unsigned int cin, std::list<link_resource_ptr>& potential_mappings, node_resource_ptr excluded)
{
  std::list<node_resource_ptr>::iterator snnit;
  std::list<link_resource_ptr>::iterator snlit;
  std::list<link_resource_ptr>::iterator lit;
  for (snnit = subnet->nodes.begin(); snnit != subnet->nodes.end(); ++snnit)
    {
      if ((*snnit)->marked && (*snnit)->type == "vgige" && (*snnit)->in == cin)
	{
	  if (!excluded || (excluded && ((*snnit) != excluded)))
	    {
	      cout << "subnet_mapped cluster " << cin << " no mapping. vgige " << (*snnit)->label << " already mapped here" << endl;
	      return true;
	    }
	}
    }
  for (snlit = subnet->links.begin(); snlit != subnet->links.end(); ++snlit)
    {
      if ((*snlit)->marked)
	{
	  int n = 0;
	  for(lit = (*snlit)->mapped_path.begin(); lit != (*snlit)->mapped_path.end(); ++lit)
	    {
	      // if ((*lit)->node1->in == cin && (*lit)->node2 == (*lit)->node1 && (*lit)->node1->type == "infrastructure")
	      //if we see two links with an endpoint on the infrastructure node then there is a link on this subnet that is mapped to this
	      //cluster
	      if (((*lit)->node1->in == cin && (*lit)->node1->type_type == "infrastructure") ||
		  ((*lit)->node2->in == cin && (*lit)->node2->type_type == "infrastructure"))
		++n;
	      if (n > 1)
		{
		  cout << "subnet_mapped cluster " << cin << " no mapping. link " << (*snlit)->label << " already mapped here" << endl;
		  return true;
		}
	    }
	}
    }

  for (snlit = potential_mappings.begin(); snlit != potential_mappings.end(); ++snlit)
    {
      int n = 0;
      for(lit = (*snlit)->mapped_path.begin(); lit != (*snlit)->mapped_path.end(); ++lit)
	{
	  //if we see two links with an endpoint on the infrastructure node then there is a link on this subnet that is mapped to this
	  //cluster
	  if (((*lit)->node1->in == cin && (*lit)->node1->type_type == "infrastructure") ||
	      ((*lit)->node2->in == cin && (*lit)->node2->type_type == "infrastructure"))
	    ++n;
	  if (n > 1)
	    {
	      cout << "subnet_mapped cluster " << cin << " no mapping. link " << (*snlit)->label << " already potentially mapped here" << endl;
		  return true;
	    }
	}
    }
  return false;
}

void
onldb::report_metrics(topology* topo, std::string username, time_t res_start, time_t res_end, time_t comp_start, int success = 1)
{
  time_t comp_end = time(NULL);
  int ic = topo->intercluster_cost;//compute_intercluster_cost();
  int hc = topo->host_cost; //compute_host_cost();

  char tstr[30];
  ctime_r(&comp_end, tstr);
  tstr[20] = '\0';

  cout << "report_metrics:";
  if (success == 1)
    cout << "Success -- ";
  else
    cout << "Fail -- ";

  cout << "reservation(" << username << ", start " << ctime(&res_start) << ", end " << ctime(&res_end) << ") time_to_compute = " << difftime(comp_end, comp_start) << " (IC,HC) = (" << ic << ", " << hc << ")" << " database calls = " << db_count << endl;
  db_count = 0;
  make_res_time = 0;
}


bool onldb::lock(std::string l) throw()
{
  try
  {
    mysqlpp::Query lock = onl->query();
    lock << "select get_lock(" << mysqlpp::quote << l << ",10) as lockres";
    mysqlpp::StoreQueryResult res = lock.store();
    lockresult lr = res[0];
    if(lr.lockres.is_null || ((int)lr.lockres.data) == 0) return false;
  } 
  catch(const mysqlpp::Exception& er)
  {
    return false;
  }
  return true;
}

void onldb::unlock(std::string l) throw()
{
  try
  {
    mysqlpp::Query lock = onl->query();
    lock << "do release_lock(" << mysqlpp::quote << l <<")";
    lock.execute();
  } 
  catch(const mysqlpp::Exception& er)
  {
    cout << "Warning: releasing lock " << l << " encountered an exception" << endl;
  }
}

onldb::onldb() throw()
{

  fstream fs("/etc/ONL/onl_database.txt", fstream::in);
  
  if(fs.fail()) onl = NULL;
  else
    {
      std::string onldb_str;
      std::string onldbhost_str;
      std::string onldbuser_str;
      std::string onldbpass_str;

      getline(fs, onldb_str);
      getline(fs, onldbhost_str);
      getline(fs, onldbuser_str);
      getline(fs, onldbpass_str);

      onl = new mysqlpp::Connection(onldb_str.c_str(), onldbhost_str.c_str(), onldbuser_str.c_str(), onldbpass_str.c_str());//ONLDB,ONLDBHOST,ONLDBUSER,ONLDBPASS);
      onl->set_option(new mysqlpp::ReconnectOption(true));
      //onl->set_option(new mysqlpp::ConnectTimeoutOption(5));
    }

  nodestates::table("nodes");
  restimes::table("reservations");
  resinfo::table("reservations");
  experimentins::table("experiments");
  reservationins::table("reservations");
}

onldb::~onldb() throw()
{
  delete onl;
}

std::string onldb::time_unix2db(time_t unix_time) throw()
{
  struct tm *stm = localtime(&unix_time);

  char char_str[16];
  sprintf(char_str,"%04d%02d%02d%02d%02d%02d",stm->tm_year+1900,stm->tm_mon+1,stm->tm_mday,stm->tm_hour,stm->tm_min,stm->tm_sec);
  std::string str = char_str;
  return str;
}

int onldb::str2int(std::string s) throw()
{
  stringstream ss(s);
  int n;
  ss >> n;
  return n;
}

time_t onldb::time_db2unix(std::string db_time) throw()
{
  struct tm stm;

  if(db_time.length() == 14)
  {
    // YYYYMMDDhhmmss
    stm.tm_sec  = str2int(db_time.substr(12,2));
    stm.tm_min  = str2int(db_time.substr(10,2));
    stm.tm_hour = str2int(db_time.substr(8,2));
    stm.tm_mday = str2int(db_time.substr(6,2));
    stm.tm_mon  = str2int(db_time.substr(4,2)) - 1;
    stm.tm_year = str2int(db_time.substr(0,4)) - 1900;
  }
  else
  {
    // YYYY-MM-DD hh:mm:ss
    stm.tm_sec  = str2int(db_time.substr(17,2));
    stm.tm_min  = str2int(db_time.substr(14,2));
    stm.tm_hour = str2int(db_time.substr(11,2));
    stm.tm_mday = str2int(db_time.substr(8,2));
    stm.tm_mon  = str2int(db_time.substr(5,2)) - 1;
    stm.tm_year = str2int(db_time.substr(0,4)) - 1900;
  }

  stm.tm_isdst = -1;
  return mktime(&stm);
}

time_t onldb::get_start_of_week(time_t time) throw()
{
  struct tm *stm = localtime(&time);

  stm->tm_hour = 0;
  stm->tm_min = 0;
  stm->tm_sec = 0;
  // substract off days so that we are on the previous monday. mktime
  // does the correct thing with negative values, potentially changing 
  // month and year.
  if(stm->tm_wday == 0) // 0 is Sunday, but we want Monday->Sunday weeks
  {
    stm->tm_mday -= 6;
  }
  else
  {
    stm->tm_mday -= (stm->tm_wday - 1);
  }

  return mktime(stm);
}

time_t onldb::discretize_time(time_t unix_time, unsigned int hour_divisor) throw()
{
  struct tm *stm = localtime(&unix_time);
  unsigned int time_chunk = 60/hour_divisor;
  stm->tm_sec = 0;
  stm->tm_min = (stm->tm_min / time_chunk) * time_chunk;
  return mktime(stm);
}

time_t onldb::add_time(time_t unix_time, unsigned int seconds) throw()
{
  struct tm *stm = localtime(&unix_time);
  stm->tm_sec += seconds;
  return mktime(stm);
} 

time_t onldb::sub_time(time_t unix_time, unsigned int seconds) throw()
{
  struct tm *stm = localtime(&unix_time);
  stm->tm_sec -= seconds;
  return mktime(stm);
} 

std::string onldb::get_type_type(std::string type) throw()
{
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select type from types where tid=" << mysqlpp::quote << type;
    ++db_count;
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty()) return "";

    typetype tt = res[0];
    return tt.type;
  }
  catch(const mysqlpp::Exception& er)
  {
    return "";
  }
}

onldb_resp onldb::get_link_vport(unsigned int linkid, unsigned int rid, int port)
{
  try
  {
    mysqlpp::Query query = onl->query();
    if (port == 1)
      query << "select port1 from linkschedule where rid=" << mysqlpp::quote << rid << " and linkid=" << mysqlpp::quote << linkid << ")";
    else
      query << "select port2 from linkschedule where rid=" << mysqlpp::quote << rid << " and linkid=" << mysqlpp::quote << linkid << ")";
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty()) return onldb_resp(-1,(std::string)"linkschedule not in database");

    vportinfo p = res[0];
    return onldb_resp(p.virtualport,(std::string)"success");
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,(std::string)"database problem");
  }
}

onldb_resp onldb::is_infrastructure(std::string node) throw()
{
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select type from types where tid in (select tid from nodes where node=" << mysqlpp::quote << node << ")";
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty()) return onldb_resp(-1,(std::string)"node not in database");

    typetype tt = res[0];
    if(tt.type == "infrastructure")
    {
      return onldb_resp(1,(std::string)"true");
    }
    return onldb_resp(0,(std::string)"false");
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,(std::string)"database problem");
  }
}

onldb_resp onldb::verify_clusters(topology *t) throw()
{
  list<node_resource_ptr>::iterator nit;
  list<link_resource_ptr>::iterator lit;
  
  struct timeval stime;
  gettimeofday(&stime, NULL);

  // set up the marked fields for all components
  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    if((*nit)->parent)
    {
      (*nit)->marked = false;
    }
    else
    {
      (*nit)->marked = true;
    }
  }

  for(lit = t->links.begin(); lit != t->links.end(); ++lit)
  {
    (*lit)->marked = true;
  }
  
  // fill in the type_type field for all nodes
  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    (*nit)->type_type = get_type_type((*nit)->type);
    if((*nit)->type_type == "")
    {
      std::string err = (*nit)->type + " is not a valid type";
      return onldb_resp(-1,err);
    }
  }

  // abstract_tops contains one abstract cluster topology for each cluster in the actual topology
  vector<topology_resource_ptr> abstract_tops;

  // make a pass over the node list, only need to match types to types
  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    // if the node is already marked, then skip it
    if((*nit)->marked) { continue; }

    unsigned int pl = (*nit)->parent->label;
    std::string pt = (*nit)->parent->type;
    std::string ptt = (*nit)->parent->type_type;

    // if the node's parent is not a cluster type, it is not valid
    if(ptt != "hwcluster")
    {
      return onldb_resp(-1,(std::string)"node " + to_string(pl) + " is not a valid cluster type");
    }

    // now go through the list of abstract topologies to find the one this node belongs to
    topology_resource_ptr abs_top;
    vector<topology_resource_ptr>::iterator topit;
    for(topit = abstract_tops.begin(); topit != abstract_tops.end(); ++topit)
    {
      if((*topit)->label == pl) 
      {
        abs_top = *topit;
        break;
      }
    }
    // if there isn't yet an abstract topology for this cluster, add it
    if(topit == abstract_tops.end())
    {
      topology_resource_ptr trp(new topology_resource());
      trp->label = pl;
      
      // build the cluster from the cluster description in the database
      try
      {
        mysqlpp::Query query = onl->query();
        query << "select compid,comptype as type from clustercomps where clustercomps.tid=" << mysqlpp::quote << pt;
        vector<clusterelems> ces;
	++db_count;
        query.storein(ces);
        if(ces.empty())
        {
          return onldb_resp(-1, (std::string)"database consistency problem");
        }

        vector<clusterelems>::iterator ceit;
        for(ceit = ces.begin(); ceit != ces.end(); ++ceit)
        {
          onldb_resp ahrr = trp->cluster.add_node(ceit->type, ceit->compid, 0);
          if(ahrr.result() != 1)
          {
            return onldb_resp(-1, (std::string)"database consistency problem");
          }
        }
      }
      catch(const mysqlpp::Exception& er)
      {
        return onldb_resp(-1,er.what());
      }
      
      abstract_tops.push_back(trp);
      abs_top = trp;
    }

    // now we have the abstract topology, so check if the current node is valid
    list<node_resource_ptr>::iterator atnit;
    for(atnit = abs_top->cluster.nodes.begin(); atnit != abs_top->cluster.nodes.end(); ++atnit)
    {
      if((*atnit)->marked) { continue; }
      if((*atnit)->type == (*nit)->type)
      {
        (*atnit)->marked = true;
        (*nit)->marked = true;
        break;
      }
    }
    if(!((*nit)->marked))
    {  
      return onldb_resp(-1,(std::string)"node " + to_string((*nit)->label) + " does not fit into cluster type " + pt);
    }
  }

  // now, all components in the topology are part of good clusters, but are the clusters complete?
  vector<topology_resource_ptr>::iterator at;
  for(at = abstract_tops.begin(); at != abstract_tops.end(); ++at)
  {
    list<node_resource_ptr>::iterator atnit;
    for(atnit = (*at)->cluster.nodes.begin(); atnit != (*at)->cluster.nodes.end(); ++atnit)
    {
      if((*atnit)->marked == false) 
      {
        return onldb_resp(-1,(std::string)"cluster " + to_string((*at)->label) + " is incomplete");
      }
    }
  }

  print_diff("onldb::verify_clusters", stime);
  return onldb_resp(1,"success");
}

//void
//onldb::print_diff(const char* lbl, clock_t end, clock_t start)
//{
//print_diff(lbl, (clock_t)(end - start));
//}

//bool onldb::add_link(topology* t, int rid, unsigned int cur_link, unsigned int linkid, unsigned int cur_cap, unsigned int node1_label, unsigned int node1_port, unsigned int node2_label, unsigned int node2_port, unsigned int rload, unsigned int lload) throw()
bool onldb::add_link(topology* t, int rid, unsigned int cur_link, unsigned int linkid, unsigned int cur_cap, unsigned int node1_label, unsigned int node1_port, unsigned int node1_rport, unsigned int node2_label, unsigned int node2_port, unsigned int node2_rport, unsigned int rload, unsigned int lload, unsigned int node1_cap, unsigned int node2_cap) throw()
{
  // there are three cases to deal with: node->node (normal) links, node->vswitch
  // links, and vswitch->vswitch links. for node->node links, both node1_label
  // and node2_label should have been set.  for node->vswitch links, only
  // node1_label is set.  for vswitch->vswitch links, neither lable has been set.
  
  // if node2_label isn't set, then this link involves at least one vswitch, so
  // read the vswitchschedule table to get all vswitches that are part of this link
  if(node2_label == 0)
  {
    unsigned int num_vs = 1;
    if(node1_label == 0) { num_vs++; }
  
    // note that we assume that vswitches can NOT have loopbacks
    mysqlpp::Query vswitchquery = onl->query();
    vswitchquery << "select vlanid,port from vswitchschedule where rid=" << mysqlpp::quote << rid << " and linkid=" << mysqlpp::quote << cur_link; 
    vector<vswitchconns> vswc;
    vswitchquery.storein(vswc);
    if(vswc.size() != num_vs) { return false; }

    if(node1_label == 0)
    {
      node1_label = t->get_label("vgige" + to_string(vswc[1].vlanid));
      node1_port = vswc[1].port;
      node1_rport = node1_port;
    }
    node2_label = t->get_label("vgige" + to_string(vswc[0].vlanid));
    node2_port = vswc[0].port;
    node1_rport = node2_port;
  }

  //onldb_resp r = t->add_link(linkid, cur_cap, node1_label, node1_port, node2_label, node2_port, rload, lload);
  onldb_resp r = t->add_link(linkid, cur_cap, node1_label, node1_port, node1_rport, node2_label, node2_port, node2_rport, rload, lload, node1_cap, node2_cap);
  if(r.result() != 1) { return false; }

  return true;
}

onldb_resp onldb::get_topology(topology *t, int rid) throw()
{
  unsigned hwid = 1;
  unsigned linkid = 1;
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select hwclusterschedule.cluster,hwclusters.tid,hwclusters.acl,hwclusterschedule.fixed from hwclusters join hwclusterschedule using (cluster) where hwclusterschedule.rid=" << mysqlpp::quote << rid;
    vector<hwclustertypes> hwct;
    query.storein(hwct);
    vector<hwclustertypes>::iterator it;
    for(it = hwct.begin(); it != hwct.end(); ++it)
    { 
      onldb_resp r = t->add_node(it->tid, hwid, 0);
      if(r.result() != 1)
      {
        return onldb_resp(-1, (std::string)"database consistency problem");
      }
      ++hwid;
      t->nodes.back()->node = it->cluster;
      t->nodes.back()->acl = it->acl;
      t->nodes.back()->cp = "unused";
      if(((int)it->fixed) == 1)
      {
        t->nodes.back()->fixed = true;
      }
    }

    mysqlpp::Query queryvlan = onl->query();
    queryvlan << "select distinct vlanid from vswitchschedule where rid=" << mysqlpp::quote << rid;
    vector<vswitches> vsw;
    queryvlan.storein(vsw);
    vector<vswitches>::iterator itv;
    for(itv = vsw.begin(); itv != vsw.end(); ++itv)
    {
      onldb_resp r = t->add_node("vgige", hwid, 0);
      if(r.result() != 1)
      {
        return onldb_resp(-1, (std::string)"database consistency problem");
      }
      ++hwid;
      t->nodes.back()->node = "vgige" + to_string(itv->vlanid);
      t->nodes.back()->acl = "unused";
      t->nodes.back()->cp = "unused";
    }

    mysqlpp::Query query_tp = onl->query();
    query_tp << "select tid,hasvport,rootonly from types";
    vector<typeresinfo> typenfo;
    query_tp.storein(typenfo);
    vector<typeresinfo>::iterator tpit;

    mysqlpp::Query query2 = onl->query();
    query2 << "select nodes.node,nodes.tid,hwclustercomps.cluster,nodes.acl,nodes.daemonhost,nodeschedule.fixed,nodeschedule.vmid,nodeschedule.cores,nodeschedule.memory from nodes join nodeschedule using (node) left join hwclustercomps using (node) where nodeschedule.rid=" << mysqlpp::quote << rid;
    vector<nodetypes> nt;
    query2.storein(nt);
    vector<nodetypes>::iterator it2;

    for(it2 = nt.begin(); it2 != nt.end(); ++it2)
    {
      unsigned int parent_label = 0;
      if(!it2->cluster.is_null)
      {
        parent_label = t->get_label(it2->cluster.data);
        if(parent_label == 0) return onldb_resp(-1, (std::string)"database consistency problem");
      }
      
      //find type info
      for(tpit = typenfo.begin(); tpit != typenfo.end(); ++tpit)
	{
	  if (tpit->tid == it2->tid) break;
	}
      if (tpit == typenfo.end())
	{
	  return onldb_resp(-1, (std::string)"no such type");
	}
      /*
	onldb_resp r = has_virtual_port(it2->tid);
      if(r.result() < 0)
      {
      return onldb_resp(-1, r.msg());
	}*/
      onldb_resp r = t->add_node(it2->tid, hwid, parent_label, ((int)tpit->hasvport != 0), it2->cores, it2->memory);
      if(r.result() != 1)
      {
        return onldb_resp(-1, (std::string)"database consistency problem");
      }
      ++hwid;
      t->nodes.back()->node = it2->node;
      t->nodes.back()->acl = it2->acl;
      t->nodes.back()->cp = it2->daemonhost;
      t->nodes.back()->vmid = it2->vmid;
      t->nodes.back()->root_only = ((int)tpit->rootonly != 0);
      if (it2->vmid > 0) 
	{
	  t->nodes.back()->has_vmsupport = true;
	}
      if(((int)it2->fixed) == 1)
      {
        t->nodes.back()->fixed = true;
      }
    }

    mysqlpp::Query query3 = onl->query();
    //query3 << "select connschedule.linkid,connschedule.capacity,connections.cid,connections.node1,connections.node1port,connections.node2,connections.node2port from connschedule join connections on connections.cid=connschedule.cid where connschedule.rid=" << mysqlpp::quote << rid << " order by connschedule.linkid";
    query3 << "select connschedule.linkid,connschedule.capacity,connections.cid,connections.node1,connections.node1port,connections.node2,connections.node2port,connschedule.rload,connschedule.lload from connschedule join connections on connections.cid=connschedule.cid where connschedule.rid=" << mysqlpp::quote << rid << " order by connschedule.linkid";
    vector<linkinfo> li;
    query3.storein(li);
    vector<linkinfo>::iterator it3;

    mysqlpp::Query query4 = onl->query();
    query4 << "select * from linkschedule where rid=" << mysqlpp::quote << rid << " order by linkschedule.linkid";
    vector<linkschedule> vps;
    query4.storein(vps);
    vector<linkschedule>::iterator vps_it;

    if(!li.empty())
    {
      unsigned int cur_link = li.begin()->linkid;
      unsigned int cur_cap = li.begin()->capacity;
      unsigned int cur_rload = li.begin()->rload;
      unsigned int cur_lload = li.begin()->lload;
      std::list<int> cur_conns;
      unsigned int node1_label = 0;
      unsigned int node1_port;
      unsigned int node1_rport;
      unsigned int node1_cap = 0;
      unsigned int node2_label = 0;
      unsigned int node2_port;
      unsigned int node2_rport;
      unsigned int node2_cap = 0;
      //need the name of node since physical node won't match up to the topology for vms i.e. get_label won't work
      std::string node1_name;
      std::string node2_name;
	 
      if (vps.empty() && cur_cap < 1000) // this is a reservation that was made with Gbps
	{
	  cur_cap = cur_cap * 1000;
	  cur_rload = cur_rload * 1000;
	  cur_lload = cur_lload * 1000;
	}

      for(vps_it = vps.begin(); vps_it != vps.end(); ++vps_it)
	{
	  if (vps_it->linkid == cur_link)
	    {
	      node1_label = t->get_label(vps_it->node1, vps_it->node1vmid);
	      node1_port = vps_it->port1;
	      node1_name = vps_it->node1;
	      node1_cap = vps_it->port1capacity;
	      node2_label = t->get_label(vps_it->node2, vps_it->node2vmid);
	      node2_name = vps_it->node2;
	      node2_port = vps_it->port2;
	      node2_cap = vps_it->port2capacity;
	      break;
	    }
	}

      for(it3 = li.begin(); it3 != li.end(); ++it3)
      {
        if(cur_link != it3->linkid)
        {
          //if(!add_link(t, rid, cur_link, linkid, cur_cap, node1_label, node1_port, node2_label, node2_port, cur_rload, cur_lload))
	  if(!add_link(t, rid, cur_link, linkid, cur_cap, node1_label, node1_port, node1_rport, node2_label, node2_port, node2_rport, cur_rload, cur_lload, node1_cap, node2_cap))
          {
            return onldb_resp(-1, (std::string)"database consistency problem");
          }
          t->links.back()->conns = cur_conns;
          ++linkid;

          cur_link = it3->linkid;
          cur_cap = it3->capacity;
	  cur_rload = it3->rload;
	  cur_lload = it3->lload;
	  if (vps.empty() && cur_cap < 1000) // this is a reservation that was made with Gbps
	    {
	      cur_cap = cur_cap * 1000;
	      cur_rload = cur_rload * 1000;
	      cur_lload = cur_lload * 1000;
	    }
          cur_conns.clear();
          node1_label = 0;
	  node1_port = 0;
	  node1_rport = 0;
	  node1_cap = 0;
          node2_label = 0;
	  node2_port = 0;
	  node2_rport = 0;
	  node2_cap = 0;
     
	  for(vps_it = vps.begin(); vps_it != vps.end(); ++vps_it)
	    {
	      if (vps_it->linkid == cur_link)
		{
		  node1_label = t->get_label(vps_it->node1, vps_it->node1vmid);
		  node1_name = vps_it->node1;
		  node1_port = vps_it->port1;
		  node1_cap = vps_it->port1capacity;
		  node2_label = t->get_label(vps_it->node2, vps_it->node2vmid);
		  node2_name = vps_it->node2;
		  node2_port = vps_it->port2;
		  node2_cap = vps_it->port2capacity;
		  break;
		}
	    }
        }
      
        cur_conns.push_back(it3->cid);
	if (node1_label == 0 || node2_label == 0) //this is an oldstyle reservation
	  {
	    // node 2 is always an infrastructure node, so just check node1
	    onldb_resp r1 = is_infrastructure(it3->node1); 
	    if(r1.result() < 0) return onldb_resp(-1, (std::string)"database consistency problem");
	    if(r1.result() == 0) //node is not infrastructure
	      {
		if(node1_label == 0) //if we haven't set the node1 label do so
		  {
		    node1_label = t->get_label(it3->node1);
		    node1_port = it3->node1port;
		    node1_rport = node1_port;
		    node1_cap = cur_cap;
		  }
		else if(node2_label == 0) //if we haven't set the node2 label do so
		  {
		    node2_label = t->get_label(it3->node1);
		    node2_port = it3->node1port;
		    node2_rport = node1_port;
		    node2_cap = cur_cap;
		  }
		else
		  {
		    return onldb_resp(-1, (std::string)"database consistency problem");
		  }
	      }
	  }
	else if (node1_label == t->get_label(it3->node1) || node1_name == it3->node1) node1_rport = it3->node1port;
	else if (node2_label == t->get_label(it3->node1) || node2_name == it3->node1) node2_rport = it3->node1port;
      }
      if(!li.empty())
      {
        //if(!add_link(t, rid, cur_link, linkid, cur_cap, node1_label, node1_port, node2_label, node2_port, cur_rload, cur_lload))
	if(!add_link(t, rid, cur_link, linkid, cur_cap, node1_label, node1_port, node1_rport, node2_label, node2_port, node2_rport, cur_rload, cur_lload, node1_cap, node2_cap))
        {
          return onldb_resp(-1, (std::string)"database consistency problem");
        }
        t->links.back()->conns = cur_conns;
      }
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
  return onldb_resp(1, (std::string)"success");
}

void onldb::build_assign_list(node_resource_ptr hw, std::list<assign_info_ptr> *l) throw()
{
  std::list<link_resource_ptr>::iterator lit;
  std::list<assign_info_ptr>::iterator ait;

  if(hw->marked) return;
  hw->marked = true;
    
  for(ait = l->begin(); ait != l->end(); ++ait)
  {
    if((*ait)->type == hw->type)
    {
      (*ait)->user_nodes.push_back(hw);
      break;
    }
  }
  if(ait == l->end())
  {
    assign_info_ptr newnode(new assign_info());
    newnode->type = hw->type;
    newnode->user_nodes.push_back(hw);
    newnode->marked = false;
    l->push_back(newnode);
  }

  for(lit = hw->links.begin(); lit != hw->links.end(); ++lit)
  {
    if((*lit)->marked) continue;
    (*lit)->marked = true;
    node_resource_ptr other_end = (*lit)->node1;
    if((*lit)->node1->label == hw->label)
    {
      other_end = (*lit)->node2;
    }
    build_assign_list(other_end, l);
  }
  return;
}

onldb_resp onldb::fill_in_topology(topology *t, int rid) throw()
{
  std::list<node_resource_ptr>::iterator nit;
  std::list<link_resource_ptr>::iterator lit;

  // make sure that all the nodes and links are not marked before we start
  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    (*nit)->marked = false;
  }
  for(lit = t->links.begin(); lit != t->links.end(); ++lit)
  {
    (*lit)->marked = false;
  }

  //first create a list of the nodes separated by type, for each disconnected sub-topology
  std::list< std::list<assign_info_ptr>* > assign_lists;
  std::list< std::list<assign_info_ptr>* >::iterator assign_list;
  std::list<assign_info_ptr>::iterator ait;
  std::list<assign_info_ptr> *new_list;

  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    if((*nit)->is_parent) continue;
    if((*nit)->marked) continue;
    new_list = new std::list<assign_info_ptr>();
    build_assign_list(*nit, new_list); //builds a list of connected nodes where an element is a type with a list of nodes of that type
    new_list->sort(req_sort_comp);
    assign_lists.push_back(new_list);
  }

  // next build the reserved topology and add its stuff to the assign_list
  topology res_top;
  onldb_resp r = get_topology(&res_top, rid);
  if(r.result() < 1)
  {
    while(!assign_lists.empty())
    {
      new_list = (std::list<assign_info_ptr> *)assign_lists.front();
      assign_lists.pop_front();
      delete new_list;
    }
    return onldb_resp(r.result(),r.msg());
  }

  std::list<assign_info_ptr> res_list;
  std::list<assign_info_ptr>::iterator rit;
 
  for(nit = res_top.nodes.begin(); nit != res_top.nodes.end(); ++nit)
  {
    if((*nit)->is_parent) continue;
    for(ait = res_list.begin(); ait != res_list.end(); ++ait)
    {
      if((*ait)->type == (*nit)->type || ((*ait)->type == "vm" && (*nit)->has_vmsupport && (*nit)->vmid > 0))
      {
        (*ait)->testbed_nodes.push_back(*nit);
        break;
      }
    }
    if(ait == res_list.end())
    {
      assign_info_ptr newnode(new assign_info());
      if ((*nit)->has_vmsupport && (*nit)->vmid > 0)
	newnode->type = "vm";
      else
	newnode->type = (*nit)->type;
      newnode->testbed_nodes.push_back(*nit);
      res_list.push_back(newnode);
    }
  }

  // check that the the number of each requested type is <= the number reserved
  for(rit = res_list.begin(); rit != res_list.end(); ++rit)
  {
    unsigned int num_abs = 0;
    for(assign_list = assign_lists.begin(); assign_list != assign_lists.end(); ++assign_list)
    {
      for(ait = (*assign_list)->begin(); ait != (*assign_list)->end(); ++ait)
      {
        if((*ait)->type == (*rit)->type)
        {
          (*ait)->marked = true;
          num_abs += (*ait)->user_nodes.size();
        }
      }
    }
    if(num_abs > (*rit)->testbed_nodes.size())
    {
      std::string s = "you requested more " + (*rit)->type + "s than you reserved";
      while(!assign_lists.empty())
      {
        new_list = (std::list<assign_info_ptr> *)assign_lists.front();
        assign_lists.pop_front();
        delete new_list;
      }
      return onldb_resp(0,s);
    }
  }
  for(assign_list = assign_lists.begin(); assign_list != assign_lists.end(); ++assign_list)
  {
    for(ait = (*assign_list)->begin(); ait != (*assign_list)->end(); ++ait)
    {
      if((*ait)->marked == false)
      {
        std::string s = "you requested more " + (*ait)->type + "s than you reserved";
        while(!assign_lists.empty())
        {
          new_list = (std::list<assign_info_ptr> *)assign_lists.front();
          assign_lists.pop_front();
          delete new_list;
        }
        return onldb_resp(0,s);
      }
    }
  }
 
  // make sure everything is unmarked and cleared before starting to find an assignment
  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    (*nit)->marked = false;
    (*nit)->node = "";
    (*nit)->acl = "unused";
    (*nit)->cp = "unused";
    (*nit)->level = 0;
  }
  for(lit = t->links.begin(); lit != t->links.end(); ++lit)
  {
    (*lit)->marked = false;
    (*lit)->conns.clear();
    (*lit)->level = 0;
  }
  for(nit = res_top.nodes.begin(); nit != res_top.nodes.end(); ++nit)
  {
    (*nit)->marked = false;
    (*nit)->level = 0;
  }
  for(lit = res_top.links.begin(); lit != res_top.links.end(); ++lit)
  {
    (*lit)->marked = false;
    (*lit)->level = 0;
  }

  // recursively try to make the assigment from reserved topology to requested topology, one subset at a time
  assign_list = assign_lists.begin();
  bool success = false;
  if(subset_assign(res_list, assign_list, assign_lists.end(), 1))
  {
    success = true;
  }

  while(!assign_lists.empty())
  {
    new_list = (std::list<assign_info_ptr> *)assign_lists.front();
    assign_lists.pop_front();
    delete new_list;
  }

  if(success) return onldb_resp(1,(std::string)"success");
  return onldb_resp(0,(std::string)"the requested topology does not match what was reserved");
}  

bool onldb::subset_assign(std::list<assign_info_ptr> rl, std::list< std::list<assign_info_ptr>* >::iterator ali, std::list< std::list<assign_info_ptr>* >::iterator end, unsigned int level) throw()
{
  if(ali == end) return true;
  std::list<assign_info_ptr> al = **ali;//look at a particular subtopology
  std::list<assign_info_ptr>::iterator ai, clean;
  std::list<assign_info_ptr>::iterator ri;
  std::list< std::list<assign_info_ptr>* >::iterator alinew;

  alinew = ali;
  ++alinew;

  // for the abstract components with the fewest number of that type, try assigning each reserved component
  // to the first such abstract component
  ai = al.begin();
  for(ri = rl.begin(); ri != rl.end(); ++ri)
  {
    if((*ai)->type == (*ri)->type) break;
  }
  if(ri == rl.end()) return false;

  std::list<node_resource_ptr>::iterator res_nit;
  std::list<node_resource_ptr>::iterator nit;
  std::list<link_resource_ptr>::iterator lit;
  for(res_nit = (*ri)->testbed_nodes.begin(); res_nit != (*ri)->testbed_nodes.end(); ++res_nit)
  {
    if((*res_nit)->marked) continue;

    if(find_mapping(*((*ai)->user_nodes.begin()), *res_nit, level))//if we find a mapping for this subtopology using this reservation node
    {
      if(subset_assign(rl, alinew, end, level+1)) return true; //try finding a mapping for the next subtopology
    }

    // have to go through and set the marked field to false for all components and links before next test
    for(clean = rl.begin(); clean != rl.end(); ++clean)
    {
      for(nit = (*clean)->testbed_nodes.begin(); nit != (*clean)->testbed_nodes.end(); ++nit)
      {
        if((*nit)->marked && (*nit)->level == level)
        { 
          (*nit)->marked = false;
          (*nit)->level = 0;
          if((*nit)->parent && (*nit)->parent->marked && (*nit)->parent->level == level)
          {
            (*nit)->parent->marked = false;
            (*nit)->parent->level = 0;
          }
          for(lit = (*nit)->links.begin(); lit != (*nit)->links.end(); ++lit)
          {
            if((*lit)->marked && (*lit)->level == level)
            { 
              (*lit)->marked = false;
              (*lit)->level = 0;
            }
          }
        }
      }
    }
    for(clean = al.begin(); clean != al.end(); ++clean)
    {
      for(nit = (*clean)->user_nodes.begin(); nit != (*clean)->user_nodes.end(); ++nit)
      {
        (*nit)->marked = false;
        (*nit)->node = "";
        (*nit)->acl = "unused";
        (*nit)->cp = "unused";
	(*nit)->vmid = 0;
        if((*nit)->parent && (*nit)->parent->marked && (*nit)->parent->level == level)
        {
          (*nit)->parent->marked = false;
          (*nit)->parent->level = 0;
        }
        for(lit = (*nit)->links.begin(); lit != (*nit)->links.end(); ++lit)
        {
          (*lit)->marked = false;
          (*lit)->conns.clear();
	  (*lit)->node1_rport = -1;
	  (*lit)->node2_rport = -1;
        }
      }
    }
  }
  return false;
}

//can we map the abstract node, abs_node, from a user topology to the physical hardware node, res_node, 
//that's part of the reserved topo 
bool onldb::find_mapping(node_resource_ptr abs_node, node_resource_ptr res_node, unsigned int level) throw()
{
  std::list<link_resource_ptr>::iterator abs_lit;
  std::list<link_resource_ptr>::iterator res_lit;

  if(abs_node->marked) return true;  //this user node is already mapped to a physical node so return success
  if(res_node->marked) return false; //this reservation node is already mapped so we're not going to map 
                                     //the user node to it return failure

  if(abs_node->parent && abs_node->parent->marked && (abs_node->parent->node != res_node->parent->node)) return false;

  abs_node->marked = true;
  res_node->marked = true;
  res_node->level = level;
  abs_node->node = res_node->node;
  abs_node->acl = res_node->acl;
  abs_node->cp = res_node->cp;
  abs_node->vmid = res_node->vmid;
  abs_node->root_only = res_node->root_only;
  if(abs_node->parent && !abs_node->parent->marked) 
  {
    abs_node->parent->node = res_node->parent->node;
    abs_node->parent->marked = true;
    abs_node->parent->level = level;
    res_node->parent->marked = true;
    res_node->parent->level = level;
  }

  for(abs_lit = abs_node->links.begin(); abs_lit != abs_node->links.end(); ++abs_lit)
  {
    if((*abs_lit)->marked) continue;

    node_resource_ptr abs_other_end = (*abs_lit)->node1;
    unsigned int abs_this_port = (*abs_lit)->node2_port;
    unsigned int abs_this_cap = (*abs_lit)->node2_capacity;
    unsigned int abs_other_port = (*abs_lit)->node1_port;
    unsigned int abs_other_cap = (*abs_lit)->node1_capacity;
    bool abs_this_is_loopback = false;
    if((*abs_lit)->node1->label == (*abs_lit)->node2->label)
    {
      abs_this_is_loopback = true;
    }
    if((*abs_lit)->node1->label == abs_node->label)
    {
      abs_other_end = (*abs_lit)->node2;
      abs_this_port = (*abs_lit)->node1_port;
      abs_other_port = (*abs_lit)->node2_port;
      abs_this_cap = (*abs_lit)->node1_capacity;
      abs_other_cap = (*abs_lit)->node2_capacity;
    }
    
    for(res_lit = res_node->links.begin(); res_lit != res_node->links.end(); ++res_lit)
    {
      if((*res_lit)->marked) continue;
  
      node_resource_ptr res_other_end = (*res_lit)->node1;
      unsigned int res_this_port = (*res_lit)->node2_port;
      unsigned int res_other_port = (*res_lit)->node1_port;
      unsigned int res_this_rport = (*res_lit)->node2_rport;
      unsigned int res_other_rport = (*res_lit)->node1_rport;
      unsigned int res_this_vmid = (*res_lit)->node2->vmid;
      unsigned int res_other_vmid = (*res_lit)->node1->vmid;
      unsigned int res_this_cap = (*res_lit)->node2_capacity;
      unsigned int res_other_cap = (*res_lit)->node1_capacity;
      bool res_this_is_loopback = false;
      if((*res_lit)->node1->label == (*res_lit)->node2->label && (*res_lit)->node1->vmid == (*res_lit)->node2->vmid)
      {
        res_this_is_loopback = true;
      }
      if(abs_this_is_loopback != res_this_is_loopback) continue;
      // dealing with loopback links, if the ports don't line up, switch them.
      // if they still don't line up, then this isn't the right link
      if( (res_this_is_loopback && (abs_this_port != res_this_port)) ||
          (!res_this_is_loopback && ((*res_lit)->node1->label == res_node->label && (*res_lit)->node1->vmid == res_node->vmid)) )
      {
        res_other_end = (*res_lit)->node2;
        res_this_port = (*res_lit)->node1_port;
        res_other_port = (*res_lit)->node2_port;
        res_this_rport = (*res_lit)->node1_rport;
        res_other_rport = (*res_lit)->node2_rport;
	res_this_vmid = (*res_lit)->node1->vmid;
	res_other_vmid = (*res_lit)->node2->vmid;
	res_this_cap = (*res_lit)->node1_capacity;
	res_other_cap = (*res_lit)->node2_capacity;
      }

      if(abs_this_port == res_this_port && (res_this_cap <= 0 || (res_this_cap == abs_this_cap)))
      {
        if(abs_other_end->type != res_other_end->type && !(abs_other_end->type == "vm" && res_other_end->has_vmsupport)) return false;
        if(abs_other_port != res_other_port) return false;

        (*abs_lit)->marked = true;
        (*res_lit)->marked = true;
        (*res_lit)->level = level;
        (*abs_lit)->conns = (*res_lit)->conns;
	//set real physical interfaces in the user link
	if ((abs_node->label == ((*abs_lit)->node1->label)) ||
	    (res_this_is_loopback && (abs_this_port == ((*abs_lit)->node1_port))))
	  {
	    (*abs_lit)->node1_rport = res_this_rport;
	    (*abs_lit)->node2_rport = res_other_rport;
	  }
	else
	  {
	    (*abs_lit)->node2_rport = res_this_rport;
	    (*abs_lit)->node1_rport = res_other_rport;
	  }

        if(find_mapping(abs_other_end, res_other_end, level) == false) return false;
        break;
      }
    }
    if(res_lit == res_node->links.end()) return false;
  }

  return true;
}

bool onldb::check_fixed_comps(std::list<node_resource_ptr>& fixed_comp, std::string begin, std::string end) throw()
{
    std::list<node_resource_ptr>::iterator fit;
    vector<nodenames>::iterator nnit;
    mysqlpp::Query query = onl->query();
    //get a list of nodes in repair or testing
    query << "select nodes.node from nodes where node in (select node from nodeschedule where rid in (select rid from reservations where state!='cancelled' and state!='timedout' and (user='testing' or user='repair') and begin<" << mysqlpp::quote << end << " and end>" << mysqlpp::quote << begin << " )) order by nodes.node";
    vector<nodenames> nn;
    ++db_count;
    query.storein(nn);

    for(fit = fixed_comp.begin(); fit != fixed_comp.end(); ++fit)
      {
	for (nnit = nn.begin(); nnit != nn.end(); ++nnit)
	  {
	    if ((*fit)->node == nnit->node) return false;
	  }
      }
    return true;
}

onldb_resp onldb::get_base_topology(topology *t, std::string begin, std::string end, bool noreservations) throw()
{
  struct timeval stime;
  gettimeofday(&stime, NULL);
  unsigned hwid = 1;
  unsigned linkid = 1;
  try
  {
    mysqlpp::Query query_tp = onl->query();
    query_tp << "select tid,hasvport,vmsupport,corecapacity,memcapacity,numinterfaces,interfacebw from types";
    vector<typeinfo> typenfo;
    query_tp.storein(typenfo);

    mysqlpp::Query query = onl->query();
    query << "select cluster,priority,tid from hwclusters where cluster not in (select cluster from hwclusterschedule where rid in (select rid from reservations where state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << end << " and end>" << mysqlpp::quote << begin << " )) order by cluster";//rand()";
    vector<baseclusterinfo> bci;
    ++db_count;
    query.storein(bci);
    vector<baseclusterinfo>::iterator it;
    for(it = bci.begin(); it != bci.end(); ++it)
    { 
      onldb_resp r = t->add_node(it->tid, hwid, 0);
      if(r.result() != 1)
      {
        return onldb_resp(-1, (std::string)"database consistency problem");
      }
      ++hwid;
      t->nodes.back()->node = it->cluster;
      t->nodes.back()->priority = (int)it->priority;
      t->nodes.back()->marked = false;
    }

    //now get the clusters already in a reservation
    mysqlpp::Query querya = onl->query();
    querya << "select cluster,priority,tid from hwclusters where cluster in (select cluster from hwclusterschedule where rid in (select rid from reservations where state!='cancelled' and state!='timedout' and user!='testing' and user!='repair' and begin<" << mysqlpp::quote << end << " and end>" << mysqlpp::quote << begin << " )) order by cluster";//rand()";
    vector<baseclusterinfo> bcia;
    ++db_count;
    querya.storein(bcia);
    for(it = bcia.begin(); it != bcia.end(); ++it)
    { 
      onldb_resp r = t->add_node(it->tid, hwid, 0);
      if(r.result() != 1)
      {
        return onldb_resp(-1, (std::string)"database consistency problem");
      }
      ++hwid;
      t->nodes.back()->node = it->cluster;
      t->nodes.back()->priority = (int)it->priority;
      if (noreservations)
	t->nodes.back()->marked = false;
      else
	t->nodes.back()->marked = true;
    }

    mysqlpp::Query query2 = onl->query();
    query2 << "select nodes.node,nodes.priority,nodes.tid,hwclustercomps.cluster from nodes left join hwclustercomps using (node) where node not in (select node from nodeschedule where rid in (select rid from reservations where state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << end << " and end>" << mysqlpp::quote << begin << " )) order by nodes.node";//rand()";
    vector<basenodeinfo> bni;
    ++db_count;
    query2.storein(bni);
    vector<basenodeinfo>::iterator it2;
    for(it2 = bni.begin(); it2 != bni.end(); ++it2)
    {
      unsigned int parent_label = 0;
      if(!it2->cluster.is_null)
      {
        parent_label = t->get_label(it2->cluster.data);
        if(parent_label == 0) continue;
      }
      onldb_resp r = t->add_node(it2->tid, hwid, parent_label);
      if(r.result() != 1)
      {
        return onldb_resp(-1, (std::string)"database consistency problem");
      }
      ++hwid;
      t->nodes.back()->node = it2->node;
      t->nodes.back()->priority = (int)it2->priority;
      t->nodes.back()->marked = false;
      fill_node_info(t->nodes.back(), typenfo);
      //t->nodes.back()->has_vport = has_vport(it2->tid, typenfo);

      onldb_resp ri = is_infrastructure(it2->node); 
      if(ri.result() < 0) return onldb_resp(-1, (std::string)"database consistency problem");
      if(ri.result() == 1)
      {
        t->nodes.back()->in = t->nodes.back()->label;
      }
    }

    //now get nodes already in a reservation
    mysqlpp::Query query2a = onl->query();
    //query2a << "select nodes.node,nodes.priority,nodes.tid,hwclustercomps.cluster from nodes left join hwclustercomps using (node) where node in (select node from nodeschedule where rid in (select rid from reservations where state!='cancelled' and state!='timedout' and user!='testing' and user!='repair' and begin<" << mysqlpp::quote << end << " and end>" << mysqlpp::quote << begin << " )) order by nodes.node";//rand()";
    query2a << "select nodes.node,nodes.priority,nodes.tid,hwclustercomps.cluster,nodeschedule.cores,nodeschedule.memory from nodeschedule join nodes using (node) left join hwclustercomps using (node) where rid in (select rid from reservations where state!='cancelled' and state!='timedout' and user!='testing' and user!='repair' and begin<" << mysqlpp::quote << end << " and end>" << mysqlpp::quote << begin << " ) order by nodes.node";//rand()";
    vector<basenodevminfo>::iterator it2a;
    vector<basenodevminfo> bnia;
    ++db_count;
    query2a.storein(bnia);
    for(it2a = bnia.begin(); it2a != bnia.end(); ++it2a)
    {
      unsigned int parent_label = 0;
      node_resource_ptr current_node = t->get_node(it2a->node);
      if(!it2a->cluster.is_null)
      {
        parent_label = t->get_label(it2a->cluster.data);
        if(parent_label == 0) continue;
      }
      if (!current_node)
	{
	  onldb_resp r = t->add_node(it2a->tid, hwid, parent_label);
	  if(r.result() != 1)
	    {
	      return onldb_resp(-1, (std::string)"database consistency problem");
	    }
	  ++hwid;
	  current_node = t->nodes.back();
	  current_node->node = it2a->node;
	  current_node->priority = (int)it2a->priority;
	  fill_node_info(current_node, typenfo);
	  //t->nodes.back()->has_vport = has_vport(it2->tid, typenfo);

	  onldb_resp ri = is_infrastructure(it2a->node); 
	  if(ri.result() < 0) return onldb_resp(-1, (std::string)"database consistency problem");
	  if(ri.result() == 1)
	    {
	      current_node->in = current_node->label;
	    }
	}
      if (noreservations)
	current_node->marked = false;
      else
	{
	  if (current_node->has_vmsupport)
	    {
	      current_node->core_capacity -= (it2a->cores);
	      current_node->mem_capacity -= (it2a->memory);
	      current_node->marked = false;
	    }
	  else 
	    {
	      current_node->core_capacity = 0;
	      current_node->mem_capacity = 0;
	      current_node->marked = true;
	    }
	}
    }

    mysqlpp::Query query3 = onl->query();
    query3 << "select cid,capacity,node1,node1port,node2,node2port from connections order by node1,node1port";
    vector<baselinkinfo> bli;
    ++db_count;
    query3.storein(bli);
    vector<baselinkinfo>::iterator it3;
    for(it3 = bli.begin(); it3 != bli.end(); ++it3)
    {
      if(it3->cid == 0) { continue; }

      int cap = it3->capacity * 1000;//capacity is in Gbps we need to convert to Mbps
      int rl = 0;
      int ll = 0;
      mysqlpp::Query query4 = onl->query();
      if (!noreservations)
	{
	  query4 << "select capacity,rload,lload from connschedule where cid=" << mysqlpp::quote << it3->cid << " and rid in (select rid from reservations where state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << end << " and end>" << mysqlpp::quote << begin << " )";
	  vector<caploadinfo> ci;
	  ++db_count;
	  query4.storein(ci);
	  vector<caploadinfo>::iterator it4;
	  for(it4 = ci.begin(); it4 != ci.end(); ++it4)
	    {
	      //cap -= it4->capacity;
	      if (it4->rload <= 10)//this is an old reservation in Gbps we need to convert the loads to Mbps
		{
		  rl += (it4->rload * 1000);
		  ll += (it4->lload * 1000);
		}
	      else
		{
		  rl += it4->rload;
		  ll += it4->lload;
		}
	    }
	}
      
      if(cap <= rl || cap <= ll) { continue; }

      unsigned int node1_label = 0;
      unsigned int node2_label = 0;

      node1_label = t->get_label(it3->node1);
      if(node1_label == 0) { continue; }
      node2_label = t->get_label(it3->node2);
      if(node2_label == 0) { continue; }

      onldb_resp r = t->add_link(linkid, cap, node1_label, it3->node1port, node2_label, it3->node2port, rl, ll);
      if(r.result() != 1) return onldb_resp(-1, (std::string)"database consistency problem");
      t->links.back()->conns.push_back(it3->cid);
      if (t->links.back()->node1->has_vport)//if the non-infrastructure node has virtual ports set capacity to the effective capacity for that port
	{
	  t->links.back()->capacity = t->links.back()->node1->port_capacities[t->links.back()->node1_port];
	}
 
      bool node1_is_in = false;
      onldb_resp ri = is_infrastructure(it3->node1); 
      if(ri.result() < 0) return onldb_resp(-1, (std::string)"database consistency problem");
      if(ri.result() == 1) { node1_is_in = true; }

      // node2 is always an infrastructure node
      if(!node1_is_in)
      {
        t->links.back()->node1->in = node2_label;
	t->links.back()->node1->port_capacities[it3->node1port] -= rl; //subtract the load 
        if(t->links.back()->node1->parent) { t->links.back()->node1->parent->in = node2_label; }
	//set port
      }

      ++linkid;
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }

  print_diff("onldb::get_base_topology", stime);

  return onldb_resp(1, (std::string)"success");
}

onldb_resp onldb::add_special_node(topology *t, std::string begin, std::string end, node_resource_ptr node) throw()
{
  struct timeval stime;
  gettimeofday(&stime, NULL);
  std::list<node_resource_ptr>::iterator nit;
  std::list<link_resource_ptr>::iterator lit;
  unsigned int hwid = 1;
  unsigned int linkid = 1;
  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    if((*nit)->label > hwid) { hwid = (*nit)->label; }
  }
  ++hwid;
  for(lit = t->links.begin(); lit != t->links.end(); ++lit)
  {
    if((*lit)->label > linkid) { linkid = (*lit)->label; }
  }
  ++linkid;

  try
  {
    mysqlpp::Query query_tp = onl->query();
    query_tp << "select tid,hasvport,vmsupport,corecapacity,memcapacity,numinterfaces,interfacebw from types";
    vector<typeinfo> typenfo;
    query_tp.storein(typenfo);

    mysqlpp::Query query = onl->query();
    query << "select nodes.node,nodes.tid,hwclustercomps.cluster from nodes left join hwclustercomps using (node) where node in (select node from nodeschedule where node=" << mysqlpp::quote << node->node << " and rid in (select rid from reservations where (user='testing' or user='repair' or user='system') and state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << end << " and end>" << mysqlpp::quote << begin << " ))";
    vector<specialnodeinfo> sni;
    ++db_count;
    query.storein(sni);
    if(sni.empty()) { return onldb_resp(0,(std::string)"node " + node->node + " is not available"); }

    mysqlpp::Query query2 = onl->query();
    query2 << "select distinct node2 from connections where node1=" << mysqlpp::quote << node->node;
    vector<node2info> n2i;
    ++db_count;
    query2.storein(n2i);
    if(n2i.size() != 1) { return onldb_resp(0,(std::string)"database consistency problem"); }
    unsigned int in = 0;  
    for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
    {
      if((*nit)->node == n2i[0].node2)
      {
        in = (*nit)->in;
        break;
      }
    }
    if(in == 0) { return onldb_resp(0,(std::string)"database consistency problem"); }

    if(sni[0].cluster.is_null)
    {
      onldb_resp anr = t->add_node(sni[0].tid, hwid, 0);
      if(anr.result() != 1) { onldb_resp(0,(std::string)"database consistency problem"); }
      t->nodes.back()->node = node->node;
      t->nodes.back()->fixed = true;
      t->nodes.back()->in = in;
      fill_node_info(t->nodes.back(), typenfo);
      // anr = has_virtual_port(sni[0].tid);
      //if (anr.result() < 0) { onldb_resp(0,(std::string)"database consistency problem"); }
      //else t->nodes.back()->has_vport = anr.result();
      node->in = in;

      mysqlpp::Query cquery = onl->query();
      cquery << "select cid,capacity,node1,node1port,node2,node2port from connections where node1=" << mysqlpp::quote << node->node;
      vector<baselinkinfo> bli;
      cquery.storein(bli);
      vector<baselinkinfo>::iterator cit;
      for(cit = bli.begin(); cit != bli.end(); ++cit)
      {
        unsigned int node2_label = 0;
        node2_label = t->get_label(cit->node2);
        if(node2_label == 0) { continue; }
        
        onldb_resp alr = t->add_link(linkid, (cit->capacity * 1000), hwid, cit->node1port, node2_label, cit->node2port, 0, 0);
        if(alr.result() != 1) { onldb_resp(0,(std::string)"database consistency problem"); }
        t->links.back()->conns.push_back(cit->cid);
        linkid++;
      }

      return onldb_resp(1,(std::string)"success");
    }

    mysqlpp::Query pquery = onl->query();
    pquery << "select tid from hwclusters where cluster=" << mysqlpp::quote << sni[0].cluster.data;
    vector<typenameinfo> ps;
    ++db_count;
    pquery.storein(ps);
    if(ps.size() != 1) { return onldb_resp(0,(std::string)"database consistency problem"); }

    unsigned int parent_label = hwid;
    t->add_node(ps[0].tid, hwid, 0);
    t->nodes.back()->node = sni[0].cluster.data;
    t->nodes.back()->fixed = true;
    t->nodes.back()->in = in;
    fill_node_info(t->nodes.back(), typenfo);
    ++hwid;

    mysqlpp::Query query3 = onl->query();
    query3 << "select node,tid from nodes where node in (select node from hwclustercomps where cluster=" << mysqlpp::quote << sni[0].cluster << ")";
    vector<specnodeinfo> ni;
    ++db_count;
    query3.storein(ni);
    vector<specnodeinfo>::iterator niit;
    for(niit = ni.begin(); niit != ni.end(); ++niit)
    {
      onldb_resp anr = t->add_node(niit->tid, hwid, parent_label);
      if(anr.result() != 1) { return onldb_resp(0, (std::string)"database consistency problem"); }
      t->nodes.back()->node = niit->node;
      t->nodes.back()->in = in;
      fill_node_info(t->nodes.back(), typenfo);
      // anr = has_virtual_port(niit->tid);
      //if (anr.result() < 0) { onldb_resp(0,(std::string)"database consistency problem"); }
      //else t->nodes.back()->has_vport = anr.result();
      if(niit->node == node->node)
      {
        t->nodes.back()->fixed = true;
        node->in = in;
        node->parent->fixed = true;
        node->parent->in = in;
        node->parent->node = sni[0].cluster.data;
      }

      mysqlpp::Query cquery = onl->query();
      cquery << "select cid,capacity,node1,node1port,node2,node2port from connections where node1=" << mysqlpp::quote << niit->node;
      vector<baselinkinfo> bli;
      ++db_count;
      cquery.storein(bli);
      vector<baselinkinfo>::iterator cit;
      for(cit = bli.begin(); cit != bli.end(); ++cit)
      {
        unsigned int node2_label = 0;
        node2_label = t->get_label(cit->node2);
        if(node2_label == 0) { continue; }

        onldb_resp alr = t->add_link(linkid, cit->capacity, hwid, cit->node1port, node2_label, cit->node2port, 0, 0);
        if(alr.result() != 1) { onldb_resp(0,(std::string)"database consistency problem"); }
        t->links.back()->conns.push_back(cit->cid);
        linkid++;
      }
      ++hwid;
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
  print_diff("onldb::add_special_node", stime);
  return onldb_resp(1, (std::string)"success");
}

//JP changed to set remapped reservation to "used" 3/29/2012
//onldb_resp onldb::try_reservation(topology *t, std::string user, std::string begin, std::string end) throw()
onldb_resp onldb::try_reservation(topology *t, std::string user, std::string begin, std::string end, std::string state, bool noreservations) throw()
{
  fnd_path = 0;
  //first create a list of the nodes separated by type
  struct timeval stime;
  gettimeofday(&stime, NULL);
  std::list<assign_info_ptr> assign_list;

  std::list<assign_info_ptr>::iterator ait;
  std::list<node_resource_ptr>::iterator nit;
  std::list<node_resource_ptr>::iterator fixed_comp;
  std::list<link_resource_ptr>::iterator lit;
  std::list<mapping_cluster_ptr> cluster_list; //list of infrastructure nodes cooresponding to the clusters

  std::list<node_resource_ptr> fixed_comps;

  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    (*nit)->marked = false;
    if((*nit)->is_parent) { continue; }

    if((*nit)->fixed)
    {
      fixed_comps.push_back(*nit);
    }

    for(ait = assign_list.begin(); ait != assign_list.end(); ++ait)
    {
      if((*ait)->type == (*nit)->type)
      {
        (*ait)->user_nodes.push_back(*nit);
        break;
      }
    }

    if(ait == assign_list.end())
    {
      assign_info_ptr newnode(new assign_info());
      newnode->type = (*nit)->type;
      std::string tt = get_type_type((*nit)->type);
      if(tt == "") return onldb_resp(-1, (std::string)"database consistency problem");
      newnode->type_type = tt;
      newnode->user_nodes.push_back(*nit);
      assign_list.push_back(newnode);
    }
  }

  // sort the requested list by increasing number of comps to facilitate matching
  assign_list.sort(req_sort_comp);


  // next build the base topology
  topology base_top;
  onldb_resp r = get_base_topology(&base_top, begin, end, noreservations);
  if(r.result() < 1) return onldb_resp(r.result(),r.msg());

  onldb_resp ra = is_admin(user);
  if(ra.result() < 0) return onldb_resp(ra.result(),ra.msg());
  bool admin = false;
  if(ra.result() == 1) { admin = true; }

  //if not admin. check if they are requesting fixed components in testing or repair and say no
  if (!fixed_comps.empty() && !admin && !check_fixed_comps(fixed_comps, begin, end)) 
     return onldb_resp(0, "fixed nodes in testing or repair");

  // handle fixed components, potentially adding admin stuff to the base topology
  for(fixed_comp = fixed_comps.begin(); fixed_comp != fixed_comps.end(); ++fixed_comp)
  {
    for(nit = base_top.nodes.begin(); nit != base_top.nodes.end(); ++nit)
      {
	//nodes with vm support have marked=false even if there is something scheduled
	if((*nit)->node == (*fixed_comp)->node && !(*nit)->marked)
	  {
	    (*fixed_comp)->in = (*nit)->in;
	    (*nit)->fixed = true;
	    if((*nit)->parent)
	      {
		(*fixed_comp)->parent->fixed = true;
		(*fixed_comp)->parent->in = (*nit)->in;
		(*nit)->parent->fixed = true;
		(*fixed_comp)->parent->node = (*nit)->parent->node;
	      }
	    break;
	  }
      }
    if(nit == base_top.nodes.end())
      {
	if(!admin)
	  {
	    return onldb_resp(0, "fixed node " + (*fixed_comp)->node + " is not available");
	  }
	onldb_resp fcr = add_special_node(&base_top, begin, end, *fixed_comp);
	if(fcr.result() < 1) return onldb_resp(fcr.result(), fcr.msg());
      }
  }

  // add everything from the base topology to the assign list
  cout << "try_reservation base topology" << endl << "BASE NODES:" << endl;
  for(nit = base_top.nodes.begin(); nit != base_top.nodes.end(); ++nit)
  {
    cout << "(" << (*nit)->type << (*nit)->label << ", " << (*nit)->node << ", " << (*nit)->has_vport << ", " <<  (*nit)->marked << ", " 
	 << (*nit)->has_vmsupport << ", " << (*nit)->core_capacity << ", " << (*nit)->mem_capacity << ", ports[";
    int max = (*nit)->port_capacities.size();
    
    for (int i = 0; i < max; ++i)
      {
	if (i > 0)
	  cout << ", ";
	cout << (*nit)->port_capacities[i];
      }
    cout << "])" << endl;
    //(*nit)->marked = false;
    if((*nit)->is_parent || (*nit)->marked) { continue; }

    for(ait = assign_list.begin(); ait != assign_list.end(); ++ait)
    {
      if((*ait)->type == (*nit)->type)
      {
        (*nit)->type_type = (*ait)->type_type;
        (*ait)->testbed_nodes.push_back(*nit);
        break;
      }
    }
    if(ait == assign_list.end())
    {
      assign_info_ptr newnode(new assign_info());
      newnode->type = (*nit)->type;
      std::string tt = get_type_type((*nit)->type);
      if(tt == "") return onldb_resp(-1, (std::string)"database consistency problem");
      newnode->type_type = tt;
      (*nit)->type_type = tt;
      newnode->testbed_nodes.push_back(*nit);
      assign_list.push_back(newnode);
    }
    if((*nit)->type_type == "infrastructure") 
      {
	mapping_cluster_ptr mcluster(new mapping_cluster_resource());
	mcluster->cluster = (*nit);
	fill_in_cluster(mcluster);
	cluster_list.push_back(mcluster); //JP added for new scheduling
      }
  }

  cout << "end BASE NODES" << endl;
  
  // check that the the number of each requested type is <= to the number available
  for(ait = assign_list.begin(); ait != assign_list.end(); ++ait)
  {
    if((*ait)->type == "vgige" || (*ait)->type =="vm") { continue; }
    if((*ait)->user_nodes.size() > (*ait)->testbed_nodes.size())
    {
      std::string s = "too many " + (*ait)->type + "s already reserved";
      return onldb_resp(0,s);
    }
    
    // sort the base comp list by priority
    (*ait)->testbed_nodes.sort(base_sort_comp);
  }
      
  // for(nit = base_top.nodes.begin(); nit != base_top.nodes.end(); ++nit)
  // {
  // (*nit)->marked = false;
  //}
  for(lit = base_top.links.begin(); lit != base_top.links.end(); ++lit)
  {
    (*lit)->marked = false;
  }
  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    if(!((*nit)->fixed))
    {
      (*nit)->node = "";
    }
    (*nit)->marked = false;
  }
  for(lit = t->links.begin(); lit != t->links.end(); ++lit)
  {
    (*lit)->marked = false;
    (*lit)->conns.clear();
  }

  if(find_embedding(t, &base_top, cluster_list, noreservations))
    {
      print_diff("onldb::try_reservation succeeded", stime);
      if (!noreservations)
	{
	  onldb_resp r = add_reservation(t,user,begin,end,state);//JP changed 3/29/2012
	  if(r.result() < 1) return onldb_resp(r.result(),r.msg());
	}
      std::string s = "success! reservation made from " + begin + " to " + end;
      cout << "onldb::try_reservation find_cheapest_path called " << fnd_path << " times." << endl;
      return onldb_resp(1,s);
    }
  cout << "onldb::try_reservation find_cheapest_path called " << fnd_path << " times." << endl;
  
  print_diff("onldb::try_reservation failed", stime);
  return onldb_resp(0,(std::string)"topology doesn't fit during that time");
}

bool onldb::find_embedding(topology *orig_req,  topology* base, std::list<mapping_cluster_ptr> cl, bool noreservations) throw()
{
  
  struct timeval stime;
  gettimeofday(&stime, NULL);
  std::list<node_resource_ptr>::iterator reqnit;
  std::list<node_resource_ptr>::iterator it;
  std::list<link_resource_ptr>::iterator lit;

  bool inserted_new = false;
  std::list<node_resource_ptr> ordered_nodes;
  
  //make a copy of the original request topology. this allows us to make node/link changes as needed without affecting original nodes
  topology req;
  cout << "find_embedding request topology" << endl << "   nodes:" ;
  for (reqnit = orig_req->nodes.begin(); reqnit != orig_req->nodes.end(); ++reqnit)
    {
      req.add_copy_node((*reqnit));
      cout << "(" << (*reqnit)->type << (*reqnit)->label << ")";
    }
  
  cout << endl << "    links:";

  for (lit = orig_req->links.begin(); lit != orig_req->links.end(); ++lit)
    {
      req.add_copy_link((*lit));
      cout << "(link" << (*lit)->label << "," << (*lit)->node1->type << (*lit)->node1->label << ":" << (*lit)->node1_port
	   << "," << (*lit)->node2->type << (*lit)->node2->label << ":" << (*lit)->node2_port << ")";
    }

  cout << endl;


  calculate_node_costs(&req);

  for(reqnit = req.nodes.begin(); reqnit != req.nodes.end(); ++reqnit)
    {
      if ((*reqnit)->parent) continue; //if this is a child node skip it, we'll add the parent and treat the hwcluster as a single node
      inserted_new = false;
      //order nodes fixed nodes first the rest based on cost, most costly first. 
      for(it = ordered_nodes.begin(); it != ordered_nodes.end(); ++it)
	{
	  if (((*reqnit)->fixed && ((!(*it)->fixed) || ((*reqnit)->cost > (*it)->cost))) ||
	      (!(*reqnit)->fixed && !(*it)->fixed &&  ((*reqnit)->cost > (*it)->cost)))
	    {
	      ordered_nodes.insert(it, (*reqnit));
	      inserted_new = true;
	      break;
	    }
	}
      if (!inserted_new)
	ordered_nodes.push_back(*reqnit);
    }

  mapping_cluster_ptr fcluster;
  node_resource_ptr new_node;
  for(reqnit = ordered_nodes.begin(); reqnit != ordered_nodes.end(); ++reqnit)
    {
      fcluster = find_feasible_cluster(*reqnit, cl, &req, base);
      if (fcluster)
	{
	  //new_node = NULL;
	  new_node = map_node(*reqnit, &req, fcluster, cl, base);
	  //if new_node is returned then add it into the ordered node list
	  if (new_node)
	    {
	      inserted_new = false;
	      bool at_start = false;
	      for (it = ordered_nodes.begin(); it != ordered_nodes.end(); ++it)
		{
		  if (at_start &&  
		      ((new_node->fixed && ((!(*it)->fixed) || (new_node->cost > (*it)->cost))) ||
		       (!new_node->fixed && !(*it)->fixed &&  (new_node->cost > (*it)->cost))))
		    {
		      ordered_nodes.insert(it, new_node);
		      inserted_new = true;
		      break;
		    }
		  if ((*it) == (*reqnit)) at_start = true; //first get to the point we were at when we created the new node
		}
	      if (!inserted_new) ordered_nodes.push_back(new_node);
	    } 
	  reset_cluster(fcluster);
	}
      else
	{
	  cout << " reservation failed on node " << (*reqnit)->type << (*reqnit)->label << endl;
          //report_metrics(req, al);
          //unmap_reservation(req);//al);
	  return false;
	}
    }
  //return if just checking for valid topology and not really mapping
  if (noreservations) return true;

  //map assignments onto original request so database is updated properly
  //copy mapped_paths instead of conn ids
  std::list<link_resource_ptr>::iterator blit;
  for (reqnit = req.nodes.begin(); reqnit != req.nodes.end(); ++reqnit)
    {
      if ((*reqnit)->type != "vgige")
	{
	  node_resource_ptr unode = (*reqnit)->user_nodes.front();
	  unode->marked = true;
	  unode->node = (*reqnit)->mapped_node->node;
	}
      else
	{
	  for (it = (*reqnit)->user_nodes.begin(); it != (*reqnit)->user_nodes.end(); ++it)
	    {
	      (*it)->marked = true;
	    }
	  /*
	  link_resource_ptr link_to_add;
	  
	  for (lit = (*reqnit)->links.begin(); lit != (*reqnit)->links.end(); ++lit)
	  {
	    if ((*lit)->user_link) 
	  	{
	  	  link_to_add = (*lit)->user_link;
	  	  break;
	  	}
		}*/

	  node_resource_ptr unode_to_add_to = (*reqnit)->user_nodes.front();
	  link_resource_ptr link_to_add_to;
	  //find the first link with a user link (i.e. one that was not added due to a split)
	  for (lit = (*reqnit)->links.begin(); lit != (*reqnit)->links.end(); ++lit)
	    {
	      if (!(*lit)->added) 
		{
		  link_to_add_to = (*lit)->user_link;
		  cout << "onldb::find_embedding remapping links for node " << (*reqnit)->type << (*reqnit)->label << " link to add:(" << (*lit)->node1->type << (*lit)->node1->label << ","  << (*lit)->node2->type << (*lit)->node2->label << ") conns:(";
		  for (blit = (*lit)->mapped_path.begin(); blit != (*lit)->mapped_path.end(); ++blit)
		    {
		      cout << (*blit)->conns.front() << ",";
		    }
		  cout << ")" << endl;
		  break;
		}
	    }
	  for (lit = (*reqnit)->links.begin(); lit != (*reqnit)->links.end(); ++lit)
	    {
	      //check if this was a newly created link between a split vgige that will not appear in req.links
	      if ((*lit)->added) //this link will appear in two nodes but only add it to one
		{
		  //add to link_to_add_to->added_vgige_links and remove link from other node in link so we don't see it twice
		  
		  //add the extra connections for the new link to an existing user link for a real user node represented by the vgige
		  if (link_to_add_to) 
		    {
		      link_resource_ptr add_vlinkp(new link_resource());
		      add_vlinkp->mapped_path.assign((*lit)->mapped_path.begin(), (*lit)->mapped_path.end());
		      add_vlinkp->mp_linkdir.assign((*lit)->mp_linkdir.begin(), (*lit)->mp_linkdir.end());
		      add_vlinkp->rload = (*lit)->rload;
		      add_vlinkp->lload = (*lit)->lload;
		      add_vlinkp->capacity = (*lit)->capacity;
		      add_vlinkp->added = true;
		      add_vlinkp->linkid = 0;
		      link_to_add_to->added_vgige_links.push_back(add_vlinkp);
		      //remove link from other side so we only add this link once
		      if ((*lit)->node1 == (*reqnit)) (*lit)->node2->links.remove(*lit);
		      else (*lit)->node1->links.remove(*lit);
		    }
		  else
		    {
		      cerr << "onldb::find_embedding error adding link for split vgige" << endl;
		      return false;
		    }
		  
		  cout << " adding link (" << (*reqnit)->type << (*reqnit)->label;
		  if ((*lit)->node1 == (*reqnit)) cout << (*lit)->node2->type << (*lit)->node2->label << ")" << endl;
		  else  cout << (*lit)->node1->type << (*lit)->node1->label << ")" << endl;
		}
	    }
	}
    }
  for (lit = req.links.begin(); lit != req.links.end(); ++lit)
    {
      if ((*lit)->added || !((*lit)->user_link)) continue; //this is a special link added to handle a split vgige
      //if ((*lit)->node1->type == "vgige" || (*lit)->node2->type == "vgige") continue;
      for (blit = (*lit)->mapped_path.begin(); blit != (*lit)->mapped_path.end(); ++blit)
	{
	  (*lit)->user_link->conns.push_back((*blit)->conns.front());
	}
      (*lit)->user_link->marked = true;
      (*lit)->user_link->rload = (*lit)->rload;
      (*lit)->user_link->lload = (*lit)->lload;
      (*lit)->user_link->cost = (*lit)->cost;
      (*lit)->user_link->node1_rport = (*lit)->node1_rport;
      (*lit)->user_link->node2_rport = (*lit)->node2_rport;
      (*lit)->user_link->mapped_path.assign((*lit)->mapped_path.begin(), (*lit)->mapped_path.end());
      (*lit)->user_link->mp_linkdir.assign((*lit)->mp_linkdir.begin(), (*lit)->mp_linkdir.end());
    }
  orig_req->intercluster_cost = req.compute_intercluster_cost();
  orig_req->host_cost = req.compute_host_cost();
  print_diff("onldb::find_embedding", stime);
  return true;
}


void 
onldb::calculate_node_costs(topology* req) throw()
{
  //req->calculate_subnets();
  calculate_edge_loads(req);
  struct timeval stime;
  gettimeofday(&stime, NULL);
  //merge vgige's //this needs to happen after new vgige structures are created in find_embedding
  merge_vswitches(req);
  std::list<node_resource_ptr>::iterator reqnit;
  std::list<node_resource_ptr>::iterator hwclnit;
  std::list<link_resource_ptr>::iterator reqlit;
  for (reqnit = req->nodes.begin(); reqnit != req->nodes.end(); ++reqnit)
    {
      int cost = 0;
      //calculate a separate cost for a cluster as the cost of all childrens' links
      if ((*reqnit)->is_parent)
	{
	  for(hwclnit = (*reqnit)->node_children.begin(); hwclnit != (*reqnit)->node_children.end(); ++hwclnit)
	    {
	      int child_cost = 0;
	      for(reqlit = (*hwclnit)->links.begin(); reqlit != (*hwclnit)->links.end(); ++reqlit)
		child_cost += (*reqlit)->cost;
	      (*hwclnit)->cost = child_cost;
	      cost += child_cost;
	    }
	}
      else if (!((*reqnit)->parent))
	{
	  for(reqlit = (*reqnit)->links.begin(); reqlit != (*reqnit)->links.end(); ++reqlit)
	    cost += (*reqlit)->cost;
	}
      (*reqnit)->cost = cost;
    }
  print_diff("onldb::calculate_node_costs", stime);
}

void
onldb::merge_vswitches(topology* req) throw()
{
  bool found_merge = true;
  std::list<link_resource_ptr>::iterator reqlit;
  std::list<link_resource_ptr>::iterator nlit;
  node_resource_ptr node1;
  node_resource_ptr node2;
  unsigned int total_load;
  std::list<node_resource_ptr>::iterator nit;

  //check if this is a special fixed topology
  for (nit = req->nodes.begin(); nit != req->nodes.end(); ++nit)
    {
      if ((*nit)->fixed) return;
    }

  while (found_merge)
    {
      found_merge = false;
      for (reqlit = req->links.begin(); reqlit != req->links.end(); ++reqlit)
	{
	  total_load = (*reqlit)->lload + (*reqlit)->rload;
	  if ((*reqlit)->node1->type == "vgige" && 
	      (*reqlit)->node2->type == "vgige" && 
	      //total_load <= MAX_INTERCLUSTER_CAPACITY)
	      ((*reqlit)->lload  <= MAX_INTERCLUSTER_CAPACITY) &&
	      ((*reqlit)->rload <= MAX_INTERCLUSTER_CAPACITY))
	    {
	      node1 = (*reqlit)->node1;
	      node2 = (*reqlit)->node2;
	      cout << "merge nodes " << node1->type << node1->label << " and " << node2->type << node2->label << endl;
	      //merge node 1 and 2
	      //change node2's links to point to node1, add links to node 1
	      for (nlit = node2->links.begin(); nlit != node2->links.end(); ++nlit)
		{
		  //cout << "merge link (" << (*nlit)->node1->type << (*nlit)->node1->label << ", " << (*nlit)->node2->type << (*nlit)->node2->label << ")" << endl;
		  if ((*nlit) != (*reqlit))
		    {
		      if ((node1 == (*nlit)->node1 && node2 == (*nlit)->node2) || (node2 == (*nlit)->node1 && node1 == (*nlit)->node2))
			req->links.remove(*nlit);
		      else
			{
			  if ((*nlit)->node1 == node2) 
			    (*nlit)->node1 = node1;
			  else
			    (*nlit)->node2 = node1;
			  node1->links.push_back(*nlit);
			}
		    }
		}
	      node2->links.clear();
	      //add usernodes represented by node2 to node1
	      node1->user_nodes.insert(node1->user_nodes.end(), node2->user_nodes.begin(), node2->user_nodes.end());
	      node2->user_nodes.clear();
	      //remove link from node1->links, node2->links and req->links
	      node1->links.remove(*reqlit);
	      //node2->links.remove(*reqlit);
	      reqlit = req->links.erase(reqlit);
	      //remove node2 from req->nodes
	      req->nodes.remove(node2);
	      found_merge = true;
	      break;
	    }
	}
    }
}

void
onldb::calculate_edge_loads(topology* req) throw()
{
  struct timeval stime;
  gettimeofday(&stime, NULL);
  std::list<node_resource_ptr>::iterator reqnit;
  std::list<link_resource_ptr>::iterator reqlit;
  std::list<link_resource_ptr> links_seen;
  //int load = 0;
  
  for (reqnit = req->nodes.begin(); reqnit != req->nodes.end(); ++reqnit)
    {
      if (((*reqnit)->type != "vgige") && (!(*reqnit)->is_parent)) 
	{
	  links_seen.clear();
	  //cycle through edges for each node in edge calculate the load generated from each side
	  for (reqlit = (*reqnit)->links.begin(); reqlit != (*reqnit)->links.end(); ++reqlit)
	    {
	      //load = (*reqlit)->capacity;
	      if ((*reqlit)->node1 == (*reqnit))
		add_edge_load((*reqlit)->node1, (*reqlit)->node1_port, (*reqlit)->node1_capacity, links_seen);
	      else
		add_edge_load((*reqlit)->node2, (*reqlit)->node2_port, (*reqlit)->node2_capacity, links_seen);
	    }
	}
    }
  //now calculate the edge costs
  for (reqlit = req->links.begin(); reqlit != req->links.end(); ++reqlit)
    {
      (*reqlit)->cost = calculate_edge_cost((*reqlit)->rload, (*reqlit)->lload);
    }
  print_diff("onldb::calculate_edge_loads", stime);
}

void
onldb::add_edge_load(node_resource_ptr node, int port, int load, std::list<link_resource_ptr>& links_seen) throw()
{
  std::list<link_resource_ptr>::iterator nlit;
  std::list<link_resource_ptr>::iterator lslit;
  bool is_seen = false;
  bool is_vgige = false;
  if (node->type == "vgige") is_vgige = true;

  for (nlit = node->links.begin(); nlit != node->links.end(); ++nlit)
    {
      is_seen = false;
      for (lslit = links_seen.begin(); lslit != links_seen.end(); ++lslit)
	{
	  if (*nlit == *lslit)
	    { 
	      is_seen = true;
	      break;
	    }
	}
      if (is_seen) continue;
      if ((*nlit)->node1 == node && ((*nlit)->node1_port == (unsigned int)port || is_vgige))
	{
	  links_seen.push_back(*nlit); 
	  (*nlit)->rload += load;
	  add_edge_load((*nlit)->node2, (*nlit)->node2_port, load, links_seen);
	}
      else if ((*nlit)->node2 == node && ((*nlit)->node2_port == (unsigned int)port || is_vgige))
	{
	  links_seen.push_back(*nlit);
	  (*nlit)->lload += load;
	  add_edge_load((*nlit)->node1, (*nlit)->node1_port, load, links_seen);
	}
    }
}



mapping_cluster_ptr
onldb::find_feasible_cluster(node_resource_ptr node, std::list<mapping_cluster_ptr>& cl, topology* req, topology* base) throw()
{
  struct timeval stime;
  gettimeofday(&stime, NULL);
  std::list<mapping_cluster_ptr>::iterator clnit; //clusters

  if (node->marked) //if the node is already mapped just return the cluster index of the mapped node
    {
      for(clnit = cl.begin(); clnit != cl.end(); ++clnit)
	{
	  if (node->in == (*clnit)->cluster->in)
	    {
	      // reset_cluster(*clnit);
	      return (*clnit);
	    }
	}
    }

  mapping_cluster_ptr rtn_cluster;
  int cluster_cost = -1;
  int current_cost = -1;


  //look through the clusters to see if we can find one that works
  cout << "cluster cost for node:" << node->label << " -- ";
  for (clnit = cl.begin(); clnit != cl.end(); ++clnit)
    {
      if (!node->fixed || (node->in == (*clnit)->cluster->in)) //if the node is fixed only look at the fixed node's cluster
	{
	  cluster_cost = compute_mapping_cost(*clnit, node, req, cl, base);
	  cout << "(c" << (*clnit)->cluster->label << ", " << cluster_cost << ")";
	  if (cluster_cost >= 0 && (cluster_cost < current_cost || !rtn_cluster))
	    {
	      if (rtn_cluster) reset_cluster(rtn_cluster);
	      rtn_cluster = *clnit;
	      current_cost = cluster_cost;
	    }
	  else reset_cluster(*clnit);
	  if (node->fixed)
	    {
	      cout << endl;
	      return rtn_cluster;
	    }
	}
    }
  cout << endl;
  //if (rtn_cluster != NULL)
  print_diff("onldb::find_feasible_cluster", stime);
  return rtn_cluster;
  //else return NULL;
}

//node_resource_ptr
//onldb::find_fixed_node(node_resource_ptr cluster, std::string node) throw()
//{
//  std::list<node_resource_ptr> nodes_used;
//  return (find_fixed_node(cluster, node, nodes_used));
//}


node_resource_ptr
onldb::find_fixed_node(mapping_cluster_ptr cluster, node_resource_ptr node) throw()
{
  return (find_available_node(cluster, node, true));
  //node_resource_ptr rtn;
  //std::list<node_resource_ptr>::iterator clnit;
  //std::list<node_resource_ptr>::iterator nit;
  //for (clnit = cluster->nodes.begin(); clnit != cluster->nodes.end(); ++clnit)
  //{
  //  node_resource_ptr n;
  //  if ((*clnit)->node == node->node ) n = (*clnit);
  //  else if ((*clnit)->parent && (*clnit)->parent->node == node->node) n = (*clnit)->parent;
  //
  //  if (n)
  //	{
  //	  if (n->potential_corecap > 0 && (n->potential_corecap >= node->core_capacity && n->potential_memcap >= node->mem_capacity))
  //	    {
  //	      return n;
  //	    }
  //	  else break;
  //	}
  //}
  //return rtn;
}


node_resource_ptr
onldb::find_available_node(mapping_cluster_ptr cluster, node_resource_ptr node) throw()
{
  return (find_available_node(cluster, node, false));
}


node_resource_ptr
onldb::find_available_node(mapping_cluster_ptr cluster, node_resource_ptr node, bool fixed) throw()
{
  node_resource_ptr rtn;
  std::list<node_resource_ptr>::iterator clnit;
  std::list<node_resource_ptr>::iterator nit;
  std::list<node_resource_ptr> nodes_available;
  subnet_info_ptr subnet(new subnet_info());
  int rtn_num_vms = 0;

  if (node->type == "vm") get_subnet(subnet, node, 0);
  if (node->type == "vgige" && !in_list(cluster->cluster, cluster->rnodes_used)) return cluster->cluster;
  for (clnit = cluster->nodes.begin(); clnit != cluster->nodes.end(); ++clnit)
    {
      node_resource_ptr n;
      if (fixed)
	{
	  if ((*clnit)->node == node->node ) n = (*clnit);
	  else if ((*clnit)->parent && (*clnit)->parent->node == node->node) n = (*clnit)->parent;
	}
      else
	{
	  if ((*clnit)->type == node->type || (node->type == "vm" && (*clnit)->has_vmsupport)) n = (*clnit);
	  else if ((*clnit)->parent && ((*clnit)->parent->type == node->type || (node->type == "vm" && (*clnit)->has_vmsupport))) n = (*clnit)->parent;
	  if (node->type != "vm" && n && n->marked) continue;
	}

      if (n && (!in_list(n, cluster->rnodes_used) || node->type == "vm")) //check core, mem, and interface bw capacities to see if there is enough for this node
	{
	  if (n->potential_corecap > 0 && (n->potential_corecap >= node->core_capacity && n->potential_memcap >= node->mem_capacity))
	    {
	      //if it's a vm or a node with virtual ports requested by new RLI (node's port_capacities will be empty for older requests || whole node request)
	      if (node->type == "vm" || ((n->has_vport) && !node->port_capacities.empty()))
		{
		  std::map<int,int>::iterator uportit;
		  std::map<int,int>::iterator rportit = n->port_capacities.begin();
		  int cap_total = 0;
		  //check for interface bandwidth for vmsupport nodes and nodes that use virtual ports
		  for (uportit = node->port_capacities.begin(); uportit != node->port_capacities.end(); ++uportit)
		    {
		      cap_total += (*uportit).second;
		      while (rportit->second < cap_total && rportit != n->port_capacities.end())
			{
			  cap_total = (*uportit).second;
			  ++rportit;
			}
		      if (rportit == n->port_capacities.end())//this node doesn't have enough interface bw 
			break;
		    }
		  if (rportit != n->port_capacities.end()) //node has enough interface bw
		    {
		      //here for vm check if there is already a vm from this reservation here if there is go on to the next keep track of node with the least number of vms from this reservation on it. 
		      if (node->type == "vm" && !n->user_nodes.empty())
			{
			  int tmp_num = num_in_subnet(subnet, n->user_nodes);
			  if (tmp_num == 0) return n;
			  if (!rtn || (rtn_num_vms > tmp_num))
			    {
			      rtn_num_vms = tmp_num;
			      rtn = n;//keep this if this is the lowest utilized node we've seen but keep looking for one with no vms
			    }
			}
		      else
			return n;
		    }
		  else if (fixed) //the node we want doesn't have enough bw
		    break;
		  //keep looking
		}
	      else
		return n;
	    }
	  else if (fixed) 
	    break;
	}
    }

  //if vm and we didn't find one that was free use the one with the least of this reservation on it. If there is one.
  //in the calling process introduce a larger penalty to a cluster that has to map to a machine that already has a vm from this 
  //reservation on it.
  
  return rtn;
}


void
onldb::get_subnet(subnet_info_ptr subnet, node_resource_ptr source, int port) throw()
{
  struct timeval stime;
  gettimeofday(&stime, NULL);
  std::list<link_resource_ptr>::iterator lit;
  std::list<link_resource_ptr>::iterator snlit;
  std::list<node_resource_ptr>::iterator pnit;
  node_resource_ptr n;

  std::list<node_resource_ptr> pending_nodes;
  subnet->nodes.push_back(source);
  if (source->type != "vgige")
    {
      for (lit = source->links.begin(); lit != source->links.end() ; ++lit)
	{
	  node_resource_ptr n;
	  if ((*lit)->node1 == source && (*lit)->node1_port == port) n = (*lit)->node2;
	  else if ((*lit)->node2 == source && (*lit)->node2_port == port) n = (*lit)->node1;
	  if (n && !in_list(n, subnet->nodes))
	    {
	      subnet->nodes.push_back(n);
	      if (!in_list((*lit), subnet->links)) 
		{
		  subnet->links.push_back(*lit);
		}
	      if ((n->type == "vgige") && (!in_list(n, pending_nodes))) pending_nodes.push_back(n);
	    }
	}
    }
  else
    pending_nodes.push_back(source);

  while(!pending_nodes.empty())
    {
      pnit = pending_nodes.begin();
      for(lit = (*pnit)->links.begin(); lit != (*pnit)->links.end(); ++lit)
	{
	  node_resource_ptr n;
	  if ((*lit)->node1 == (*pnit)) n = (*lit)->node2;
	  else n = (*lit)->node1;
	  //check if we've already seen this node
	  if (!in_list(n, subnet->nodes)) 
	    {
	      subnet->nodes.push_back(n);
	      if (!in_list((*lit), subnet->links)) 
		{
		  subnet->links.push_back(*lit);
		}
	      if ((n->type == "vgige") && (!in_list(n, pending_nodes))) pending_nodes.push_back(n);
	    }
	}
      pending_nodes.erase(pnit);
    }
  print_diff("onldb::get_subnet", stime);
}


bool
onldb::is_cluster_mapped(mapping_cluster_ptr cluster) throw()
{
  std::list<link_resource_ptr>::iterator lit;
  if (cluster->cluster->marked) return true;
  //make sure that base topology represents the full topology even what is in use
  for (lit = cluster->cluster->links.begin(); lit != cluster->cluster->links.end(); ++lit)
    {
      if (((*lit)->node1 == cluster->cluster && (*lit)->node2->marked && ((*lit)->node2->type_type != "infrastructure")) ||
	  ((*lit)->node2 == cluster->cluster && (*lit)->node1->marked && ((*lit)->node1->type_type != "infrastructure")))
	return true;
    }
  cout << " (cluster" << cluster->cluster->in << " not mapped)";
  return false;
}


void
onldb::initialize_base_potential_loads(topology* base)
{
  std::list<link_resource_ptr>::iterator lit;
  for (lit = base->links.begin(); lit != base->links.end(); ++lit)
    {
      //if ((*lit)->node1->type_type == "infrastructure" && (*lit)->node2->type_type == "infrastructure") //we only care about intercluster links
      //{
      if ((*lit)->lload < (*lit)->capacity)
	(*lit)->potential_lcap = ((*lit)->capacity) - ((*lit)->lload);
      else (*lit)->potential_lcap = 0;
      if ((*lit)->rload < (*lit)->capacity)
	(*lit)->potential_rcap = ((*lit)->capacity) - ((*lit)->rload);
      else (*lit)->potential_rcap = 0;
      //}
    }
}

void
onldb::get_mapped_edges(node_resource_ptr node, std::list<link_resource_ptr>& mapped_edges)
{
  std::list<node_resource_ptr>::iterator hwclnit; //hwclusters
  std::list<link_resource_ptr>::iterator lit;
  //make a list of links where the end point has already been mapped for hwclusters look at child node links
  if (node->is_parent)
    {
      for(hwclnit = node->node_children.begin(); hwclnit != node->node_children.end(); ++hwclnit)
	{
	  for(lit = (*hwclnit)->links.begin(); lit != (*hwclnit)->links.end(); ++lit)
	    {
	      if (((*lit)->node1 == (*hwclnit) && (*lit)->node2->marked) ||
		  ((*lit)->node2 == (*hwclnit) && (*lit)->node1->marked))
		mapped_edges.push_back(*lit);
	    }
	}
    }
  else
    {
      for(lit = node->links.begin(); lit != node->links.end(); ++lit)
	{
	  if (((*lit)->node1 == node && (*lit)->node2->marked) ||
	      ((*lit)->node2 == node && (*lit)->node1->marked))
	    mapped_edges.push_back(*lit);
	}
    }
}

int
onldb::compute_mapping_cost(mapping_cluster_ptr mcluster, node_resource_ptr node, topology* req, std::list<mapping_cluster_ptr>& clusters, topology* base) throw()
{
  struct timeval stime;
  gettimeofday(&stime, NULL);
  int cluster_cost = 0;
  unsigned int cin = mcluster->cluster->in;
  std::list<link_resource_ptr>::iterator clusterlit;
  std::list<node_resource_ptr>::iterator reqnit;
  std::list<mapping_cluster_ptr>::iterator clusterit;
  std::list<node_resource_ptr>::iterator hwclnit;
  std::list<node_resource_ptr>::iterator nit;
  std::list<link_resource_ptr>::iterator lit;
  std::list<node_resource_ptr> nodes_used;
  reset_cluster(mcluster);
  

  cout << "compute_mapping_cost for cluster " << mcluster->cluster->node << ":";
  node_resource_ptr n;
  //first find an available node in the cluster to map to //probably need to deal with hwclusters here
  if (node->type == "vgige")
    n = mcluster->cluster;
  else if (node->fixed)
    n = find_fixed_node(mcluster, node);//, nodes_used);
  else
    n = find_available_node(mcluster, node);//->type, nodes_used);
  if (!n) return -1; //we couldn't find an available node of the right type in this cluster
  
  if (!mcluster->mapped)//!is_cluster_mapped(cluster)) 
    {
      cluster_cost += UNUSED_CLUSTER_COST; //penalize for not being mapped to any user_graph
      cout << " not mapped," ;
    }
  else //penalize if haven't been used for this user_graph yet
    {
      bool in_use = false;
      for(reqnit = req->nodes.begin(); reqnit != req->nodes.end(); ++reqnit)
	{
	  if ((*reqnit)->marked && (*reqnit)->in == cin)
	    {
	      in_use = true;
	      break;
	    }
	}
      if (!in_use) 
	{
	  cluster_cost += USER_UNUSED_CLUSTER_COST;
	  cout << " not mapped to this reservation,";
	}
    }

  //penalize if vm and couldn't find a completely free node on this cluster
  subnet_info_ptr subnet(new subnet_info());
  if (node->type == "vm" && !n->user_nodes.empty()) 
    {    
      get_subnet(subnet, node, 0);
      int num_vms = num_in_subnet(subnet, n->user_nodes);
      cluster_cost += (VM_COST * num_vms);
    }

  //if the node is a vgige, we need to check if either an edge or vgige in it's subnet is already allocated to this cluster
  if (node->type == "vgige")
    {
      get_subnet(subnet, node, 0);
      if (subnet_mapped(subnet, cin))
	{
	  cout << " subnet link or vgige already mapped to this cluster" << endl;
	  return -1;
	}
    }

  //look through all the edges with a mapped node and make sure there is a feasible path
  //first set testbed links' potential loads equal to their actual loads
  initialize_base_potential_loads(base);

  node_resource_ptr source;
  node_resource_ptr sink;

  //check to see if we can find feasible paths from this cluster to nodes we're connected to that are already mapped
  int path_costs = 0;
  if (node->is_parent) //for an hwcluster we need to look at the child nodes
    {
      std::list<node_resource_ptr> used;
      std::list<node_resource_ptr>::iterator realnit;
      for(hwclnit = node->node_children.begin(); hwclnit != node->node_children.end(); ++hwclnit)
	{
	  path_costs = -1;
	  for (realnit = n->node_children.begin(); realnit != n->node_children.end(); ++realnit)
	    {
	      if (((*realnit)->type == (*hwclnit)->type) && (!in_list((*realnit), used)))
		{
		  path_costs = compute_path_costs((*hwclnit), (*realnit));
		  if (path_costs < 0) return -1;
		  else cluster_cost += path_costs;
		  used.push_back(*realnit);
		  break;
		}
	    }
	  if (path_costs < 0)
	    {
	      cout << " problem mapping hwcluster " << node->label << " child " << (*hwclnit)->label << endl;
	      return -1;
	    }
	}
    }
  else
    {
      path_costs = compute_path_costs(node, n);
      if (path_costs < 0) return -1;
      else cluster_cost += path_costs;
    }
  //look at the potential cost of the neighbor nodes we can't map to this cluster  
  //order unmapped leaf neighbors by cost try and map highest cost first
  node_resource_ptr neighbor;
  std::list<node_load_ptr> lneighbors;
  // std::list<node_resource_ptr> nodes_to_process;
  //std::list<int> neighbor_cost;
  std::list<node_load_ptr>::iterator lnit;
  //std::list<int>::iterator ncit;

  get_lneighbors(node, lneighbors); //returns ordered list of all leaf neighbors ordered by cost
  node_load_ptr nlnode(new node_load_resource());
  nlnode->node = node;
  nlnode->load = 0;

  //mapping_cluster_ptr mcluster(new mapping_cluster_resource());
  //mcluster->cluster = cluster;
  //mcluster->used = false;
  reset_cluster(mcluster);
  mcluster->rnodes_used.push_back(n);
  mcluster->nodes_used.push_back(nlnode);

  
  
  //go through ordered list starting with highest cost, unmapped, leaf neighbor
  std::list<node_load_ptr> unmapped_nodes;
  std::list<node_load_ptr> mapped_nodes;
  node_resource_ptr available_node;
  //nodes_used.clear();
  //nodes_used.push_back(n);
  //ncit = neighbor_cost.begin();

  int total_load = find_neighbor_mapping(mcluster, unmapped_nodes, node, lneighbors);
  int unmapped_cost = 0;

  //add cost of unmapped nodes to cluster cost
  for (lnit = unmapped_nodes.begin(); lnit != unmapped_nodes.end(); ++lnit)
    {
      unmapped_cost += ((*lnit)->node->cost);
    }

  cout << " unmapped cost " << unmapped_cost << ",";
  cluster_cost += unmapped_cost;

  //add a cost for the available nodes left on the cluster this will favor a mapping 
  //that gives a higher utilization of a cluster. cost equal number of available nodes left
  //after mapping cluster divided by 2
  //  int cluster_available = get_available_cluster_size(cluster);
  //cluster_available = (cluster_available - (lneighbors.size() - unmapped_nodes.size()))/2;
  //cluster_cost += cluster_available;
  //cout << " cluster utilization cost " << cluster_available << ",";

  //the rest of the computation only applies for vgiges that have some unmapped nodes
  if (node->type != "vgige" || (unmapped_nodes.empty() && total_load > 0)) 
    {
      //clock_t etime = clock();
      //if it's a vgige check if all subnet leaves have been mapped to the same node (this could happen with vms)
      //if so move the vgige to the node the leaves are on
      if (node->type == "vgige")
	{
	  node_resource_ptr subnet_node = one_node_subnet(subnet, mcluster);
	  if (subnet_node)
	    {
	      //change the assigned node to the subnet host rather than the infrastructure node
	      mcluster->rnodes_used.pop_front();
	      mcluster->rnodes_used.push_front(subnet_node);
	    }
	}
      print_diff("onldb::compute_mapping_costs", stime);
      return cluster_cost;
    }

  //if we can't map any nodes or the total_load is still too big reject this cluster
  if (node->type == "vgige" && (mcluster->nodes_used.size() <= 1 || total_load > MAX_INTERCLUSTER_CAPACITY))
    {
      cout << "compute_mapping_cost reject cluster " << mcluster->cluster->in << " node:" << node->label;
      if (total_load > MAX_INTERCLUSTER_CAPACITY)
	cout << " we've already mapped too many neighbors to this cluster and we can't map the whole set of neighbors" << endl;
      else
	cout << " can't map any neighbors on cluster" << endl;
      return -1;
    }


  //see if there is a set of clusters that we can map the split switch to 
  //calculate the cost of splitting this switch 
  if (node->type == "vgige" && unmapped_nodes.size() > 0)
    {
      int split_cost = 0;
      if (unmapped_nodes.size() == 1) split_cost = CANT_SPLIT_VGIGE_COST;
      else
	{
	  //jp - 1/19/15 fixes trying to split vgige that are too large that would have a link that was larger than MAX_INTERCLUSTER_CAP 
	  // in either direction
	  //this should be put in split_vgige?
	  int unmapped_load = 0;
	  std::list<node_load_ptr>::iterator um_nlit;
	  for (um_nlit = unmapped_nodes.begin(); um_nlit != unmapped_nodes.end(); ++um_nlit)
	    {
	      unmapped_load += (*um_nlit)->load;
	    }
	  if (unmapped_load > MAX_INTERCLUSTER_CAPACITY) //reject the cluster if it result in one side having a larger load than the max intercluster bandwidth
	    {
	      cout << "vgige split is infeasible the load is larger than max intercluster bandwidth" << endl;
	      return -1;
	    }
	  //see if there is a set of clusters that we can map the split switch to 
	  std::list<node_resource_ptr> vgige_nodes;
	  link_resource vgige_lnk;
	  vgige_lnk.node1 = node;
	  vgige_lnk.node2 = node;
	  vgige_lnk.rload = 0;
	  vgige_lnk.lload = 0;
	  
	  std::list<mapping_cluster_ptr> vgige_clusters;
	  
	  //make a list of clusters available for mapping splits to
	  for (clusterit = clusters.begin(); clusterit != clusters.end(); ++clusterit)
	    {
	      if ((*clusterit) != mcluster && 
		  !subnet_mapped(subnet, (*clusterit)->cluster->in))
		{
		  mapping_cluster_ptr new_vcluster(new mapping_cluster_resource());
		  copy_cluster(*clusterit, new_vcluster);
		  //new_vcluster->cluster = (*clusterit);
		  //new_vcluster->load = 0;
		  //new_vcluster->used = false;
		  //reset_cluster(new_vcluster);
		  vgige_clusters.push_back(new_vcluster);
		}
	    }
	  
	  split_cost = split_vgige(vgige_clusters, unmapped_nodes, node, mcluster->cluster, base);
	  split_cost += UNUSED_CLUSTER_COST;
	}
      cout << " split cost " << split_cost << ",";
      cluster_cost += split_cost;
    }
    


  //clock_t etime = clock();
  print_diff("onldb::compute_mapping_costs", stime);
  return cluster_cost;
}

int 
onldb::find_neighbor_mapping(mapping_cluster_ptr cluster, std::list<node_load_ptr>& unmapped_nodes, node_resource_ptr root_node)
{
  std::list<node_load_ptr> neighbors;
  get_lneighbors(root_node, neighbors);
  return (find_neighbor_mapping(cluster, unmapped_nodes, root_node, neighbors));
}

int 
onldb::find_neighbor_mapping(mapping_cluster_ptr cluster, std::list<node_load_ptr>& unmapped_nodes, node_resource_ptr root_node, std::list<node_load_ptr>& neighbors)
{
  //look at each unmapped node and see if you can map to this cluster up to a max load
  
  std::list<node_load_ptr>::iterator nit;
  std::list<node_load_ptr>::reverse_iterator rev_nit;
  std::list<node_load_ptr>::iterator clnit;
  
  
  
  //go through ordered list starting with highest cost, unmapped, leaf neighbor
  node_resource_ptr available_node;
  node_resource_ptr null_ptr;
  //nodes_used.clear();
  //nodes_used.push_back(n);
  //ncit = neighbor_cost.begin();
  int total_load = 0;
  for (nit = neighbors.begin(); nit != neighbors.end(); ++nit)
    {
      bool there = false;
      //check if node was marked in a previous mapping
      if ((*nit)->node->marked)
	{
	  if ((*nit)->node->mapped_node->in == cluster->cluster->in) 
	    total_load += (*nit)->load;
	  
	  there = true;
	}
      else 
	{ 
	  //check if in the nodes_used
	  for (clnit = cluster->nodes_used.begin(); clnit != cluster->nodes_used.end(); ++clnit)
	    {
	      if ((*clnit)->node == (*nit)->node)
		{
		  there = true;
		  total_load += (*nit)->load;
		  break;
		}
	    }
	}
      
      if (!there)
	{
	  //if this is a whole gige or the total_load is less than the max try and map the node
	  if (!root_node->is_split || ((total_load+(*nit)->load) <= MAX_INTERCLUSTER_CAPACITY))
	    {
	      if ((*nit)->node->fixed)
		{
		  if (cluster->cluster->in == (*nit)->node->in)
		    available_node = find_fixed_node(cluster, (*nit)->node);
		  else
		    available_node = null_ptr;
		}
	      else
		available_node = find_available_node(cluster, (*nit)->node);
	      if (available_node)
		{
		  if ((*nit)->node->type == "vm")
		    {
		      available_node->potential_corecap -= (*nit)->node->core_capacity;
		      available_node->potential_memcap -= (*nit)->node->mem_capacity;
		    }
		  else available_node->potential_corecap = 0;
		  //JP: PROBLEM need to check if there is a feasible path to any neighbor of the neighbor that has already been mapped
		  if (compute_path_costs((*nit)->node, available_node) < 0)//couldn't reach some neighbor of the neighbor from this cluster
		    {
		      unmapped_nodes.push_back(*nit);
		      continue;
		    }//END FIX
		  cluster->rnodes_used.push_back(available_node);
		  cluster->nodes_used.push_back(*nit);
		  total_load += (*nit)->load;
		}
	      else
		unmapped_nodes.push_back(*nit);
	    }
	  else
	    unmapped_nodes.push_back(*nit);
	}
    }

  if (root_node->type == "vgige" && unmapped_nodes.size() > 0 && total_load > MAX_INTERCLUSTER_CAPACITY)
    {
      std::list<node_load_ptr>::iterator umnit;
      for (rev_nit = cluster->nodes_used.rbegin(); rev_nit != cluster->nodes_used.rend(); ++rev_nit)
	{
	  total_load -= (*rev_nit)->load;
	  bool added = false;
	  for (umnit = unmapped_nodes.begin(); umnit != unmapped_nodes.end(); ++umnit)
	    {
	      if ((*rev_nit)->node->cost >= (*umnit)->node->cost)
		{
		  added = true;
		  unmapped_nodes.insert(umnit, (*rev_nit));
		  break;
		}
	    }
	  if (!added) unmapped_nodes.push_back((*rev_nit));
	  
	  cluster->nodes_used.remove(*rev_nit);
	  cluster->rnodes_used.back()->potential_corecap += (*rev_nit)->node->core_capacity;
	  cluster->rnodes_used.back()->potential_memcap += (*rev_nit)->node->mem_capacity; 
	  cluster->rnodes_used.pop_back();
	  if (total_load <= MAX_INTERCLUSTER_CAPACITY) break;
	}
    }
  return total_load;
}


//returns a split cost equal to the cost of the path to each potential new vgige + ((#nodes unmapped to a split switch) * penalty)
int
onldb::split_vgige(std::list<mapping_cluster_ptr>& clusters, std::list<node_load_ptr>& unmapped_nodes, node_resource_ptr root_vgige, node_resource_ptr root_rnode, topology* base)
{
  bool fail = false;
  int potential_cost = 0;
  std::list<node_load_ptr> unodes;
  std::list<mapping_cluster_ptr>::iterator clusterit;
  std::list<node_resource_ptr> rnodes_used;
  std::list<node_resource_ptr>::iterator nit;
  std::list<node_load_ptr>::iterator nlit;
  std::list<link_resource_ptr>::iterator lit;
  node_resource_ptr avail_node;   
  link_resource_ptr vgige_lnk(new link_resource());
  vgige_lnk->node1 = root_vgige;
  vgige_lnk->node2 = root_vgige;
  vgige_lnk->rload = 0;
  vgige_lnk->lload = 0;
  for (lit = root_vgige->links.begin(); lit != root_vgige->links.end(); ++lit)
    {
      if ((*lit)->node1 == root_vgige && (in_list((*lit)->node2, unmapped_nodes)))
	{
	  vgige_lnk->rload += (*lit)->lload;
	}
      else if ((*lit)->node2 == root_vgige && (in_list((*lit)->node1, unmapped_nodes)))
	{
	  vgige_lnk->rload += (*lit)->rload;
	}
    }
  if (vgige_lnk->rload > MAX_INTERCLUSTER_CAPACITY) vgige_lnk->rload = MAX_INTERCLUSTER_CAPACITY;
  //subnet_info_ptr subnet(new subnet_info());
  //get_subnet(root_vgige, subnet, 0);
  //fill unodes
  unodes.assign(unmapped_nodes.begin(), unmapped_nodes.end());
  while (!(unodes.empty() || fail))
    {
      fail = true;
      mapping_cluster_ptr best_cluster;
      int best_pcost = 0;
      //find the cluster which can support the most of the unmapped nodes
      for (clusterit = clusters.begin(); clusterit != clusters.end(); ++clusterit)
	{
	  if (!(*clusterit)->used)
	    {
	      //rnodes_used.clear();
	      for (nlit = unodes.begin(); nlit != unodes.end(); ++nlit)
		{
		  if (((*clusterit)->load + (*nlit)->load) > MAX_INTERCLUSTER_CAPACITY)
		    break;
		  if ((*nlit)->node->fixed)
		    avail_node = find_fixed_node((*clusterit), (*nlit)->node);
		  else
		    avail_node = find_available_node((*clusterit), (*nlit)->node);
		  if (avail_node)
		    {
		      (*clusterit)->nodes_used.push_back((*nlit));
		      (*clusterit)->rnodes_used.push_back(avail_node);
		      (*clusterit)->load += (*nlit)->load;
		      fail = false;
		    }
		}
	    }
	}
      if (fail) break;
      for (clusterit = clusters.begin(); clusterit != clusters.end(); ++clusterit)
	{
	  if (!(*clusterit)->used)
	    {
	      if ((!best_cluster && (*clusterit)->load > 0) || (best_cluster && ((*clusterit)->load > best_cluster->load)))
		{
		  vgige_lnk->lload = (*clusterit)->load;
		  vgige_lnk->cost = calculate_edge_cost(vgige_lnk->rload, vgige_lnk->lload);
		  link_resource_ptr potential_path(new link_resource());
		  potential_path->node1 = root_rnode;
		  potential_path->node1_port = -1;
		  potential_path->node2 = (*clusterit)->cluster;
		  potential_path->node2_port = -1;
		  initialize_base_potential_loads(base);//potential problem if potential loads are used by caller
		  int pcost = find_cheapest_path(vgige_lnk, potential_path); 
		  if (pcost >= 0) 
		    {
		      if (best_cluster) 
			{
			  best_cluster->nodes_used.clear();
			  (best_cluster)->load = 0;
			}
		      best_cluster = (*clusterit);
		      best_pcost = pcost;
		    }
		}
	      else
		{
		  (*clusterit)->nodes_used.clear();
		  (*clusterit)->load = 0;
		}
	    }
	}
      
      if (best_cluster)
	{
	  //mark cluster used
	  best_cluster->used = true;
	  //remove mapped nodes from unmapped list
	  for (nlit = best_cluster->nodes_used.begin(); nlit != best_cluster->nodes_used.end(); ++nlit)
	    {
	      unodes.remove((*nlit));
	    }
	  potential_cost += best_pcost;
	  if (unodes.size() <= 1) break;
	}
      else
	{
	  fail = true;
	  break;
	}
    }
  potential_cost += (CANT_SPLIT_VGIGE_COST * unodes.size());
  //if (fail) return -1;
  //else 
  return potential_cost;
}



//return lneighbors as a cost ordered list of unmapped leaf neighbors of node
void
onldb::get_unmapped_lneighbors(node_resource_ptr node, std::list<node_load_ptr>& lneighbors) 
{
  std::list<node_resource_ptr> nodes_to_process;
  //std::list<int> neighbor_cost;
  std::list<node_load_ptr>::iterator lnit;
  std::list<node_resource_ptr>::iterator nit;
  std::list<link_resource_ptr>::iterator lit;
  node_resource_ptr neighbor;

  if (node->is_parent)
    nodes_to_process.assign(node->node_children.begin(), node->node_children.end());
  else nodes_to_process.push_back(node);
  //create a list of ordered unmapped leaf neighbors
  for (nit = nodes_to_process.begin(); nit != nodes_to_process.end(); ++nit)
    {
      for (lit = (*nit)->links.begin(); lit != (*nit)->links.end(); ++lit)
	{
	  bool added = false;
	  if ((*lit)->node1 == (*nit)) neighbor = (*lit)->node2;
	  else neighbor = (*lit)->node1;
	  //if this is not a leaf node ignore or already mapped
	  if (neighbor->type == "vgige" || neighbor->marked || (*nit) == neighbor) continue;
	  added = false;
	  if (neighbor->parent) //if this is the child of a hwcluster add the cluster to the list not the child
	    {
	      neighbor = neighbor->parent;
	    }
	  node_load_ptr nlptr = get_element(neighbor, lneighbors);
	  if (nlptr != NULL) //we've already seen this node but we need to add the load to the total load for the node
	    {
	      nlptr->load += (*lit)->capacity;
	      continue;
	    }

	  node_load_ptr new_nlptr(new node_load_resource());
	  new_nlptr->node = neighbor;
	  new_nlptr->load = (*lit)->capacity;

	  for (lnit = lneighbors.begin(); lnit != lneighbors.end(); ++lnit)
	    {
	      if (neighbor->cost >= (*lnit)->node->cost)
		{
		  lneighbors.insert(lnit, new_nlptr);
		  added = true;
		  break;
		}
	    }
	  if (!added && (neighbor != (*nit))) 
	    {
	      lneighbors.push_back(new_nlptr);
	    }
	}
    }
}



//return lneighbors as a cost ordered list of unmapped leaf neighbors of node
void
onldb::get_lneighbors(node_resource_ptr node, std::list<node_load_ptr>& lneighbors) 
{
  std::list<node_resource_ptr> nodes_to_process;
  //std::list<int> neighbor_cost;
  std::list<node_load_ptr>::iterator lnit;
  std::list<node_resource_ptr>::iterator nit;
  std::list<link_resource_ptr>::iterator lit;
  node_resource_ptr neighbor;

  if (node->is_parent)
    nodes_to_process.assign(node->node_children.begin(), node->node_children.end());
  else nodes_to_process.push_back(node);
  //create a list of ordered unmapped leaf neighbors
  for (nit = nodes_to_process.begin(); nit != nodes_to_process.end(); ++nit)
    {
      for (lit = (*nit)->links.begin(); lit != (*nit)->links.end(); ++lit)
	{
	  bool added = false;
	  if ((*lit)->node1 == (*nit)) neighbor = (*lit)->node2;
	  else neighbor = (*lit)->node1;
	  //if this is not a leaf node or it's a loop ignore 
	  if (neighbor->type == "vgige" || (*nit) == neighbor) continue;
	  added = false;
	  if (neighbor->parent) //if this is the child of a hwcluster add the cluster to the list not the child
	    {
	      neighbor = neighbor->parent;
	    }
	  node_load_ptr nlptr = get_element(neighbor, lneighbors);
	  if (nlptr != NULL) //we've already seen this node but we need to add the load to the total load for the node
	    {
	      nlptr->load += (*lit)->capacity;
	      continue;
	    }

	  node_load_ptr new_nlptr(new node_load_resource());
	  new_nlptr->node = neighbor;
	  new_nlptr->load = (*lit)->capacity;

	  for (lnit = lneighbors.begin(); lnit != lneighbors.end(); ++lnit)
	    {
	      if (neighbor->cost >= (*lnit)->node->cost)
		{
		  lneighbors.insert(lnit, new_nlptr);
		  added = true;
		  //neighbor_cost.insert(ncit, (*lit)->cost);
		  break;
		}
	    }
	  if (!added && (neighbor != (*nit))) 
	    {
	      lneighbors.push_back(new_nlptr);
	    }
	}
    }
}

int
onldb::compute_path_costs(node_resource_ptr node, node_resource_ptr n) throw()
{
  struct timeval stime;
  gettimeofday(&stime, NULL);
  std::list<link_resource_ptr>::iterator lit;
  std::list<link_resource_ptr> mapped_edges;
  node_resource_ptr source;
  node_resource_ptr sink;

  int rtn_cost = 0;
  for(lit = node->links.begin(); lit != node->links.end(); ++lit) //for each req link involving the node find the cheapest path to any already mapped node
    {
      if (((*lit)->node1 == node && !(*lit)->node2->marked) ||
	  ((*lit)->node2 == node && !(*lit)->node1->marked) ||
	  ((*lit)->marked))
	continue;
      if ((*lit)->node1 == node)
	{
	  source = n;
	  sink = (*lit)->node2->mapped_node;
	}
      else
	{
	  source = (*lit)->node1->mapped_node;
	  sink = n;
	}
      // std::list<link_resource_ptr> potential_path;
      link_resource_ptr potential_path(new link_resource());
      potential_path->node1 = source;
      if ((*lit)->node1->type == "vgige") potential_path->node1_port = -1;
      else potential_path->node1_port = (*lit)->node1_port;
      potential_path->node2 = sink;
      if ((*lit)->node2->type == "vgige") potential_path->node2_port = -1;
      else potential_path->node2_port = (*lit)->node2_port;
      int pe_cost = find_cheapest_path(*lit, potential_path, mapped_edges);
      if (pe_cost < 0)
	{
	  cout << "compute_path_cost node:" << node->label << " failed to find a path for link " << (*lit)->label << " mapped nodes:(" << source->label << "," << sink->label << ")" << endl;
	  return -1;
	}
      else //valid path was found
	{
	  rtn_cost += pe_cost;
	  std::list<link_resource_ptr>::iterator plit;
	  int l_rload = (*lit)->rload;
	  if (l_rload > MAX_INTERCLUSTER_CAPACITY) l_rload =  MAX_INTERCLUSTER_CAPACITY;
	  int l_lload = (*lit)->lload;
	  if (l_lload > MAX_INTERCLUSTER_CAPACITY) l_lload =  MAX_INTERCLUSTER_CAPACITY;
	  
	  //update potential loads on intercluster links
	  node_resource_ptr last_visited = source;
	  node_resource_ptr other_node;
	  bool port_matters = true;
	  for (plit = potential_path->mapped_path.begin(); plit != potential_path->mapped_path.end(); ++plit)
	    {
	      if (((*plit)->node1->type_type == "infrastructure" && (*plit)->node2->type_type == "infrastructure") && ((*plit)->node1 != (*plit)->node2))
		{
		  if (last_visited->type_type == "infrastructure") port_matters = false;
		  if (((*plit)->node1 == last_visited) && (!port_matters || (*lit)->node1_port == (*plit)->node1_port))
		    {
		      (*plit)->potential_rcap -= l_rload;
		      (*plit)->potential_lcap -= l_lload;
		      last_visited = (*plit)->node2;
		    }
		  else if (((*plit)->node2 == last_visited) && (!port_matters || (*lit)->node1_port == (*plit)->node2_port))
		    {
		      (*plit)->potential_rcap -= l_lload;
		      (*plit)->potential_lcap -= l_rload;
		      last_visited = (*plit)->node1;
		    }
		}
	    }
	  mapped_edges.push_back(potential_path);
	}
    }
  print_diff("onldb::compute_path_costs", stime);
  return rtn_cost;
}


//int
//onldb::find_cheapest_path(link_resource_ptr ulink, link_resource_ptr potential_path) throw()
//{
//subnet_info_ptr(new subnet_info());
//get_subnet(ulink->node1, subnet);
//return (find_cheapest_Path(ulink, potential_path, subnet);
//}


int
onldb::find_cheapest_path(link_resource_ptr ulink, link_resource_ptr potential_path) throw()
{
  std::list<link_resource_ptr> potential_mappings;
  return (find_cheapest_path(ulink, potential_path, potential_mappings));
}

int
onldb::find_cheapest_path(link_resource_ptr ulink, link_resource_ptr potential_path, std::list<link_resource_ptr>& potential_mappings) throw()
{
  ++fnd_path;

  int num_paths = 0;
  std::list<node_resource_ptr> nodes_to_search;
  std::list<node_resource_ptr> nodes_seen;
  std::list<link_path_ptr> paths;
  std::list<node_resource_ptr>::iterator nsearch_it;
  std::list<link_path_ptr>::iterator pathit; 
  std::list<link_resource_ptr>::iterator lit;
  bool is_right = true;

  subnet_info_ptr subnet(new subnet_info());
  get_subnet(subnet, ulink->node1, ulink->node1_port);

  node_resource_ptr source = potential_path->node1;
  int src_port = potential_path->node1_port;
  node_resource_ptr sink = potential_path->node2;
  int sink_port = potential_path->node2_port;

  //if these are 2 vms or a vm and vgige allocated to the same node then their links will be handled internally 
  //within the node and there is no need to allocate a path out of the node
  if (source == sink && 
      ((ulink->node1->type == "vm" && ulink->node2->type == "vm") ||
       (ulink->node1->type == "vm" && ulink->node2->type == "vgige") ||
       (ulink->node1->type == "vgige" && ulink->node2->type == "vm")))
    {
      potential_path->cost = 0;
      return (0);
    }

  int rload = ulink->rload;
  int lload = ulink->lload;
  if (rload > MAX_INTERCLUSTER_CAPACITY) rload =  MAX_INTERCLUSTER_CAPACITY;
  if (lload > MAX_INTERCLUSTER_CAPACITY) lload =  MAX_INTERCLUSTER_CAPACITY;

  link_path_ptr sink_path;
  nodes_to_search.push_back(source);

  link_path_ptr start_path_ptr(new link_path());
  start_path_ptr->sink = source;
  start_path_ptr->sink_port = src_port;
  start_path_ptr->cost = 0;
  
  paths.push_back(start_path_ptr);

  while (!sink_path && !paths.empty())
    {
      int max = paths.size();//what paths are at this level of this search because we're going to add new ones to the back
      for (int i = 0; i < max; ++i)//cycle through the current set of paths at this level of the search
	{
	  link_path_ptr path_ptr = paths.front();
	  paths.pop_front();
	  node_resource_ptr node1_ptr = path_ptr->sink;
	  int node1_port = path_ptr->sink_port;

	  //look at all outgoing links from this node
	  for (lit = node1_ptr->links.begin(); lit != node1_ptr->links.end(); ++lit)
	    {
	      node_resource_ptr node2_ptr;
	      int node2_port = -1;

	      //first find the other endpoint
	      if ((*lit)->node1 == node1_ptr && 
		  (node1_ptr != source || 
		   (node1_port < 0 || node1_port == (int)(*lit)->node1_port || node1_ptr->has_vport)))
		{
		  node2_ptr = (*lit)->node2;
		  if (node2_ptr->type_type != "infrastructure")
		    node2_port = (*lit)->node2_port;
		  is_right = true;
		}
	      else if ((*lit)->node2 == node1_ptr && 
		       (node1_ptr != source || 
			(node1_port < 0 || node1_port == (*lit)->node2_port || node1_ptr->has_vport)))
		{
		  node2_ptr = (*lit)->node1;
		  if (node2_ptr->type_type != "infrastructure")
		    node2_port = (*lit)->node1_port;
		  is_right = false;
		}

	      //only consider the node if it's an infrastructure node or the sink
	      if (node2_ptr && 
		  ((!in_list(node2_ptr, nodes_seen) && node2_ptr->type_type == "infrastructure") || //node is infrastructure we've never seen before
		   (node2_ptr == sink && (node2_port == sink_port || sink->has_vport)))) //node is the sink
		{
		  //if this is an infrastructure node need to check if there's enough bandwidth 
		  //and if the link passes through a switch which already has this subnet's vgige or link mapped.
		  //we should also check capacity for virtual port links
		  
		  
		  int pcost = path_ptr->cost;
		  int lcap = 0;
		  int rcap = 0;
		  if (is_right) 
		    {
		      lcap = (*lit)->potential_lcap;
		      rcap = (*lit)->potential_rcap;
		    }
		  else
		    {
		      lcap = (*lit)->potential_rcap;
		      rcap = (*lit)->potential_lcap;
		    }
		  if (node1_ptr->type_type == "infrastructure" && node2_ptr->type_type == "infrastructure") //interswitch link
		    {
		      //check that there is enough capacity
		      if ((rcap < rload) || (lcap < lload)) //not enough capacity
			//(subnet_mapped(subnet, othernode->in, othernode))))
			continue;
		      else //check that no link or vgige is already mapped to this cluster
			{
			  if (node2_ptr == sink)
			    {
			      if (node1_ptr == source)
				{
				  if (subnet_mapped(subnet, node2_ptr->in, potential_mappings, ulink->node2) ||
				      subnet_mapped(subnet, node1_ptr->in, potential_mappings, ulink->node1)) 
				    continue;
				}
			      else
				{
				  if (subnet_mapped(subnet, node2_ptr->in, potential_mappings, ulink->node2) ||
				      subnet_mapped(subnet, node1_ptr->in, potential_mappings)) 
				    continue;
				}
			    }
			  else
			    {
			      if (node1_ptr == source)
				{
				  if (subnet_mapped(subnet, node2_ptr->in, potential_mappings) ||
				      subnet_mapped(subnet, node1_ptr->in, potential_mappings, ulink->node1))
				    continue;
				}
			      else
				{
				  if (subnet_mapped(subnet, node2_ptr->in, potential_mappings) ||
				      subnet_mapped(subnet, node1_ptr->in, potential_mappings))
				    continue;
				}
			    }
			}
		      lcap -= lload;
		      rcap -= rload;
		      pcost += (ulink->cost) + ((rcap + lcap)/2); //want to favor links that have higher utilization
		    }

		  //check available capacity for virtual ports at sink or source
		  if (node1_ptr == source && source->has_vport && rcap < ulink->node1_capacity) //virtual port at source
		    continue;
		  if (node2_ptr == sink && sink->has_vport && lcap < ulink->node2_capacity) //virtual port at sink
		    continue;

		  //is there already a path to this node? if so only consider this one if it's cheaper
		  bool seen = false;
		  for (pathit = paths.begin(); pathit != paths.end(); ++pathit)
		    {
		      if ((*pathit)->sink == node2_ptr)
			{
			  seen = true;
			  if ((*pathit)->cost > pcost)
			    {
			      (*pathit)->path.clear();
			      (*pathit)->path.assign(path_ptr->path.begin(), path_ptr->path.end());
			      (*pathit)->path.push_back(*lit);
			      (*pathit)->cost = pcost;
			      (*pathit)->sink_port = node2_port;
			      ++num_paths;
			    }
			  break;
			}
		    }
		  if (!seen) //if there isn't an entry for a path to this node yet add one
		    {
		      link_path_ptr tmp_path(new link_path());
		      tmp_path->path.assign(path_ptr->path.begin(), path_ptr->path.end());
		      tmp_path->path.push_back(*lit);
		      ++num_paths;
		      tmp_path->sink = node2_ptr;
		      tmp_path->sink_port = node2_port; 
		      tmp_path->cost = pcost;   
		      paths.push_back(tmp_path);
		    }
		}
	    }
	}
      //now cycle through the paths left and note if the sink has been found or any new nodes have been seen
      for (pathit = paths.begin(); pathit != paths.end(); ++pathit)
	{
	  if ((*pathit)->sink == sink) 
	    sink_path = (*pathit);
	  else
	    {
	      if (!in_list((*pathit)->sink, nodes_seen))
		nodes_seen.push_back((*pathit)->sink);
	    }
	}
    }

  cout << "onldb::find_cheapest_path path_addition_count:" << num_paths <<  endl;
  //if we found a sink path put it into the potential path and return the cost otw return -1
  if (sink_path)
    {
      potential_path->cost = sink_path->cost;
      while (!sink_path->path.empty())
	{ 
	  potential_path->mapped_path.push_back(sink_path->path.front());
	  sink_path->path.pop_front();
	}
      return (potential_path->cost);
    }
  return -1;
}

node_resource_ptr
onldb::map_node(node_resource_ptr node, topology* req, mapping_cluster_ptr cluster, std::list<mapping_cluster_ptr>& clusters, topology* base) throw()
{
  std::list<node_resource_ptr>::iterator nit;
  std::list<link_resource_ptr>::iterator lit;

  node_resource_ptr rnode;
  node_resource_ptr nullnode;
  
  //if (node->marked) return nullnode; //it's been map_node was already called
  //node->marked = true;
  if (node->marked) 
    {
      rnode = node->mapped_node;
    }
  else 
    {
      rnode = cluster->rnodes_used.front();
      // std::list<node_resource_ptr> nodes_used;
      //if (node->fixed)
      //rnode = find_fixed_node(cluster, node->node, nodes_used);
      //else
      //rnode = find_available_node(cluster, node->type, nodes_used);
      node->marked = true;
      node->mapped_node = rnode;
      node->in = cluster->cluster->in;
      rnode->marked = true;
      rnode->core_capacity -= node->core_capacity;
      rnode->mem_capacity -= node->mem_capacity;
      rnode->user_nodes.push_back(node);
      if (node->is_parent) map_children(node, rnode);
    }

  cout << "map_node request node (" << node->type << node->label << "," << node->user_nodes.front()->label << ") real node (" << rnode->type << rnode->label << ", " << rnode->node << ")" << endl;
  if (node->is_parent)
    {
      for(nit = node->node_children.begin(); nit != node->node_children.end(); ++nit)
	{
	  if ((*nit)->marked) map_edges((*nit), (*nit)->mapped_node, base);
	  else 
	    {
	      cout << "Error: map_node child node " << (*nit)->label << " not mapped " << endl; 
	      return nullnode;
	    }
	}
    }
  else
    map_edges(node, rnode, base);


  // mapping_cluster_ptr mcluster(new mapping_cluster_resource());
  std::list<node_load_ptr> unmapped_nodes;
  //get_lneighbors(node, unmapped_nodes);
  //mcluster->cluster = cluster;
  //mcluster->rnodes_used.push_back(rnode);
  //node_load_ptr nlnode(new node_load_resource());
  //nlnode->node = node;
  //mcluster->nodes_used.push_back(nlnode);
  int total_load = find_neighbor_mapping(cluster, unmapped_nodes, node);
 
 //now mark the mappable neighbors
  std::list<node_load_ptr>::iterator nbrit;
  std::list<node_load_ptr>::iterator unmapit;
  std::list<node_resource_ptr>::iterator nrnode_it = cluster->rnodes_used.begin();
  for (nbrit = cluster->nodes_used.begin(); nbrit != cluster->nodes_used.end(); ++nbrit)
    {
      if ((*nbrit)->node != node)
	{
	  (*nbrit)->node->marked = true;
	  (*nbrit)->node->mapped_node = (*nrnode_it);
	  (*nbrit)->node->in = cluster->cluster->in;
	  (*nrnode_it)->marked = true;
	  (*nrnode_it)->core_capacity -= (*nbrit)->node->core_capacity;
	  (*nrnode_it)->mem_capacity -= (*nbrit)->node->mem_capacity;
	  (*nrnode_it)->user_nodes.push_back((*nbrit)->node);
	  if ((*nbrit)->node->is_parent) map_children((*nbrit)->node, (*nrnode_it));
	}
      cout << "     side effect: request node (" << (*nbrit)->node->type << (*nbrit)->node->label << "," << (*nbrit)->node->user_nodes.front()->label << ") real node (" << (*nrnode_it)->type << (*nrnode_it)->label << ", " << (*nrnode_it)->node << ")" << endl;
      ++nrnode_it;
      for (unmapit = unmapped_nodes.begin(); unmapit != unmapped_nodes.end(); ++unmapit)
	{
	  if ((*unmapit)->node == (*nbrit)->node)
	    {
	      unmapped_nodes.remove(*unmapit);
	      break;
	    }
	}
    }

  if (node->is_parent)
    {
      for(nit = node->node_children.begin(); nit != node->node_children.end(); ++nit)
	{
	  if ((*nit)->marked) 
	    {
	      map_edges((*nit), (*nit)->mapped_node, base);
	      cout << "     side effect: request node (" << (*nit)->type << (*nit)->label << "," << (*nit)->user_nodes.front()->label << ") real node (" << (*nit)->mapped_node->type << (*nit)->mapped_node->label << ", " << (*nit)->mapped_node->node << ")" << endl;
	    }
	  else 
	    {
	      cout << "Error: map_node child node " << (*nit)->label << " not mapped " << endl; 
	      return nullnode;
	    }
	}
    }
  else
    map_edges(node, rnode, base);//call map_edges again since may be more edges now that neighbors have been mapped
  
  //if this is not a vgige just return 
  if (node->type != "vgige") return nullnode;

  //STOPPED HERE 6/11/13 what is the right thing to insert one large vgige or a split
  //if this is a vswitch and we have more than 1 unmapped leaf split the node and insert a new node into the graph. return the new node
  //need to make sure the unmapped nodes do not contribute a load that exceeds the bandwidth of the interswitch edge
  if (unmapped_nodes.size() > 1)
    {

      subnet_info_ptr subnet(new subnet_info());
      get_subnet(subnet, node, 0);
      std::list<mapping_cluster_ptr> vgige_clusters;
      std::list<mapping_cluster_ptr>::iterator clusterit;
  
      //make a list of clusters available for mapping splits to
      for (clusterit = clusters.begin(); clusterit != clusters.end(); ++clusterit)
	{
	  if ((*clusterit) != cluster && 
	      !subnet_mapped(subnet, (*clusterit)->cluster->in))
	    {
	      //mapping_cluster_ptr new_vcluster(new mapping_cluster_resource());
	      //new_vcluster->cluster = (*clusterit);
	      //new_vcluster->load = 0;
	      //new_vcluster->used = false;
	      reset_cluster(*clusterit);
	      vgige_clusters.push_back(*clusterit);
	    }
	}
      
      //do split to see if I can map any nodes to a separate vgige
      split_vgige(vgige_clusters, unmapped_nodes, node, rnode, base);
      std::list<mapping_cluster_ptr>::iterator vclusterit;
      bool dosplit = false;
      for (vclusterit = vgige_clusters.begin(); vclusterit != vgige_clusters.end(); ++vclusterit)
	{
	  if ((*vclusterit)->used)
	    {
	      dosplit = true;
	      break;
	    }
	} 

      if (!dosplit) return nullnode;

      //if a split is possible just return one big vgige to be split later
      std::list<node_resource_ptr> vgige_nodes;
      link_resource_ptr vgige_lnk(new link_resource());
      vgige_lnk->node1 = node;
      vgige_lnk->node2 = node;
      vgige_lnk->rload = 0;
      vgige_lnk->lload = 0;
      vgige_lnk->capacity = MAX_INTERCLUSTER_CAPACITY;
      //bool can_split = false;

      //first calculate the loads for the new link created between the split vgige parts
      for (lit = node->links.begin(); lit != node->links.end(); ++lit)
	{
	  if ((*lit)->node1 == node)
	    {
	      if (in_list((*lit)->node2, unmapped_nodes)) 
		{
		  vgige_lnk->lload += (*lit)->lload;
		  if ((*lit)->node2->type != "vgige") vgige_nodes.push_back((*lit)->node2);
		}
	      else vgige_lnk->rload += (*lit)->lload;
	    }
	  else
	    {
	      if (in_list((*lit)->node1, unmapped_nodes))
		{
		  vgige_lnk->lload += (*lit)->rload;
		  if ((*lit)->node1->type != "vgige") vgige_nodes.push_back((*lit)->node1);
		}
	      else vgige_lnk->rload += (*lit)->rload;
	    }
	}
      //if (vgige_lnk->rload > MAX_INTERCLUSTER_CAPACITY) vgige_lnk->rload = MAX_INTERCLUSTER_CAPACITY;
      //if (vgige_lnk->rload > MAX_INTERCLUSTER_CAPACITY) vgige_lnk->lload = MAX_INTERCLUSTER_CAPACITY;
      /*
      //if the new link requires less than or equal the intercluster capacity, 

      if ((vgige_lnk->rload > MAX_INTERCLUSTER_CAPACITY) || (vgige_lnk->lload > MAX_INTERCLUSTER_CAPACITY)) 
	{
	  return nullnode;
	}
      */
      //return a new vgige to add to the ordered list
      int ncost = 0;//used to calculate the new node's cost and the new cost of the old node
      node_resource_ptr new_vgige = get_new_vswitch(req);
      cout << "onldb::map_node splitting vgige " << node->type << node->label << " adding new vgige " << new_vgige->type << new_vgige->label;
      new_vgige->is_split = true;
      new_vgige->user_nodes.insert(new_vgige->user_nodes.begin(), node->user_nodes.begin(), node->user_nodes.end());
      vgige_lnk->node1 = node;
      vgige_lnk->node2 = new_vgige;
      vgige_lnk->node1_port = 0;
      vgige_lnk->node2_port = 0;
      std::list<link_resource_ptr> lnk_rms;
      for (lit = node->links.begin(); lit != node->links.end(); ++lit)
	{
	  if ((*lit)->node1 == node && in_list((*lit)->node2, unmapped_nodes)) 
	    {
	      //remove link and add to new_vgige
	      new_vgige->links.push_back(*lit);
	      (*lit)->node1 = new_vgige;
	      ncost += (*lit)->cost;
	      //node->links.erase(lit);//SEGV can't erase here
	      lnk_rms.push_back(*lit);
	    }
	  else if ((*lit)->node2 == node && in_list((*lit)->node1, unmapped_nodes)) 
	    {
	      //remove link and add to new_vgige
	      new_vgige->links.push_back(*lit);
	      (*lit)->node2 = new_vgige;
	      ncost += (*lit)->cost;
	      //node->links.erase(lit);
	      lnk_rms.push_back(*lit);
	    }
	  //else ++lit;
	}
      for (lit = lnk_rms.begin(); lit != lnk_rms.end(); ++lit)
	{
	  node->links.remove(*lit);
	}
      vgige_lnk->cost = calculate_edge_cost(vgige_lnk->rload, vgige_lnk->lload);
      vgige_lnk->added = true;
      cout << " new link (" << vgige_lnk->node1->type << vgige_lnk->node1->label << "," 
	   << vgige_lnk->node2->type << vgige_lnk->node2->label << ") (rload,lload,cost)=" << vgige_lnk->rload
	   << "," << vgige_lnk->lload << "," << vgige_lnk->cost << ")";
      node->links.push_back(vgige_lnk);
      node->cost = node->cost - ncost + vgige_lnk->cost;
      new_vgige->links.push_back(vgige_lnk);
      new_vgige->cost = ncost + vgige_lnk->cost;
      req->nodes.push_back(new_vgige);
      req->links.push_back(vgige_lnk);
      return new_vgige;
    }

  return nullnode;
}

node_resource_ptr
onldb::get_new_vswitch(topology* req) throw()
{
  unsigned int newlabel = 1;
  std::list<node_resource_ptr>::iterator nit;
  bool found = false;
  while(!found)
    {
      found = true;
      for (nit = req->nodes.begin(); nit != req->nodes.end(); ++nit)
	{
	  if ((*nit)->label == newlabel)
	    {
	      ++newlabel;
	      found = false;
	      break;
	    }
	}
    }
  node_resource_ptr newnode(new node_resource());
  newnode->label = newlabel;
  newnode->type = "vgige";
  //newnode->is_mapped = false;
  newnode->marked = false;
  newnode->priority = 0;
  newnode->in = 0;
  newnode->cost = 0;
  newnode->is_split = false;
  return newnode;
}


void
onldb::map_children(node_resource_ptr unode, node_resource_ptr rnode)
{
  std::list<node_resource_ptr>::iterator ucit;
  std::list<node_resource_ptr>::iterator rcit;

  for(ucit = unode->node_children.begin(); ucit != unode->node_children.end(); ++ucit)
    {
      if ((*ucit)->marked) continue;
      for(rcit = rnode->node_children.begin(); rcit != rnode->node_children.end(); ++rcit)
	{
	  if (!(*rcit)->marked && (*rcit)->type == (*ucit)->type)
	    {
	      (*ucit)->marked = true;
	      (*ucit)->mapped_node = (*rcit);
	      (*ucit)->in = (*rcit)->in;
	      (*rcit)->marked = true;
	      (*rcit)->core_capacity -= (*ucit)->core_capacity;
	      (*rcit)->mem_capacity -= (*ucit)->mem_capacity;
	      (*rcit)->user_nodes.push_back((*ucit));
	      break;
	    }
	}
      if (!(*ucit)->marked)
	cerr << "ERROR: map_children can't map " << (*ucit)->label << endl; 
    }
}


void
onldb::map_edges(node_resource_ptr unode, node_resource_ptr rnode, topology* base) throw()
{
  //make note of link directions of mapped_path in link_resource::mp_linkdir
  std::list<link_resource_ptr>::iterator lit;
  node_resource_ptr source;
  node_resource_ptr sink;
  //subnet_info_ptr subnet(new subnet_info());
  //get_subnet(unode, subnet);
  initialize_base_potential_loads(base);
  //TO DO: need to handle hwcluster
  for(lit = unode->links.begin(); lit != unode->links.end(); ++lit)
    {
      if ((*lit)->marked) continue;
      if ((*lit)->node1 == unode)
	{
	  if (!(*lit)->node2->marked) continue;
	  source = rnode;
	  sink = (*lit)->node2->mapped_node;
	}
      else
	{
	  if (!(*lit)->node1->marked) continue;
	  sink = rnode;
	  source = (*lit)->node1->mapped_node;
	}

      //std::list<link_resource_ptr> found_path;
      link_resource_ptr found_path(new link_resource());
      found_path->node1 = source;
      if ((*lit)->node1->type == "vgige") found_path->node1_port = -1;
      else found_path->node1_port = (*lit)->node1_port;
      found_path->node2 = sink;
      if ((*lit)->node2->type == "vgige") found_path->node2_port = -1;
      else found_path->node2_port = (*lit)->node2_port;
      std::list<link_resource_ptr>::iterator fpit;
      int pcost = find_cheapest_path(*lit, found_path);
      if (pcost >= 0)
	{
	  cout << "mapping link " << to_string((*lit)->label) << ": (" << to_string((*lit)->node1->label) << "p" << to_string((*lit)->node1_port) << ", " << to_string((*lit)->node2->label) << "p" << to_string((*lit)->node2_port) << ") capacity " << to_string((*lit)->capacity) << " load(" << (*lit)->rload << "," << (*lit)->lload << ") mapping to ";
	  int l_rload = (*lit)->rload;
	  //if (l_rload > (*lit)->capacity) l_rload =  (*lit)->capacity;
	  int l_lload = (*lit)->lload;
	  //if (l_lload > (*lit)->capacity) l_lload =  (*lit)->capacity;

	  node_resource_ptr last_visited = source;
	  node_resource_ptr other_node;
	  bool port_matters = true;
	  bool source_seen = false;

	  //update link's mapped path to the found_path. update loads on links.
	  for (fpit = found_path->mapped_path.begin(); fpit != found_path->mapped_path.end(); ++fpit)
	    {
	      //TODO update node's interface bandwidth capacities.
	      (*lit)->mapped_path.push_back(*fpit);
	      if (last_visited  == (*fpit)->node1)
		{
		  (*lit)->mp_linkdir.push_back(1);//record that this is a right link in this path
		  //update load on link
		  (*fpit)->rload += l_rload;
		  (*fpit)->lload += l_lload;
		  if ((*fpit)->rload > (*fpit)->capacity)
		  {
		    (*fpit)->rload = (*fpit)->capacity;
		    (*fpit)->potential_rcap = 0;
		  }
		  else
		    (*fpit)->potential_rcap -= l_rload;
		  if ((*fpit)->lload > (*fpit)->capacity)
		  {
		    (*fpit)->lload = (*fpit)->capacity;
		    (*fpit)->potential_lcap = 0;
		  }
		  else
		    (*fpit)->potential_lcap -= l_lload;
		  //set source or sink real ports if necessary
		  if (last_visited == source && !source_seen)
		    {
		      source_seen = true;
		      (*lit)->node1_rport = (*fpit)->node1_port;
		      source->port_capacities[(*fpit)->node1_port] -= (*fpit)->rload;//update port capacities for vms and vports
		    }
		  else if ((*fpit)->node2 == sink && source_seen)
		    {
		      (*lit)->node2_rport = (*fpit)->node2_port;
		      sink->port_capacities[(*fpit)->node2_port] -= (*fpit)->lload;//update port capacities for vms and vports
		    }
		  //set last_visited
		  last_visited = (*fpit)->node2;
		  
		}
	      else 
		{
		  (*lit)->mp_linkdir.push_back(0);//record that this as a left link in this path
		  (*fpit)->rload += l_lload;
		  (*fpit)->lload += l_rload;
		  if ((*fpit)->rload > (*fpit)->capacity)
		  {
		    (*fpit)->rload = (*fpit)->capacity;
		    (*fpit)->potential_rcap = 0;
		  }
		  else
		    (*fpit)->potential_rcap -= l_lload;
		  if ((*fpit)->lload > (*fpit)->capacity)
		  {
		    (*fpit)->lload = (*fpit)->capacity;
		    (*fpit)->potential_lcap = 0;
		  }
		  else
		    (*fpit)->potential_lcap -= l_rload;
		  //set source or sink real ports if necessary
		  if (last_visited == source && !source_seen)
		    {
		      source_seen = true;
		      (*lit)->node1_rport = (*fpit)->node2_port;
		      source->port_capacities[(*fpit)->node2_port] -= (*fpit)->lload;//update port capacities for vms and vports
		    }
		  else if ((*fpit)->node1 == sink && source_seen)
		    {
		      (*lit)->node2_rport = (*fpit)->node1_port;
		      sink->port_capacities[(*fpit)->node1_port] -= (*fpit)->rload;//update port capacities for vms and vports
		    }
		  last_visited = (*fpit)->node1;
		}
	      //cout << ((*fpit)->label) << ":" << "(" << ((*fpit)->node1->label) << "," << ((*fpit)->node2->label) << ",rl:" << ((*fpit)->rload) << ",ll:" << ((*fpit)->lload) << "),";
	      cout << ((*fpit)->label) << "(cid " << (*fpit)->conns.front() << "):" << "(" << ((*fpit)->node1->node) << "," << ((*fpit)->node2->node) << ",rl:" << ((*fpit)->rload) << ",ll:" << ((*fpit)->lload) << "),";
	    }
	  (*lit)->marked = true;
	  cout << endl;
	}
    }
}


bool onldb::embed(node_resource_ptr user, node_resource_ptr testbed) throw()
{
  std::list<link_resource_ptr>::iterator tlit;
  std::list<link_resource_ptr>::iterator ulit;

  if(user->marked || testbed->marked) return false;
 
  if(user->fixed)
  {
    if(user->node != testbed->node) return false;
  }

  user->node = testbed->node;
  user->marked = true;
  testbed->marked = true;
  
  for(ulit = user->links.begin(); ulit != user->links.end(); ++ulit)
  {
    bool loopback = false;
    if((*ulit)->node1->label == (*ulit)->node2->label) { loopback = true; }

    bool user_this_is_node1 = false;
    unsigned int user_this_port = (*ulit)->node2_port;
    if(loopback)
    {
      if((*ulit)->conns.empty())
      {
        user_this_is_node1 = true;
        user_this_port = (*ulit)->node1_port;
      }
    }
    else if((*ulit)->node1->label == user->label)
    {
      user_this_is_node1 = true;
      user_this_port = (*ulit)->node1_port;
    }

    for(tlit = testbed->links.begin(); tlit != testbed->links.end(); ++tlit)
    {
      unsigned int testbed_port = (*tlit)->node2_port;
      if((*tlit)->node1->label == testbed->label)
      {
        testbed_port = (*tlit)->node1_port;
      }
      if(testbed_port == user_this_port) break;
    }
    if(tlit == testbed->links.end()) return false;

    if(user_this_is_node1)
    {
      (*ulit)->conns.push_front((*tlit)->conns.front());
    }
    else
    {
      (*ulit)->conns.push_back((*tlit)->conns.front());
    }
  }

  return true;
}

//JP change for remap so reservation set as used 3/29/2012
//onldb_resp onldb::add_reservation(topology *t, std::string user, std::string begin, std::string end) throw()
onldb_resp onldb::add_reservation(topology *t, std::string user, std::string begin, std::string end, std::string state) throw()
{
  timeval stime;
  gettimeofday(&stime, NULL);
  int ar_db_count = 0;
  //PROBLEM: the way the connections are added the rload and lload may be wrong since we don't take into account which direction on the path the connection is
  try
  {
    //define lists for multiple db inserts
    std::vector<connschedule> db_connections;
    std::vector<vswitchschedule> db_vswitches;
    std::vector<linkschedule> db_links;
    std::vector<hwclusterschedule> db_hwclusters;
    std::vector<nodeschedule> db_nodes;

    // add the reservation entry
    reservationins res(user, mysqlpp::DateTime(begin), mysqlpp::DateTime(end), state);//"pending");//JP change 3/29/2012
    mysqlpp::Query ins = onl->query();
    ins.insert(res);
    ++ar_db_count;
    mysqlpp::SimpleResult sr = ins.execute();

    unsigned int rid = sr.insert_id();

    std::list<node_resource_ptr>::iterator nit;
    std::list<link_resource_ptr>::iterator lit;
    std::list<link_resource_ptr>::iterator mplit;
    std::list<int>::iterator cit;
    std::list<int>::iterator dirit;

    unsigned int vlanid = 1;
    unsigned int linkid = 1;
    // unsigned int vmid = 0;
    
    cout << "add_reservation(" << rid << ") nodes mapped:";

    //clear marked field so can keep track of vport scheduling which should be no more than 1 per link
    for(lit = t->links.begin(); lit != t->links.end(); ++lit)
    {
      (*lit)->marked = false;
    }

    // need to give unique names to every vswitch first
    for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
    {
      if((*nit)->type == "vgige")
      {
        (*nit)->node = "vgige" + to_string(vlanid);
        (*nit)->vmid = vlanid;
        ++vlanid;
      }
      cout << "(" << (*nit)->type << (*nit)->label << ", " << (*nit)->node << ")";
    }
    cout << endl;

    for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
    {
      unsigned int fixed = 0;
      if((*nit)->fixed == true)
      {
        fixed = 1;
      }

      if((*nit)->type == "vgige")
      {
        // add the virtual switch entries
	std::list<link_resource_ptr> added_links;
        for(lit = (*nit)->links.begin(); lit != (*nit)->links.end(); ++lit)
        {
	  if ((*lit)->added)//this shouldn't happen
	  {
	    //added_links.push_back(*lit);
	    continue;
	  }
          if((*lit)->linkid == 0)
          {
            if((*lit)->mapped_path.empty())
            {
              // if a vswitch->vswitch connection, and both are mapped to same switch,
              // then there are no cids in the list. as a hack to make this work, there
              // is a cid=0 entry in the table that we use here so that the necessary
              // table entries are there for every link
              connschedule null_cs(linkid, rid, 0, (*lit)->capacity, (*lit)->rload, (*lit)->lload);
	      db_connections.push_back(null_cs);
            }
            //for(cit = (*lit)->conns.begin(); cit != (*lit)->conns.end(); ++cit)
	    for(mplit = (*lit)->mapped_path.begin(), dirit = (*lit)->mp_linkdir.begin(); mplit != (*lit)->mapped_path.end(); ++mplit, ++dirit)
            {
	      int rload = (*lit)->rload;
	      int lload = (*lit)->lload;
	      if ((*dirit) != 0) //this link is right for this path
		{
		  if (rload > (*mplit)->capacity) rload = (*mplit)->capacity;
		  if (lload > (*mplit)->capacity) lload = (*mplit)->capacity;
		}
	      else //this link is left for this path
		{
		  rload = (*lit)->lload;
		  lload = (*lit)->rload;
		  if (rload > (*mplit)->capacity) rload = (*mplit)->capacity;
		  if (lload > (*mplit)->capacity) lload = (*mplit)->capacity;
		}
	      
              connschedule cs(linkid, rid, (*mplit)->conns.front(), (*lit)->capacity, rload, lload);//(*lit)->rload, (*lit)->lload); //for vgige to vgige links won't this cause the link to be added twice
             
	      db_connections.push_back(cs);

	      ++ar_db_count;
	    }
	    //now process any links that were added
	    if (!(*lit)->added_vgige_links.empty())
	      {
		std::list<link_resource_ptr>::iterator addlit;
		for(addlit = (*lit)->added_vgige_links.begin(); addlit != (*lit)->added_vgige_links.end(); ++addlit)
		  { 
		    for(mplit = (*addlit)->mapped_path.begin(), dirit = (*addlit)->mp_linkdir.begin(); mplit != (*addlit)->mapped_path.end(); ++mplit, ++dirit)
		      {
			int rload = (*addlit)->rload;
			int lload = (*addlit)->lload;
			if ((*dirit) != 0) //this link is right for this path
			  {
			    if (rload > (*mplit)->capacity) rload = (*mplit)->capacity;
			    if (lload > (*mplit)->capacity) lload = (*mplit)->capacity;
			  }
			else //this link is left for this path
			  {
			    rload = (*lit)->lload;
			      lload = (*lit)->rload;
			      if (rload > (*mplit)->capacity) rload = (*mplit)->capacity;
			      if (lload > (*mplit)->capacity) lload = (*mplit)->capacity;
			  }
			connschedule cs(linkid, rid, (*mplit)->conns.front(), (*addlit)->capacity, rload, lload);//(*lit)->rload, (*lit)->lload); //for vgige to vgige links won't this cause the link to be added twice
			
			db_connections.push_back(cs);
			
			++ar_db_count;
		      }
		  }
	      }
	    //}
	    (*lit)->linkid = linkid;
	    ++linkid;
	  }

	  // int port = (*lit)->node1_port;
	  //if((*lit)->node2->node == (*nit)->node) 
	  //{
	  //  port = (*lit)->node2_port;
	  //}
	  //vswitchschedule vs((*nit)->cost, rid, port,linkid);//reused cost field for vlanid
	  
	  //db_vswitches.push_back(vs);
	  
	  //++ar_db_count;
	  
	}
	vswitchschedule vs((*nit)->vmid, rid, 0, (linkid-1));//reused cost field for vlanid, only need one entry for each vgige. linkschedule lists the rest
	  
	  db_vswitches.push_back(vs);
	  
	  ++ar_db_count;
	/*
        for(lit = added_links.begin(); lit != added_links.end(); ++lit)
        {  
	  if((*lit)->linkid == 0)
	    {
	      if((*lit)->conns.empty())
		{
		  // if a vswitch->vswitch connection, and both are mapped to same switch,
		  // then there are no cids in the list. as a hack to make this work, there
		  // is a cid=0 entry in the table that we use here so that the necessary
		  // table entries are there for every link
		  (*lit)->conns.push_back(0);
		}
	      for(cit = (*lit)->conns.begin(); cit != (*lit)->conns.end(); ++cit)
		{
		  int rload = (*lit)->rload;
		  if (rload > MAX_INTERCLUSTER_CAPACITY) rload = MAX_INTERCLUSTER_CAPACITY;//(*lit)->capacity) rload = (*lit)->capacity;
		  int lload = (*lit)->lload;
		  if (lload > MAX_INTERCLUSTER_CAPACITY) lload = MAX_INTERCLUSTER_CAPACITY;//(*lit)->capacity) lload = (*lit)->capacity;
		  connschedule cs(linkid, rid, *cit, (*lit)->capacity, rload, lload);//(*lit)->rload, (*lit)->lload); //for vgige to vgige links won't this cause the link to be added twice
             
		  db_connections.push_back(cs);

		  //mysqlpp::Query ins = onl->query();
		  //ins.insert(cs);
		  ++ar_db_count;
		  //ins.execute();
		}
	      (*lit)->linkid = linkid;
	    }
	    }*/
      }
      else if((*nit)->type_type == "hwcluster")
      {
        // add the hwcluster entries
        hwclusterschedule hwcs((*nit)->node, rid, fixed);

	db_hwclusters.push_back(hwcs);

	// mysqlpp::Query ins = onl->query();
        //ins.insert(hwcs);
	++ar_db_count;
        //ins.execute();
      }
      else
      {
        // add the node entries
	if ((*nit)->type == "vm") 
	  {
	    (*nit)->vmid = vlanid++;
	  }
	else (*nit)->vmid = 0;
        nodeschedule ns((*nit)->node, rid, fixed, (*nit)->vmid, (*nit)->core_capacity, (*nit)->mem_capacity);
	
	db_nodes.push_back(ns);

	++ar_db_count;
      }
    }

    if (!db_connections.empty())
      {
    	mysqlpp::Query ins = onl->query();
    	ins.insert(db_connections.begin(), db_connections.end());
    	ins.execute();
    	db_connections.clear();
      }
    if (!db_vswitches.empty())
      {
	mysqlpp::Query ins = onl->query();
	ins.insert(db_vswitches.begin(), db_vswitches.end());
	ins.execute();
	db_vswitches.clear();
      } 
    if (!db_hwclusters.empty())
      {
	mysqlpp::Query ins = onl->query();
	ins.insert(db_hwclusters.begin(), db_hwclusters.end());
	ins.execute();
	db_hwclusters.clear();
      }
    if (!db_nodes.empty())
      {
	mysqlpp::Query ins = onl->query();
	ins.insert(db_nodes.begin(), db_nodes.end());
	ins.execute();
	db_nodes.clear();
      }

    for(lit = t->links.begin(); lit != t->links.end(); ++lit)
    {
      //handle any virtual ports. need to put an entry in to link virtual ports to real physical interfaces
      //if ((*lit)->node1->has_vport ||(*lit)->node2->has_vport)
      //{
        unsigned int tmp_linkid = linkid;
        if((*lit)->node1->type == "vgige" || (*lit)->node2->type == "vgige") tmp_linkid = (*lit)->linkid;
	linkschedule lnks(tmp_linkid, rid, (*lit)->node1->node, (*lit)->node1_port, (*lit)->node1_capacity, (*lit)->node2->node, (*lit)->node2_port, (*lit)->node2_capacity, (*lit)->node1->vmid, (*lit)->node2->vmid);//cost is overloaded with vmid or vswitchid
	db_links.push_back(lnks);
	++ar_db_count;
	  //}
	if((*lit)->node1->type == "vgige" || (*lit)->node2->type == "vgige") continue;
	// add the link entries
	if((*lit)->mapped_path.empty())
	  {
	    // if a vm->vm connection, and both are mapped to same node,
	    // then there are no cids in the list. as a hack to make this work, there
	    // is a cid=0 entry in the table that we use here so that the necessary
	    // table entries are there for every link
	    connschedule null_cs(linkid, rid, 0, (*lit)->capacity, (*lit)->rload, (*lit)->lload);
	    db_connections.push_back(null_cs);
	  }
	for(mplit = (*lit)->mapped_path.begin(), dirit = (*lit)->mp_linkdir.begin(); mplit != (*lit)->mapped_path.end(); ++mplit, ++dirit)
	  {
	    int rload = (*lit)->rload;
	    int lload = (*lit)->lload;
	    if ((*dirit) != 0) //this link is right for this path
	      {
		if (rload > (*mplit)->capacity) rload = (*mplit)->capacity;
		if (lload > (*mplit)->capacity) lload = (*mplit)->capacity;
	      }
	    else //this link is left for this path
	      {
		rload = (*lit)->lload;
		lload = (*lit)->rload;
		if (rload > (*mplit)->capacity) rload = (*mplit)->capacity;
		if (lload > (*mplit)->capacity) lload = (*mplit)->capacity;
	      }
	    connschedule cs(linkid, rid, (*mplit)->conns.front(), (*lit)->capacity, rload, lload);//(*lit)->rload, (*lit)->lload);
	    db_connections.push_back(cs);
	    ++ar_db_count;
	  }
	++linkid;
    }
    if (!db_connections.empty())
      {
	mysqlpp::Query ins = onl->query();
	ins.insert(db_connections.begin(), db_connections.end());
	ins.execute();
	db_connections.clear();
      }
    if (!db_links.empty())
      {
	mysqlpp::Query ins = onl->query();
	ins.insert(db_links.begin(), db_links.end());
	ins.execute();
	db_links.clear();
      }
  }
  catch (const mysqlpp::Exception& er)
  {
    print_diff("onldb::add_reservation fail", stime);
    return onldb_resp(-1,er.what());
  }

  char tmp_lbl[256];
  sprintf(tmp_lbl, "onldb::add_reservation db_calls:%d", ar_db_count);
  print_diff(tmp_lbl, stime);
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::check_interswitch_bandwidth(topology* t, std::string begin, std::string end) throw()
{
  std::list<link_resource_ptr>::iterator lit;
  std::list<int>::iterator cit;

  std::map<int, int> caps;
  std::map<int, int> rls;
  std::map<int, int> lls;
  std::map<int, int> conn_caps;
  std::map<int, int>::iterator mit;

  for(lit = t->links.begin(); lit != t->links.end(); ++lit)
  {
    for(cit = (*lit)->conns.begin(); cit != (*lit)->conns.end(); ++cit) 
    {
      if(*cit == 0) { continue; }
      if(caps.find(*cit) == caps.end())
      {
        caps[*cit] = (*lit)->capacity;	
	//get the connection capacity
	mysqlpp::Query query2 = onl->query();
	query2 << "select capacity from connections where cid=" << mysqlpp::quote << (*cit);
	vector<capinfo> ci2;
	query2.storein(ci2);
	if(ci2.size() != 1) { return onldb_resp(-1, (std::string)"database consistency problem");} 
	conn_caps[*cit] = ci2[0].capacity * 1000; //convert capacity to Mbps
	if (conn_caps[*cit] < (*lit)->rload) rls[*cit] = conn_caps[*cit];
	else rls[*cit] = (*lit)->rload;
	if (conn_caps[*cit] < (*lit)->lload) lls[*cit] = conn_caps[*cit];
	else lls[*cit] = (*lit)->lload;
      }
      else
      {
        caps[*cit] += (*lit)->capacity;
	if (conn_caps[*cit] < (*lit)->rload) rls[*cit] += conn_caps[*cit];
	else rls[*cit] += (*lit)->rload;
	if (conn_caps[*cit] < (*lit)->lload) lls[*cit] += conn_caps[*cit];
	else lls[*cit] += (*lit)->lload;
	//rls[*cit] += (*lit)->rload;
	//lls[*cit] += (*lit)->lload;
      }
    }
  }
  
  try
  {
    for(mit = caps.begin(); mit != caps.end(); ++mit)
    {
      mysqlpp::Query query = onl->query();
      query << "select capacity,rload,lload from connschedule where cid=" << mysqlpp::quote << mit->first << " and rid in (select rid from reservations where state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << end << " and end>" << mysqlpp::quote << begin << ")";
      vector<caploadinfo> ci;
      query.storein(ci);
      vector<caploadinfo>::iterator cap;
      for(cap = ci.begin(); cap != ci.end(); ++cap) 
      {
        caps[mit->first] += cap->capacity;
	if (conn_caps[mit->first] < cap->rload) rls[*cit] += conn_caps[mit->first];
	else rls[*cit] += cap->rload;
	if (conn_caps[mit->first] < cap->lload) lls[*cit] += conn_caps[mit->first];
	else lls[mit->first] += cap->lload;
        //rls[mit->first] += cap->rload;
        //lls[mit->first] += cap->lload;
      }

      //mysqlpp::Query query2 = onl->query();
      //query2 << "select capacity from connections where cid=" << mysqlpp::quote << mit->first;
      //vector<capinfo> ci2;
      //query2.storein(ci2);
      //if(ci2.size() != 1) { return onldb_resp(-1, (std::string)"database consistency problem"); }
      //if(rls[mit->first] > ci2[0].capacity || lls[mit->first] > ci2[0].capacity)
      if(rls[mit->first] > conn_caps[mit->first] || lls[mit->first] > conn_caps[mit->first])
      {
        return onldb_resp(0,(std::string)"too many resources in use");
      }
    }
  }
  catch (const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::print_types() throw()
{
  try
  {
    vector<types> typesres;
    mysqlpp::Query query = onl->query();

    query << "select * from types";
    query.storein(typesres);

    vector<types>::iterator type;
    for(type = typesres.begin(); type != typesres.end(); ++type)
    {
      cout << type->tid << endl;
    }
  }
  catch (const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::clear_all_experiments() throw()
{
  try
  {
    // first get the list of active experiments so that we can clear out 
    // all soft state (groups, exports) that may be left over from before
    mysqlpp::Query oldres = onl->query();
    oldres << "select rid,user from reservations where rid in (select rid from experiments where begin=end)";
    vector<userres> reses;
    oldres.storein(reses);
    vector<userres>::iterator res;
    for(res = reses.begin(); res != reses.end(); ++res)
    {
      mysqlpp::Query nodeq = onl->query();
      nodeq << "select node,daemonhost,acl from nodes where node in (select node from nodeschedule where rid=" << mysqlpp::quote << res->rid << ")"; 
      vector<oldnodes> nodes;
      nodeq.storein(nodes);
      vector<oldnodes>::iterator node;
      for(node = nodes.begin(); node != nodes.end(); ++node)
      {
        std::string cmd = "/usr/testbed/scripts/system_session_update.pl remove " + res->user + " " + node->daemonhost + " " + node->acl;
        int ret = system(cmd.c_str());
        if(ret < 0 || WEXITSTATUS(ret) != 1) cout << "Warning: " << res->user << "'s home area was not unexported to " << node->daemonhost << " and user was not removed from group " << node->acl << endl;
      }

      mysqlpp::Query clusq = onl->query();
      clusq << "select cluster,acl from hwclusters where cluster in (select cluster from hwclusterschedule where rid=" << mysqlpp::quote << res->rid << ")"; 
      vector<oldclusters> clusters;
      clusq.storein(clusters);
      vector<oldclusters>::iterator cluster;
      for(cluster = clusters.begin(); cluster != clusters.end(); ++cluster)
      {
        std::string cmd = "/usr/testbed/scripts/system_session_update.pl remove " + res->user + " unused " + cluster->acl;
        int ret = system(cmd.c_str());
        if(ret < 0 || WEXITSTATUS(ret) != 1) cout << "Warning: " << res->user << " was not removed from group " << cluster->acl << endl;
      }
    }
    if(!reses.empty())
    {
      std::string cmd = "/usr/testbed/scripts/system_session_update.pl update";
      int ret = system(cmd.c_str());
      if(ret < 0 || WEXITSTATUS(ret) != 1) cout << "clear_all_experiments: warning: update failed, so groups may not have been cleaned up" << endl;
    }

    // update any 'active' reservations to be used
    mysqlpp::Query resup = onl->query();
    resup << "update reservations set state='used' where state='active'";
    resup.execute();
  
    // update any 'active' experiments to have now as their end time 
    std::string current_time = time_unix2db(time(NULL));
    mysqlpp::Query expup = onl->query();
    expup << "update experiments set end=" << mysqlpp::quote << current_time << " where begin=end";
    expup.execute();

    // update all node states that are not testing,spare,repair to be free
    mysqlpp::Query nodeup = onl->query();
    nodeup << "update nodes set state='free' where state='active' or state='initializing' or state='refreshing'";
    nodeup.execute();

    // update all hwcluster states that are not testing,spare,repair to be free
    mysqlpp::Query clup = onl->query();
    clup << "update hwclusters set state='free' where state='active' or state='initializing' or state='refreshing'";
    clup.execute();
  }
  catch (const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::get_switch_list(switch_info_list& list) throw()
{
  list.clear();
  try
  {
    vector<nodenames> switches;
    mysqlpp::Query query = onl->query();
    query << "select node from nodes where tid in (select tid from types where type='infrastructure')";
    query.storein(switches);
    vector<nodenames>::iterator sw;
    for(sw = switches.begin(); sw != switches.end(); ++sw)
    {
      mysqlpp::Query query2 = onl->query();
      query2 << "select max(port) as numports from interfaces where tid in (select tid from nodes where node=" << mysqlpp::quote << sw->node << ")";
      mysqlpp::StoreQueryResult res2 = query2.store();
      if(res2.empty())
      {
        return onldb_resp(-1,"database inconsistency error");
      }
      switchports sp = res2[0];

      mysqlpp::Query query3 = onl->query();
      query3 << "select port from interfaces where interface='management' and tid in (select tid from nodes where node=" << mysqlpp::quote << sw->node << ")";
      mysqlpp::StoreQueryResult res3 = query3.store();
      unsigned int mgmt_port = 0;
      if(!res3.empty())
      {
        mgmtport mp = res3[0];
        mgmt_port = mp.port;
      }

      switch_info si(sw->node, sp.numports, mgmt_port);
      list.push_back(si);
    }
  }
  catch (const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::get_base_node_list(node_info_list& list) throw()
{
  list.clear();
  try
  {
    vector<nodeinfo> nodes;
    mysqlpp::Query query = onl->query();
    query << "select nodes.node,nodes.state,types.daemon,types.keeboot,nodes.daemonhost,nodes.daemonport,types.tid as type,hwclustercomps.dependent,hwclustercomps.cluster from nodes join types using (tid) left join hwclustercomps using (node) where types.type='base' order by node";
    query.storein(nodes);

    vector<nodeinfo>::iterator node;
    for(node = nodes.begin(); node != nodes.end(); ++node)
    {
      bool has_cp;
      if(((int)node->daemon) == 0) { has_cp = false; }
      else { has_cp = true; }
      bool do_keeboot;
      if(((int)node->keeboot) == 0) { do_keeboot = false; }
      else { do_keeboot = true; }
      bool is_dependent;
      if(node->dependent.is_null) { is_dependent = false; }
      else if(((int)node->dependent.data) == 0) { is_dependent = false; }
      else { is_dependent = true; }
      node_info new_node;
      //if (strcmp(node->cluster, "NULL") == 0)
      //new_node = node_info(node->node,node->state,has_cp,do_keeboot,node->daemonhost,node->daemonport,node->type,is_dependent,"");
      //else
      new_node = node_info(node->node,node->state,has_cp,do_keeboot,node->daemonhost,node->daemonport,node->type,is_dependent,node->cluster);

      list.push_back(new_node);
      
    }
  }
  catch (const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::get_node_info(std::string node, bool is_cluster, node_info& info) throw()
{
  if(node.substr(0,5) == "vgige")
  {
    info = node_info(node,"free",false,false,"",0,"vgige",false,"");
    return onldb_resp(1,(std::string)"success");
  }
  try
  {
    if(!is_cluster)
    {
      mysqlpp::Query query = onl->query();
      query << "select nodes.node,nodes.state,types.daemon,types.keeboot,nodes.daemonhost,nodes.daemonport,types.tid as type,hwclustercomps.dependent,hwclustercomps.cluster from nodes join types using (tid) left join hwclustercomps using (node) where types.type='base' and nodes.node=" << mysqlpp::quote << node;
      mysqlpp::StoreQueryResult res = query.store();

      if(res.empty())
      {
        std::string errmsg;
        errmsg = "node " + node + " not in the database";
        return onldb_resp(-1,errmsg);
      }
      nodeinfo ni = res[0];
    
      bool has_cp;
      if(((int)ni.daemon) == 0) { has_cp = false; }
      else { has_cp = true; }
      bool do_keeboot;
      if(((int)ni.keeboot) == 0) { do_keeboot = false; }
      else { do_keeboot = true; }
      bool is_dependent;
      if(ni.dependent.is_null) { is_dependent = false; }
      else if(((int)ni.dependent.data) == 0) { is_dependent = false; }
      else { is_dependent = true; }
      //if (strcmp(ni.cluster,"NULL") == 0)
      //info = node_info(ni.node,ni.state,has_cp,do_keeboot,ni.daemonhost,ni.daemonport,ni.type,is_dependent,"");
      //else
      info = node_info(ni.node,ni.state,has_cp,do_keeboot,ni.daemonhost,ni.daemonport,ni.type,is_dependent,ni.cluster);
    }
    else
    {
      mysqlpp::Query query = onl->query();
      query << "select hwclusters.cluster as cluster,hwclusters.state as state,types.tid as type from hwclusters,types where types.type='hwcluster' and hwclusters.tid=types.tid and hwclusters.cluster=" << mysqlpp::quote << node;
      mysqlpp::StoreQueryResult res = query.store();

      if(res.empty())
      {
        std::string errmsg;
        errmsg = "node " + node + " not in the database";
        return onldb_resp(-1,errmsg);
      }
      clusterinfo ci = res[0];
    
      info = node_info(ci.cluster,ci.state,false,false,"",0,ci.type,false,"");
    }
  }
  catch (const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::get_state(std::string node, bool is_cluster) throw()
{
  if(node.substr(0,5) == "vgige")
  {
    return onldb_resp(1,(std::string)"free");
  }
  try
  {
    if(!is_cluster)
    {
      mysqlpp::Query query = onl->query();
      query << "select state from nodes where node=" << mysqlpp::quote << node;
      mysqlpp::StoreQueryResult res = query.store();
      if(res.empty())
      {
        std::string errmsg;
        errmsg = "node " + node + " not in the database";
        return onldb_resp(-1,errmsg);
      }
      nodes n = res[0];
      return onldb_resp(1,n.state);
    }
    else
    {
      mysqlpp::Query query = onl->query();
      query << "select state from hwclusters where cluster=" << mysqlpp::quote << node;
      mysqlpp::StoreQueryResult res = query.store();
      if(res.empty())
      {
        std::string errmsg;
        errmsg = "node " + node + " not in the database";
        return onldb_resp(-1,errmsg);
      }
      clusterstates c = res[0];
      return onldb_resp(1,c.state);
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
}

onldb_resp onldb::set_state(std::string node, std::string state, unsigned int len) throw()
{
  if(node.substr(0,5) == "vgige")
  {
    return onldb_resp(1,(std::string)"success");
  }
  std::string old_state;
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select state from nodes where node=" << mysqlpp::quote << node;
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "node " + node + " not in the database";
      return onldb_resp(-1,errmsg);
    } 
    nodestates n = res[0];
    old_state = n.state;
    if(n.state != state && n.state != "testing")
    {
      mysqlpp::Query update = onl->query();
      update << "update nodes set state=" << mysqlpp::quote << state << " where node=" << mysqlpp::quote << node;
      update.execute();
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }

  if(old_state != "testing")
  {
    if(state == "repair" && old_state != "repair")
    {
      onldb_resp r = handle_special_state(state, node, len, false);
      if(r.result() <= 0)
      {
        return onldb_resp(r.result(), r.msg());
      }
    }
    else if(old_state == "repair" && state != "repair")
    {
      onldb_resp r = clear_special_state(old_state, state, node);
      if(r.result() <= 0)
      {
        return onldb_resp(r.result(), r.msg());
      }
    }
  }
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::get_node_from_cp(std::string cp) throw()
{
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select node from nodes where daemonhost=" << mysqlpp::quote << cp;
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "cp " + cp + " not in the database";
      return onldb_resp(-1,errmsg);
    }
   
    // assuming a one-to-one mapping b/w cp and node
    nodenames nn = res[0];
    return onldb_resp(1,(std::string)nn.node);
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
}

onldb_resp onldb::put_in_testing(std::string node, unsigned int len) throw()
{
  std::string old_state;
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select state from nodes where node=" << mysqlpp::quote << node;
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "node " + node + " not in the database";
      return onldb_resp(-1,errmsg);
    }
    nodestates n = res[0];
    old_state = n.state;
    if(old_state == "testing")
    {
      std::string errmsg;
      errmsg = "node " + node + " already in testing";
      return onldb_resp(0, errmsg);
    }
    mysqlpp::Query update = onl->query();
    update << "update nodes set state='testing' where node=" << mysqlpp::quote << node;
    update.execute();
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }

  if(old_state == "repair")
  {
    onldb_resp r = clear_special_state(old_state, "testing", node);
    if(r.result() <= 0)
    {
      return onldb_resp(r.result(), r.msg());
    }
  }

  onldb_resp t = handle_special_state("testing", node, len, false);
  if(t.result() <= 0)
  {
    return onldb_resp(t.result(), t.msg());
  }

  return onldb_resp(1, (std::string)"success");
}

onldb_resp onldb::remove_from_testing(std::string node) throw()
{
  std::string old_state;
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select state from nodes where node=" << mysqlpp::quote << node;
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "node " + node + " not in the database";
      return onldb_resp(-1,errmsg);
    }
    nodestates n = res[0];
    old_state = n.state;
    if(old_state != "testing")
    {
      std::string errmsg;
      errmsg = "node " + node + " is not in testing";
      return onldb_resp(0, errmsg);
    }
    mysqlpp::Query update = onl->query();
    update << "update nodes set state='free' where node=" << mysqlpp::quote << node;
    update.execute();
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }

  onldb_resp r = clear_special_state(old_state, "free", node);
  if(r.result() <= 0)
  {
    return onldb_resp(r.result(), r.msg());
  }
  
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::extend_repair(std::string node, unsigned int len) throw()
{
  onldb_resp r = handle_special_state("repair", node, len, true);
  return onldb_resp(r.result(), r.msg());
}

onldb_resp onldb::extend_testing(std::string node, unsigned int len) throw()
{
  onldb_resp r = handle_special_state("testing", node, len, true);
  return onldb_resp(r.result(), r.msg());
}

onldb_resp onldb::handle_special_state(std::string state, std::string node, unsigned int len, bool extend) throw()
{
  unsigned int divisor;
  // get the hour divisor from policy so that we can force times into discrete slots
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select value from policy where parameter='divisor'";
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "policy database error.. divisor is not in the database";
      return onldb_resp(-1,errmsg);
    }
    policyvals pv = res[0];
    divisor = pv.value;
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
  
  std::string begin, end;
  bool is_parent = false;
  std::string parent_node = "";

  if(!extend)
  {
    if(len == 0)
    {
      try
      {
        mysqlpp::Query query = onl->query();
        query << "select value from policy where parameter='repair_time'";
        mysqlpp::StoreQueryResult res = query.store();
        if(res.empty())
        {
          std::string errmsg;
          errmsg = "policy database error.. repair_time is not in the database";
          return onldb_resp(-1,errmsg);
        }
        policyvals pv = res[0];
        len = pv.value;
      }
      catch(const mysqlpp::Exception& er)
      {
        return onldb_resp(-1,er.what());
      }
    }

    topology t;
    std::string type;
    unsigned int parent_label = 0;
    try
    {
      mysqlpp::Query query = onl->query();
      query << "select nodes.node,nodes.tid,hwclustercomps.cluster from nodes left join hwclustercomps using (node) where nodes.node=" << mysqlpp::quote << node;
      mysqlpp::StoreQueryResult res = query.store();
      if(res.empty()) return onldb_resp(-1,(std::string)"database error");
      specialnodeinfo sni = res[0];
      type = sni.tid;
      if(!sni.cluster.is_null)
      {
        parent_label = 2;
        onldb_resp pt = get_type(sni.cluster.data, true);
        if(pt.result() < 1) return onldb_resp(pt.result(), pt.msg());
        std::string parent_type = pt.msg();
        t.add_node(parent_type, parent_label, 0);
        is_parent = true;
        parent_node = sni.cluster.data;

        mysqlpp::Query query2 = onl->query();
        query2 << "select nodes.node,nodes.tid from nodes where node in (select node from hwclustercomps where node!=" << mysqlpp::quote << node << " and cluster=" << mysqlpp::quote << parent_node << ")";
        vector<specnodeinfo> complist;
        vector<specnodeinfo>::iterator comp;
        query2.storein(complist);
        unsigned int next_label = 3;
        for(comp = complist.begin(); comp != complist.end(); ++comp)
        {
          t.add_node(comp->tid,next_label,parent_label);
          onldb_resp blah = fix_component(&t, next_label, comp->node);
          if(blah.result() < 1)
          {
            return onldb_resp(blah.result(),blah.msg());
          }
          ++next_label;
          mysqlpp::Query update = onl->query();
          update << "update nodes set state=" << mysqlpp::quote << state << "where node=" << mysqlpp::quote << comp->node;
          update.execute();
        }
      }
    }
    catch(const mysqlpp::Exception& er)
    {
      return onldb_resp(-1,er.what());
    }

    t.add_node(type, 1, parent_label);
    onldb_resp fc = fix_component(&t, 1, node);
    if(fc.result() < 1)
    {
      return onldb_resp(fc.result(),fc.msg());
    }

    if(state == "repair")
    {
      char cmd[128];
      sprintf(cmd, "/usr/testbed/scripts/send_repair_email.pl %s %u", node.c_str(), len);
      int ret = system(cmd);
      if(ret < 0 || WEXITSTATUS(ret) != 1) cout << "Warning: " << node << " was put into repair but email was not sent" << endl;
    }
  
    time_t current_time_unix = time(NULL);
    current_time_unix = discretize_time(current_time_unix, divisor);
    begin = time_unix2db(current_time_unix);
    time_t end_time_unix = add_time(current_time_unix, len*60*60);
    end = time_unix2db(end_time_unix);

    if(lock("reservation") == false) return onldb_resp(0,"database locking problem");

    onldb_resp vcr = verify_clusters(&t);
    if(vcr.result() != 1)
    {
      unlock("reservation");
      return onldb_resp(vcr.result(),vcr.msg());
    }
    
    onldb_resp r = add_reservation(&t, state, begin, end);
    if(r.result() < 1)
    {
      unlock("reservation");
      return onldb_resp(r.result(), r.msg());
    }
  }
  else
  {
    if(len == 0)
    {
      return onldb_resp(0,"zero hours doesn't make sense");
    }

    time_t current_time_unix = time(NULL);
    current_time_unix = discretize_time(current_time_unix, divisor);
    std::string current_time_db = time_unix2db(current_time_unix);
    
    if(lock("reservation") == false) return onldb_resp(0,"database locking problem");

    time_t end_unix, new_end_unix;
    std::string end_db, new_end_db;
    unsigned int rid;
    try
    {
      mysqlpp::Query query = onl->query();
      query << "select nodes.node,nodes.tid,hwclustercomps.cluster from nodes left join hwclustercomps using (node) where nodes.node=" << mysqlpp::quote << node;
      mysqlpp::StoreQueryResult res = query.store();
      if(res.empty()) return onldb_resp(-1,(std::string)"database error");
      specialnodeinfo sni = res[0];
      if(!sni.cluster.is_null)
      {
        is_parent = true;
        parent_node = sni.cluster.data;
      }

      query = onl->query();
      if(is_parent)
      {
        query << "select begin,end,rid from reservations where user=" << mysqlpp::quote << state << " and begin<=" << mysqlpp::quote << current_time_db << " and end>=" << mysqlpp::quote << current_time_db << " and state!='timedout' and state!='cancelled' and rid in (select rid from hwclusterschedule where cluster=" << mysqlpp::quote << parent_node << ")";
      }
      else
      {
        query << "select begin,end,rid from reservations where user=" << mysqlpp::quote << state << " and begin<=" << mysqlpp::quote << current_time_db << " and end>=" << mysqlpp::quote << current_time_db << " and state!='timedout' and state!='cancelled' and rid in (select rid from nodeschedule where node=" << mysqlpp::quote << node << ")";
      }
      res = query.store();
      if(res.empty())
      {
        std::string errmsg;
        errmsg = "this is no current " + state + " reservation for " + node;
        unlock("reservation");
        return onldb_resp(0,errmsg);
      }
      curresinfo curres = res[0];
      end_unix = time_db2unix(curres.end);
      end_db = time_unix2db(end_unix);
      rid = curres.rid;
    }
    catch(const mysqlpp::Exception& er)
    {
      unlock("reservation");
      return onldb_resp(-1,er.what());
    }

    new_end_unix = add_time(end_unix, len*60*60);
    new_end_db = time_unix2db(new_end_unix);

    topology t;
    onldb_resp r = get_topology(&t, rid);
    if(r.result() < 1)
    {
      unlock("reservation");
      return onldb_resp(r.result(),r.msg());
    }

    try
    {
      mysqlpp::Query up = onl->query();
      up << "update reservations set end=" << mysqlpp::quote << new_end_db << " where rid=" << mysqlpp::quote << rid;
      up.execute();
    }
    catch(const mysqlpp::Exception& er)
    {
      unlock("reservation");
      return onldb_resp(-1,er.what());
    }

    begin = end_db;
    end = new_end_db;
  }

  vector<reservations> reslist;
  vector<reservations>::iterator res;
  try
  {
    mysqlpp::Query query = onl->query();
    if(is_parent)
    {
      query << "select rid,user,begin,end,state from reservations where end>=" << mysqlpp::quote << begin << " and begin<=" << mysqlpp::quote << end << " and state!='cancelled' and state!='timedout' and state!='active' and user!='repair' and user!='testing' and user!='system' and rid in (select rid from hwclusterschedule where cluster=" << mysqlpp::quote << parent_node << ")";
    }
    else
    {
      query << "select rid,user,begin,end,state from reservations where end>=" << mysqlpp::quote << begin << " and begin<=" << mysqlpp::quote << end << " and state!='cancelled' and state!='timedout' and state!='active' and user!='repair' and user!='testing' and user!='system' and rid in (select rid from nodeschedule where node=" << mysqlpp::quote << node << ")";
    }
    query.storein(reslist);
    for(res = reslist.begin(); res != reslist.end(); ++res)
    {
      mysqlpp::Query can = onl->query();
      can << "update reservations set state='cancelled' where rid=" << mysqlpp::quote << res->rid;
      can.execute();

      topology existing_top;
      onldb_resp r = get_topology(&existing_top, res->rid);
      // if something went wrong, this res is hosed anyway, so leave it cancelled
      if(r.result() < 1) continue;

      r = verify_clusters(&existing_top);
      // if something went wrong, this res is hosed anyway, so leave it cancelled
      if(r.result() < 1) continue;

      // clear out the existing assignments
      bool remap = true;
      std::list<node_resource_ptr>::iterator nit;
      for(nit = existing_top.nodes.begin(); nit != existing_top.nodes.end(); ++nit)
      {
        // if node is the node being handled, and it is fixed, then we can't remap this res
        if(((*nit)->node == parent_node || (*nit)->node == node) && (*nit)->fixed == true)
        {
          remap = false;
          break;
        }
        (*nit)->node = "";
        (*nit)->acl = "unused";
        (*nit)->cp = "unused";
      }
      std::list<link_resource_ptr>::iterator lit;
      for(lit = existing_top.links.begin(); lit != existing_top.links.end(); ++lit)
      {
        (*lit)->conns.clear();
      }

      if(remap)
      {
        std::string res_begin = time_unix2db(time_db2unix(res->begin));
        std::string res_end = time_unix2db(time_db2unix(res->end));
        //r = try_reservation(&existing_top, res->user, res_begin, res_end);//JP changed 3/29/12
        r = try_reservation(&existing_top, res->user, res_begin, res_end, "used");
        // if result is 1, then we made a new res for user, so go to next one
        if(r.result() == 1) continue;
        // again, if something bad happend, this res is probably hosed, so leave it cancelled
        if(r.result() < 0) continue;
      }

      // here, couldn't remap res, so need to uncancel original one
      mysqlpp::Query uncan = onl->query();
      uncan << "update reservations set state=" << mysqlpp::quote << res->state << " where rid=" << mysqlpp::quote << res->rid;
      uncan.execute();
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    unlock("reservation");
    return onldb_resp(-1,er.what());
  }
  
  unlock("reservation");
  return onldb_resp(1, "success");
}

onldb_resp onldb::clear_special_state(std::string state, std::string new_state, std::string node) throw()
{
  if(lock("reservation") == false) return onldb_resp(0,"database locking problem");
  try
  {
    std::string current_time = time_unix2db(time(NULL));

    bool is_parent = false;
    std::string parent_node = "";
    mysqlpp::Query query = onl->query();
    query << "select nodes.node,nodes.tid,hwclustercomps.cluster from nodes left join hwclustercomps using (node) where nodes.node=" << mysqlpp::quote << node;
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty()) return onldb_resp(-1,(std::string)"database error");
    specialnodeinfo sni = res[0];
    if(!sni.cluster.is_null)
    {
      is_parent = true;
      parent_node = sni.cluster.data;

      mysqlpp::Query query2 = onl->query();
      query2 << "select node,tid from nodes where node in (select node from hwclustercomps where node!=" << mysqlpp::quote << node << " and cluster=" << mysqlpp::quote << parent_node << ")";
      vector<specnodeinfo> complist;
      vector<specnodeinfo>::iterator comp;
      query2.storein(complist);
      for(comp = complist.begin(); comp != complist.end(); ++comp)
      {
        mysqlpp::Query update = onl->query();
        update << "update nodes set state=" << mysqlpp::quote << new_state << " where node=" << mysqlpp::quote << comp->node;
        update.execute();
      }
    }

    mysqlpp::Query can = onl->query();
    if(is_parent)
    {
      can << "update reservations set state='cancelled' where user=" << mysqlpp::quote << state << " and begin<" << mysqlpp::quote << current_time << " and end>" << mysqlpp::quote << current_time << " and (state='pending' or state='used') and rid in (select rid from hwclusterschedule where cluster=" << mysqlpp::quote << parent_node << ")";
    }
    else
    {
      can << "update reservations set state='cancelled' where user=" << mysqlpp::quote << state << " and begin<" << mysqlpp::quote << current_time << " and end>" << mysqlpp::quote << current_time << " and (state='pending' or state='used') and rid in (select rid from nodeschedule where node=" << mysqlpp::quote << node << ")";
    }
    can.execute();
  }
  catch(const mysqlpp::Exception& er)
  {
    unlock("reservation");
    return onldb_resp(-1,er.what());
  }
  unlock("reservation");
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::get_type(std::string node, bool is_cluster) throw()
{
  if(node.substr(0,5) == "vgige")
  {
    return onldb_resp(1,(std::string)"vgige");
  }
  try
  {
    if(!is_cluster)
    {
      mysqlpp::Query query = onl->query();
      query << "select tid from nodes where node=" << mysqlpp::quote << node;
      mysqlpp::StoreQueryResult res = query.store();
      if(res.empty())
      {
        std::string errmsg;
        errmsg = "node " + node + " not in the database";
        return onldb_resp(-1,errmsg);
      }
      typenameinfo nameinfo = res[0];
      return onldb_resp(1,nameinfo.tid);
    }
    else
    {
      mysqlpp::Query query = onl->query();
      query << "select tid from hwclusters where cluster=" << mysqlpp::quote << node;
      mysqlpp::StoreQueryResult res = query.store();
      if(res.empty())
      {
        std::string errmsg;
        errmsg = "node " + node + " not in the database";
        return onldb_resp(-1,errmsg);
      }
      typenameinfo nameinfo = res[0];
      return onldb_resp(1,nameinfo.tid);
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
}

onldb_resp onldb::authenticate_user(std::string username, std::string password_hash) throw()
{
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select password from users where user=" << mysqlpp::quote << username;
    mysqlpp::StoreQueryResult res = query.store(); 
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "user " + username + " not in the database";
      return onldb_resp(-1,errmsg);
    }
    passwords p = res[0];
    if(p.password == password_hash)
    {
      return onldb_resp(1,(std::string)"user authenticated");
    }
    else
    {
      return onldb_resp(0,(std::string)"password incorrect");
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
}

onldb_resp onldb::is_admin(std::string username) throw()
{
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select priv from users where user=" << mysqlpp::quote << username;
    ++db_count;
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "user " + username + " not in the database";
      return onldb_resp(-1,errmsg);
    }
    privileges p = res[0];
    if(((int)p.priv) > 0)
    {
      return onldb_resp(1,(std::string)"user is admin");
    }
    else
    {
      return onldb_resp(0,(std::string)"not admin");
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
 return onldb_resp(0,(std::string)"not admin");
}

onldb_resp onldb::reserve_all(std::string begin, unsigned int len) throw()
{
  unsigned int horizon;
  unsigned int divisor;

  time_t current_time_unix;
  time_t begin_unix;
  time_t end_unix;
  std::string current_time_db;
  std::string begin_db;
  std::string end_db;

  // get the hour divisor from policy so that we can force times into discrete slots
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select value from policy where parameter=" << mysqlpp::quote << "divisor";
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "policy database error.. divisor is not in the database";
      return onldb_resp(-1,errmsg);
    }
    policyvals pv = res[0];
    divisor = pv.value;
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }

  current_time_unix = time(NULL);
  current_time_unix = discretize_time(current_time_unix, divisor);
  current_time_db = time_unix2db(current_time_unix);

  begin_unix = time_db2unix(begin);
  begin_unix = discretize_time(begin_unix, divisor);
  begin_db = time_unix2db(begin_unix);

  if(begin_unix < current_time_unix)
  {
    std::string errmsg;
    errmsg = "begin time is in the past";
    return onldb_resp(0,errmsg);
  }

  if(len <= 0)
  {
    std::string errmsg;
    errmsg = "length of " + to_string(len) + " minutes does not make sense";
    return onldb_resp(0,errmsg);
  }

  unsigned int chunk = 60/divisor;
  len = (((len-1) / chunk)+1) * chunk;

  end_unix = add_time(begin_unix, len*60);
  end_db = time_unix2db(end_unix);

  // now start doing policy based checking
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select value from policy where parameter=" << mysqlpp::quote << "horizon";
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "policy database error.. horizon is not in the database";
      return onldb_resp(-1,errmsg);
    }
    policyvals pv = res[0];
    horizon = pv.value;
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }

  time_t horizon_limit_unix = add_time(current_time_unix, horizon*24*60*60);

  if(end_unix > horizon_limit_unix)
  {
    std::string errmsg;
    errmsg = "requested times too far into the future (currently a " + to_string(horizon) + " day limit)";
    return onldb_resp(0,errmsg);
  }

  if(lock("reservation") == false) return onldb_resp(0,"database locking problem.. try again later");

  try
  {
    mysqlpp::Query query = onl->query();
    query << "select user,begin,end from reservations where state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << end_db << " and end>" << mysqlpp::quote << begin_db << " order by begin";
    vector<otherrestimes> rts;
    query.storein(rts);
    vector<otherrestimes>::iterator restime;
    for(restime = rts.begin(); restime != rts.end(); ++restime)
    {
      if(restime->user != "testing" && restime->user != "repair")
      {
        unlock("reservation");
        return onldb_resp(0,(std::string)"there are non-system reservations during that time!");
      }
    }

    reservationins res("system", mysqlpp::DateTime(begin_db), mysqlpp::DateTime(end_db), "pending");
    mysqlpp::Query ins = onl->query();
    ins.insert(res);
    mysqlpp::SimpleResult sr = ins.execute();
    unsigned int rid = sr.insert_id();

    query = onl->query();
    query << "select cluster from hwclusters join types using(tid) where type='hwcluster' and cluster not in (select cluster from hwclusterschedule where rid in (select rid from reservations where begin<" << mysqlpp::quote << end_db << " and end>" << mysqlpp::quote << begin_db << " and state!='cancelled' and state!='testing'))";
    vector<clusternames> clusterlist;
    query.storein(clusterlist);
    vector<clusternames>::iterator clusterit;
    for(clusterit = clusterlist.begin(); clusterit != clusterlist.end(); ++clusterit)
    {
      hwclusterschedule hwcs(clusterit->cluster, rid, 1);
      mysqlpp::Query ins = onl->query();
      ins.insert(hwcs);
      ins.execute();
    }

    query = onl->query();
    query << "select node from nodes join types using(tid) where type='base' and node not in (select node from nodeschedule where rid in (select rid from reservations where begin<" << mysqlpp::quote << end_db << " and end>" << mysqlpp::quote << begin_db << " and state!='cancelled' and state!='testing'))";
    vector<nodenames> nodelist;
    query.storein(nodelist);
    vector<nodenames>::iterator nodeit;
    for(nodeit = nodelist.begin(); nodeit != nodelist.end(); ++nodeit)
    {
      nodeschedule ns(nodeit->node, rid, 1);
      mysqlpp::Query ins = onl->query();
      ins.insert(ns);
      ins.execute();
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    unlock("reservation");
    return onldb_resp(-1,er.what());
  }

  unlock("reservation");
  return onldb_resp(1,"success");
}

onldb_resp onldb::make_reservation(std::string username, std::string begin1, std::string begin2, unsigned int len, topology *t) throw()
{
  std::string tz;
  unsigned int horizon;
  unsigned int divisor;

  struct timeval stime;
  gettimeofday(&stime, NULL);
  time_t current_time;
  time_t current_time_unix;
  time_t begin1_unix;
  time_t begin2_unix;
  time_t end1_unix;
  time_t end2_unix;
  std::string current_time_db;
  std::string begin1_db;
  std::string begin2_db;
  std::string end1_db;
  std::string end2_db;

  vector<type_info_ptr> type_list;

  list<node_resource_ptr>::iterator hw;
  list<link_resource_ptr>::iterator link;
  vector<type_info_ptr>::iterator ti;

  //reset performance counters
  db_count = 0;
  make_res_time = 0;

  current_time = time(NULL);
  // get the hour divisor from policy so that we can force times into discrete slots
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select value from policy where parameter=" << mysqlpp::quote << "divisor";
    mysqlpp::StoreQueryResult res = query.store();
    ++db_count;
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "policy database error.. divisor is not in the database";
      return onldb_resp(-1,errmsg);
    }
    policyvals pv = res[0];
    divisor = pv.value;
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }

  current_time_unix = time(NULL);
  //set the current time to the start of the closest slot
  current_time_unix = discretize_time(current_time_unix, divisor);
  current_time_db = time_unix2db(current_time_unix);

  begin1_unix = time_db2unix(begin1);
  begin1_unix = discretize_time(begin1_unix, divisor);
  begin1_db = time_unix2db(begin1_unix);

  begin2_unix = time_db2unix(begin2);
  begin2_unix = discretize_time(begin2_unix, divisor);
  begin2_db = time_unix2db(begin2_unix);
  
  std::string JDD="jdd";
  std::string JP="jp";
  /*
  std::string demo1_begin = "20090814100000";
  std::string demo1_end = "20090814120000";

  std::string demo2_begin = "20090818030000";
  std::string demo2_end = "20090818090000";

  std::string demo3_begin = "20090827090000";
  std::string demo3_end = "20090827120000";

  time_t demo1_begin_unix = time_db2unix(demo1_begin);
  time_t demo1_end_unix = time_db2unix(demo1_end);
  time_t demo2_begin_unix = time_db2unix(demo2_begin);
  time_t demo2_end_unix = time_db2unix(demo2_end);
  time_t demo3_begin_unix = time_db2unix(demo3_begin);
  time_t demo3_end_unix = time_db2unix(demo3_end);
  */
  // start with some basic sanity checking of arguments
  try
  {
    //check that a timezone is set for this user
    mysqlpp::Query query = onl->query();
    query << "select timezone from users where user=" << mysqlpp::quote << username;
    ++db_count;
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "user " + username + " not in the database";
      return onldb_resp(-1,errmsg);
    }
    timezones tzdata = res[0];
    tz = tzdata.timezone;
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
 
 //check beginning of start range is less than the end, i.e. we don't have a negative start window 
  if(begin1_unix > begin2_unix)
  {
    begin2_db = begin1_db;
    begin2_unix = begin1_unix;
  }
 
  //check if the start range is in the past 
  if(begin1_unix < current_time_unix && begin2_unix < current_time_unix)
  {
    std::string errmsg;
    errmsg = "range of begin times from " + begin1_db + " to " + begin2_db + " is in the past";
    return onldb_resp(0,errmsg);
  }
 
  //check if the beginning of the start range is in the past. if it is set the start range to begin at the current time 
  if(begin1_unix < current_time_unix)
  {
    begin1_db = current_time_db;
    begin1_unix = current_time_unix;
  }

  //check if length of the experiment is valid
  if(len <= 0)
  {
    std::string errmsg;
    errmsg = "length of " + to_string(len) + " minutes does not make sense";
    return onldb_resp(0,errmsg);
  }

  //chunk is the number of minutes in a time slot 
  unsigned int chunk = 60/divisor;
  //convert the length to be a number of minutes that is a multiple of the chunk
  len = (((len-1) / chunk)+1) * chunk;

  //check if no components have been specified
  if(t->nodes.size() == 0)
  {
    std::string errmsg;
    errmsg = "no components requested";
    return onldb_resp(0,errmsg);
  }

  // build a vector of type information for each type that is represented in the topology.
  // go through the topology, verify that the type is in the database and count the number of each type
  // this loop ends with a list type_list that has an entry for each type in the topology and 
  // the number of instances requested
  for(hw = t->nodes.begin(); hw != t->nodes.end(); ++hw)
  {
    std::string type_type = get_type_type((*hw)->type);
    //check if the type is valid
    if(type_type == "")
    {
      std::string errmsg;
      errmsg = "type " + (*hw)->type + " not in the database";
      return onldb_resp(-1,errmsg);
    }
    
    //if it's a cluster don't do anything
    if((*hw)->parent)
    {
      continue;
    }

    //if there is already an entry for the type in type_list increment the number of instances
    for(ti = type_list.begin(); ti != type_list.end(); ++ti)
    {
      if((*ti)->type == (*hw)->type)
      {
        ++((*ti)->num);
        break;
      }
    }
   
    //check if we found an entry for the type in type_list if so continue to the next node
    if(ti != type_list.end())
    {
      continue;
    }

    //if we get here then we found a type we hadn't seen before in the topology.
    //create a new entry for the type set the instance count to 1 and add it to type_list
    type_info_ptr new_type(new type_info());
    new_type->type = (*hw)->type;
    new_type->type_type = type_type;
    new_type->num = 1;
    new_type->grpmaxnum = 0;
    type_list.push_back(new_type);
  }

  //check to see if any of the port capacities violate the max capacity for the type's interfaces
  std::list<link_resource_ptr>::iterator lit;
  for (lit = t->links.begin(); lit != t->links.end(); ++lit)
    {
      if ((*lit)->node1->type != "vgige" && (*lit)->node1->type != "vm")
	{
	  onldb_resp r = get_capacity((*lit)->node1->type);
	  if ((int)(*lit)->node1_capacity > r.result())
	    {
	      std::string errmsg;
	      //errmsg = "node1 in link(" + (*lit)->node1->type + (*lit)->node1->label + ".p" + (*lit)->node1_port + ", "  + (*lit)->node2->type + (*lit)->node2->label + ".p" + (*lit)->node2_port + ") violates bandwidth of interface";
	      errmsg = "node1 in link(" + (*lit)->node1->type + ", "  + (*lit)->node2->type + ") violates bandwidth of interface";
	      return onldb_resp(-1,errmsg);
	    }
	} 
      if ((*lit)->node2->type != "vgige" && (*lit)->node2->type != "vm")
	{
	  onldb_resp r = get_capacity((*lit)->node2->type);
	  if ((int)(*lit)->node2_capacity > r.result())
	    {
	      std::string errmsg;
	      //errmsg = "node2 in link(" + (*lit)->node1->type + (*lit)->node1->label + ".p" + (*lit)->node1_port + ", "  + (*lit)->node2->type + (*lit)->node2->label + ".p" + (*lit)->node2_port + ") violates bandwidth of interface";
	      errmsg = "node2 in link(" + (*lit)->node1->type + ", "  + (*lit)->node2->type + ") violates bandwidth of interface";
	      return onldb_resp(-1,errmsg);
	    }
	}
    }

  //end1 is the first possible end time for the experiment if it started at begin1
  end1_unix = add_time(begin1_unix, len*60);
  end1_db = time_unix2db(end1_unix);

  //end2 is the last possible end time for the experiment if it started at begin2
  end2_unix = add_time(begin2_unix, len*60);
  end2_db = time_unix2db(end2_unix);

  // now start doing policy based checking
  try
  {
    //get the horizon from the policy object in the database.
    //return an error if the horizon is not found
    mysqlpp::Query query = onl->query();
    query << "select value from policy where parameter=" << mysqlpp::quote << "horizon";
    ++db_count;
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "policy database error.. horizon is not in the database";
      return onldb_resp(-1,errmsg);
    }
    policyvals pv = res[0];
    horizon = pv.value;
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }

  time_t horizon_limit_unix = add_time(current_time_unix, horizon*24*60*60);

  //first check that we're not asking for a reservation too far into the future 
  //return error if the first possible experiment end time is beyond the horizon
  if(end1_unix > horizon_limit_unix)
  {
    std::string errmsg;
    errmsg = "requested times too far into the future (currently a " + to_string(horizon) + " day limit)";
    return onldb_resp(0,errmsg);
  }

  //if the last possible experiment endtime is past the horizon
  //adjust the begin2 and end2 times  so that end2 = horizon and begin2 = horizon - length 
  if(end2_unix > horizon_limit_unix)
  {
    end2_unix = horizon_limit_unix;
    end2_db = time_unix2db(end2_unix);
    begin2_unix = sub_time(end2_unix, len*60);
    begin2_db = time_unix2db(begin2_unix);
  }

  //get the reservation lock
  if(lock("reservation") == false) return onldb_resp(0,"database locking problem.. try again later");

  // need to check that all components listed as being in clusters are actually in those clusters
  // and clusters are actual clusters
  onldb_resp vcr = verify_clusters(t);
  if(vcr.result() != 1)
  {
    unlock("reservation");
    return onldb_resp(vcr.result(),vcr.msg());
  }
 
  // based on subsequent checking, the time slots may be broken into discontinuous chunks, so maintain a
  // list of time ranges that are still possible. in almost every case, it will only be one entry, so the
  // overhead should be fairly minimal
  vector<time_range_ptr> possible_times;
  time_range_ptr orig_time(new time_range());
  //create a possible time range instance from the original time range and add it to the list of possible times 
  orig_time->b1_unix = begin1_unix;
  orig_time->b2_unix = begin2_unix;
  orig_time->e1_unix = end1_unix;
  orig_time->e2_unix = end2_unix;
  possible_times.push_back(orig_time);

  vector<time_range_ptr> new_possible_times;
  vector<time_range_ptr>::iterator tr;

  // do per-type checking
  // go through the type_list created earlier and check for per type policy violations
  for(ti = type_list.begin(); ti != type_list.end(); ++ti)
  {
    typepolicyvals tpv;
   
    try
    {
      //check if user is allowed to use this type
      mysqlpp::Query query = onl->query();
      query << "select maxlen,usermaxnum,usermaxusage,grpmaxnum,grpmaxusage from typepolicy where tid=" << mysqlpp::quote << (*ti)->type << " and begin<" << mysqlpp::quote << current_time_db << " and end>" << mysqlpp::quote << current_time_db << " and grp in (select grp from members where user=" << mysqlpp::quote << username << " and prime=1)";
      ++db_count;
      mysqlpp::StoreQueryResult res = query.store();
      if(res.empty())
      {
        std::string errmsg;
        errmsg = "you do not have access to type " + (*ti)->type;
        unlock("reservation");
        return onldb_resp(0,errmsg);
      }
      tpv = res[0];
    }
    catch(const mysqlpp::Exception& er)
    {
      unlock("reservation");
      return onldb_resp(-1,er.what());
    }
 
    //check if request exceeds maximum length allowed for this type 
    if(len > (unsigned int)(tpv.maxlen*60))
    {
      std::string errmsg;
      errmsg = "requested time too long (currently a " + to_string(tpv.maxlen) + " hour limit)";
      unlock("reservation");
      return onldb_resp(0,errmsg);
    }

    //check if request violates the maximum number a user can request of this time at one time
    if((*ti)->num > tpv.usermaxnum)
    {
      std::string errmsg;
      errmsg = "you do not have access to that many " + (*ti)->type + "s (currently a " + to_string(tpv.usermaxnum) + " limit)";
      unlock("reservation");
      return onldb_resp(0,errmsg);
    }

    //now go through list of possible times and check for any times that would violate the type's weekly usage policy (either per group or per user)
    for(tr = possible_times.begin(); tr != possible_times.end(); ++tr)
    {
      //find the start of the week for the first possible start time b1 and the end of the week for the last possible end time e2
      time_t week_start = get_start_of_week((*tr)->b1_unix);
      time_t last_week_start = get_start_of_week((*tr)->e2_unix);
      time_t end_week = add_time(last_week_start,60*60*24*7);
      while(week_start != end_week)
      {
        //look at one week at a time
        time_t this_week_end = add_time(week_start,60*60*24*7);

        int user_usage = 0;
        int grp_usage = 0;
        try
        {
          //ask for the users reservations for the week that use this type
          mysqlpp::Query query = onl->query();
          std::string week_start_db = time_unix2db(week_start);
          std::string week_end_db = time_unix2db(this_week_end);
          if((*ti)->type_type == "hwcluster")
          {
            query << "select begin,end from reservations where user=" << mysqlpp::quote << username << " and state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << week_end_db << " and end> " << mysqlpp::quote << week_start_db << " and rid in ( select hwclusterschedule.rid from hwclusterschedule,hwclusters where hwclusters.tid=" << mysqlpp::quote << (*ti)->type << " and hwclusterschedule.cluster=hwclusters.cluster )";
          }
          else
          {
            query << "select begin,end from reservations where user=" << mysqlpp::quote << username << " and state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << week_end_db << " and end> " << mysqlpp::quote << week_start_db << " and rid in ( select nodeschedule.rid from nodeschedule,nodes where nodes.tid=" << mysqlpp::quote << (*ti)->type << " and nodeschedule.node=nodes.node )";
          }
          vector<restimes> rts;
	  ++db_count;
          query.storein(rts);
    
          vector<restimes>::iterator restime;
          //figure out the amount of time in the week used by each reservation 
          //and add it to the week's total user_usage
          for(restime = rts.begin(); restime != rts.end(); ++restime)
          {
            time_t rb = time_t(restime->begin);
            if(rb < week_start) { rb = week_start; }
            time_t re = time_t(restime->end);
            if(re > this_week_end) { re = this_week_end; }
            user_usage += (re-rb);
          }
          rts.clear();

          onl->query();
          //now ask for reservations for this type, week and group
          if((*ti)->type_type == "hwcluster")
          {
            query << "select begin,end from reservations where state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << week_end_db << " and end> " << mysqlpp::quote << week_start_db << " and rid in ( select hwclusterschedule.rid from hwclusterschedule,hwclusters where hwclusters.tid=" << mysqlpp::quote << (*ti)->type << " and hwclusterschedule.cluster=hwclusters.cluster ) and user in ( select user from members where prime=1 and grp in (select grp from members where prime=1 and user=" << mysqlpp::quote << username << "))";
          }
          else
          {
            query << "select begin,end from reservations where state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << week_end_db << " and end> " << mysqlpp::quote << week_start_db << " and rid in ( select nodeschedule.rid from nodeschedule,nodes where nodes.tid=" << mysqlpp::quote << (*ti)->type << " and nodeschedule.node=nodes.node ) and user in ( select user from members where prime=1 and grp in (select grp from members where prime=1 and user=" << mysqlpp::quote << username << "))";
          }
	  ++db_count;
          query.storein(rts);

          //figure out the amount of time in the week used by each reservation 
          //and add it to the week's total group_usage
          for(restime = rts.begin(); restime != rts.end(); ++restime)
          {
            time_t rb = time_t(restime->begin);
            if(rb < week_start) { rb = week_start; }
            time_t re = time_t(restime->end);
            if(re > this_week_end) { re = this_week_end; }
            grp_usage += (re-rb);
          }

        }
        catch(const mysqlpp::Exception& er)
        {
          unlock("reservation");
          return onldb_resp(-1,er.what());
        }
       
        //figure out the maximum amount of time in the considered week, the requested experiment could use if it
        //was scheduled in this possible time range 
        int max_week_length;
        if((*tr)->b1_unix < week_start && (*tr)->e2_unix > this_week_end)
        {
          max_week_length = std::min(len, (unsigned int)7*24*60);
        }
        else if((*tr)->b1_unix < week_start)
        {
          max_week_length = std::min(len, (unsigned int)((*tr)->e2_unix - week_start)/60);
        }
        else if((*tr)->e2_unix > this_week_end)
        {
          max_week_length = std::min(len, (unsigned int)(this_week_end - (*tr)->b1_unix)/60);
        }
        else
        {
          max_week_length = len;
        }
        //calculate the new potential user and group usage for the week if an experiment
        //was scheduled in this possible range
        int new_user_usage = (user_usage/60) + (max_week_length * ((*ti)->num));
        int new_grp_usage = (grp_usage/60) + (max_week_length * ((*ti)->num));

        // if the usage would be over the limit, then remove this week from the possible times,
        // potentially with some stuff at the beginning and end if some hours were left for the week
        if(new_user_usage > (tpv.usermaxusage*60) || new_grp_usage > (tpv.grpmaxusage*60))
        {
          int user_slack_seconds = (tpv.usermaxusage*60*60) - user_usage;
          int grp_slack_seconds = (tpv.grpmaxusage*60*60) - grp_usage;
          int slack_seconds = std::min(user_slack_seconds, grp_slack_seconds);
          if(slack_seconds < 0) { slack_seconds = 0; }
          time_t adjusted_week_start = add_time(week_start,slack_seconds);
          time_t adjusted_week_end = sub_time(this_week_end,slack_seconds);
          if((*tr)->b1_unix < adjusted_week_start)
          {
            if(adjusted_week_start >= (*tr)->e1_unix)
            {
              time_range_ptr new_time(new time_range());
              new_time->b1_unix = (*tr)->b1_unix;
              new_time->e1_unix = (*tr)->e1_unix;
              new_time->b2_unix = sub_time((*tr)->b2_unix, ((*tr)->e2_unix) - adjusted_week_start);
              new_time->e2_unix = adjusted_week_start;
              new_possible_times.push_back(new_time);
            }
          }
          if((*tr)->e2_unix > adjusted_week_end)
          {
            if(adjusted_week_end <= (*tr)->b2_unix)
            {
              (*tr)->e1_unix = add_time((*tr)->e1_unix, (adjusted_week_end - (*tr)->b1_unix));
              (*tr)->b1_unix = adjusted_week_end;
            }
          }
        }
        else if((*tr)->e2_unix < this_week_end)
        {
          time_range_ptr new_time(new time_range());
          new_time->b1_unix = (*tr)->b1_unix;
          new_time->e1_unix = (*tr)->e1_unix;
          new_time->b2_unix = (*tr)->b2_unix;
          new_time->e2_unix = (*tr)->e2_unix;
          new_possible_times.push_back(new_time);
        }

        week_start = add_time(week_start,60*60*24*7);
      }
    }

    possible_times.clear();
    possible_times = new_possible_times;
    new_possible_times.clear();
    
    // grpmaxnum has to be done on a per time slot basis, so save it here for use later
    (*ti)->grpmaxnum = tpv.grpmaxnum;
  }

  //if no possible times are left then either the group or user usage policy would be violated 
  //by a reservation in the requested range.
  if(possible_times.size() == 0)
  {
    unlock("reservation");
    return onldb_resp(0,(std::string)"your usage during that time period is already maxed out");
  }


  if (username == JDD || username == JP) {
    cout << "Warning: Making reservation for " << username << ": testing for overlap" << endl;
  }
  // now go through each time frame and remove any time where the user already has a reservation
  for(tr = possible_times.begin(); tr != possible_times.end(); ++tr)
  {

    try
    {
      //first get the list of the user's reservations which overlap the possible range
      //i.e. all of the user's reservations that begin before the last possible end time e2
      //and end after the first possible start time b1
      mysqlpp::Query query = onl->query();
      std::string b1_db = time_unix2db((*tr)->b1_unix);
      std::string e2_db = time_unix2db((*tr)->e2_unix);
      query << "select begin,end from reservations where user=" << mysqlpp::quote << username << " and state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << e2_db << " and end>" << mysqlpp::quote << b1_db << " order by begin";
      vector<restimes> rts;
      ++db_count;
      query.storein(rts);
 
      //if there are no overlapping reservations in this range then add this range to the new list of possible ranges 
      if(rts.empty())
      {
        if (username == JDD || username == JP) {
	cout << "Warning: " << username << ": found no reservations for: b1_db: " << b1_db << "e2_db: " << e2_db << endl;
        }
        time_range_ptr new_time(new time_range());
        new_time->b1_unix = (*tr)->b1_unix;
        new_time->e1_unix = (*tr)->e1_unix;
        new_time->b2_unix = (*tr)->b2_unix;
        new_time->e2_unix = (*tr)->e2_unix;
        new_possible_times.push_back(new_time);
      }
      else //otw we found some overlapping reservations
      {
        // each time in here overlaps the time range in tr
        bool add_left_over = false;
        vector<restimes>::iterator restime;
        //if (username == JDD || username == JP) {
        //cout << "Warning: " << username << ": found reservations for: b1_db: " << b1_db << "e2_db: " << e2_db << endl;}
       
        //look at each overlapping reservation and find any unused portions of the possible range that we 
        //could schedule the experiment 
        for(restime = rts.begin(); restime != rts.end(); ++restime)
        {
          // JDD: Added 9/9/10: seems like add_left_over needs to be reset each time we iterate in this loop
          // JDD: That way only if something is left at the very end is it an acceptable time.
          //
          //add_left_over = false;

          // JDD: restime is an existing reservation time
          // JDD: rb is the begin time of current reservation we are testing against.
          // JDD: re is the  end  time of current reservation we are testing against.
          // JDD: tr is a possible time frame -- still not sure exactly what that means.
          time_t rb = time_t(restime->begin);
          time_t re = time_t(restime->end);
          if (username == JDD || username == JP) {
            cout << "Warning: " << username << ": testing restime->begin: " << restime->begin << "restime->end: " << restime->end << endl;
            {
              std::string s = time_unix2db((*tr)->b1_unix);
              std::string e = time_unix2db((*tr)->e1_unix);
              cout << "Warning: " << username << ": testing (*tr)->b1_unix: " << s << "(*tr)->e1_unix: " << e << endl;
            }
            {
              std::string s = time_unix2db((*tr)->b2_unix);
              std::string e = time_unix2db((*tr)->e2_unix);
              cout << "Warning: " << username << ": testing (*tr)->b2_unix: " << s << "(*tr)->e2_unix: " << e << endl;
            }
          }
 
          // if the reservation begin time is after both the begin and end time of the time frame (tr) being checked,
          // there is some time before the reservation that is a possible time to schedule the experiment
          if((*tr)->b1_unix < rb && rb >= (*tr)->e1_unix)
          {
            time_range_ptr new_time(new time_range());
            //create a new possible time range with the same b1 and e1 as the time range considered
            //the last possible end time e2 = the start of the reservation and b2 = the start of the reservation - length
            new_time->b1_unix = (*tr)->b1_unix;
            new_time->e1_unix = (*tr)->e1_unix;
            new_time->b2_unix = sub_time((*tr)->b2_unix, ((*tr)->e2_unix) - rb);
            new_time->e2_unix = rb;
            if (username == JDD || username == JP) {
              cout << "Warning: " << username << ": calling new_possible_times.push_back()" << endl;
            }
            //add the new time range to the new list of possible times
            new_possible_times.push_back(new_time);
          }
          // if the reservation ends before the last end time e2 or the last begin time of the time frame (tr) being checked,
          // there is some time after the reservation that is a possible time to schedule the experiment
          if((*tr)->e2_unix > re && re <= (*tr)->b2_unix)
          {
            //change the current time range currently considered to have the first possible start time (b1) be the 
            //end of the reservation (re) and the first possible end time (e1) be the end of the reservation plus 
            //the length of the experiment
            (*tr)->e1_unix = add_time((*tr)->e1_unix, (re - (*tr)->b1_unix));
            (*tr)->b1_unix = re;
            if (username == JDD || username == JP) {
              cout << "Warning: " << username << ": setting add_left_over = true" << endl;
              {
                std::string s = time_unix2db((*tr)->b1_unix);
                std::string e = time_unix2db((*tr)->e1_unix);
                cout << "Warning: " << username << ": NEW (*tr)->b1_unix: " << s << "(*tr)->e1_unix: " << e << endl;
              }
            }
            //mark this to add after we've considered all of the reservations
            //we may find a later reservation that makes this range no longer possible
            add_left_over = true;
          }
         //JP: added 11_21_11 to fix duplicate reservations
         //if the reservation begins before the first possible end (e1) for the range and ends after the last possible start
         //if the reservation we're testing makes the current range unusable there's no point looking at the rest of the reservations
         if ((*tr)->e1_unix >= rb && (*tr)->b2_unix < re)
         {
           //if add_left_over was set to true at this point, we need to set add_left_over to false
           //so we don't accidentally add a range that's unusable
           add_left_over = false;
           break;
         }
        }//end of loop testing overlapping reservations for the user

        //if the possible range was modified while processing the overlapping requests
        //and the range is still viable we should add it to the new possible times
        if(add_left_over)
        {
          time_range_ptr new_time(new time_range());
          new_time->b1_unix = (*tr)->b1_unix;
          new_time->e1_unix = (*tr)->e1_unix;
          new_time->b2_unix = (*tr)->b2_unix;
          new_time->e2_unix = (*tr)->e2_unix;
          if (username == JDD || username == JP) {
            cout << "Warning: " << username << ": in if (add_left_over) calling new_possible_times.push_back() " << endl;
          }
          new_possible_times.push_back(new_time);
        }
      }
    }
    catch(const mysqlpp::Exception& er)
    {
      unlock("reservation");
      return onldb_resp(-1,er.what());
    }
  }

  possible_times.clear();
  possible_times = new_possible_times;
  new_possible_times.clear();

  //if we don't have any possible times left then the user already has blocking reservations during
  //the requested time period    
  if(possible_times.size() == 0)
  {
    unlock("reservation");
    return onldb_resp(0,(std::string)"you already have reservations during that time period");
  }

  // don't forget to check grpmaxnum from typepolicy..
  
  //std::cout << "current db time: " << current_time_db << std::endl << std::endl;
  //iterate through the possible time ranges to see if we can make a reservation
  for(tr = possible_times.begin(); tr != possible_times.end(); ++tr) 
  {
    //std::cout << "begin1 db time: " << time_unix2db((*tr)->b1_unix) << std::endl;
    //std::cout << "begin2 db time: " << time_unix2db((*tr)->b2_unix) << std::endl;
    //std::cout << "end1 db time: " << time_unix2db((*tr)->e1_unix) << std::endl;
    //std::cout << "end2 db time: " << time_unix2db((*tr)->e2_unix) << std::endl << std::endl;


    //get a list of any reservations overlapping the current time range.
    //i.e. reservations that begin before the last possible end(e2) and end after the first possible begin(b1)
    mysqlpp::Query query = onl->query();
    std::string b1_db = time_unix2db((*tr)->b1_unix);
    std::string e2_db = time_unix2db((*tr)->e2_unix);
    query << "select begin,end from reservations where state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << e2_db << " and end>" << mysqlpp::quote << b1_db << " order by begin";
    vector<restimes> rts;
    ++db_count;
    query.storein(rts);

    //go through all the overlapping reservations and mark the beginning and end of each reservation as a point of interest
    //in other words this is where resource availability might change freeing up resources that might not have been available before
    //or using resources that were available
    vector<restimes>::iterator restime;
    for(restime = rts.begin(); restime != rts.end(); ++restime)
    {
      time_t rb = time_t(restime->begin);
      time_t re = time_t(restime->end);
      if(rb > (*tr)->b1_unix && rb < (*tr)->e2_unix)
      { 
        (*tr)->times_of_interest.push_back(rb);
      }
      if(re > (*tr)->b1_unix && re < (*tr)->e2_unix)
      { 
        (*tr)->times_of_interest.push_back(re);
      }
    }

    (*tr)->times_of_interest.sort();
    (*tr)->times_of_interest.unique();

    std::list<time_t>::iterator toi_start = (*tr)->times_of_interest.begin();
    std::list<time_t>::iterator toi_end = (*tr)->times_of_interest.begin();
    //set the times of interest end iterator to the first time that is >= the first possible end time
    while(toi_end != (*tr)->times_of_interest.end() && *toi_end < (*tr)->e1_unix) ++toi_end;
 
    time_t cur_start, cur_end;
    unsigned int increment = (60/divisor)*60;
    bool changed = true;
    //first check if this a valid topology
    cur_start = (*tr)->b1_unix;
    cur_end = (*tr)->e1_unix;
    onldb_resp trr = try_reservation(t, username, time_unix2db(cur_start), time_unix2db(cur_end), "pending", true);
    if(trr.result() != 1)
      {
	cout << username << ": unmappable topology" << endl;
	unlock("reservation");
	return onldb_resp(-1,"Unable to map topology. If this is a class, see your TA. Otherwise, contact ONL testbedops");
      }
    //look at times starting at the first possible beginning time(b1) and incrementing by the time slot interval set by the policy
    //try to make a reservation for the first possible time and then each time we hit or pass a time of interest
    //return success if we successfully make a reservation
    //stop and return an error if we get to the end of the range and haven't successfully made a reservation
    for(cur_start = (*tr)->b1_unix, cur_end = (*tr)->e1_unix; cur_start <= (*tr)->b2_unix; cur_start = add_time(cur_start, increment), cur_end = add_time(cur_end, increment))
    {
      if (username == JDD || username == JP) {
        // JDD
        std::string s = time_unix2db(cur_start);
        std::string e = time_unix2db(cur_end);
        cout << "Warning: " << username << ": testing cur_start: " << s << "cur_end: " << e << endl;
      }

      if(toi_start != (*tr)->times_of_interest.end() && *toi_start <= cur_start)
      //if(toi_start != (*tr)->times_of_interest.end() && *toi_start < cur_start) //JP changed to fix bug of reservation gaps
      {
        changed = true;
        ++toi_start;
      }
      if(toi_end != (*tr)->times_of_interest.end() && *toi_end <= cur_end)
      //if(toi_end != (*tr)->times_of_interest.end() && *toi_end < cur_end)//JP changed to fix bug of reservation gaps
      {
        changed = true;
        ++toi_end;
      }
      if(changed)
      {
        onldb_resp trr = try_reservation(t, username, time_unix2db(cur_start), time_unix2db(cur_end));
        if(trr.result() == 1)
        {
          std::string s = time_unix2db(cur_start);
          unlock("reservation");
	  print_diff("onldb::make_reservation", stime);
	  report_metrics(t, username, cur_start, cur_end, current_time);
          return onldb_resp(1,s);
        }
        if(trr.result() < 0)
        {
          unlock("reservation");
          return onldb_resp(-1,trr.msg());
        }
        changed = false;
      }
    }
  }

  unlock("reservation");
  cout << "reservation(" << username << ") failed. time_to_compute = " << difftime(time(NULL), current_time) << " database calls = " << db_count <<  " function time = " << make_res_time << endl;
  db_count = 0;
  make_res_time = 0;
  return onldb_resp(0,(std::string)"too many resources used by others during those times");
}

onldb_resp onldb::extend_current_reservation(std::string username, int min) throw()
{
  unsigned int divisor;
  time_t current_time_unix;
  time_t begin_unix;
  time_t end_unix;
  std::string current_time_db;
  std::string begin_db;
  std::string end_db;
  unsigned int rid;
  time_t new_end_unix;
  std::string new_end_db;

  try
  {
    mysqlpp::Query query = onl->query();
    query << "select value from policy where parameter=" << mysqlpp::quote << "divisor";
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "policy database error.. divisor is not in the database";
      return onldb_resp(-1,errmsg);
    }
    policyvals pv = res[0];
    divisor = pv.value;
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }

  current_time_unix = time(NULL);
  current_time_unix = discretize_time(current_time_unix, divisor);
  current_time_db = time_unix2db(current_time_unix);

  if(min <= 0)
  {
    std::string errmsg;
    errmsg = "length of " + to_string(min) + " minutes does not make sense";
    return onldb_resp(0,errmsg);
  }
 
  unsigned int chunk = 60/divisor;
  min = (((min-1) / chunk)+1) * chunk;

  if(lock("reservation") == false) return onldb_resp(0,"database locking problem.. try again later");

  // get the existing reservation
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select begin,end,rid from reservations where user=" << mysqlpp::quote << username << " and begin<=" << mysqlpp::quote << current_time_db << " and end>=" << mysqlpp::quote << current_time_db << " and state!='timedout' and state!='cancelled'";
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg;
      errmsg = "you do not have a current reservation";
      unlock("reservation");
      return onldb_resp(0,errmsg);
    }
    curresinfo curres = res[0];
    begin_unix = time_db2unix(curres.begin);
    end_unix = time_db2unix(curres.end);
    begin_db = time_unix2db(begin_unix);
    end_db = time_unix2db(end_unix);
    rid = curres.rid;
  }
  catch(const mysqlpp::Exception& er)
  {
    unlock("reservation");
    return onldb_resp(-1,er.what());
  }
  
  new_end_unix = add_time(end_unix, min*60);
  new_end_db = time_unix2db(new_end_unix);
  
  unsigned int len = (new_end_unix - current_time_unix)/60;

  topology res_top;
  onldb_resp r = get_topology(&res_top, rid);
  if(r.result() < 1)
  {
    unlock("reservation");
    return onldb_resp(r.result(),r.msg());
  }

  vector<type_info_ptr> type_list;
  vector<type_info_ptr>::iterator ti;
  list<node_resource_ptr>::iterator hw;
  list<link_resource_ptr>::iterator link;

  // build a vector of type information for each type that is represented in the topology.
  for(hw = res_top.nodes.begin(); hw != res_top.nodes.end(); ++hw)
  {
    std::string type_type = get_type_type((*hw)->type);
    if(type_type == "")
    {
      std::string errmsg;
      errmsg = "type " + (*hw)->type + " not in the database";
      unlock("reservation");
      return onldb_resp(-1,errmsg);
    }

    if((*hw)->parent)
    {
      continue;
    }

    for(ti = type_list.begin(); ti != type_list.end(); ++ti)
    {
      if((*ti)->type == (*hw)->type)
      {
        ++((*ti)->num);
        break;
      }
    }
    if(ti != type_list.end())
    {
      continue;
    }

    type_info_ptr new_type(new type_info());
    new_type->type = (*hw)->type;
    new_type->type_type = type_type;
    new_type->num = 1;
    new_type->grpmaxnum = 0;
    type_list.push_back(new_type);
  }

  // do per-type checking
  for(ti = type_list.begin(); ti != type_list.end(); ++ti)
  {
    typepolicyvals tpv;
   
    try
    {
      mysqlpp::Query query = onl->query();
      query << "select maxlen,usermaxnum,usermaxusage,grpmaxnum,grpmaxusage from typepolicy where tid=" << mysqlpp::quote << (*ti)->type << " and begin<" << mysqlpp::quote << current_time_db << " and end>" << mysqlpp::quote << current_time_db << " and grp in (select grp from members where user=" << mysqlpp::quote << username << " and prime=1)";
      mysqlpp::StoreQueryResult res = query.store();
      if(res.empty())
      {
        std::string errmsg;
        errmsg = "you do not have access to type " + (*ti)->type;
        unlock("reservation");
        return onldb_resp(0,errmsg);
      }
      tpv = res[0];
    }
    catch(const mysqlpp::Exception& er)
    {
      unlock("reservation");
      return onldb_resp(-1,er.what());
    }

    if(len > (unsigned int)(tpv.maxlen*60))
    {
      std::string errmsg;
      errmsg = "requested time too long (currently a " + to_string(tpv.maxlen) + " hour limit)";
      unlock("reservation");
      return onldb_resp(0,errmsg);
    }

    time_t week_start = get_start_of_week(current_time_unix);
    time_t last_week_start = get_start_of_week(new_end_unix);
    time_t end_week = add_time(last_week_start,60*60*24*7);
    while(week_start != end_week)
    {
      time_t this_week_end = add_time(week_start,60*60*24*7);

      int user_usage = 0;
      int grp_usage = 0;
      try
      {
        mysqlpp::Query query = onl->query();
        std::string week_start_db = time_unix2db(week_start);
        std::string week_end_db = time_unix2db(this_week_end);
        if((*ti)->type_type == "hwcluster")
        {
          query << "select begin,end from reservations where user=" << mysqlpp::quote << username << " and state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << week_end_db << " and end> " << mysqlpp::quote << week_start_db << " and rid in ( select hwclusterschedule.rid from hwclusterschedule,hwclusters where hwclusters.tid=" << mysqlpp::quote << (*ti)->type << " and hwclusterschedule.cluster=hwclusters.cluster )";
        }
        else
        {
          query << "select begin,end from reservations where user=" << mysqlpp::quote << username << " and state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << week_end_db << " and end> " << mysqlpp::quote << week_start_db << " and rid in ( select nodeschedule.rid from nodeschedule,nodes where nodes.tid=" << mysqlpp::quote << (*ti)->type << " and nodeschedule.node=nodes.node )";
        }
        vector<restimes> rts;
        query.storein(rts);
        vector<restimes>::iterator restime;
        for(restime = rts.begin(); restime != rts.end(); ++restime)
        {
          time_t rb = time_t(restime->begin);
          if(rb < week_start) { rb = week_start; }
          time_t re = time_t(restime->end);
          if(re > this_week_end) { re = this_week_end; }
          user_usage += (re-rb);
        }
        rts.clear();

        onl->query();
        if((*ti)->type_type == "hwcluster")
        {
          query << "select begin,end from reservations where state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << week_end_db << " and end> " << mysqlpp::quote << week_start_db << " and rid in ( select hwclusterschedule.rid from hwclusterschedule,hwclusters where hwclusters.tid=" << mysqlpp::quote << (*ti)->type << " and hwclusterschedule.cluster=hwclusters.cluster ) and user in ( select user from members where prime=1 and grp in (select grp from members where prime=1 and user=" << mysqlpp::quote << username << "))";
        }
        else
        {
          query << "select begin,end from reservations where state!='cancelled' and state!='timedout' and begin<" << mysqlpp::quote << week_end_db << " and end> " << mysqlpp::quote << week_start_db << " and rid in ( select nodeschedule.rid from nodeschedule,nodes where nodes.tid=" << mysqlpp::quote << (*ti)->type << " and nodeschedule.node=nodes.node ) and user in ( select user from members where prime=1 and grp in (select grp from members where prime=1 and user=" << mysqlpp::quote << username << "))";
        }
        query.storein(rts);
        for(restime = rts.begin(); restime != rts.end(); ++restime)
        {
          time_t rb = time_t(restime->begin);
          if(rb < week_start) { rb = week_start; }
          time_t re = time_t(restime->end);
          if(re > this_week_end) { re = this_week_end; }
          grp_usage += (re-rb);
        }
      }
      catch(const mysqlpp::Exception& er)
      {
        unlock("reservation");
        return onldb_resp(-1,er.what());
      }

      int max_week_length;
      if(current_time_unix < week_start && new_end_unix > this_week_end)
      {
        max_week_length = std::min(len, (unsigned int)7*24*60);
      }
      else if(current_time_unix < week_start)
      {
        max_week_length = std::min(len, (unsigned int)(new_end_unix - week_start)/60);
      }
      else if(new_end_unix > this_week_end)
      {
        max_week_length = std::min(len, (unsigned int)(this_week_end - current_time_unix)/60);
      }
      else
      {
        max_week_length = len;
      }
      int new_user_usage = (user_usage/60) + (max_week_length * ((*ti)->num));
      int new_grp_usage = (grp_usage/60) + (max_week_length * ((*ti)->num));

      if(new_user_usage > (tpv.usermaxusage*60) || new_grp_usage > (tpv.grpmaxusage*60))
      {
        unlock("reservation");
        return onldb_resp(0,(std::string)"your usage during this time period is already maxed out");
      }
      week_start = add_time(week_start,60*60*24*7);
    }
  }

  onldb_resp ra = is_admin(username);
  if(ra.result() < 0)
  {
    unlock("reservation");
    return onldb_resp(ra.result(),ra.msg());
  }
  bool admin = false;
  if(ra.result() == 1) admin = true;

  try
  {
    mysqlpp::Query query = onl->query();
    query << "select begin,end from reservations where user=" << mysqlpp::quote << username << " and state!='cancelled' and state!='timedout' and begin<=" << mysqlpp::quote << new_end_db << " and end>" << mysqlpp::quote << end_db;
    vector<restimes> rts;
    query.storein(rts);
  
    if(!rts.empty())
    {
      unlock("reservation");
      return onldb_resp(0,(std::string)"you already have a reservation during that time period");
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    unlock("reservation");
    return onldb_resp(-1,er.what());
  }

  for(hw = res_top.nodes.begin(); hw != res_top.nodes.end(); ++hw)
  {
    if((*hw)->parent) continue;
    if((*hw)->type == "vgige") continue;
    
    try
    {
      mysqlpp::Query query = onl->query();
      if((*hw)->is_parent)
      {
        if(admin == true)
        {
          query << "select rid from hwclusterschedule where cluster=" << mysqlpp::quote << (*hw)->node << " and rid in (select rid from reservations where state!='cancelled' and state!='timedout' and user!='system' and user!='testing' and user!='repair' and begin<=" << mysqlpp::quote << new_end_db << " and end>" << mysqlpp::quote << end_db << ")";
        }
        else
        {
          query << "select rid from hwclusterschedule where cluster=" << mysqlpp::quote << (*hw)->node << " and rid in (select rid from reservations where state!='cancelled' and state!='timedout' and begin<=" << mysqlpp::quote << new_end_db << " and end>" << mysqlpp::quote << end_db << ")";
        }
      }
      else
      {
        if(admin == true)
        {
          query << "select rid from nodeschedule where node=" << mysqlpp::quote << (*hw)->node << " and rid in (select rid from reservations where state!='cancelled' and state!='timedout' and user!='system' and user!='testing' and user!='repair' and begin<=" << mysqlpp::quote << new_end_db << " and end>" << mysqlpp::quote << end_db << ")";
        }
        else
        {
          query << "select rid from nodeschedule where node=" << mysqlpp::quote << (*hw)->node << " and rid in (select rid from reservations where state!='cancelled' and state!='timedout' and begin<=" << mysqlpp::quote << new_end_db << " and end>" << mysqlpp::quote << end_db << ")";
        }
      }
      mysqlpp::StoreQueryResult res = query.store();
      if(!res.empty())
      {
        unlock("reservation");
        return onldb_resp(0,(std::string)"resources used by others during that time");
      }
    }
    catch(const mysqlpp::Exception& er)
    {
      unlock("reservation");
      return onldb_resp(-1,er.what());
    }
  }
  
  // check capacity along all switch->switch links to make sure there's enough capacity
  // to extend this res.  returns 1 if there is enough
  onldb_resp bwr = check_interswitch_bandwidth(&res_top, end_db, new_end_db);
  if(bwr.result() < 1)
  {
    unlock("reservation");
    return onldb_resp(bwr.result(),bwr.msg());
  }

  try
  {
    mysqlpp::Query up = onl->query();
    up << "update reservations set end=" << mysqlpp::quote << new_end_db << " where rid=" << mysqlpp::quote << rid;
    up.execute();
  }
  catch(const mysqlpp::Exception& er)
  {
    unlock("reservation");
    return onldb_resp(-1,er.what());
  }

  unlock("reservation");
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::fix_component(topology *t, unsigned int label, std::string node) throw()
{
  list<node_resource_ptr>::iterator nit;
  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    if((*nit)->label == label)
    {
      (*nit)->node = node;
      (*nit)->fixed = true;
      break;
    }
  }
  if(nit == t->nodes.end()) return onldb_resp(0,"no such label");

  if((*nit)->parent) 
  {
    std::string parent = "";
    try
    {
      mysqlpp::Query query = onl->query();
      query << "select cluster from hwclustercomps where node=" << mysqlpp::quote << node;
      mysqlpp::StoreQueryResult res = query.store();
      if(res.empty()) return onldb_resp(0,(std::string)"topology error");
      clustercluster c = res[0];
      parent = c.cluster;
    }
    catch(const mysqlpp::Exception& er)
    {
      return onldb_resp(-1,er.what());
    }

    (*nit)->parent->node = parent;
    (*nit)->parent->fixed = true;
  }

  return onldb_resp(1,"success");
}

onldb_resp onldb::cancel_current_reservation(std::string username) throw()
{
  if(lock("reservation") == false) return onldb_resp(0,"database locking problem.. try again later");
  try
  {
    std::string current_time = time_unix2db(time(NULL));
    mysqlpp::Query can = onl->query();
    can << "update reservations set state='cancelled' where user=" << mysqlpp::quote << username << " and begin<" << mysqlpp::quote << current_time << " and end>" << mysqlpp::quote << current_time << " and (state='pending' or state='used')";
    can.execute();
  }
  catch(const mysqlpp::Exception& er)
  {
    unlock("reservation");
    return onldb_resp(-1,er.what());
  }
  unlock("reservation");
  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::has_reservation(std::string username) throw()
{
  if(lock("reservation") == false) return onldb_resp(0,"database locking problem.. try again later");
  try
  {
    std::string current_time = time_unix2db(time(NULL));
    mysqlpp::Query has = onl->query();
    has << "select end from reservations where user=" << mysqlpp::quote << username << " and begin<" << mysqlpp::quote << current_time << " and end>" << mysqlpp::quote << current_time << " and (state='active' or state='pending' or state='used')";
    mysqlpp::StoreQueryResult res = has.store();
    if(res.empty())
    {
      unlock("reservation");
      return onldb_resp(-1,(std::string)"no reservation");
    }
    resend res_end = res[0];
    std::string end_time = res_end.end.str();
    unlock("reservation");
    int seconds_left = time_db2unix(end_time) - time_db2unix(current_time);
    if(seconds_left > 0) return onldb_resp(seconds_left/60,(std::string)"success");
    return onldb_resp(0,(std::string)"no time left");
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,er.what());
  }
}

onldb_resp onldb::assign_resources(std::string username, topology *t) throw()
{
  if(lock("reservation") == false) return onldb_resp(0,"database locking problem.. try again later");

  std::string current_time = time_unix2db(time(NULL));
  try
  {
    // first verify that the user has a reservation right now
    mysqlpp::Query query = onl->query();
    query << "select rid,state from reservations where user=" << mysqlpp::quote << username << " and begin<" << mysqlpp::quote << current_time << " and end>" << mysqlpp::quote << current_time << " and (state='pending' or state='active' or state='used')";
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg = "reservation not found";
      unlock("reservation");
      return onldb_resp(0,errmsg);
    }
    resinfo ri = res[0];

    // verify that the topology clusters are correct
    onldb_resp r1 = verify_clusters(t);
    if(r1.result() < 1)
    {
      unlock("reservation");
      return onldb_resp(0,r1.msg());
    }

    // now fill in the physical resources from the reservation 
    onldb_resp r2 = fill_in_topology(t, ri.rid);
    if(r2.result() < 1)
    {
      unlock("reservation");
      return onldb_resp(0,r2.msg());
    }

    // update the reservation state to show that it is active now
    resinfo orig_ri = ri;
    ri.state = "active";
    query.update(orig_ri, ri);
    query.execute();

    mysqlpp::DateTime ct(current_time);
    // add an experiment for this reservation
    experimentins exp(ct, ct, ri.rid);
    mysqlpp::Query ins = onl->query();
    ins.insert(exp);
    ins.execute();
  }
  catch(const mysqlpp::Exception& er)
  {
    unlock("reservation");
    return onldb_resp(-1,er.what());
  }
  unlock("reservation");

  // now update the soft system state, to add the user to the correct SSH ACLs
  // and export their home area to their nodes
  // note that the script checks for "unused" arguments and ignores them
  std::list<node_resource_ptr>::iterator nit;
  bool use_exportfs = false;

  #ifdef USE_EXPORTFS
  use_exportfs = true;
  #endif

  if (username.compare("jdd") == 0)
    {
      for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
	{
	  if ((*nit)->node.compare("pc48core01") == 0)
	    {
	      use_exportfs = true;
	      break;
	    }
	}
    }
  //#ifdef USE_EXPORTFS
  std::string exportList;
  //#endif
  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    //#ifdef USE_EXPORTFS
    if (use_exportfs && ((*nit)->cp).compare("unused") && !(*nit)->root_only) {
      exportList.append((*nit)->cp);
      exportList.append(":/users/");
      exportList.append(username);
      exportList.append(" ");
    }
    //#endif

    if (!(*nit)->root_only)
      {
	std::string cmd = "/usr/testbed/scripts/system_session_update2.pl add " + username + " " + (*nit)->cp + " " + (*nit)->acl;
	int ret = system(cmd.c_str());
	if(ret < 0 || WEXITSTATUS(ret) != 1) cout << "Warning: " << username << "'s home area was not exported to " << (*nit)->cp << " and user was not added to group " << (*nit)->acl << endl;
      }
  }
  //#ifdef USE_EXPORTFS
  if (use_exportfs)
    cout << "exportList: > " << exportList << " < " << endl;
  //#endif

  //#ifdef USE_EXPORTFS
  if (use_exportfs)
    {
      std::string cmd = "/usr/sbin/exportfs -o rw,sync,no_root_squash " + exportList;
      int ret = system(cmd.c_str());
      if(ret < 0 || ((ret !=0) && WEXITSTATUS(ret) != 1)) cout << "Warning: " << " exportfs failed (ret = " << ret << ") for " << cmd << endl;
      else cout << "exportfs succeeded for " << cmd << endl;
    }
  //#endif
  {
    std::string cmd = "/usr/testbed/scripts/system_session_update2.pl update";
    int ret = system(cmd.c_str());
    if(ret < 0 || WEXITSTATUS(ret) != 1) cout << "Warning: " << username << " was not added to any groups" << endl;
  }

  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::return_resources(std::string username, topology *t) throw()
{
  if(lock("reservation") == false) return onldb_resp(0,"database locking problem.. try again later");

  std::string current_time = time_unix2db(time(NULL));
  try
  {
    // get the active experiment for this user's active reservation
    mysqlpp::Query query = onl->query();
    query << "select eid,begin,end,rid from experiments where begin=end and rid in (select rid from reservations where user=" << mysqlpp::quote << username << " and state='active')";
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      std::string errmsg = "experiment not found";
      unlock("reservation");
      return onldb_resp(-1,errmsg);
    }
    experiments exp = res[0];

    // update the experiment to have the correct endtime
    experiments orig_exp = exp;
    mysqlpp::sql_datetime cur_time(current_time);
    exp.end = cur_time;
    query.update(orig_exp, exp);
    query.execute();

    // update the reservation to be used
    mysqlpp::Query resup = onl->query();
    resup << "update reservations set state='used' where rid=" << mysqlpp::quote << orig_exp.rid;
    resup.execute();
  }
  catch(const mysqlpp::Exception& er)
  {
    unlock("reservation");
    return onldb_resp(-1,er.what());
  }
  unlock("reservation");

  // now remove all the soft system state that was added in assign_resources
  std::list<node_resource_ptr>::iterator nit;
  bool use_exportfs = false;

  #ifdef USE_EXPORTFS
  use_exportfs = true;
  #endif

  if (username.compare("jdd") == 0)
    {
      for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
	{
	  if ((*nit)->node.compare("pc48core01") == 0)
	    {
	      use_exportfs = true;
	      break;
	    }
	}
    }
  //#ifdef USE_EXPORTFS
  std::string exportList;
  //#endif
  for(nit = t->nodes.begin(); nit != t->nodes.end(); ++nit)
  {
    std::string cmd = "/usr/testbed/scripts/system_session_update2.pl remove " + username + " " + (*nit)->cp + " " + (*nit)->acl;
    //#ifdef USE_EXPORTFS
    if (use_exportfs && ((*nit)->cp).compare("unused")) {
      exportList.append((*nit)->cp);
      exportList.append(":/users/");
      exportList.append(username);
      exportList.append(" ");
    }
    //#endif
    int ret = system(cmd.c_str());
    if(ret < 0 || WEXITSTATUS(ret) != 1) cout << "Warning: " << username << "'s home area was not unexported to " << (*nit)->cp << " and user was not removed from group " << (*nit)->acl << endl;
  }
  //#ifdef USE_EXPORTFS
  if (use_exportfs)
    cout << "exportList: > " << exportList << " < " << endl;
  //#endif
  {
    std::string cmd = "/usr/testbed/scripts/system_session_update2.pl update";
    int ret = system(cmd.c_str());
    if(ret < 0 || WEXITSTATUS(ret) != 1) cout << "Warning: " << username << " was not removed from any groups" << endl;
  }

  //#ifdef USE_EXPORTFS
  if (use_exportfs)
    {
      std::string cmd = "/usr/sbin/exportfs -u " + exportList;
      int ret = system(cmd.c_str());
      if(ret < 0 || ((ret !=0) && WEXITSTATUS(ret) != 1)) cout << "Warning: " << " exportfs failed (ret = " << ret << ") for " << cmd << endl;
      else cout << "exportfs succeeded for " << cmd << endl;
    }
  //#endif

  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::get_expired_sessions(std::list<std::string>& users) throw()
{
  users.clear();

  std::string current_time = time_unix2db(time(NULL));

  try
  {
    mysqlpp::Query query = onl->query();
    query << "select user from reservations where end<=" << mysqlpp::quote << current_time << " and rid in (select rid from experiments where begin=end)";
    vector<usernames> expired_users;
    query.storein(expired_users);
    if(expired_users.empty())
    {
      return onldb_resp(1,(std::string)"success");
    }

    std::map<int, int> caps;
    std::map<int, int> rls;
    std::map<int, int> lls;
    std::map<int, int>::iterator mit;

    mysqlpp::Query query2 = onl->query();
    query << "select cid,capacity,rload,lload from connschedule where rid in (select rid from experiments where begin=end)";
    vector<capconninfo> cci;
    query.storein(cci);
    vector<capconninfo>::iterator conn;
    for(conn = cci.begin(); conn != cci.end(); ++conn)
    {
      if(conn->cid == 0) { continue; }
      if(caps.find(conn->cid) == caps.end())
      {
        caps[conn->cid] = conn->capacity;
	rls[conn->cid] = conn->rload;
	lls[conn->cid] = conn->lload;
      }
      else
      {
        caps[conn->cid] += conn->capacity;
	rls[conn->cid] += conn->rload;
	lls[conn->cid] += conn->lload;
      }
    }

    for(mit = caps.begin(); mit != caps.end(); ++mit)
    {
      mysqlpp::Query query3 = onl->query();
      query3 << "select capacity from connections where cid=" << mysqlpp::quote << mit->first;
      vector<capinfo> ci;
      query3.storein(ci);
      if(ci.size() != 1) { return onldb_resp(-1, (std::string)"database consistency problem"); }
      if(rls[mit->first] > ci[0].capacity || lls[mit->first] > ci[0].capacity)
      {
        vector<usernames>::iterator u;
        for(u = expired_users.begin(); u != expired_users.end(); ++u)
        {
          users.push_back(u->user);
        }
        return onldb_resp(1,(std::string)"success");
      }
    }
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,(std::string)"database consistency problem");
  }

  return onldb_resp(1,(std::string)"success");
}

onldb_resp onldb::get_capacity(std::string type, unsigned int port) throw()
{
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select bandwidth from interfacetypes where interface in (select interface from interfaces where tid=" << mysqlpp::quote << type << " and port=" << mysqlpp::quote << port << ")";
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      return onldb_resp(-1,(std::string)"no such port");
    }
    bwinfo bw = res[0];
    //bandwidth is returned in Gbps we need to convert it to Mbps
    return onldb_resp((1000*bw.bandwidth),(std::string)"success");
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,(std::string)"no such port");
  }
}


onldb_resp onldb::has_virtual_port(std::string type) throw()
{
  try
  {
    mysqlpp::Query query = onl->query();
    query << "select hasvport from types where tid=" << mysqlpp::quote << type;// << ")";
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      return onldb_resp(-1,(std::string)"no such type");
    }
    vporttype vpi = res[0];
    return onldb_resp(vpi.hasvport,(std::string)"success");
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,(std::string)"no such type");
  }
}

onldb_resp onldb::get_switch_ports(unsigned int cid, switch_port_info& info1, switch_port_info& info2) throw()
{
  if(cid == 0)
  {
    info1 = switch_port_info("",0,false,false);
    info2 = switch_port_info("",0,false,false);
    return onldb_resp(1,(std::string)"success");
  }

  try
  {
    mysqlpp::Query query = onl->query();
    query << "select node1,node1port,node2,node2port from connections where cid=" << mysqlpp::quote << cid;
    mysqlpp::StoreQueryResult res = query.store();
    if(res.empty())
    {
      return onldb_resp(-1,(std::string)"no such port");
    }
    conninfo conn = res[0];

    bool n1inf = false;
    bool n2inf = false;

    onldb_resp r1 = is_infrastructure(conn.node1); 
    if(r1.result() < 0) return onldb_resp(-1, (std::string)"database consistency problem");
    if(r1.result() == 1) { n1inf = true; }
    
    onldb_resp r2 = is_infrastructure(conn.node2); 
    if(r2.result() < 0) return onldb_resp(-1, (std::string)"database consistency problem");
    if(r2.result() == 1) { n2inf = true; }
    
    std::string n1 = "";
    unsigned int n1p = 0;
    std::string n2 = "";
    unsigned int n2p = 0;
    bool inf_port = false;
    bool pass_tag1 = false;
    bool pass_tag2 = false;

    if(n1inf)
    {
      n1 = conn.node1;
      n1p = conn.node1port;
    }
    if(n2inf) 
    {
      n2 = conn.node2;
      n2p = conn.node2port;
    }
    if(n1inf && n2inf)
    {
      inf_port = true;
    }
    else
      {
	mysqlpp::Query query2 = onl->query();
	if (n1inf)
	  query2 << "select hasvport from types join nodes using (tid) where nodes.node=" << mysqlpp::quote << conn.node2;	    
	else
	  query2 << "select hasvport from types join nodes using (tid) where nodes.node=" << mysqlpp::quote << conn.node1;
	mysqlpp::StoreQueryResult res = query2.store();
	if (!res.empty())
	  {
	    vporttype vpi = res[0];
	    if ((int)vpi.hasvport != 0) 
	      {
		if (n1inf) pass_tag1 = true;
		else pass_tag2 = true;
	      }
	  }
      }

    info1 = switch_port_info(n1,n1p,inf_port,pass_tag1);
    info2 = switch_port_info(n2,n2p,inf_port,pass_tag2);

    return onldb_resp(1,(std::string)"success");
  }
  catch(const mysqlpp::Exception& er)
  {
    return onldb_resp(-1,(std::string)"database consistency problem");
  }
}
