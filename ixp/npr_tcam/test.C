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

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <exception>

#include <unistd.h> 
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <npr/nprd_types.h>
#include "tcamd_cmds.h"

int tcp_sock;

void startrouter();
void stoprouter();
void addroute(unsigned int);
void delroute(unsigned int);
void queryroute();
void addpfilter(unsigned int);
void delpfilter(unsigned int);
void querypfilter();
void addafilter(unsigned int);
void delafilter(unsigned int);
void queryafilter();
void addarp(unsigned int);
void delarp(unsigned int);
void queryarp();

void startrouter()
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::ROUTERINIT;

  std::cout << "sending ROUTERINIT" << std::endl;
  if(write(tcp_sock, buf, 4) != 4)
  {
    std::cout << "write failed for ROUTERINIT" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for ROUTERINIT response" << std::endl;
    exit(1);
  }

  if(*result != tcamd::SUCCESS)
  {
    std::cout << "result for ROUTERINIT is not good!" << std::endl;
    exit(1);
  }
  std::cout << "ROUTERINIT succeeded" << std::endl << std::endl; 
}

void stoprouter()
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::ROUTERSTOP;

  std::cout << "sending ROUTERSTOP" << std::endl;
  if(write(tcp_sock, buf, 4) != 4)
  {
    std::cout << "write failed for ROUTERSTOP" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for ROUTERSTOP response" << std::endl;
    exit(1);
  }

  if(*result != tcamd::SUCCESS)
  {
    std::cout << "result for ROUTERSTOP is not good!" << std::endl;
    exit(1);
  }
  std::cout << "ROUTERSTOP succeeded" << std::endl << std::endl;
}

void addroute(unsigned int idx)
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::ADDROUTE;

  tcamd::route_info *route = (tcamd::route_info *)&buf[4];
  if(idx == 1)
  {
    route->key.daddr = 0xC0A80120; //aaaaaaaa
    route->key.saddr = 0x00000000; //bbbbbbbb
    route->key.ptag = 0x00; //10
    route->key.port = 0x0;  //4 

    route->mask.daddr = 0xfffffff0;
    route->mask.saddr = 0x00000000; //ffffffff
    route->mask.ptag = 0x00; //1f
    route->mask.port = 0x0;

    route->result = 0x00000010; //01234567
    route->index = idx;
  }
  else
  {
    route->key.daddr = 0xC0A80120; //aaaaaaaa
    route->key.saddr = 0x00000000; //bbbbbbbb
    route->key.ptag = 0x00; //10
    route->key.port = 0x1;  //4 

    route->mask.daddr = 0xfffffff0;
    route->mask.saddr = 0x00000000; //ffffffff
    route->mask.ptag = 0x00; //1f
    route->mask.port = 0x7;

    route->result = 0x00000020; //01234567
    route->index = idx;
  }
  
  std::cout << "sending ADDROUTE" << std::endl;
  if(write(tcp_sock, buf, sizeof(tcamd::route_info)+4) != sizeof(tcamd::route_info)+4)
  {
    std::cout << "write failed for ADDROUTE" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for ADDROUTE response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for ADDROUTE is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    std::cout << "ADDROUTE succeeded" << std::endl << std::endl;
  }
  else
  {
    std::cout << "ADDROUTE failed because the index is already in use" << std::endl << std::endl;
  }
}

void delroute(unsigned int idx)
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::DELROUTE;
  unsigned int *index = (unsigned int *)&buf[4];
  *index = idx;

  std::cout << "sending DELROUTE" << std::endl;
  if(write(tcp_sock, buf, 8) != 8)
  {
    std::cout << "write failed for DELROUTE" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for DELROUTE response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for DELROUTE is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    std::cout << "DELROUTE succeeded" << std::endl << std::endl;
  }
  else
  {
    std::cout << "DELROUTE ok, but the index was already invalid" << std::endl << std::endl;
  }
}

void queryroute()
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::QUERYROUTE;

  npr::route_key *key = (npr::route_key *)&buf[4];
  key->daddr = 0xaaaaaaaa;
  key->saddr = 0xbbbbbbbb;
  key->ptag = 0x10;
  key->port = 0x4;

  std::cout << "sending QUERYROUTE" << std::endl;
  if(write(tcp_sock, buf, sizeof(npr::route_key)+4) != sizeof(npr::route_key)+4)
  {
    std::cout << "write failed for QUERYROUTE" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for QUERYROUTE response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for QUERYROUTE is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    unsigned int *lookup_result = (unsigned int *)&rbuf[4];
    std::string s;
    std::ostringstream o;
    o << *lookup_result;
    s = o.str();

    std::cout << "QUERYROUTE succeeded, with result: " + s << std::endl << std::endl;
  } 
  else
  {
    std::cout << "QUERYROUTE succeeded, but nothing matched the key" << std::endl << std::endl;
  }
}

void addpfilter(unsigned int idx)
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::ADDPFILTER;

  tcamd::pfilter_info *pfilter = (tcamd::pfilter_info *)&buf[4];
  pfilter->key.daddr = 0xcccccccc;
  pfilter->key.saddr = 0xdddddddd;
  pfilter->key.ptag = 0x10;
  pfilter->key.port = 0x4;
  pfilter->key.proto = 0x1;
  pfilter->key.dport = 0x1111;
  pfilter->key.sport = 0x2222;
  pfilter->key.exceptions = 0x0;
  pfilter->key.tcp_flags = 0x444;
  pfilter->key.res = 0x0;

  pfilter->mask.daddr = 0xffffffff;
  pfilter->mask.saddr = 0xffffffff;
  pfilter->mask.ptag = 0x1f;
  pfilter->mask.port = 0x7;
  pfilter->mask.proto = 0xff;
  pfilter->mask.dport = 0xffff;
  pfilter->mask.sport = 0xffff;
  pfilter->mask.exceptions = 0xffff;
  pfilter->mask.tcp_flags = 0xfff;
  pfilter->mask.res = 0x0;

  pfilter->result = 0x1fffffff;
  pfilter->index = idx;
  
  std::cout << "sending ADDPFILTER" << std::endl;
  if(write(tcp_sock, buf, sizeof(tcamd::pfilter_info)+4) != sizeof(tcamd::pfilter_info)+4)
  {
    std::cout << "write failed for ADDPFILTER" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for ADDPFILTER response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for ADDPFILTER is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    std::cout << "ADDPFILTER succeeded" << std::endl << std::endl;
  }
  else
  {
    std::cout << "ADDPFILTER failed because the index is already in use" << std::endl << std::endl;
  }
}

void delpfilter(unsigned int idx)
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::DELPFILTER;
  unsigned int *index = (unsigned int *)&buf[4];
  *index = idx;

  std::cout << "sending DELPFILTER" << std::endl;
  if(write(tcp_sock, buf, 8) != 8)
  {
    std::cout << "write failed for DELPFILTER" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for DELPFILTER response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for DELPFILTER is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    std::cout << "DELPFILTER succeeded" << std::endl << std::endl;
  }
  else
  {
    std::cout << "DELPFILTER ok, but the index was already invalid" << std::endl << std::endl;
  }
}

void querypfilter()
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::QUERYPFILTER;

  npr::pfilter_key *key = (npr::pfilter_key *)&buf[4];
  key->daddr = 0xcccccccc;
  key->saddr = 0xdddddddd;
  key->ptag = 0x10;
  key->port = 0x4;
  key->proto = 0x1;
  key->dport = 0x1111;
  key->sport = 0x2222;
  key->exceptions = 0x0;
  key->tcp_flags = 0x444;
  key->res = 0x5;

  std::cout << "sending QUERYPFILTER" << std::endl;
  if(write(tcp_sock, buf, sizeof(npr::pfilter_key)+4) != sizeof(npr::pfilter_key)+4)
  {
    std::cout << "write failed for QUERYPFILTER" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for QUERYPFILTER response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for QUERYPFILTER is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    unsigned int *lookup_result = (unsigned int *)&rbuf[4];
    std::string s;
    std::ostringstream o;
    o << *lookup_result;
    s = o.str();

    std::cout << "QUERYPFILTER succeeded, with result: " + s << std::endl << std::endl;
  } 
  else
  {
    std::cout << "QUERYPFILTER succeeded, but nothing matched the key" << std::endl << std::endl;
  }
}

void addafilter(unsigned int idx)
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::ADDAFILTER;

  tcamd::afilter_info *afilter = (tcamd::afilter_info *)&buf[4];
  afilter->key.daddr = 0xcccccccc;
  afilter->key.saddr = 0xdddddddd;
  afilter->key.ptag = 0x10;
  afilter->key.port = 0x4;
  afilter->key.proto = 0x1;
  afilter->key.dport = 0x1111;
  afilter->key.sport = 0x2222;
  afilter->key.exceptions = 0x0;
  afilter->key.tcp_flags = 0x444;
  afilter->key.res = 0x0;

  afilter->mask.daddr = 0xffffffff;
  afilter->mask.saddr = 0xffffffff;
  afilter->mask.ptag = 0x1f;
  afilter->mask.port = 0x7;
  afilter->mask.proto = 0xff;
  afilter->mask.dport = 0xffff;
  afilter->mask.sport = 0xffff;
  afilter->mask.exceptions = 0xffff;
  afilter->mask.tcp_flags = 0xfff;
  afilter->mask.res = 0x0;

  afilter->result = 0x1fffffff;
  afilter->index = idx;
  
  std::cout << "sending ADDAFILTER" << std::endl;
  if(write(tcp_sock, buf, sizeof(tcamd::afilter_info)+4) != sizeof(tcamd::afilter_info)+4)
  {
    std::cout << "write failed for ADDAFILTER" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for ADDAFILTER response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for ADDAFILTER is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    std::cout << "ADDAFILTER succeeded" << std::endl << std::endl;
  }
  else
  {
    std::cout << "ADDAFILTER failed because the index is already in use" << std::endl << std::endl;
  }
}

void delafilter(unsigned int idx)
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::DELAFILTER;
  unsigned int *index = (unsigned int *)&buf[4];
  *index = idx;

  std::cout << "sending DELAFILTER" << std::endl;
  if(write(tcp_sock, buf, 8) != 8)
  {
    std::cout << "write failed for DELAFILTER" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for DELAFILTER response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for DELAFILTER is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    std::cout << "DELAFILTER succeeded" << std::endl << std::endl;
  }
  else
  {
    std::cout << "DELAFILTER ok, but the index was already invalid" << std::endl << std::endl;
  }
}

void queryafilter()
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::QUERYAFILTER;

  npr::afilter_key *key = (npr::afilter_key *)&buf[4];
  key->daddr = 0xcccccccc;
  key->saddr = 0xdddddddd;
  key->ptag = 0x10;
  key->port = 0x4;
  key->proto = 0x1;
  key->dport = 0x1111;
  key->sport = 0x2222;
  key->exceptions = 0x0;
  key->tcp_flags = 0x444;
  key->res = 0x5;

  std::cout << "sending QUERYAFILTER" << std::endl;
  if(write(tcp_sock, buf, sizeof(npr::afilter_key)+4) != sizeof(npr::afilter_key)+4)
  {
    std::cout << "write failed for QUERYAFILTER" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for QUERYAFILTER response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for QUERYAFILTER is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    unsigned int *lookup_result = (unsigned int *)&rbuf[4];
    std::string s;
    std::ostringstream o;
    o << *lookup_result;
    s = o.str();

    std::cout << "QUERYAFILTER succeeded, with result: " + s << std::endl << std::endl;
  } 
  else
  {
    std::cout << "QUERYAFILTER succeeded, but nothing matched the key" << std::endl << std::endl;
  }
}

void addarp(unsigned int idx)
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::ADDARP;

  tcamd::arp_info *arp = (tcamd::arp_info *)&buf[4];
  if(idx == 1)
  {
    arp->key.daddr = 0xC0A80130;
    arp->key.res1 = 0x0;
    arp->key.res2 = 0x0;

    arp->mask.daddr = 0xffffffff;
    arp->mask.res1 = 0x0;
    arp->mask.res2 = 0x0;

    arp->result = 0x00001010;
    arp->index = idx;
  }
  else
  {
    arp->key.daddr = 0xC0A80120;
    arp->key.res1 = 0x0;
    arp->key.res2 = 0x0;

    arp->mask.daddr = 0xffffffff;
    arp->mask.res1 = 0x0;
    arp->mask.res2 = 0x0;

    arp->result = 0x00001020;
    arp->index = idx;
  }
  
  std::cout << "sending ADDARP" << std::endl;
  if(write(tcp_sock, buf, sizeof(tcamd::arp_info)+4) != sizeof(tcamd::arp_info)+4)
  {
    std::cout << "write failed for ADDARP" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for ADDARP response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for ADDARP is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    std::cout << "ADDARP succeeded" << std::endl << std::endl;
  }
  else
  {
    std::cout << "ADDARP failed because the index is already in use" << std::endl << std::endl;
  }
}

void delarp(unsigned int idx)
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::DELARP;
  unsigned int *index = (unsigned int *)&buf[4];
  *index = idx;

  std::cout << "sending DELARP" << std::endl;
  if(write(tcp_sock, buf, 8) != 8)
  {
    std::cout << "write failed for DELARP" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for DELARP response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for DELARP is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    std::cout << "DELARP succeeded" << std::endl << std::endl;
  }
  else
  {
    std::cout << "DELARP ok, but the index was already invalid" << std::endl << std::endl;
  }
}

void queryarp()
{
  char buf[2000];
  unsigned int *cmd = (unsigned int *)&buf[0];

  char rbuf[2000];
  int bytes_read;
  unsigned int *result = (unsigned int *)&rbuf[0];

  *cmd = tcamd::QUERYARP;

  npr::arp_key *key = (npr::arp_key *)&buf[4];
  key->daddr = 0xc0a80130;
  key->res1 = 0x01234567;
  key->res2 = 0x89;

  std::cout << "sending QUERYARP" << std::endl;
  if(write(tcp_sock, buf, sizeof(npr::arp_key)+4) != sizeof(npr::arp_key)+4)
  {
    std::cout << "write failed for QUERYARP" << std::endl;
    exit(1);
  }

  bytes_read = read(tcp_sock, rbuf, 2000);
  if(bytes_read < 1)
  {
    std::cout << "read failed for QUERYARP response" << std::endl;
    exit(1);
  }

  if(*result == tcamd::FAILURE)
  {
    std::cout << "result for QUERYARP is not good!" << std::endl;
    exit(1);
  }
  if(*result == tcamd::SUCCESS)
  {
    unsigned int *lookup_result = (unsigned int *)&rbuf[4];
    std::string s;
    std::ostringstream o;
    o << *lookup_result;
    s = o.str();

    std::cout << "QUERYARP succeeded, with result: " + s << std::endl << std::endl;
  } 
  else
  {
    std::cout << "QUERYARP succeeded, but nothing matched the key" << std::endl << std::endl;
  }
}

int main()
{
  char input[10];

  unsigned short port = DEFAULT_TCAMD_PORT;
  unsigned int addr = 0x0a00020c;

  struct sockaddr_in sa;
  int socklen;

  if((tcp_sock = socket(PF_INET,SOCK_STREAM,0)) < 0)
  {
    std::cout << "socket failed" << std::endl;
    exit(1);
  }

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = htonl(addr);

  socklen = sizeof(struct sockaddr_in);
  if(connect(tcp_sock, (struct sockaddr *)&sa, socklen) < 0)
  {
    std::cout << "connect failed" << std::endl;
    exit(1);
  }

  startrouter();

/*
  addroute(1);
  addroute(1);
  queryroute();
  delroute(1);
  queryroute();
  delroute(2);

  addpfilter(2);
  addpfilter(2);
  querypfilter();
  delpfilter(2);
  querypfilter();
  delpfilter(3);

  addafilter(0);
  addafilter(0);
  queryafilter();
  delafilter(0);
  queryafilter();
  delafilter(1);

*/

  addroute(1);
//  addroute(2);
 // std::cout << "Press enter to delete the routes.\n";
  //std::cin.getline(input,10);
 // delroute(1);
 // delroute(2);
  std::cout << "Press enter when you want to stop the router.\n";
  std::cin.getline(input,10);

  stoprouter();

  close(tcp_sock);
  return 0;
}
