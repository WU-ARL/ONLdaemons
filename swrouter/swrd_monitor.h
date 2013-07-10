#ifndef _SWRD_MONITOR_H
#define _SWRD_MONITOR_H

namespace swr
{
  class monitor_exception: public std::runtime_error
  {
    public:
      monitor_exception(const std::string& e): std::runtime_error("Monitor exception: " + e) {}
  };

  class Monitor
  {
    public:
      Monitor() throw();
      ~Monitor() throw();
   
      #define MIN_COUNTER 0
      #define PREQ_BYTE 0
      #define PREQ_PKT 1
      #define POSTQ_BYTE 2
      #define POSTQ_PKT 3
      #define MAX_COUNTER 3

      // read a stats counter one time
      unsigned int read_stats_counter(unsigned int, unsigned int) throw(monitor_exception);

      // read a register one time
      unsigned int read_stats_register(unsigned int) throw(monitor_exception);

      // read a queue length one time
      unsigned int read_queue_length(unsigned int, unsigned int) throw(monitor_exception);
  };
};

#endif // _SWRD_MONITOR_H
