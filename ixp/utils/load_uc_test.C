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
#include <getopt.h>
#include <sys/types.h>
#include <sys/ioctl.h>
  
#include "halMev2Api.h"
#include "uclo.h"

#if (IOSTYLE != HARDWARE)
#error IOSTYLE must be HARDWARE
#endif

void print_usage(char *);
std::string int2str(unsigned int);

void print_usage(char *prog)
{
  std::cout << prog << " [--help] [--base router_base_uof_file] [--plugin plugin_uof_file]\n"
            << "\t--help: print this message\n"
            << "\t--base router_base_uof_file: default is HW_NPUA.uof\n"
            << "\t--plugin plugin_uof_file: default is null.uof\n";
  exit(0);
}

std::string int2str(unsigned int i)
{
  std::string s;
  std::ostringstream o;
  o << i;
  s = o.str();
  return s;
}

int main(int argc, char **argv)
{
  std::string base_uof = "HW_NPUA.uof";
  std::string plugin_uof = "null.uof";
  char input[10];
  
  int status;

  void *base_handle;
  void *plugin_handle;
  unsigned int base_me_mask;
  unsigned int plugin_me_mask;
  unsigned int me;
  unsigned int ctx_mask = ME_ALL_CTX;

  static struct option longopts[] =
  {{"help",   0, NULL, 'h'},
   {"base",   1, NULL, 'b'},
   {"plugin", 1, NULL, 'p'},
   {NULL,     0, NULL,  0}
  };

  int c;
  while ((c = getopt_long(argc, argv, "hb:p:", longopts, NULL)) != -1) {
    switch (c) {
      case 'h':
        print_usage(argv[0]);
        break;
      case 'b':
        base_uof = optarg;
        break;
      case 'p':
        plugin_uof = optarg;
        break;
      default:
        print_usage(argv[0]);
        exit(1);
    }
  }

  std::cout << "Calling UcLo_InitLib...";
  UcLo_InitLib();
  std::cout << "done.\n";

  std::cout << "Calling UcLo_LoadObjFile for the router base...";
  if((status = UcLo_LoadObjFile(&base_handle, (char *)base_uof.c_str())) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "Calling UcLo_LoadObjFile for the plugin...";
  if((status = UcLo_LoadObjFile(&plugin_handle, (char *)plugin_uof.c_str())) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  base_me_mask = UcLo_GetAssignedMEs(base_handle);
  plugin_me_mask = UcLo_GetAssignedMEs(plugin_handle);
  
  std::cout << "Calling UcLo_WriteUimageAll for router base...";
  if((status = UcLo_WriteUimageAll(base_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "Calling halMe_Start for router base microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(base_me_mask & (1<<me))) continue;

    unsigned int wkupEvents;

    if(halMe_PutCtxArb(me, 0) != HALME_SUCCESS)
    {  
      std::cout << "putctxarb failed for me: " + int2str(me) + "\n";
      exit(1);
    }
    if(halMe_GetCtxWakeupEvents(me, 0, &wkupEvents) != HALME_SUCCESS)
    {
      std::cout << "getctxwakeupevents failed for me: " + int2str(me) + "\n";
      exit(1);
    }
    if(!(wkupEvents & 0xffff))
    {
      if(halMe_PutCtxWakeupEvents(me, ctx_mask, wkupEvents | XCWE_VOLUNTARY) != HALME_SUCCESS)
      {
        std::cout << "putctxwakeupevents failed for me: " + int2str(me) + "\n";
        exit(1);
      }
    }

    if(halMe_Start(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Press enter when you want to add the plugin (note that the router base microengines will be stopped and restarted).\n";
  std::cin.getline(input,10);

  std::cout << "Calling halMe_Stop for router base microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(base_me_mask & (1<<me))) continue;
    if(halMe_Stop(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Calling UcLo_WriteUimageAll for the plugin...";
  if((status = UcLo_WriteUimageAll(plugin_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "Calling halMe_Start for all used microengines...";
  for(me=0; me < 0x18; me++)
  {
    if((!(base_me_mask & (1<<me))) && (!(plugin_me_mask & (1<<me)))) continue;

    unsigned int wkupEvents;

    if(halMe_PutCtxArb(me, 0) != HALME_SUCCESS)
    {
      std::cout << "putctxarb failed for me: " + int2str(me) + "\n";
      exit(1);
    }
    if(halMe_GetCtxWakeupEvents(me, 0, &wkupEvents) != HALME_SUCCESS)
    {
      std::cout << "getctxwakeupevents failed for me: " + int2str(me) + "\n";
      exit(1);
    }
    if(!(wkupEvents & 0xffff))
    {
      if(halMe_PutCtxWakeupEvents(me, ctx_mask, wkupEvents | XCWE_VOLUNTARY) != HALME_SUCCESS)
      {
        std::cout << "putctxwakeupevents failed for me: " + int2str(me) + "\n";
        exit(1);
      }
    }

    if(halMe_Start(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Press enter when you want to stop the microengines.\n";
  std::cin.getline(input,10);

  std::cout << "Calling halMe_Stop for all used microengines...";
  for(me=0; me < 0x18; me++)
  {
    if((!(base_me_mask & (1<<me))) && (!(plugin_me_mask & (1<<me)))) continue;
    if(halMe_Stop(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Calling UcLo_DeleObj for the router base...";
  if((status = UcLo_DeleObj(base_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed!\n";
    exit(1);
  } 
  std::cout << "done.\n";

  std::cout << "Calling UcLo_DeleObj for the plugin...";
  if((status = UcLo_DeleObj(plugin_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed!\n";
    exit(1);
  }
  std::cout << "done.\n";
 
  return 0;
}
