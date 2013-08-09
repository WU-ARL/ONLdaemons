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
#include "hal_sram.h"
#include "hal_scratch.h"

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
  char input[10];
  
  int status;
  int bank;
  int addr;
   
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
    std::cin.getline(input,10);
    if(input[0] == 'q') break;

    if(input[0] == '0') bank=0;
    else if(input[0] == '1') bank=1;
    else if(input[0] == '2') bank=2;
    else if(input[0] == '3') bank=3;
    else
    {
      addr = strtol(input, NULL, 10);
      val = SRAM_READ(bank,addr);

      std::cout << int2str(val) << "\n";
    }
  }

  std::cout << "Calling halMe_DelLib...";
  halMe_DelLib();
  std::cout << "done.\n";

  return 0;
}
