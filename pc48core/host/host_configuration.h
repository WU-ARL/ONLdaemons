#ifndef _HOST_CONFIGURATION_H
#define _HOST_CONFIGURATION_H

namespace host
{
  typedef struct _port_info 
  {
    std::string nic;
    std::string ip_addr;
    std::string subnet;
    std::string next_hop;
  } port_info;

  class configuration
  {
    public:
      configuration();
      ~configuration();
   
      void set_port_info(uint32_t portnum, std::string ip, std::string subnet, std::string nexthop);
      
      bool add_route(uint16_t portnum, std::string prefix, uint32_t mask, std::string nexthop);
      bool delete_route(std::string prefix, uint32_t mask);

      std::string addr_int2str(uint32_t addr);

    private:
      static const uint32_t max_port = 1;
      port_info port[max_port+1];

      int system_cmd(std::string cmd);
  };
};

#endif // _HOST_CONFIGURATION_H
