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
#include "hal_global.h"
#include "uclo.h"

extern "C"
{
  #include "api.h"
}

#if (IOSTYLE != HARDWARE)
#error IOSTYLE must be HARDWARE
#endif

std::string int2str(unsigned int);
void print_usage(char *);

void print_usage(char *prog)
{
  std::cout << prog << " [--help] -p plugin_uof_file -m ME]\n";
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
  char input[10];
  
  int status;

  std::string plugin_uof;
  void *plugin_handle;
  unsigned int plugin_me;
  unsigned int plugin_me_mask;
  unsigned int me;
  unsigned int ctx_mask = ME_ALL_CTX;

  static struct option longopts[] =
  {{"help",   0, NULL, 'h'},
   {"plugin", 1, NULL, 'p'},
   {"me",     1, NULL, 'm'},
   {NULL,     0, NULL,  0}
  };

  int c;
  while ((c = getopt_long(argc, argv, "hp:m:", longopts, NULL)) != -1) {
    switch (c) {
      case 'p':
        plugin_uof = optarg;
        break;
      case 'm':
        plugin_me = atoi(optarg);
        break;
      case 'h':
      default:
        print_usage(argv[0]);
        exit(1);
    }
  }

  std::cout << "Calling UcLo_InitLib...";
  UcLo_InitLib();
  std::cout << "done.\n";

  std::cout << "Calling halMe_Init...";
  if((status = halMe_Init(0x0)) != HALME_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::string ver = "2800";
  unsigned int prodId = GET_GLB_CSR(PRODUCT_ID);
  unsigned int majRev = (prodId & PID_MAJOR_REV) >> PID_MAJOR_REV_BITPOS;
  if(majRev >= 3) { ver = "2805"; }

  std::string full_plugin_path = plugin_uof + "_" + int2str(plugin_me) + "_" + ver;

  std::cout << "Calling UcLo_LoadObjFile for the plugin " << full_plugin_path << "...";
  if((status = UcLo_LoadObjFile(&plugin_handle, (char *)full_plugin_path.c_str())) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  plugin_me_mask = UcLo_GetAssignedMEs(plugin_handle);
  
  std::cout << "Calling UcLo_WriteUimageAll for plugin...";
  if((status = UcLo_WriteUimageAll(plugin_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  unsigned int mask = 0xf0007f;

  std::cout << "Calling halMe_Stop for router base microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(mask & (1<<me))) continue;

    if(halMe_Stop(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  mask = mask | plugin_me_mask;

  std::cout << "Calling halMe_Start for router base and plugin microengines...";
  for(me=0; me < 0x18; me++)
  {
    if(!(mask & (1<<me))) continue;

    if(halMe_Start(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Press enter when you want to stop and remove the plugin.\n";
  std::cin.getline(input,10);

  std::cout << "Calling halMe_Stop for plugin microengine...";
  for(me=0; me < 0x18; me++)
  {
    if(!(plugin_me_mask & (1<<me))) continue;
    if(halMe_Stop(me, ctx_mask) != HALME_SUCCESS)
    {
      std::cout << "failed for me: " + int2str(me) + "\n";
      exit(1);
    }
  }
  std::cout << "done.\n";

  std::cout << "Calling UcLo_DeleObj for the plugin...";
  if((status = UcLo_DeleObj(plugin_handle)) != UCLO_SUCCESS)
  {
    std::cout << "failed!\n";
    exit(1);
  } 
  std::cout << "done.\n";

/*
  std::cout << "Calling halMe_DelLib...";
  halMe_DelLib();
  std::cout << "done.\n";
*/

  return 0;
}
