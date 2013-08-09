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
#include "hal_dram.h"

#if (IOSTYLE != HARDWARE)
#error IOSTYLE must be HARDWARE
#endif

std::string int2str(unsigned int i)
{
  std::string s;
  std::ostringstream o;
  o << i;
  s = o.str();
  return s;
}

int main()
{
  char input[16];
  
  int status;
  unsigned int addr;
  unsigned int val;

  std::cout << "Calling halMe_Init...";
  if((status = halMe_Init(0x0)) != HALME_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  while(1)
  {
    std::cin.get(input,16,' ');
    addr = strtoul(input, NULL, 0);
    addr = addr & 0x0fffffff;

    std::cin.get(input,16,'\n');
    val = strtoul(input, NULL, 0);

    if(!std::cin.good()) { break; }

    //std::cout << int2str(addr) << std::endl;
    //std::cout << int2str(val) << std::endl;
    DRAM_WRITE(addr,val);
  }

  std::cout << "Calling halMe_DelLib...";
  halMe_DelLib();
  std::cout << "done.\n";

  return 0;
}
