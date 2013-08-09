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
#include <sys/wait.h>
#include <sys/ioctl.h>
  
#include "halMev2Api.h"
#include "uclo.h"

extern "C"
{
  #include "api.h"
}

#if (IOSTYLE != HARDWARE)
#error IOSTYLE must be HARDWARE
#endif

#define SUCCESS   RC_SUCCESS
#define FAILURE   0xFFFFFFFF

std::string int2str(unsigned int i)
{
  std::string s;
  std::ostringstream o;
  o << i;
  s = o.str();
  return s;
}

int main(void)
{
  int status;

  Misc_Device_Info_t blah;

  std::cout << "Calling rpclOpen...";
  RppClientOpts rpp_options = {50, FALSE, FALSE, ""};
  char rpp_addr[16];
  memset(rpp_addr, 0, 16);
  strcpy(rpp_addr, "192.168.111.1");
  if((status = rpclOpen((UCHAR *)rpp_addr,&rpp_options)) != RC_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "Calling MiscGetDeviceInfo...";
  status = MiscGetDeviceInfo(&blah);
  if(status != RC_SUCCESS)
  {
    std::cout << "MiscGetDeviceInfo failed\n";
    exit(1);
  }
  std::cout << "done.\n";

  std::cout << "RTM: " << int2str(blah.RTM) << "\n";
  std::cout << "FIC: " << int2str(blah.FIC) << "\n";
  std::cout << "TCAM: " << int2str(blah.TCAM) << "\n";

/*
  std::cout << "Calling MiscIsMaster...";
  unsigned int npu = MiscIsMaster();
  if(npu != 0 && npu != 1)
  {
    std::cout << "MiscIsMaster failed\n";
    exit(1);
  }
  std::cout << "done.\n";
  std::cout << "NPU is " << int2str(npu) << "\n";
*/

  std::cout << "Calling /root/whichNpu...";
  status = system("/root/whichNpu");
  if(status == -1)
  {
    std::cout << "system failed\n";
    exit(1);
  }
  if(WIFEXITED(status) == 0)
  {
    std::cout << "call failed\n";
    exit(1);
  }
  std::cout << "done.\n";
  unsigned int npu = WEXITSTATUS(status);
  std::cout << "NPU is " << int2str(npu) << "\n";

  for(unsigned int i=0; i<=4; ++i)
  {
    rtm_mac_address_t rmat;
    rmat.portNumber = i+1;
    if(npu == 0) // NPUB 
    {
      rmat.portNumber += 5;
    }
    std::cout << "Calling RtmGetMacAddress...";
    if(RtmGetMacAddress(&rmat) != RC_SUCCESS)
    {
      std::cout << "RtmGetMacAddress failed\n";
      exit(1);
    }
    std::cout << "done.\n";

    char blahstr[256];
    sprintf(blahstr, "Port %d has MAC addy %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", i, rmat.macAddr[5], rmat.macAddr[4], rmat.macAddr[3], rmat.macAddr[2], rmat.macAddr[1], rmat.macAddr[0]);
    std::cout << blahstr;
  }

  return 0;
}
