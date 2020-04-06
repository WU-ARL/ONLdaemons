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
#include <netinet/in.h>

#include "shared.h"

#include "keeboot_globals.h"
#include "keeboot_requests.h"

namespace keeboot
{
  dispatcher* the_dispatcher;
  nccp_listener* crd_conn;
};

using namespace keeboot;

int main()
{
  the_dispatcher = dispatcher::get_dispatcher();
  crd_conn = NULL;

  try
  {
    std::string tenDot="10.";
    //crd_conn = new nccp_listener("10.", Default_ND_Port);
    crd_conn = new nccp_listener(tenDot, Default_ND_Port);
  }
  catch(std::exception& e)
  {
    write_log(e.what());
    exit(1);
  }

  register_req<start_experiment_req>(NCCP_Operation_StartExperiment);
  register_req<refresh_req>(NCCP_Operation_Refresh);

  crd_conn->receive_messages(true);

  nccp_connection* tmp_conn = NULL;
  i_am_up* upmsg = NULL;
  try
  {
    write_log("main: trying to connect to CRD");
    //tmp_conn = new nccp_connection("onlsrv", Default_CRD_Port);
    tmp_conn = new nccp_connection("10.0.1.2", Default_CRD_Port);
    i_am_up* upmsg = new i_am_up();
    upmsg->set_connection(tmp_conn);
    write_log("main: trying to send upmsg to CRD");
    upmsg->send();
    write_log("main: after send upmsg to CRD");
  }
  catch(std::exception& e)
  {
    write_log("main: warning: could not connect CRD");
  }
  if(upmsg) delete upmsg;
  if(tmp_conn) delete tmp_conn;

  write_log("main: about to pthread_exit");
  pthread_exit(NULL);
}
