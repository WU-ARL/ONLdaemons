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

#include "host_configuration.h"
#include "host_globals.h"
#include "host_requests.h"

namespace host
{
  dispatcher* the_dispatcher;
  nccp_listener* rli_conn;
  
  configuration* conf;
};

using namespace host;

int main()
{
  log = new log_file("/tmp/host.log");
  the_dispatcher = dispatcher::get_dispatcher();
  rli_conn = NULL;
  conf = new configuration();

  try
  {
    rli_conn = new nccp_listener("127.0.0.1", Default_ND_Port);
  }
  catch(std::exception& e)
  {
    write_log(e.what());
    exit(1);
  }

  register_req<configure_node_req>(NCCP_Operation_CfgNode);

  register_req<add_route_req>(HOST_AddRoute);
  register_req<delete_route_req>(HOST_DeleteRoute);

  rli_conn->receive_messages(false);

  pthread_exit(NULL);
}
