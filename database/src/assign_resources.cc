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
    cout << "usage: assign_resource username (node type label parent|link capacity label port label port)*..." << endl;
    cout << "       where parent is the label of the cluster that this node belongs to, and 0 if none" << endl;
    cout << "       example: make_reservation wiseman node ixpcluster 1 0 node ixp 2 1 node ixp 3 1 node host 4 0 node host 5 0 link 2 0 4 0 0 link 3 0 5 0 0 link 2 4 3 4 0" << endl;
    cout << "       the example specifies one ixpcluster (which consists of two ixps), two hosts, and links from host 4 to ixp 2 port 0, from host 5 to ixp 3 port 0, and from ixp 2 port 4 to ixp 3 port 4" << endl;
}

int main(int argc, char **argv)
{
  onldb *db = new onldb();

  std::string user;

  if(argc < 3)
  {
    usage();
    return -1;
  }

  user = argv[1];

  topology top;
  unsigned int linkid = 1;

  for(int i=2; i<argc; )
  {
    std::string resource = argv[i];
    if(resource == "node" && i+3 < argc)
    {
      std::string type = argv[i+1];
      unsigned int label = strtoul(argv[i+2],NULL,10);
      int parent_label = strtol(argv[i+3],NULL,10);
      onldb_resp r = top.add_node(type,label,parent_label);
      if(r.result() != 1)
      {
        cout << r.msg() << endl;
        return -1;
      }
      i+=4;
    }
    else if(resource == "link" && i+5 < argc)
    {
      unsigned int cap    = strtoul(argv[i+1],NULL,10);
      unsigned int label1 = strtoul(argv[i+2],NULL,10);
      unsigned int port1  = strtoul(argv[i+3],NULL,10);
      unsigned int label2 = strtoul(argv[i+4],NULL,10);
      unsigned int port2  = strtoul(argv[i+5],NULL,10);
      onldb_resp r = top.add_link(linkid,cap,label1,port1,label2,port2);
      linkid++;
      if(r.result() != 1)
      {
        cout << r.msg() << endl;
        return -1;
      }
      i+=6;
    }
    else
    {
      usage();
      return -1;
    }
  }

  onldb_resp rv = db->assign_resources(user, &top);
  cout << rv.msg() << endl;
  if(rv.result() < 1) return -1;

  top.print_resources();

  sleep(5);

  onldb_resp rv2 = db->return_resources(user, &top);
  cout << rv2.msg() << endl;

  delete db;
}
