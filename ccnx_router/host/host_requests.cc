#include <iostream>
#include <sstream>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <exception>
#include <stdexcept>

#include <ctime>

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "shared.h"

#include "host_configuration.h"
#include "host_globals.h"
#include "host_requests.h"

#define ONL_CCNX_DIR "/usr/local/bin" //users/onl/ccnx/ccnx-head"
using namespace host;

namespace {
class error : public std::exception {
  private:
	const char *_what;
  public:
	error(const char *what) : _what(what) {};
	virtual const char *what() const throw() { return _what; };
	virtual ~error() throw() {};
};

std::string dns_resolve(std::string hostname) {
	struct addrinfo *res = 0;
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int errcode;
	if((errcode = getaddrinfo(hostname.c_str(), "9695", &hints, &res))) {
		static std::string msg = "while resolving " + hostname + ": " + std::string(gai_strerror(errcode));
		throw error(msg.c_str());
	}

	/* Just use the first one and don't worry too much about it,
	 * even if this attitude will probably lead to unforeseen and hard
	 * to debug problems 10 years down the line :-)
	 */
	struct in_addr addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	freeaddrinfo(res);

	std::string address = inet_ntoa(addr);
	return address;
}	

} // namespace


configure_node_req::configure_node_req(uint8_t *mbuf, uint32_t size): configure_node(mbuf, size)
{
}

configure_node_req::~configure_node_req()
{
}

bool
configure_node_req::handle()
{
  write_log("configure_node_req::handle()");

  conf->set_port_info(node_conf.getPort(), node_conf.getIPAddr(), node_conf.getSubnet(), node_conf.getNHIPAddr());

  crd_response* resp = new crd_response(this, NCCP_Status_Fine);
  resp->send();
  delete resp;

  return true;
}

add_route_req::add_route_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

add_route_req::~add_route_req()
{
}

bool
add_route_req::handle()
{
  write_log("add_route_req::handle()");

  std::string prefixstr = conf->addr_int2str(prefix);
  if(prefixstr == "")
  {
    write_log("add_route_req: handle(): got bad prefix");
    rli_response* rliresp = new rli_response(this, NCCP_Status_Failed);
    rliresp->send();
    delete rliresp;
    return true;
  }
  
  std::string nhstr = conf->addr_int2str(nexthop_ip);
  if(nhstr == "")
  {
    write_log("add_route_req: handle(): got bad next hop");
    rli_response* rliresp = new rli_response(this, NCCP_Status_Failed);
    rliresp->send();
    delete rliresp;
    return true;
  }
  
  NCCP_StatusType stat = NCCP_Status_Fine;
  if(!conf->add_route(port, prefixstr, mask, nhstr))
  {
    stat = NCCP_Status_Failed;
  }

  rli_response* rliresp = new rli_response(this, stat);
  rliresp->send();
  delete rliresp;

  return true;
}

void
add_route_req::parse()
{
  rli_request::parse();

  prefix = params[0].getInt();
  mask = params[1].getInt();
  output_port = params[2].getInt();
  nexthop_ip = params[3].getInt();
  stats_index = params[4].getInt();
}

delete_route_req::delete_route_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

delete_route_req::~delete_route_req()
{
}

bool
delete_route_req::handle()
{
  write_log("delete_route_req::handle()");

  std::string prefixstr = conf->addr_int2str(prefix);
  if(prefixstr == "")
  {
    write_log("delete_route_req: handle(): got bad prefix");
    rli_response* rliresp = new rli_response(this, NCCP_Status_Failed);
    rliresp->send();
    delete rliresp;
    return true;
  }
  
  NCCP_StatusType stat = NCCP_Status_Fine;
  if(!conf->delete_route(prefixstr, mask))
  {
    stat = NCCP_Status_Failed;
  }

  rli_response* rliresp = new rli_response(this, stat);
  rliresp->send();
  delete rliresp;

  return true;
}

void
delete_route_req::parse()
{
  rli_request::parse();

  prefix = params[0].getInt();
  mask = params[1].getInt();
}

//---------------------- CCNd logging level
set_log_level_req::set_log_level_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

set_log_level_req::~set_log_level_req()
{
}

bool
set_log_level_req::handle()
{
  write_log("set_log_level_req::handle()");

  //std::string t = ONL_CCNX_DIR + "/ccnx/bin/ccndlogging " + level; 
  //std::string t = ONL_CCNX_DIR + "/ccndlogging " + level; 
  std::string t = "/usr/local/bin/ccndlogging " + level; 
  int rtn = std::system(t.c_str());
  write_log("set_log_level_req::handle system(" + t +") returned " + int2str(rtn));
  NCCP_StatusType stat = NCCP_Status_Fine;
  //if(rtn != 0)
  //{
   // stat = NCCP_Status_Failed;
  //}

  rli_response* rliresp = new rli_response(this, stat);
  rliresp->send();
  delete rliresp;

  return true;
}

void
set_log_level_req::parse()
{
  rli_request::parse();

  level = params[0].getString();
}
// --------------- Monitoring stuff ----------------------

#define QUERY_PART_1 "/sbin/iptables -nvx -L "
// then chain name
#define QUERY_PART_2 " -Z | /usr/bin/tail -n-2 | /usr/bin/head -1 | /bin/sed -e 's/[[:blank:]]\\+/ /g' | /bin/cut -d\" \" -f "
// then field number (2 for packets, 3 for bytes)

get_rx_pkt_req::get_rx_pkt_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_rx_pkt_req::~get_rx_pkt_req()
{
}

bool
get_rx_pkt_req::handle()
{
  rli_response* rliresp;
  FILE *fp = 0;
  try
  {
	std::ostringstream query;
	query << QUERY_PART_1 << "MONITOR_PKT_IN" << QUERY_PART_2 << "2"; 
	fp = popen(query.str().c_str(), "r");
	if(!fp)
		throw error("cannot execute command");

	uint32_t result;
	char tmp[1024];
	if(!fgets(tmp, 1024, fp))
		throw error("cannot read command output");

	if(sscanf(tmp, "%u", &result) != 1)
		throw error("cannot parse string");

	std::ostringstream ss;
	ss << "get_rx_pkt_req::handle(): read " << result << " packets since last call";
	write_log(ss.str().c_str());

    rliresp = new rli_response(this, NCCP_Status_Fine, result);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_rx_pkt_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  if(fp != 0)
	fclose(fp);

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_rx_pkt_req::parse()
{
  rli_request::parse();
}
 
get_rx_byte_req::get_rx_byte_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_rx_byte_req::~get_rx_byte_req()
{
}

bool
get_rx_byte_req::handle()
{
  rli_response* rliresp;
  FILE *fp;
  try
  {
	std::ostringstream query;
	query << QUERY_PART_1 << "MONITOR_BYTE_IN" << QUERY_PART_2 << "3"; 
	fp = popen(query.str().c_str(), "r");
	if(!fp)
		throw error("cannot execute command");

	uint32_t result;
	char tmp[1024];
	if(!fgets(tmp, 1024, fp))
		throw error("cannot read command output");

	if(sscanf(tmp, "%u", &result) != 1)
		throw error("cannot parse string");
    rliresp = new rli_response(this, NCCP_Status_Fine, result);

	std::ostringstream ss;
	ss << "get_rx_byte_req::handle(): read " << result << " bytes since last call";
	write_log(ss.str().c_str());
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_rx_byte_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  if(fp != 0)
	fclose(fp);

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_rx_byte_req::parse()
{
  rli_request::parse();
}
 
get_tx_pkt_req::get_tx_pkt_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_tx_pkt_req::~get_tx_pkt_req()
{
}

bool
get_tx_pkt_req::handle()
{
  rli_response* rliresp;
  FILE *fp;
  try
  {
	std::ostringstream query;
	query << QUERY_PART_1 << "MONITOR_PKT_OUT" << QUERY_PART_2 << "2"; 
	fp = popen(query.str().c_str(), "r");
	if(!fp)
		throw error("cannot execute command");

	uint32_t result;
	char tmp[1024];
	if(!fgets(tmp, 1024, fp))
		throw error("cannot read command output");

	if(sscanf(tmp, "%u", &result) != 1)
		throw error("cannot parse string");
    rliresp = new rli_response(this, NCCP_Status_Fine, result);

	std::ostringstream ss;
	ss << "get_tx_pkt_req::handle(): read " << result << " packets since last call";
	write_log(ss.str().c_str());
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_tx_pkt_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  if(fp != 0)
	fclose(fp);

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_tx_pkt_req::parse()
{
  rli_request::parse();
}
 
get_tx_byte_req::get_tx_byte_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_tx_byte_req::~get_tx_byte_req()
{
}

bool
get_tx_byte_req::handle()
{
  rli_response* rliresp;
  FILE *fp;
  try
  {
	std::ostringstream query;
	query << QUERY_PART_1 << "MONITOR_BYTE_OUT" << QUERY_PART_2 << "3"; 
	fp = popen(query.str().c_str(), "r");
	if(!fp)
		throw error("cannot execute command");

	uint32_t result;
	char tmp[1024];
	if(!fgets(tmp, 1024, fp))
		throw error("cannot read command output");

	if(sscanf(tmp, "%u", &result) != 1)
		throw error("cannot parse string");
    rliresp = new rli_response(this, NCCP_Status_Fine, result);

	std::ostringstream ss;
	ss << "get_tx_byte_req::handle(): read " << result << " bytes since last call";
	write_log(ss.str().c_str());
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_tx_byte_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  if(fp != 0)
	fclose(fp);

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_tx_byte_req::parse()
{
  rli_request::parse();
}

// --------------- Memory and CPU usage ------------------

#ifdef QUERY_PART_1
#undef QUERY_PART_1
#endif

#define QUERY_PART_1 "/usr/bin/free | head -n3 | tail -n1 | cut -d\":\" -f 2"

get_mem_usage_req::get_mem_usage_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_mem_usage_req::~get_mem_usage_req()
{
}

bool
get_mem_usage_req::handle()
{
  rli_response* rliresp;
  FILE *fp;
  try
  {
	fp = popen(QUERY_PART_1, "r");
	if(!fp)
		throw error("cannot execute command");

	uint32_t used, free;
	char tmp[1024];
	if(!fgets(tmp, 1024, fp))
		throw error("cannot read command output");

	if(sscanf(tmp, "%u%u", &used, &free) != 2)
		throw error("cannot parse string");

	double _used = used;
	_used = _used * 100.0 / (_used + free);

    rliresp = new rli_response(this, NCCP_Status_Fine, (uint32_t)_used);

	std::ostringstream ss;
	ss << "get_mem_usage_req::handle(): free: " << free << " bytes, used: " << used << " bytes (" << _used << "%)";
	write_log(ss.str().c_str());
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_mem_usage_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  if(fp != 0)
	fclose(fp);

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_mem_usage_req::parse()
{
  rli_request::parse();
}

#ifdef QUERY_PART_1
#undef QUERY_PART_1
#endif

#define QUERY_PART_1 "/usr/bin/uptime | sed -e 's/[[:blank:]]\\+/ /g' | cut -d\",\" -f 4 | cut -d\":\" -f 2"

get_cpu_usage_req::get_cpu_usage_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_cpu_usage_req::~get_cpu_usage_req()
{
}

bool
get_cpu_usage_req::handle()
{
  rli_response* rliresp;
  FILE *fp;
  try
  {
	fp = popen(QUERY_PART_1, "r");
	if(!fp)
		throw error("cannot execute command");

	float load;
	char tmp[1024];
	if(!fgets(tmp, 1024, fp))
		throw error("cannot read command output");

	if(sscanf(tmp, "%g", &load) != 1)
		throw error("cannot parse string");

	load *= 100;
    rliresp = new rli_response(this, NCCP_Status_Fine, (uint32_t)load);

	std::ostringstream ss;
	ss << "get_cpu_usage_req::handle(): cpu load: " << load << "%";
	write_log(ss.str().c_str());
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_cpu_usage_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
  }

  if(fp != 0)
	fclose(fp);

  rliresp->send();
  delete rliresp;

  return true;
}

void
get_cpu_usage_req::parse()
{
  rli_request::parse();
}

// --------------- CCN role implementation ---------------

get_do_role_req::get_do_role_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_do_role_req::~get_do_role_req()
{
}

bool
get_do_role_req::handle()
{
	assert(params.size() == 3);

	// XXX: for now just print the parameters and be happy about it.
	// Param order: role - path - router
	std::string role = params[0].getString();
	std::string path = params[1].getString();
	//std::string router = params[2].getString();

	write_log("get_do_role_req::handle(): got called with role = " + role + ", path = " + path );//+ ", router = " + router);

	rli_response *rliresp;

	try {
		// Check parameter sanity
		if(role != "producer" && role != "consumer")
			throw error("unknown role");

		// Resolve router host name
	/*	std::string address = dns_resolve(router);

		std::ostringstream ss;
		ss << "get_do_role_req::handle(): resolved address for " << router << " is " << address;
		write_log(ss.str().c_str());
*/
		std::string t;

#if 0
		// Launch proxy
 		t = "sudo -i -u " + host::username + " ";
		t += "/users/" + host::username + "/ccnx/scripts/start_proxy.sh " + address +
			" 9695 2>&1 > /tmp/proxy.txt &";
		std::system(t.c_str());
		write_log("get_do_role_req::handle(): executed: " + t);
		std::system("chmod a+r /tmp/proxy.txt");
#endif


		// Launch producer/consumer
 		//t = "sudo -i -u " + host::username + " ";
 		t = "";
		if(role == "consumer") {
			t += "/users/onl/pc1core/ccnx/scripts/launch_consumer.sh " + path +
				" 2>&1 > /tmp/consumer.txt &";
			std::system(t.c_str());
			std::system("chmod a+r /tmp/consumer.txt");
		} else if(role == "producer") {
			t += "/users/onl/pc1core/ccnx/scripts/launch_producer.sh " + path +
				" 2>&1 > /tmp/producer.txt &";
			std::system(t.c_str());
			std::system("chmod a+r /tmp/producer.txt");
		}

		write_log("get_do_role_req::handle(): executed: " + t);

		rliresp = new rli_response(this, NCCP_Status_Fine);
	} catch(std::exception& e) {
		std::string msg = e.what();
		write_log("get_do_role_req::handle(): got exception: " + msg);
		rliresp = new rli_response(this, NCCP_Status_Failed, msg);
	}

	rliresp->send();
	delete rliresp;

	return true;
}

void
get_do_role_req::parse()
{
  rli_request::parse();
}

get_do_daemon_req::get_do_daemon_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_do_daemon_req::~get_do_daemon_req()
{
}

bool
get_do_daemon_req::handle()
{
	write_log("get_do_daemon_req::handle(): got called");

	rli_response *rliresp;

	//int role = params[0].getInt();
	//std::string router = params[1].getString();

	try {
		std::string t;

		//std::string prologue = "sudo -i -u " + host::username + " /users/onl/ccnx/scripts/";
		std::string prologue = "/users/onl/ccnx/scripts/";

		// Kill all proxies and daemons (so don't use it if you want to preserve existing connections)
		//std::system((prologue + "kill_proxy.sh").c_str());
		std::system((prologue + "kill_daemon.sh").c_str());
		
		write_log("get_do_daemon_req::handle(): killed daemon.");
		// Start either a daemon or a proxy
		//switch(role) {
		//	case 1:
				t = prologue + "launch_daemon.sh 2>&1 > /tmp/ccnd.log &";
				std::system(t.c_str());
				std::system("chmod a+r /tmp/ccnd.log");
				write_log("get_do_daemon_req::handle(): executed: " + t);
		//	break;

		//case 0: {
		//	std::string address = dns_resolve(router);
		//	t = prologue + "start_proxy.sh " + address + " 9695 2>&1 > /tmp/proxy.txt &";
		//	std::system(t.c_str());
		//	std::system("chmod a+r /tmp/proxy.txt");
		//	write_log("get_do_daemon_req::handle(): executed: " + t);
		//	break;
		//}

		//default:
		//	throw error("cannot invalid daemon/proxy choice");
		//

		// Wait for a little while for daemon/proxy to come up
		struct timespec interval;
		interval.tv_sec = 1;
		interval.tv_nsec = 0;

		nanosleep(&interval, 0);

		rliresp = new rli_response(this, NCCP_Status_Fine);
	} catch(std::exception& e) {
		std::string msg = e.what();
		write_log("get_do_daemon_req::handle(): got exception: " + msg);
		rliresp = new rli_response(this, NCCP_Status_Failed, msg);
	}

	rliresp->send();
	delete rliresp;
	return true;
}

void
get_do_daemon_req::parse()
{
  rli_request::parse();
}


// --------------- CCNx init -------------------

get_subtype_init_req::get_subtype_init_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_subtype_init_req::~get_subtype_init_req()
{
}

bool
get_subtype_init_req::handle()
{
	write_log("get_subtype_init_req::handle(): got called");

	rli_response *rliresp;

	int run_daemon = params[0].getBool();
	std::string router = params[1].getString();
	std::string role = params[2].getString();
	std::string path = params[3].getString();

	try {
		std::string t;

		//std::string prologue = "sudo -i -u " + host::username + " /users/onl/ccnx/scripts/";
		std::string prologue = "/users/onl/ccnx/scripts/";

		// Kill all proxies and daemons (so don't use it if you want to preserve existing connections)
		//std::system((prologue + "kill_proxy.sh").c_str());
		std::system((prologue + "kill_daemon.sh").c_str());
		
		// Start either a daemon or a proxy
		//switch(run_daemon) {
		//	case 1:
				t = prologue + "launch_daemon.sh 2>&1 > /tmp/ccnd.log &";

				// XXX: lock here so no add route commands arrive before we process the pending list
				pthread_mutex_lock(&host::mutex);
				write_log("get_subtype_init_req::handle(): in mutex");

				std::system(t.c_str());
				std::system("chmod a+r /tmp/ccnd.log");
				write_log("get_subtype_init_req::handle(): executed: " + t);

				// Wait for a little while for daemon/proxy to come up
				// XXX: it's bad design to sleep with the mutex held, but that's the way it is for now.
				{
					struct timespec interval;
					interval.tv_sec = 3;
					interval.tv_nsec = 0;
					nanosleep(&interval, 0);
				}

				// XXX: we should fill in the routing table around here. Maybe wait a little while before.
				assert(paths.size() == nexthops.size());
				for(size_t i = 0; i < paths.size(); ++i) {
					std::string ip = dns_resolve(nexthops[i]);

					//std::string t = "sudo -i -u " + host::username + " /users/onl/ccnx/scripts/add_route.sh " + path + " " + ip;
					std::string t = "/users/onl/ccnx/scripts/add_route.sh " + path + " " + ip;
					std::system(t.c_str());
					write_log("get_subtype_init_req::handle(): executed: " + t);
				}

				write_log("get_subtype_init_req::handle(): releasing mutex");
				pthread_mutex_unlock(&host::mutex); // XXX: check that this is always executed (exception-safe?)

		//		break;
/*
			case 0: {
				std::string address = dns_resolve(router);
				t = prologue + "start_proxy.sh " + address + " 9695 2>&1 > /tmp/proxy.txt &";
				std::system(t.c_str());
				std::system("chmod a+r /tmp/proxy.txt");
				write_log("get_subtype_init_req::handle(): executed: " + t);
				break;
			}

			default:
				throw error("invalid daemon/proxy choice");
		}*/

		// Wait for a little while for daemon/proxy to come up
		struct timespec interval;
		interval.tv_sec = 1;
		interval.tv_nsec = 0;

		nanosleep(&interval, 0);


		// Launch producer/consumer
 		t = "";//sudo -i -u " + host::username + " ";

		if(role == "consumer") {
			t += "/users/onl/ccnx/scripts/launch_consumer.sh " + path +
				" 2>&1 > /tmp/consumer.txt &";
			std::system(t.c_str());
			std::system("chmod a+r /tmp/consumer.txt");
		} else if(role == "producer") {
			t += "/users/onl/ccnx/scripts/launch_producer.sh " + path +
				" 2>&1 > /tmp/producer.txt &";
			std::system(t.c_str());
			std::system("chmod a+r /tmp/producer.txt");
		} else if(role == "none") {
			t += "<nothing>";
		}

		write_log("get_subtype_init_req::handle(): executed: " + t);

		rliresp = new rli_response(this, NCCP_Status_Fine);
	} catch(std::exception& e) {
		std::string msg = e.what();
		write_log("get_subtype_init_req::handle(): got exception: " + msg);
		rliresp = new rli_response(this, NCCP_Status_Failed, msg);
	}

	rliresp->send();
	delete rliresp;
	return true;
}

void
get_subtype_init_req::parse()
{
  rli_request::parse();
}



// --------------- CCNx routing -------------------

void
get_add_route_req::parse()
{
  rli_request::parse();
}

get_add_route_req::get_add_route_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_add_route_req::~get_add_route_req()
{
}

bool
get_add_route_req::handle()
{
	write_log("get_add_route_req::handle(): got called");

	assert(params.size() == 2);

	// param order: path, next hop
	std::string path = params[0].getString();
	std::string nh	 = params[1].getString();

	rli_response *rliresp;

	/* XXX: it might be the case that this gets called BEFORE the daemon starts.
	 * To handle this case we save the list of routes, and read it back from
	 * the subtype init command. */
	pthread_mutex_lock(&mutex);
	write_log("get_add_route_req::handle(): in mutex");
	paths.push_back(path);
	nexthops.push_back(nh);
	write_log("get_add_route_req::handle(): releasing mutex");
	pthread_mutex_unlock(&mutex);

	try {
		std::string ip = dns_resolve(nh);

		//std::string t = "sudo -i -u " + host::username + " /users/onl/ccnx/scripts/add_route.sh " + path + " " + ip;
		std::string t = "/users/onl/ccnx/scripts/add_route.sh " + path + " " + ip;
		std::system(t.c_str());
		write_log("get_add_route_req::handle(): executed: " + t);

		rliresp = new rli_response(this, NCCP_Status_Fine);
	} catch(std::exception& e) {
		std::string msg = e.what();
		write_log("get_add_route_req::handle(): got exception: " + msg);
		rliresp = new rli_response(this, NCCP_Status_Failed, msg);
	}

	rliresp->send();
	delete rliresp;

	return true;
}



get_del_route_req::get_del_route_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_del_route_req::~get_del_route_req()
{
}

bool
get_del_route_req::handle()
{
	write_log("get_del_route_req::handle(): got called");

	assert(params.size() == 2);

	// param order: path, next hop
	std::string path = params[0].getString();
	std::string nh	 = params[1].getString();

	rli_response *rliresp;

	try {
		std::string ip = dns_resolve(nh);

		//std::string t = "sudo -i -u " + host::username + " /users/onl/ccnx/scripts/del_route.sh " + path + " " + ip;
		std::string t = "/users/onl/ccnx/scripts/del_route.sh " + path + " " + ip;
		std::system(t.c_str());
		write_log("get_del_route_req::handle(): executed: " + t);

		rliresp = new rli_response(this, NCCP_Status_Fine);
	} catch(std::exception& e) {
		std::string msg = e.what();
		write_log("get_del_route_req::handle(): got exception: " + msg);
		rliresp = new rli_response(this, NCCP_Status_Failed, msg);
	}

	rliresp->send();
	delete rliresp;

	return true;
}

void
get_del_route_req::parse()
{
  rli_request::parse();
}



get_update_route_req::get_update_route_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

get_update_route_req::~get_update_route_req()
{
}

bool
get_update_route_req::handle()
{
	write_log("get_update_route_req::handle(): got called");

	std::ostringstream ss;
	for(size_t i = 0; i < params.size(); ++i)
		ss << "params[" << i << "]: " << params[i].getString() << '\n';

	write_log("get_update_route_req::handle(): params:\n" + ss.str());

	rli_response *rliresp;

	try {
		rliresp = new rli_response(this, NCCP_Status_Fine);
	} catch(std::exception& e) {
		std::string msg = e.what();
		write_log("get_del_route_req::handle(): got exception: " + msg);
		rliresp = new rli_response(this, NCCP_Status_Failed, msg);
	}

	rliresp->send();
	delete rliresp;

	return true;
}

void
get_update_route_req::parse()
{
  rli_request::parse();
}



// --------------- CCNx-specific stuff -------------------

req_logger::req_logger(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size) {
	// nop
	return;
}

req_logger::~req_logger() {
	// nop
	return;
}


void req_logger::parse() {
	rli_request::parse();
	return;
}

bool
req_logger::handle()
{
	std::string msg = "req_logger::handle: ";
	msg += "id: " + id;
	msg += ", port: " + port;
	msg += ", param count: " + params.size();

	write_log(msg);

	rli_response* resp = new rli_response(this, NCCP_Status_Fine);
	resp->send();
	delete resp;

	return true;
}

