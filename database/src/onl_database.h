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

#ifndef _ONL_DATABASE_H
#define _ONL_DATABASE_H

#include <boost/shared_ptr.hpp>

// going to put all forward declarations here for now
namespace mysqlpp
{
  class Connection;
};

namespace onl
{
  struct _node_resource;
  typedef struct _node_resource node_resource;
  typedef boost::shared_ptr<node_resource> node_resource_ptr;
  struct _link_resource;
  typedef struct _link_resource link_resource;
  typedef boost::shared_ptr<link_resource> link_resource_ptr;
  struct _assign_info;
  typedef struct _assign_info assign_info;
  typedef boost::shared_ptr<assign_info> assign_info_ptr;
};

#include <onldb_resp.h>
#include <topology.h>
#include <gurobi_c++.h>
#include <onldb.h>
#include <onltempdb.h>

#endif // _ONL_DATABASE_H
