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
  int type = 0; // 0 from sram, 1 for scratch
   
  int ring;
  unsigned int val;

  std::cout << "Calling halMe_Init...";
  //if((status = halMe_Init(0xff00ff)) != HALME_SUCCESS)
  if((status = halMe_Init(0x0)) != HALME_SUCCESS)
  {
    std::cout << "failed with status " + int2str(status) + "!\n";
    exit(1);
  }
  std::cout << "done.\n";

  val = 0x01234567;
  SRAM_RING_PUT(1,8,val);
  val = 0x89abcdef;
  SRAM_RING_PUT(1,8,val);
/*
  SRAM_RING_PUT(3,9,val);
  SRAM_RING_PUT(3,10,val);
  SRAM_RING_PUT(3,11,val);
  SRAM_RING_PUT(3,12,val);
  SRAM_RING_PUT(3,13,val);
  SRAM_RING_PUT(3,14,val);
  SRAM_RING_PUT(3,15,val);
  SRAM_RING_PUT(3,16,val);
  SRAM_RING_PUT(3,17,val);
  SRAM_RING_PUT(3,18,val);
  SCRATCH_RING_PUT(1,0x89abcdef);
*/

  std::cout << "Enter sram or scratch to switch to that kind of ring, ring number to read a word, and 0 to quit.\n";
  while(1)
  {
    std::cin.getline(input,10);
    if(input[0] == '0') break;
    
    if(strcmp(input, "scratch") == 0) type = 1;
    else if(strcmp(input, "sram") == 0) type = 0;
    else
    {
      ring = strtol(input, NULL, 10);
      if(ring < 1 || ring > 18) { continue; }
      if(type == 0)
      {
        val = SRAM_RING_GET(1,ring);
      }
      else
      {
        val = SCRATCH_RING_GET(ring);
      }

      std::cout << int2str(val) << "\n";
    }
  }

  std::cout << "Calling halMe_DelLib...";
  halMe_DelLib();
  std::cout << "done.\n";

  return 0;
}
