#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <exception>
#include <stdexcept>

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "shared.h"

#include "keeboot_globals.h"
#include "keeboot_requests.h"

using namespace keeboot;

start_experiment_req::start_experiment_req(uint8_t *mbuf, uint32_t size): start_experiment(mbuf, size)
{
}

start_experiment_req::~start_experiment_req()
{
}

bool
start_experiment_req::handle()
{
  write_log("start_experiment_req::handle()");

  if(init_params.empty())
  {
    crd_response* resp = new crd_response(this, NCCP_Status_Failed);
    resp->send();
    delete resp;
    return true;
  }

  param p = init_params.front();
  init_params.pop_front();
  if(p.getType() != string_param)
  {
    crd_response* resp = new crd_response(this, NCCP_Status_Failed);
    resp->send();
    delete resp;
    return true;
  }

  std::string filesystem = p.getString();

  crd_response* resp = new crd_response(this, NCCP_Status_Fine);
  resp->send();
  delete resp;

  write_log("start_experiment_req::handle() about to call do_kexec with filesystem: " + filesystem);
  do_kexec(filesystem);
  
  return true;
}

void
start_experiment_req::do_system(std::string filesystem)
{
  std::string keeboot_script = "/onl_boot_script.sh";
  write_log("start_experiment_req::do_system(): about to do system() call " + keeboot_script + " " + filesystem);
/*
  if(fork() == 0)
  {
    execl(keeboot_script.c_str(), keeboot_script.c_str(), filesystem.c_str(), (char *)NULL);
    exit(-1);
  }
*/
}

void
start_experiment_req::do_kexec(std::string filesystem)
{
  std::string keeboot_script = "/onl_boot_script.sh";
  write_log("start_experiment_req::do_kexec(): about to fork/exec " + keeboot_script + " " + filesystem);
  if(fork() == 0)
  {
    execl(keeboot_script.c_str(), keeboot_script.c_str(), filesystem.c_str(), (char *)NULL);
    exit(-1);
  }
}

refresh_req::refresh_req(uint8_t *mbuf, uint32_t size): refresh(mbuf, size)
{
  write_log("refresh_req::refresh_req: got refresh message");
}

refresh_req::~refresh_req()
{
}

bool
refresh_req::handle()
{
  write_log("refresh_req::handle: about to send Fine response");

  crd_response* resp = new crd_response(this, NCCP_Status_Fine);
  resp->send();
  delete resp;

  //system("/sbin/reboot -f");
  system("/bin/reboot -f");

  return true;
}
