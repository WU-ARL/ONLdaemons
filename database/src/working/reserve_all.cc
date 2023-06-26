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

void usage()
{
  cout << "usage: reserve_all begin length_minutes" << endl;
}

int main(int argc, char **argv)
{
  onldb *db = new onldb();

  std::string begin;
  unsigned int len;

  if(argc != 3)
  {
    usage();
    return -1;
  }

  begin = argv[1];
  len = strtoul(argv[2],NULL,10);

  onldb_resp rv = db->reserve_all(begin, len);
  cout << rv.msg() << endl;

  delete db;
}
