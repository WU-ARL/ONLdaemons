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
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>

#include "shared.h"

#include "swrd_types.h"
#include "swrd_configuration.h"
#include "swrd_monitor.h"


using namespace swr;

Monitor::Monitor() throw()
{
}

Monitor::~Monitor() throw()
{ 
}

// index is one set of couters, counter is one of: pre-q packet, pre-q byte, post-q packet, post-q byte
unsigned int Monitor::read_stats_counter(unsigned int index, unsigned int counter) throw (monitor_exception)
{
  unsigned int val = 0;
  
  write_log("read_stats_counter: index " + int2str(index) + ", counter " + int2str(counter));

  if(counter > MAX_COUNTER)
  {
    throw monitor_exception("counter is not valid");
  }
  if(index > ONL_ROUTER_STATS_INDEX_MAX)
  {
    throw monitor_exception("index is not valid");
  }

  
  return val;
}

unsigned int Monitor::read_stats_register(unsigned int index) throw(monitor_exception)
{
  unsigned int val = 0;

  write_log("read_stats_register: index " + int2str(index));

  if(index > ONL_ROUTER_REGISTERS_MAX)
  {
    throw monitor_exception("index is not valid");
  }

  return val;
}

unsigned int Monitor::read_queue_length(unsigned int port, unsigned int queue) throw(monitor_exception)
{
  unsigned int val = 0;

  write_log("read_queue_length: port " + int2str(port) + ", queue " + int2str(queue));

  if(port >= QM_NUM_PORTS)
  {
    throw monitor_exception("port is not valid");
  }
  if(queue >= QM_NUM_QUEUES)
  {
    throw monitor_exception("queue is not valid");
  }

  return val;
}
