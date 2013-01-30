#ifndef _HOST_GLOBALS_H
#define _HOST_GLOBALS_H

#include <string>
#include <vector>
#include <pthread.h>

namespace host
{
  extern dispatcher* the_dispatcher;
  extern nccp_listener* rli_conn;
  extern configuration* conf;
  extern std::string username;
  extern std::vector<std::string> paths;
  extern std::vector<std::string> nexthops;
  extern pthread_mutex_t mutex;			// saves the world from race condition over vector updates
}; // namespace host

#endif // _HOST_GLOBALS_H
