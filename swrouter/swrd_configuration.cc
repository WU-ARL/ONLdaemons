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
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <glob.h>

#include "halMev2Api.h"
#include "hal_sram.h"
#include "hal_scratch.h"
#include "hal_global.h"
#include "uclo.h"

#include "shared.h"

#include "swrd_types.h"
#include "swrd_configuration.h"


using namespace swr;

Configuration::Configuration(rtm_mac_addrs *ms) throw(configuration_exception)
{
  unsigned short port = DEFAULT_TCAMD_PORT;
  unsigned int tcamd_addr;

  struct ifreq ifr;
  char addr_str[INET_ADDRSTRLEN];
  struct sockaddr_in sa,*temp;
  int socklen;

  int temp_sock;

  int status;


  // get my ip addr
  if((temp_sock = socket(PF_INET,SOCK_DGRAM,0)) < 0)
  {
    throw configuration_exception("socket failed");
  }
  strcpy(ifr.ifr_name, "eth0");
  if(ioctl(temp_sock, SIOCGIFADDR, &ifr) < 0)
  {
    throw configuration_exception("ioctl failed");
  }
  close(temp_sock);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  temp = (struct sockaddr_in *)&ifr.ifr_addr;
  sa.sin_addr.s_addr = temp->sin_addr.s_addr;
  control_address = (unsigned int) sa.sin_addr.s_addr;
  if(inet_ntop(AF_INET,&sa.sin_addr,addr_str,INET_ADDRSTRLEN) == NULL)
  {
    throw configuration_exception("inet_ntop failed");
  }
  // done getting ip addr

  // last initialize all the locks used here
  pthread_mutex_init((&conf_lock), NULL);

  state = STOP;

  next_id = 0;

  for(int i=0; i<=MAX_PREFIX_LEN; ++i)
  {
    mc_routes[i] = NULL;
    uc_routes[i] = NULL;
  }
  for(int i=0; i<=MAX_PRIORITY; ++i)
  {
    pfilters[i] = NULL;
  }

  for(unsigned int i=0; i<5; ++i)
  {
    macs.port_hi16[i] = ms->port_hi16[i];
    macs.port_low32[i] = ms->port_low32[i];
  }

  username = "";

}

Configuration::~Configuration() throw()
{
  if(state == START)
  {
    stop_router();
  }

  pthread_mutex_destroy(&conf_lock);

}

void Configuration::start_router() throw(configuration_exception)
{
  unsigned int addr;
  
  int status;
  
  autoLock glock(conf_lock);

  if(state == START)
  {
    write_log("start_router called, but the router has already been started..");
    return;
  }

  write_log("start_router: entering");

 
    

  // initialize port rates
  port_rates pr;
  pr.port0_rate = DEF_PORT_RATE;
  pr.port1_rate = DEF_PORT_RATE;
  pr.port2_rate = DEF_PORT_RATE;
  pr.port3_rate = DEF_PORT_RATE;
  pr.port4_rate = DEF_PORT_RATE;
  set_port_rates(&pr);


  char logstr[256];
  sprintf(logstr, "start_router: port 0 mac address: %.4x%.8x", macs.port_hi16[0],macs.port_low32[0]);
  write_log(logstr);
  sprintf(logstr, "start_router: port 1 mac address: %.4x%.8x", macs.port_hi16[1],macs.port_low32[1]);
  write_log(logstr);
  sprintf(logstr, "start_router: port 2 mac address: %.4x%.8x", macs.port_hi16[2],macs.port_low32[2]);
  write_log(logstr);
  sprintf(logstr, "start_router: port 3 mac address: %.4x%.8x", macs.port_hi16[3],macs.port_low32[3]);
  write_log(logstr);
  sprintf(logstr, "start_router: port 4 mac address: %.4x%.8x", macs.port_hi16[4],macs.port_low32[4]);
  write_log(logstr);



  write_log("start_router: done initializing route lists");

 

  write_log("start_router: done initializing filter lists");


  write_log("start_router: done initializing arp list");

  /* now get the router base uof file into memory, load the microengines and start them */
  write_log("start_router: done setting up router base code");

  start_mes();

  /* the router is all ready to go now */
  state = START;

  // give the MEs a chance to get going before we do anything else
  sleep(2);

  glock.unlock();

  return;
}

void Configuration::stop_router() throw()
{

  autoLock glock(conf_lock);

  if(state == STOP)
  { 
    write_log("stop_router called, but the router is already stopped..");
    return;
  }
  state = STOP;

  write_log("stop_router: done");

  return;
}

unsigned int Configuration::router_started() throw()
{
  autoLock glock(conf_lock);
  if(state == START) return 1;
  return 0;
}

void
Configuration::configure_port(unsigned int port, std::string ip, std::string nexthop) throw(configuration_exception)
{
  // add default filters so that packets for the router are delivered

  struct in_addr addr;
  inet_pton(AF_INET, ip.c_str(), &addr);
  port_addrs[port] = addr.s_addr;
  write_log("Configuration::configure_port: port=" + int2str(port) + ", ipstr=" + ip + ", ip=" + int2str(port_addrs[port]));

  inet_pton(AF_INET, nexthop.c_str(), &addr);
  next_hops[port] = addr.s_addr;

}


void Configuration::add_route(route_key *key, route_key *mask, route_result *result) throw(configuration_exception)
{
  bool failed = false;
  bool is_multicast = false;
  unsigned int prefix_len;
  write_log("add_route: entering");
  

  // for routes, there are two cases: unicast and multicast
  // for unicast, we do LPM on destination IP addy, so TCAM entries need to be in LP first order on dest IP
  // for multicast, we do exact match on dest IP addy and LPM on source IP addy, so TCAM entries need to be in LP first order on source IP, and all multicast entries should come before the unicast entries

  autoLock glock(conf_lock);
  // get an index in the TCAM for this entry
  if(is_multicast) // multicast route
  {
    unsigned int prefix_length = get_prefix_length(mask->saddr); //not sure we care about this anymore left it in because it was harmless
    write_log("add_route: got multicast route, prefix length " + int2str(prefix_length));

  }
  else // unicast route
  {
    unsigned int prefix_length = get_prefix_length(mask->daddr);
    write_log("add_route: got unicast route, prefix length " + int2str(prefix_length));

  }

  // get an index into SRAM for the result

  glock.unlock();

  if(failed)
  {
    throw configuration_exception("adding the route failed!");
  }

  write_log("add_route: done");

  return;
}

void Configuration::del_route(route_key *key) throw(configuration_exception)
{
  bool failed = false;

  write_log("del_route: entering");

  *k = *key;

  if(failed)
  {
    throw configuration_exception("deleting the route failed!");
  }


  write_log("del_route: done");

  return;
}

unsigned int Configuration::conv_str_to_uint(std::string str) throw(configuration_exception)
{
  const char* cstr = str.c_str();
  char* endptr;
  unsigned int val;
  errno = 0;
  val = strtoul(cstr, &endptr, 0);
  if((errno == ERANGE && (val == ULONG_MAX)) || (errno != 0 && val == 0))
  {
    throw configuration_exception("invalid value");
  }
  if(endptr == cstr)
  {
    throw configuration_exception("invalid value");
  }
  if(*endptr != '\0')
  {
    throw configuration_exception("invalid value");
  }
  return val;
}

unsigned int Configuration::get_proto(std::string proto_str) throw(configuration_exception)
{
  if(proto_str == "icmp" || proto_str == "ICMP")
  {
    return 1;
  }
  if(proto_str == "igmp" || proto_str == "IGMP")
  {
    return 2;
  }
  if(proto_str == "tcp" || proto_str == "tcp")
  {
    return 6;
  }
  if(proto_str == "udp" || proto_str == "udp")
  {
    return 17;
  }
  if(proto_str == "*")
  {
    return WILDCARD_VALUE;
  }
  unsigned int val;
  try
  {
    val = conv_str_to_uint(proto_str);
  }
  catch(std::exception& e)
  {
    throw configuration_exception("protocol string is not valid");
  }
  return val;
}

unsigned int Configuration::get_tcpflags(unsigned int fin, unsigned int syn, unsigned int rst, unsigned int psh, unsigned int ack, unsigned urg) throw(configuration_exception)
{
  unsigned val = 0;

  if(fin == 0 || fin == WILDCARD_VALUE) { }
  else if(fin == 1) { val = val | 1; }
  else { throw configuration_exception("tcpfin value is not valid"); }

  if(syn == 0 || syn == WILDCARD_VALUE) { }
  else if(syn == 1) { val = val | 2; }
  else { throw configuration_exception("tcpsyn value is not valid"); }

  if(rst == 0 || rst == WILDCARD_VALUE) { }
  else if(rst == 1) { val = val | 4; }
  else { throw configuration_exception("tcprst value is not valid"); }

  if(psh == 0 || psh == WILDCARD_VALUE) { }
  else if(psh == 1) { val = val | 8; }
  else { throw configuration_exception("tcppsh value is not valid"); }

  if(ack == 0 || ack == WILDCARD_VALUE) { }
  else if(ack == 1) { val = val | 16; }
  else { throw configuration_exception("tcpack value is not valid"); }

  if(urg == 0 || urg == WILDCARD_VALUE) { }
  else if(urg == 1) { val = val | 32; }
  else { throw configuration_exception("tcpurg value is not valid"); }

  return val;
}

unsigned int Configuration::get_tcpflags_mask(unsigned int fin, unsigned int syn, unsigned int rst, unsigned int psh, unsigned int ack, unsigned urg) throw(configuration_exception)
{
  unsigned val = 0;

  if(fin == WILDCARD_VALUE) { }
  else if(fin == 0 || fin == 1) { val = val | 1; }
  else { throw configuration_exception("tcpfin value is not valid"); }

  if(syn == WILDCARD_VALUE) { }
  else if(syn == 0 || syn == 1) { val = val | 2; }
  else { throw configuration_exception("tcpsyn value is not valid"); }

  if(rst == WILDCARD_VALUE) { }
  else if(rst == 0 || rst == 1) { val = val | 4; }
  else { throw configuration_exception("tcprst value is not valid"); }

  if(psh == WILDCARD_VALUE) { }
  else if(psh == 0 || psh == 1) { val = val | 8; }
  else { throw configuration_exception("tcppsh value is not valid"); }

  if(ack == WILDCARD_VALUE) { }
  else if(ack == 0 || ack == 1) { val = val | 16; }
  else { throw configuration_exception("tcpack value is not valid"); }

  if(urg == WILDCARD_VALUE) { }
  else if(urg == 0 || urg == 1) { val = val | 32; }
  else { throw configuration_exception("tcpurg value is not valid"); }

  return val;
}

unsigned int Configuration::get_exceptions(unsigned int nonip, unsigned int arp, unsigned int ipopt, unsigned int ttl) throw(configuration_exception)
{
  unsigned val = 0;

  if(nonip == 0 || nonip == WILDCARD_VALUE) { }
  else if(nonip == 1) { val = val | 1; }
  else { throw configuration_exception("nonip value is not valid"); }

  if(arp == 0 || arp == WILDCARD_VALUE) { }
  else if(arp == 1) { val = val | 2; }
  else { throw configuration_exception("arp value is not valid"); }

  if(ipopt == 0 || ipopt == WILDCARD_VALUE) { }
  else if(ipopt == 1) { val = val | 4; }
  else { throw configuration_exception("ipopt value is not valid"); }

  if(ttl == 0 || ttl == WILDCARD_VALUE) { }
  else if(ttl == 1) { val = val | 8; }
  else { throw configuration_exception("ttl value is not valid"); }

  return val;
}

unsigned int Configuration::get_exceptions_mask(unsigned int nonip, unsigned int arp, unsigned int ipopt, unsigned int ttl) throw(configuration_exception)
{
  unsigned val = 0;

  if(nonip == WILDCARD_VALUE) { }
  else if(nonip == 0 || nonip == 1) { val = val | 1; }
  else { throw configuration_exception("nonip value is not valid"); }

  if(arp == WILDCARD_VALUE) { }
  else if(arp == 0 || arp == 1) { val = val | 2; }
  else { throw configuration_exception("arp value is not valid"); }

  if(ipopt == WILDCARD_VALUE) { }
  else if(ipopt == 0 || ipopt == 1) { val = val | 4; }
  else { throw configuration_exception("ipopt value is not valid"); }

  if(ttl == WILDCARD_VALUE) { }
  else if(ttl == 0 || ttl == 1) { val = val | 8; }
  else { throw configuration_exception("ttl value is not valid"); }

  return val;
}

unsigned int Configuration::get_pps(std::string pps_str, bool multicast) throw(configuration_exception)
{
  if(pps_str == "plugin(unicast)")
  {
    if(multicast) { throw configuration_exception("invalid port_plugin_selection for multicast filter"); }
    return 1;
  }
  if(pps_str == "port(unicast)")
  {
    if(multicast) { throw configuration_exception("invalid port_plugin_selection for multicast filter"); }
    return 0;
  }
  if(pps_str == "ports and plugins (multicast)")
  {
    if(!multicast) { throw configuration_exception("invalid port_plugin_selection for unicast filter"); }
    return 0;
  }
  if(pps_str == "plugins(multicast)")
  {
    if(!multicast) { throw configuration_exception("invalid port_plugin_selection for unicast filter"); }
    return 1;
  }
  throw configuration_exception("invalid port_plugin_selection");
  return 0;
}

unsigned int Configuration::get_output_port(std::string port_str) throw(configuration_exception)
{
  if(port_str == "")
  {
    return 0;
  }
  if(port_str == "use route")
  {
    return FILTER_USE_ROUTE;
  }
  
  unsigned int val;
  try
  {
    val = conv_str_to_uint(port_str);
  }
  catch(std::exception& e)
  {
    throw configuration_exception("output port is not valid");
  }
  if(val > 4)
  {
    throw configuration_exception("output port is not valid");
  }
  return val;
}

unsigned int Configuration::get_output_plugin(std::string plugin_str) throw(configuration_exception)
{
  if(plugin_str == "")
  {
    return 0;
  }

  unsigned int val;
  try
  {
    val = conv_str_to_uint(plugin_str);
  }
  catch(std::exception& e)
  {
    throw configuration_exception("output plugin is not valid");
  }
  if(val > 4)
  {
    throw configuration_exception("output plugin is not valid");
  }
  return val;
}

unsigned int Configuration::get_outputs(std::string port_str, std::string plugin_str) throw(configuration_exception)
{
  unsigned int ports = 0;
  char portcstr[port_str.size()+1];
  strcpy(portcstr, port_str.c_str());
  char *tok, *saveptr;
  tok = strtok_r(portcstr, ",", &saveptr);
  try
  {
    while(tok)
    {
      std::string tmp = tok;
      unsigned int val = conv_str_to_uint(tmp);
      if(val > 4)
      {
        throw configuration_exception("output port is not valid");
      }
      ports = ports | (1 << val); 
    
      tok = strtok_r(NULL, ",", &saveptr);
    }
  }
  catch(std::exception& e)
  {
    throw configuration_exception("output port is not valid");
  }

  unsigned int plugins = 0;
  char plugincstr[plugin_str.size()+1];
  strcpy(plugincstr, plugin_str.c_str());
  char *tok2, *saveptr2;
  tok2 = strtok_r(plugincstr, ",", &saveptr2);
  try
  {
    while(tok2)
    {
      std::string tmp = tok2;
      unsigned int val = conv_str_to_uint(tmp);
      if(val > 4)
      {
        throw configuration_exception("output plugin is not valid");
      }
      plugins = plugins | (1 << val);

      tok2 = strtok_r(NULL, ",", &saveptr2);
    }
  }
  catch(std::exception& e)
  {
    throw configuration_exception("output plugin is not valid");
  }

  return ((ports << 5) | (plugins));
}


void Configuration::add_filter(pfilter_key *key, pfilter_key *mask, unsigned int priority, pfilter_result *result) throw(configuration_exception)
{
  bool failed = false;

  write_log("add_filter: entering");

  if(priority > MAX_PRIORITY)
  {
    throw configuration_exception("invalid priority");
  }


  autoLock glock(conf_lock);
  glock.unlock();

  if(failed)
  {
    throw configuration_exception("adding the filter failed!");
  }

  write_log("add_filter: done");

  return;
}

void Configuration::del_filter(pfilter_key *key) throw(configuration_exception)
{
  bool failed = false;
  write_log("del_filter: entering");


  if(failed)
  {
    throw configuration_exception("deleting the filter failed!");
  }

  write_log("del_filter: done");

  return;
}

/*
void Configuration::get_sample_rates(aux_sample_rates *rates) throw()
{
  unsigned int addr = COPY_BLOCK_CONTROL_MEM_BASE;

  write_log("get_sample_rates:");
  
  autoLock slock(sram_lock);

  rates->rate00 = SRAM_READ(SRAM_BANK_3, addr);
  rates->rate01 = SRAM_READ(SRAM_BANK_3, addr+4);
  rates->rate10 = SRAM_READ(SRAM_BANK_3, addr+8);
  rates->rate11 = SRAM_READ(SRAM_BANK_3, addr+12);
}

void Configuration::set_sample_rates(aux_sample_rates *rates) throw()
{
  unsigned int addr = COPY_BLOCK_CONTROL_MEM_BASE;

  write_log("set_sample_rates:");

  autoLock slock(sram_lock);

  SRAM_WRITE(SRAM_BANK_3, addr, rates->rate00);
  SRAM_WRITE(SRAM_BANK_3, addr+4, rates->rate01);
  SRAM_WRITE(SRAM_BANK_3, addr+8, rates->rate10);
  SRAM_WRITE(SRAM_BANK_3, addr+12, rates->rate11);
}
*/


unsigned int Configuration::get_queue_quantum(unsigned int port, unsigned int queue) throw()
{

  queue = queue | ((port+1) << 13);
  write_log("get_queue_quantum: queue " + int2str(queue));

  unsigned int quantum = 0;
  return quantum;
}

//JP stopped here 7/8/13
void Configuration::set_queue_quantum(unsigned int port, unsigned int queue, unsigned int quantum) throw()
{
  unsigned int addr = QPARAMS_BASE_ADDR;

  queue = queue | ((port+1) << 13);
  write_log("set_queue_quantum: queue " + int2str(queue) + ", quantum " + int2str(quantum));


  addr += queue * QPARAMS_UNIT_SIZE;
}

unsigned int Configuration::get_queue_threshold(unsigned int port, unsigned int queue) throw()
{
  unsigned int threshold = 0;

  queue = queue | ((port+1) << 13);
  write_log("get_queue_threshold: queue " + int2str(queue));

  return threshold;
}

void Configuration::set_queue_threshold(unsigned int port, unsigned int queue, unsigned int threshold) throw()
{

  queue = queue | ((port+1) << 13);
  write_log("set_queue_threshold: queue " + int2str(queue) + ", threshold " + int2str(threshold));

}

// rates are in Kbps
void Configuration::get_port_rates(port_rates *rates) throw()
{
  unsigned int z = 1;

  write_log("get_port_rates:");

  rates->port0_rate = z * QPORT_RATE_CONVERSION;
  rates->port1_rate = z * QPORT_RATE_CONVERSION;
  rates->port2_rate = z * QPORT_RATE_CONVERSION;
  rates->port3_rate = z * QPORT_RATE_CONVERSION;
  rates->port4_rate = z * QPORT_RATE_CONVERSION;
}

// rates are in Kbps
void Configuration::set_port_rates(port_rates *rates) throw()
{
  unsigned int z;

  write_log("set_port_rates:");
  
  autoLock slock(sram_lock);

  z = rates->port0_rate / QPORT_RATE_CONVERSION;
  z = rates->port1_rate / QPORT_RATE_CONVERSION;

  z = rates->port2_rate / QPORT_RATE_CONVERSION;

  z = rates->port3_rate / QPORT_RATE_CONVERSION;

  z = rates->port4_rate / QPORT_RATE_CONVERSION;
}

// rates should be in Kbps
unsigned int Configuration::get_port_rate(unsigned int port) throw(configuration_exception)
{
  write_log("get_port_rate:");

  if(port > 4)
  {
    throw configuration_exception("invalid port");
  }

  unsigned int z = 1;

  return (z * QPORT_RATE_CONVERSION);
}

// rates should be in Kbps
void Configuration::set_port_rate(unsigned int port, unsigned int rate) throw(configuration_exception)
{
  write_log("set_port_rate:");

  if(port > 4)
  {
    throw configuration_exception("invalid port");
  } 
}

unsigned int Configuration::get_port_mac_addr_hi16(unsigned int port) throw(configuration_exception)
{
  if(port > 4)
  {
    throw configuration_exception("invalid port");
  }

  return macs.port_hi16[port];
}

unsigned int Configuration::get_port_mac_addr_low32(unsigned int port) throw(configuration_exception)
{
  if(port > 4)
  {
    throw configuration_exception("invalid port");
  }

  return macs.port_low32[port];
}

unsigned int Configuration::get_port_addr(unsigned int port) throw()
{
  if(port > 4)
  {
    return 0;
  }
  return port_addrs[port];
}

unsigned int Configuration::get_next_hop_addr(unsigned int port) throw()
{
  if(port > 4)
  {
    return 0;
  }
  return next_hops[port];
}

void Configuration::set_username(std::string un) throw()
{
  username = un;
  write_log("set_username: got user name " + username);
}

std::string Configuration::get_username() throw()
{
  return username;
}

unsigned int Configuration::get_prefix_length(unsigned int addr_mask) throw()
{
  unsigned int length = 0;
  for(int i=31; i>=0; --i)
  {
    if((addr_mask & (1<<i)) != 0)
    {
      ++length;
    }
    else
    {
      break;
    }
  }
  return length;
}

unsigned int Configuration::get_free_index(indexlist **head) throw(configuration_exception)
{
  indexlist *node;
  unsigned int index;

  if((*head) == NULL)
  {
    throw configuration_exception("no more free TCAM entries");
  }

  // we simply take the index from the left side of the first free range
  index = (*head)->left;

  // if the range has more entries, simply update the left value
  if((*head)->left != (*head)->right)
  {
    (*head)->left++;
  }
  // otherwise, remove the current node from the list
  else
  {
    node = *head;
    *head = (*head)->next;
    delete node;
  }

  return index;
}

void Configuration::add_free_index(indexlist **head, unsigned int index) throw()
{
  indexlist *node, *last, *tmp;

  if((*head) == NULL)
  {
    *head = new indexlist();
    (*head)->left = index;
    (*head)->right = index;
    (*head)->next = NULL;
  }
  else
  {
    last = *head;
    node = *head;
    while((node != NULL) && (node->right < index))
    {
      last = node;
      node = node->next;
    }

    if(last->right == (index - 1))
    {
      last->right = index;
      if((node != NULL) && (node->left == (index + 1)))
      {
        last->right = node->right;
        last->next = node->next;
        delete node;
      }
    }
    else if(node == NULL)
    {
      node = new indexlist();
      node->left = index;
      node->right = index;
      node->next = NULL;
      last->next = node;
    }
    else if(node->left == (index + 1))
    {
      node->left = index;
    }
    else
    {
      tmp = new indexlist();
      tmp->left = index;
      tmp->right = index;
      tmp->next = node;
      if(last == node)
      {
        *head = tmp;
      }
      else
      {
        last->next = tmp;
      }
    }
  }
}

void Configuration::free_list_mem(indexlist **list) throw()
{
  indexlist *tmp;
  while(*list != NULL)
  {
    tmp = *list;
    (*list) = (*list)->next;
    delete tmp;
  }
} 

