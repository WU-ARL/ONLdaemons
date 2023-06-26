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

#include <errno.h>

#include "onl_database.h"

using namespace std;
using namespace onl;

int main(int argc, char **argv)
{
  onldb *db = new onldb();

  std::string node;

  if(argc != 2)
  {
    cout << "usage: remove_from_testing node" << endl;
    return -1;
  }

  node = argv[1];

  onldb_resp rv = db->remove_from_testing(node);
  if(rv.result() < 0)
  {
    cout << rv.msg();
    delete db;
    return -1;
  }

  delete db;
  return 0;
}
