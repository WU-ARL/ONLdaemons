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

#include <netlink/socket.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/link.h>
#include <netlink/route/tc.h>
#include <netlink/route/class.h>
#include <netlink/route/qdisc/netem.h>
#include <netlink/route/qdisc/htb.h>



using namespace std;

void usage()
{
  cout << "usage: test_libnl <port> <delay>" << endl;
}

int main(int argc, char **argv)
{

  if(argc < 3)
    {
      usage();
      return -1;
    }
  

  
  int port = strtol(argv[1], NULL,10);
  int dtime = strtol(argv[2], NULL,10);
  struct nl_sock *my_sock = nl_socket_alloc();
  struct nl_cache *link_cache, *all_qdiscs;
  struct rtnl_link *link_ptr;
  struct rtnl_qdisc *qdisc;
  std::string nic;
  

  nl_connect(my_sock, NETLINK_ROUTE);
  if (rtnl_link_alloc_cache(my_sock, AF_UNSPEC, &link_cache) < 0)
    {
      nl_socket_free(my_sock);
      cout << "rtnl_link_alloc_cache failed" << endl;
      return -1;
    }
  char tmp_str[256];
  sprintf(tmp_str, "data%d", port);
  nic = tmp_str;
  if (!(link_ptr = rtnl_link_get_by_name(link_cache, nic.c_str())))
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      cout << "rtnl_link_get_by_name failed" << endl;
      return -1;
    }
  
  //int ifindex = rtnl_link_get_ifindex(link_ptr);  
  /*
    if ((rtnl_qdisc_alloc_cache(my_sock, &all_qdiscs)) < 0)
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      throw monitor_exception("rtnl_qdisc_alloc_cache failed ");
      }*/
  
  qdisc = rtnl_qdisc_alloc();               
  
  //rtnl_tc_set_ifindex(TC_CAST(qdisc), ifindex);
  rtnl_tc_set_link(TC_CAST(qdisc), link_ptr);
  //rtnl_tc_set_parent(TC_CAST(qdisc), TC_HANDLE((port + 1),1));
  //rtnl_tc_set_handle(TC_CAST(qdisc), TC_HANDLE((port + 7),0));
  rtnl_tc_set_parent(TC_CAST(qdisc), TC_HANDLE(5,1));
  rtnl_tc_set_handle(TC_CAST(qdisc), TC_HANDLE(39,0));
  rtnl_tc_set_kind(TC_CAST(qdisc), "netem"); 
  rtnl_netem_set_delay(qdisc, dtime);
  rtnl_netem_set_loss(qdisc, 0);
  //if (jitter > 0)
  //rtnl_netem_set_jitter(qdisc, jitter);

  //nl_object_dump(OBJ_CAST(qdisc), NULL);
  int rtn = rtnl_qdisc_add(my_sock, qdisc, NLM_F_CREATE);
  if (rtn < 0)
    {
      cout << "rtnl_qdisc_add failed error code:" << nl_geterror(rtn) << endl;
    }
  else
    cout << "rtnl_qdisc_add succeeded." << endl;
  
  nl_socket_free(my_sock);
  nl_cache_free(link_cache);
  //nl_cache_free(all_qdiscs);
  rtnl_qdisc_put(qdisc);
  rtnl_link_put(link_ptr);
  return 0;
}
