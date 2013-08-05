#ifndef _HOST_REQUESTS_H
#define _HOST_REQUESTS_H

namespace host
{
  class configure_node_req : public configure_node
  {
    public:
      configure_node_req(uint8_t *mbuf, uint32_t size);
      virtual ~configure_node_req();
 
      virtual bool handle();
  }; // class configure_node_req

  static const NCCP_OperationType HOST_AddRoute = 73;
  class add_route_req : public rli_request
  {
    public:
      add_route_req(uint8_t *mbuf, uint32_t size);
      virtual ~add_route_req();

      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t prefix;
      uint32_t mask;
      uint32_t output_port;
      uint32_t nexthop_ip;
      uint32_t stats_index;
  }; // class add_route_req

  static const NCCP_OperationType HOST_DeleteRoute = 75;
  class delete_route_req : public rli_request
  {
    public:
      delete_route_req(uint8_t *mbuf, uint32_t size);
      virtual ~delete_route_req();

      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t prefix;
      uint32_t mask;
  }; // class delete_route_req
};

#endif // _HOST_REQUESTS_H
