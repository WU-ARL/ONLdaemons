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

#if (IOSTYLE != HARDWARE)
#error IOSTYLE must be HARDWARE
#endif

std::string int2str(unsigned int);

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
  int status;

  if((status = halMe_Init(0x0)) != HALME_SUCCESS)
  {
    std::cout << "halMe_Init failed with status " + int2str(status) + "!\n";
    exit(1);
  }

  unsigned int prodId = GET_GLB_CSR(PRODUCT_ID);

  unsigned int majType = (prodId & PID_MAJOR_PROD_TYPE) >> PID_MAJOR_PROD_TYPE_BITPOS;
  unsigned int minType = (prodId & PID_MINOR_PROD_TYPE) >> PID_MINOR_PROD_TYPE_BITPOS;
  unsigned int majRev = (prodId & PID_MAJOR_REV) >> PID_MAJOR_REV_BITPOS;
  unsigned int minRev = (prodId & PID_MINOR_REV) >> PID_MINOR_REV_BITPOS;

  if(majType != 0)
  {
    std::cout << "Major Type: " + int2str(majType) + "\n";
    std::cout << "Minor Type: " + int2str(minType) + "\n";
    std::cout << "Major Revision: " + int2str(majRev) + "\n";
    std::cout << "Minor Revision: " + int2str(minRev) + "\n";
  }
  else 
  {
    std::string ver;
    if(minType == 0)
    {
      ver = "2800";
      if(majRev >= 3) 
      {
        ver = "2805";
        majRev -= 3;
      }
    }
    else if(minType == 1)
    {
      ver="2850";
    }
    else
    {
      std::cout << "Major Type: " + int2str(majType) + "\n";
      std::cout << "Minor Type: " + int2str(minType) + "\n";
      std::cout << "Major Revision: " + int2str(majRev) + "\n";
      std::cout << "Minor Revision: " + int2str(minRev) + "\n";
    }

    char rev = 'A';
    rev += majRev;

    std::cout << "Chip is an IXP" + ver + " " + rev + int2str(minRev) + "\n";
  }

  return 0;
}
