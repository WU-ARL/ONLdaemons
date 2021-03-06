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
#include <vector>
#include <map>

#include <mysql++/mysql++.h>
#include <mysql++/ssqls.h>
#include <boost/shared_ptr.hpp>

#include "internal.h"
#include "onldb_resp.h"
#include "onltempdb.h"
#include "onltempdb_types.h"

using namespace std;
using namespace onl;

#define ONLTEMPDB     "onltempnew"
#define ONLTEMPDBHOST "localhost"
#define ONLTEMPDBUSER "onltempadmin"
#define ONLTEMPDBPASS "onltemprocks!"


onltempdb::onltempdb() throw()
{
  onltemp = new mysqlpp::Connection(ONLTEMPDB,ONLTEMPDBHOST,ONLTEMPDBUSER,ONLTEMPDBPASS);
}

onltempdb::~onltempdb() throw()
{
  delete onltemp;
}
