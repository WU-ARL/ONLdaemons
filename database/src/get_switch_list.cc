/*
 * Copyright (c) 2009-2013 Charlie Wiseman
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

#include <iostream>
#include <string>
#include <list>
#include <vector>

#include "onl_database.h"

using namespace std;
using namespace onl;

int main()
{
  onldb *db = new onldb();

  switch_info_list switch_list;
  db->get_switch_list(switch_list);
  while(!switch_list.empty())
  {
    switch_info sw = switch_list.front();
    switch_list.pop_front();

    cout << sw.get_switch() << "," << sw.get_num_ports() << "," << sw.get_mgmt_port() << endl;
  }

  delete db;
}
