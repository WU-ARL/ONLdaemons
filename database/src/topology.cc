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
#include <list>

#include <boost/shared_ptr.hpp>

#include "internal.h"
#include "onldb_resp.h"
#include "topology.h"

using namespace std;
using namespace onl;

topology::topology() throw()
{
  intercluster_cost = 0;
  host_cost = 0;
}

topology::~topology() throw()
{
  while(!nodes.empty())
  {
    node_resource_ptr hw = nodes.front();
    nodes.pop_front();

    hw->links.clear();
    hw->node_children.clear();
    hw->parent.reset();
  }

  while(!links.empty())
  {
    link_resource_ptr lnk = links.front();
    links.pop_front();

    lnk->node1.reset();
    lnk->node2.reset();
  }
}

std::string topology::lowercase(std::string s) throw()
{
 for(unsigned int i=0; i<s.length(); ++i)
 {
  s[i] = ::tolower(s[i]);
 }
 return s;
}

void topology::print_resources() throw()
{
  list<node_resource_ptr>::iterator nit;
  for(nit = nodes.begin(); nit != nodes.end(); ++nit)
  {
    cout << "node " << to_string((*nit)->label) << " is mapped to " << (*nit)->node << endl;
  }

  list<link_resource_ptr>::iterator lit;
  for(lit = links.begin(); lit != links.end(); ++lit)
  {
    cout << "link " << to_string((*lit)->label) << " is mapped to ";
    std::list<int>::iterator c; 
    for(c = (*lit)->conns.begin(); c != (*lit)->conns.end(); ++c)
    {
      cout << to_string(*c) << ",";
    }
    cout << endl;
  }
}

onldb_resp topology::add_node(std::string type, unsigned int label, unsigned int parent_label) throw()
{
  node_resource_ptr hrp(new node_resource());
  hrp->type = lowercase(type);
  hrp->label = label;
  hrp->is_parent = false;

  hrp->fixed = false;
  hrp->node = "";
  hrp->acl = "unused";
  hrp->cp = "unused";

  hrp->type_type = "";
  hrp->marked = false;
  hrp->level = 0;
  hrp->priority = 0;
  hrp->in = 0;
  hrp->cost = 0;
  hrp->is_split = false;

  bool parent_found;
  if(parent_label == 0)
  {
    parent_found = true;
  }
  else
  {
    parent_found = false;
  }

  list<node_resource_ptr>::iterator nit;
  for(nit = nodes.begin(); nit != nodes.end(); ++nit)
  {
    if((*nit)->label == hrp->label)
    {
      return onldb_resp(-1,(std::string)"label " + to_string(label) + " already used");
    }
    if((!parent_found) && ((*nit)->label == parent_label))
    {
      hrp->parent = *nit;
      (*nit)->is_parent = true;
      (*nit)->node_children.push_back(hrp);
      parent_found = true;
    }
  }
  
  if(!parent_found)
  {
    return onldb_resp(-1,(std::string)"parent_label " + to_string(parent_label) + " not found");
  }

  nodes.push_back(hrp);

  return onldb_resp(1,(std::string)"success");
}

onldb_resp topology::add_copy_node(node_resource_ptr cpnode) throw()
{
  node_resource_ptr hrp(new node_resource());
  hrp->type = cpnode->type;
  hrp->label = cpnode->label;
  hrp->is_parent = cpnode->is_parent;

  hrp->fixed = cpnode->fixed;
  hrp->node = cpnode->node;
  hrp->acl = cpnode->acl;
  hrp->cp = cpnode->cp;

  hrp->type_type = cpnode->type_type;
  hrp->marked = cpnode->marked;
  hrp->level = cpnode->level;
  hrp->priority = cpnode->priority;
  hrp->in = cpnode->in;
  hrp->cost = cpnode->cost;
  hrp->user_nodes.push_back(cpnode);
  hrp->is_split = cpnode->is_split;

  unsigned int parent_label = 0;
  if (cpnode->parent) parent_label = cpnode->parent->label;

  bool parent_found;
  if(parent_label == 0)
  {
    parent_found = true;
  }
  else
  {
    parent_found = false;
  }

  list<node_resource_ptr>::iterator nit;
  for(nit = nodes.begin(); nit != nodes.end(); ++nit)
  {
    if((*nit)->label == hrp->label)
    {
      return onldb_resp(-1,(std::string)"label " + to_string(hrp->label) + " already used");
    }
    if((!parent_found) && ((*nit)->label == parent_label))
    {
      hrp->parent = *nit;
      (*nit)->is_parent = true;
      (*nit)->node_children.push_back(hrp);
      parent_found = true;
    }
  }
  
  if(!parent_found)
  {
    return onldb_resp(-1,(std::string)"parent_label " + to_string(parent_label) + " not found");
  }

  nodes.push_back(hrp);

  return onldb_resp(1,(std::string)"success");
}

onldb_resp topology::add_link(unsigned int label, unsigned int capacity, unsigned int node1_label, unsigned int node1_port, unsigned int node2_label, unsigned int node2_port, unsigned int rload, unsigned int lload) throw()
{
  link_resource_ptr lrp(new link_resource());
  
  bool node1_found = false;
  bool node2_found = false;

  lrp->label = label;
  lrp->capacity = capacity;
  lrp->node1_port = node1_port;
  lrp->node2_port = node2_port;

  lrp->marked = false;
  lrp->level = 0;
  lrp->in = 0;
  lrp->linkid = 0;
  lrp->added = false;

  lrp->rload = 0;
  lrp->lload = 0;
  lrp->cost = 0;
  lrp->potential_rcap = capacity-rload;
  lrp->potential_lcap = capacity-lload;
  lrp->rload = rload;
  lrp->lload = lload;
  
  list<link_resource_ptr>::iterator lit;
  for(lit = links.begin(); lit != links.end(); ++lit)
  {
    if((*lit)->label == lrp->label)
    {
      return onldb_resp(-1,(std::string)"label " + to_string(label) + " already used");
    }
  }

  list<node_resource_ptr>::iterator nit;
  for(nit = nodes.begin(); nit != nodes.end(); ++nit)
  {
    if((!node1_found) && ((*nit)->label == node1_label))
    {
      lrp->node1 = *nit;
      node1_found = true;
    }
    if((!node2_found) && ((*nit)->label == node2_label))
    {
      lrp->node2 = *nit;
      node2_found = true;
    }
  }
  
  if(!node1_found)
  {
    return onldb_resp(-1,(std::string)"node1_label " + to_string(node1_label) + " not found");
  }
  if(!node2_found)
  {
    return onldb_resp(-1,(std::string)"node2_label " + to_string(node2_label) + " not found");
  }

  lrp->node1->links.push_back(lrp);
  //if(node1_label != node2_label) lrp->node2->links.push_back(lrp);
  //cgw, this may need to change back
  lrp->node2->links.push_back(lrp);
  links.push_back(lrp);

  return onldb_resp(1,(std::string)"success");
}

onldb_resp topology::add_copy_link(link_resource_ptr lnk) throw()
{
  link_resource_ptr lrp(new link_resource());
  
  bool node1_found = false;
  bool node2_found = false;

  lrp->label = lnk->label;
  lrp->capacity = lnk->capacity;
  lrp->node1_port = lnk->node1_port;
  lrp->node2_port = lnk->node2_port;

  lrp->marked = lnk->marked;
  lrp->level = lnk->level;
  lrp->in = lnk->in;
  lrp->linkid = lnk->linkid;
  lrp->added = lnk->added;

  lrp->rload = lnk->rload;
  lrp->lload = lnk->lload;
  lrp->cost = lnk->cost;
  lrp->potential_rcap = lnk->potential_rcap;
  lrp->potential_lcap = lnk->potential_lcap;
  
  lrp->user_link = lnk;

  list<link_resource_ptr>::iterator lit;
  for(lit = links.begin(); lit != links.end(); ++lit)
  {
    if((*lit)->label == lrp->label)
    {
      return onldb_resp(-1,(std::string)"label " + to_string(lrp->label) + " already used");
    }
  }

  list<node_resource_ptr>::iterator nit;
  for(nit = nodes.begin(); nit != nodes.end(); ++nit)
  {
    if((!node1_found) && ((*nit)->label == lnk->node1->label))
    {
      lrp->node1 = *nit;
      node1_found = true;
    }
    if((!node2_found) && ((*nit)->label == lnk->node2->label))
    {
      lrp->node2 = *nit;
      node2_found = true;
    }
  }
  
  if(!node1_found)
  {
    return onldb_resp(-1,(std::string)"node1_label " + to_string(lnk->node1->label) + " not found");
  }
  if(!node2_found)
  {
    return onldb_resp(-1,(std::string)"node2_label " + to_string(lnk->node2->label) + " not found");
  }

  lrp->node1->links.push_back(lrp);
  //if(node1_label != node2_label) lrp->node2->links.push_back(lrp);
  //cgw, this may need to change back
  lrp->node2->links.push_back(lrp);
  links.push_back(lrp);

  return onldb_resp(1,(std::string)"success");
}

onldb_resp topology::remove_node(unsigned int label) throw()
{
  list<node_resource_ptr>::iterator nit;
  for(nit = nodes.begin(); nit != nodes.end(); ++nit)
  {
    if((*nit)->label == label)
    {
      nodes.erase(nit);
      return onldb_resp(1,(std::string)"success");
    }
  }
  return onldb_resp(-1,(std::string)"label " + to_string(label) + " not found");
}

onldb_resp topology::remove_link(unsigned int label) throw()
{
  list<link_resource_ptr>::iterator lit;
  for(lit = links.begin(); lit != links.end(); ++lit)
  {
    if((*lit)->label == label)
    {
      links.erase(lit);
      return onldb_resp(1,(std::string)"success");
    }
  }
  return onldb_resp(-1,(std::string)"label " + to_string(label) + " not found");
}

std::string topology::get_component(unsigned int label) throw()
{
  list<node_resource_ptr>::iterator nit;
  for(nit = nodes.begin(); nit != nodes.end(); ++nit)
  {
    if((*nit)->label == label)
    {
      return (*nit)->node;
    }
  }
  return "";
}

std::string topology::get_type(unsigned int label) throw()
{
  list<node_resource_ptr>::iterator nit;
  for(nit = nodes.begin(); nit != nodes.end(); ++nit)
  {
    if((*nit)->label == label)
    {
      return (*nit)->type;
    }
  }
  return "";
}

unsigned int topology::get_label(std::string node) throw()
{
  list<node_resource_ptr>::iterator nit;
  for(nit = nodes.begin(); nit != nodes.end(); ++nit)
  {
    if((*nit)->node == node)
    {
      return (*nit)->label;
    }
  }
  return 0;
}

void topology::get_conns(unsigned int label, std::list<int>& conn_list) throw()
{
  list<link_resource_ptr>::iterator lit;
  for(lit = links.begin(); lit != links.end(); ++lit)
  {
    if((*lit)->label == label)
    {
      conn_list = (*lit)->conns;
      return;
    }
  }
}


int
topology::compute_host_cost()
{
  std::list<node_resource_ptr>::iterator nit;
  std::list<link_resource_ptr>::iterator lit;
  int rtn = 0;
  for (nit = nodes.begin(); nit != nodes.end(); ++nit)
    {
      if ((*nit)->marked && ((*nit)->type.compare(0, 2, "pc") == 0))//it's not a vgige or an ixp
	{
	  for(lit = (*nit)->links.begin(); lit != (*nit)->links.end(); ++lit)
	    {
	      rtn += (*lit)->capacity;
	      break;
	    }
	}
    }
  return (2*rtn);
}


int
topology::compute_intercluster_cost()
{
  int rtn = 0;
  std::list<link_resource_ptr>::iterator lit;
  std::list<link_resource_ptr>::iterator plit;

  for (lit = links.begin(); lit != links.end(); ++lit)
    {
      if ((*lit)->mapped_path.size() > 0)
	{
	  for (plit = (*lit)->mapped_path.begin(); plit != (*lit)->mapped_path.end(); ++plit)
	    {
	      if ((*plit)->node1->type_type == "infrastructure" &&  (*plit)->node2->type_type == "infrastructure")
		rtn += (*lit)->cost;
	    }
	}
    }
  return rtn;
}
