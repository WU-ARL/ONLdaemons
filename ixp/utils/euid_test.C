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
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
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
  //char logstr[256];
 
  char filename[256];
  strcpy(filename, "test.txt");

  char user[8];
  strcpy(user, "wiseman");

  char shcmd[256];
  sprintf(shcmd, "/usr/sbin/ypcat passwd | /bin/egrep '^%s:' | cut -d: -s -f3 > /tmp/uid", user);

  if(system(shcmd) < 0)
  {
    char e[64];
    char err[30];
    strerror_r(errno, err, 30);
    sprintf(e, "system failed: %s", err);
    std::cout << e << std::endl;
    exit(1);
  }

  FILE *uid_file = fopen("/tmp/uid", "r");
  if(uid_file == NULL)
  {
    char e[64];
    char err[30];
    strerror_r(errno, err, 30);
    sprintf(e, "fopen for /tmp/uid failed: %s", err);
    std::cout << e << std::endl;
    exit(1);
  }

  int uid;
  int rv = fscanf(uid_file, "%d", &uid);
  if(rv != 1)
  {
    std::cout << "fscanf returned no matches\n";
    exit(1);
  }

  if(fclose(uid_file) != 0)
  {
    char e[64];
    char err[30];
    strerror_r(errno, err, 30);
    sprintf(e, "close failed: %s", err);
    std::cout << e << std::endl;
    exit(1);
  }

  if(seteuid(uid) != 0)
  {
    char e[64];
    char err[30];
    strerror_r(errno, err, 30);
    sprintf(e, "seteuid failed: %s", err);
    std::cout << e << std::endl;
    exit(1);
  }

  char fn[256];
  sprintf(fn, "/users/%s/%s", user, filename);
  std::cout << fn << std::endl;

  int fd = open(fn, O_WRONLY|O_CREAT|O_TRUNC, S_IRWXU);
  if(fd < 0)
  {
    char e[64];
    char err[30];
    strerror_r(errno, err, 30);
    sprintf(e, "open failed: %s", err);
    std::cout << e << std::endl;
    exit(1);
  }

  if(close(fd) < 0)
  {
    char e[64];
    char err[30];
    strerror_r(errno, err, 30);
    sprintf(e, "close failed: %s", err);
    std::cout << e << std::endl;
    exit(1);
  }

  return 0;
}
