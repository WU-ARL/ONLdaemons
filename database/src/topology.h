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

#ifndef _TOPOLOGY_H
#define _TOPOLOGY_H

namespace onl
{
  class topology
  {
    friend class onldb;

    private:
      std::list<node_resource_ptr> nodes;
      std::list<link_resource_ptr> links;

      std::string lowercase(std::string) throw();

    public:
      topology() throw();
      ~topology() throw();
   
      void print_resources() throw();

      // you have to add the hw resource before you add any links for it
      // you also have to add parents (clusters) before adding children (cluster components)
      onldb_resp add_node(std::string type, unsigned int label, unsigned int parent_label) throw();
      onldb_resp add_link(unsigned int label, unsigned int capacity, unsigned int node1_label, unsigned int node1_port, unsigned int node2_label, unsigned int node2_port) throw();

      onldb_resp remove_node(unsigned int label) throw();
      onldb_resp remove_link(unsigned int label) throw();

      std::string get_component(unsigned int label) throw();
      std::string get_type(unsigned int label) throw();
      unsigned int get_label(std::string node) throw();

      void get_conns(unsigned int label, std::list<int>& conn_list) throw();
  };
};
#endif // _TOPOLOGY_H
