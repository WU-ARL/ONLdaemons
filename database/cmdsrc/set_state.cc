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

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

#include "onl_database.h"

using namespace std;
using namespace onl;

int main(int argc, char **argv)
{
  std::string node;
  std::string state;
  unsigned long int len;

  if(argc != 3 && argc != 4)
  {
    cout << "usage: set_state node state [reservation_length]" << endl;
    return -1;
  }

  node = argv[1];
  state = argv[2];
  len = 0;
  if(argc == 4)
  {
    char *endptr;
    errno = 0;
    len = strtoul(argv[3], &endptr, 0);
    if((errno == ERANGE && (len == ULONG_MAX)) || (errno != 0 && len == 0))
    {
      perror("strtol");
      return -1;
    }
    if(endptr == argv[3])
    {
      cout << "invalid length";
      return -1;
    }
  }

  onldb *db = new onldb();

  onldb_resp rv = db->set_state(node,state,len);
  if(rv.result() < 0)
  {
    cout << rv.msg();
    delete db;
    return -1;
  }

  delete db;
  return 0;
}
