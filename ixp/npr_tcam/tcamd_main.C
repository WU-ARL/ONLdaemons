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

#include "tcamd_logfile.h"
#include "tcamd_cmds.h"
#include "tcamd_ops.h"
#include "tcamd_util.h"

using namespace tcamd;

int main(int argc, char **argv)
{
  unsigned short port = DEFAULT_TCAMD_PORT;
  std::string log_file = DEFAULT_TCAMD_LOG;
  std::string tcam_conf = DEFAULT_TCAM_CONF_FILE;
  std::string log_level = "LOG_NORMAL";
  unsigned int my_addr;
  struct ifreq ifr;
  char addr_str[INET_ADDRSTRLEN];
  struct sockaddr_in sa,*temp;
  int socklen;
  struct sockaddr_in from;

  int temp_sock;
  int tcp_sock = -1;
  int npra_sock = -1;
  int nprb_sock = -1;
  int max_fd = 0;

  fd_set rss, rss_temp;

  enum states {CONNECTED,DISCONNECTED};
  int npra_state = DISCONNECTED;
  int nprb_state = DISCONNECTED;

  int errors;
  int err_npu;

  static struct option longopts[] =
  {{"help",       0, NULL, 'h'},
   {"port",       1, NULL, 'p'},
   {"logfile",    1, NULL, 'l'},
   {"debuglevel", 1, NULL, 'd'},
   {"tcamconf",   1, NULL, 't'},
   {NULL,         0, NULL,  0}
  };

  int c;
  while ((c = getopt_long(argc, argv, "hp:l:d:t:", longopts, NULL)) != -1) {
    switch (c) {
      case 'h':
        print_usage(argv[0]);
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'l':
        log_file = optarg;
        break;
      case 'd':
        log_level = optarg;
        break;
      case 't':
        tcam_conf = optarg;
        break;
      default:
        print_usage(argv[0]);
        break;
    }
  }

  // get my ip addr
  if((temp_sock = socket(PF_INET,SOCK_DGRAM,0)) < 0)
  {
    print_err("(temp) socket failed!");
    exit(1);
  }
  strcpy(ifr.ifr_name, "eth0");
  if(ioctl(temp_sock, SIOCGIFADDR, &ifr) < 0)
  {
    print_err("ioctl failed!");
    exit(1);
  }
  close(temp_sock);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  temp = (struct sockaddr_in *)&ifr.ifr_addr;
  sa.sin_addr.s_addr = temp->sin_addr.s_addr;
  my_addr = (unsigned int) sa.sin_addr.s_addr;
  if(inet_ntop(AF_INET,&sa.sin_addr,addr_str,INET_ADDRSTRLEN) == NULL)
  {
    print_err("inet_ntop failed!");
    exit(1);
  }
  // done getting ip addr

  char fn[256];
  sprintf(fn, "/log/%s", log_file.c_str());
  print_err(fn);
  try {
    tcamlog = new LogFile(std::string(fn),LOG_ROTATE,log_level);
  } catch (log_exception& le) {
    std::string e = "Couldn't open log file!" + std::string(le.what());
    print_err(e);
    exit(1);
  }

  write_log(LOG_NORMAL, "IP address is: " + std::string(addr_str));

  // get a tcp socket to accept the (two) incoming connections
  if((tcp_sock = socket(PF_INET,SOCK_STREAM,0)) < 0)
  {
    write_err(LOG_NORMAL, "socket failed!");
    exit(1);
  }

  if(bind(tcp_sock, (struct sockaddr *) &sa, sizeof(sa)) < 0)
  {
    write_err(LOG_NORMAL, "bind failed!");
    exit(1);
  }

  if(listen(tcp_sock,2) < 0)
  {
    write_err(LOG_NORMAL, "listed failed!");
    exit(1);
  }
  // done setting up tcp socket

  // initialize tcam 
  if(inittcam(tcam_conf) < 0)
  {
    write_log(LOG_NORMAL, "inittcam failed!");
    exit(1);
  }

  FD_ZERO(&rss);
  FD_SET(tcp_sock, &rss);
  max_fd = tcp_sock+1;

  write_log(LOG_VERBOSE, "max_fd is " + tcamlog->int2str(max_fd));

  errors = 0;
  err_npu = NONE;
  while(errors != -1)
  {
    int sock, npu;
    std::string npu_str;

    // deal with errors cases here
    if(errors == 1)
    {
      if(err_npu == NPRA)
      {
        if(npra_state != DISCONNECTED)
        {
          write_log(LOG_NORMAL, "stopping router for NPRA");
          npra_state = DISCONNECTED;
          if(routerstop(err_npu, NULL) < 0)
          {
            write_err(LOG_NORMAL, "stopping router failed for NPRA!");
            break;
          }
        }

        if(npra_sock != -1)
        {
          write_log(LOG_NORMAL, "closing socket for NPRA");
          if(close(npra_sock) < 0)
          {
            write_err(LOG_NORMAL, "close on NPRA socket failed!");
            npra_sock = -1;
            break;
          }
          FD_CLR(npra_sock, &rss);
          max_fd = std::max(nprb_sock, tcp_sock) + 1;
          write_log(LOG_VERBOSE, "max_fd is " + tcamlog->int2str(max_fd));
          npra_sock = -1;
        }

        err_npu = NONE;
        errors = 0;
      }
      else if(err_npu == NPRB)
      {
        if(nprb_state != DISCONNECTED)
        {
          write_log(LOG_NORMAL, "stopping router for NPRB");
          nprb_state = DISCONNECTED;
          if(routerstop(err_npu, NULL) < 0)
          {
            write_err(LOG_NORMAL, "stopping router failed for NPRB!");
            break;
          }
        }

        if(nprb_sock != -1)
        {
          write_log(LOG_NORMAL, "closing socket for NPRB");
          if(close(nprb_sock) < 0)
          {
            write_err(LOG_NORMAL, "close on NPRB socket failed!");
            nprb_sock = -1;
            break;
          }
          FD_CLR(nprb_sock, &rss);
          max_fd = std::max(npra_sock, tcp_sock) + 1;
          write_log(LOG_VERBOSE, "max_fd is " + tcamlog->int2str(max_fd));
          nprb_sock = -1;
        }

        err_npu = NONE;
        errors = 0;
      }
      else
      {
        write_log(LOG_NORMAL, "something is broken.. should never be here.  exiting.");
        break;
      }
    }

    rss_temp = rss;

    if(select(max_fd, &rss_temp, NULL, NULL, NULL) == -1)
    {
      write_err(LOG_NORMAL, "select failed!");
      break;
    }

    write_log(LOG_VERBOSE, "Got a message");

    if(FD_ISSET(tcp_sock, &rss_temp))
    {
      write_log(LOG_VERBOSE, "Got a message on tcp_sock");
 
      socklen = sizeof(struct sockaddr_in);
      if((temp_sock = accept(tcp_sock, (struct sockaddr *)&from, (socklen_t *)&socklen)) < 0)
      {
        write_err(LOG_NORMAL, "accept failed on tcp_sock!");
        break;
      }
      if(inet_ntop(AF_INET,&from.sin_addr,addr_str,INET_ADDRSTRLEN) == NULL)
      {
        write_err(LOG_NORMAL, "inet_ntop failed on new address!");
        break;
      }

      write_log(LOG_VERBOSE, "Accepted connection from IP address: " + std::string(addr_str));

      if((unsigned int)from.sin_addr.s_addr == my_addr && npra_state == DISCONNECTED)
      {
        npra_sock = temp_sock;
        FD_SET(npra_sock, &rss);
        max_fd = std::max(max_fd-1,npra_sock)+1;
        write_log(LOG_VERBOSE, "npra_sock is " + tcamlog->int2str(npra_sock));
        write_log(LOG_VERBOSE, "max_fd is " + tcamlog->int2str(max_fd));
        npra_state = CONNECTED;
        write_log(LOG_NORMAL, "NPRA is now connected");
      }
      else if((unsigned int)from.sin_addr.s_addr == my_addr+1 && nprb_state == DISCONNECTED)
      {
        nprb_sock = temp_sock;
        FD_SET(nprb_sock, &rss);
        max_fd = std::max(max_fd-1,nprb_sock)+1;
        write_log(LOG_VERBOSE, "nprb_sock is " + tcamlog->int2str(nprb_sock));
        write_log(LOG_VERBOSE, "max_fd is " + tcamlog->int2str(max_fd));
        nprb_state = CONNECTED;
        write_log(LOG_NORMAL, "NPRB is now connected");
      }
      else
      {
        write_log(LOG_NORMAL, "closing socket with unexpected address");

        if(close(temp_sock) < 0)
        {
          write_err(LOG_NORMAL, "close on socket with unexpected address failed");
          break;
        }
      }

      continue; 
    }

    sock = -1;
    npu = NONE;   
    npu_str = "NONE";


    if((npra_state == CONNECTED) && FD_ISSET(npra_sock, &rss_temp))
    {
      sock = npra_sock;
      npu = NPRA;
      npu_str = "NPRA";
    }
    else if((nprb_state == CONNECTED) && FD_ISSET(nprb_sock, &rss_temp))
    {
      sock = nprb_sock;
      npu = NPRB;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "Got a message but nothing is set...");
      continue;
    }
    write_log(LOG_VERBOSE, "Got a message from " + npu_str);

    // here, guaranteed to have sock and npu set to something valid
    int r;
    char buf[2000];
    unsigned int *cmd;

    if((r = read(sock, buf, 2000)) < 0)
    {
      write_err(LOG_NORMAL, "read on sock " + npu_str + " failed! Closing!");
      errors = 1;
      err_npu = npu;
      continue;
    }

    if(r<1)
    {
      write_log(LOG_NORMAL, "read 0 bytes from sock " + npu_str + ". Closing.");
      errors = 1;
      err_npu = npu;
      continue;
    }

    if(r<(int)sizeof(tcam_command))
    {
      write_log(LOG_NORMAL, "not enough bytes in sock " + npu_str = " to hold the command. Closing.");
      errors = 1;
      err_npu = npu;
      continue;
    }

    int resultbytes;
    char resultbuf[2000];
    cmd = (unsigned int *)&buf[0];
    switch(*cmd)
    {
      case ROUTERINIT:
        resultbytes = routerinit(npu,resultbuf);
        break;
      case ROUTERSTOP:
        resultbytes = routerstop(npu,resultbuf);
        break;
      case ADDROUTE:
        resultbytes = addroute(npu,r,buf,resultbuf);
        break;
      case DELROUTE:
        resultbytes = delroute(npu,r,buf,resultbuf);
        break;
      case QUERYROUTE:
        resultbytes = queryroute(npu,r,buf,resultbuf);
        break;
      case ADDPFILTER:
        resultbytes = addpfilter(npu,r,buf,resultbuf);
        break;
      case DELPFILTER:
        resultbytes = delpfilter(npu,r,buf,resultbuf);
        break;
      case QUERYPFILTER:
        resultbytes = querypfilter(npu,r,buf,resultbuf);
        break;
      case ADDAFILTER:
        resultbytes = addafilter(npu,r,buf,resultbuf);
        break;
      case DELAFILTER:
        resultbytes = delafilter(npu,r,buf,resultbuf);
        break;
      case QUERYAFILTER:
        resultbytes = queryafilter(npu,r,buf,resultbuf);
        break;
      case ADDARP:
        resultbytes = addarp(npu,r,buf,resultbuf);
        break;
      case DELARP:
        resultbytes = delarp(npu,r,buf,resultbuf);
        break;
      case QUERYARP:
        resultbytes = queryarp(npu,r,buf,resultbuf);
        break;
      default:
        resultbytes = sizeof(tcam_result);
        (unsigned int)resultbuf[0] = UNKNOWN;
        write_log(LOG_NORMAL, "invalid command from sock " + npu_str + "!");
        break;
    }

    if(resultbytes == -1)
    {
      write_err(LOG_NORMAL, "command failed for " + npu_str + "! something is broken, so closing connection");
      errors = 1;
      err_npu = npu;
      continue;
    }

    if(resultbytes > 0)
    {
      if(write(sock, resultbuf, resultbytes) != resultbytes)
      {
        write_err(LOG_NORMAL, "write to sock " + npu_str + " failed:");
        errors = 1;
        err_npu = npu;
        continue;
      }
    }
  }

  write_log(LOG_NORMAL, "daemon exiting");

  if(npra_state != DISCONNECTED) { routerstop(NPRA, NULL); }
  if(nprb_state != DISCONNECTED) { routerstop(NPRB, NULL); }
  if(npra_sock != -1) { close(npra_sock); }
  if(nprb_sock != -1) { close(nprb_sock); }
  if(tcamlog) { delete tcamlog; }

  return 0;
}
