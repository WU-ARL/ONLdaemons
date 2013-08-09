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
#include <glob.h>
  
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
  int rv;
  glob_t gb;
  char path[256];
  char globpattern[256];

  sprintf(path, "/users/onl/npr/plugins");
  sprintf(globpattern, "%s/*/uof/*.uof", path);

  rv = glob(globpattern, GLOB_NOSORT, NULL, &gb);
  if(rv == GLOB_NOMATCH)
  {
    std::cout << "no matches found\n";
    return 0;
  }
  if(rv == GLOB_ABORTED || rv == GLOB_NOSPACE)
  {
    std::cout << "glob error\n";
    return 1;
  }

  for(unsigned int i=0; i<gb.gl_pathc; ++i)
  {
    std::cout << gb.gl_pathv[i] << "\n";
  }
  return 0;
}
