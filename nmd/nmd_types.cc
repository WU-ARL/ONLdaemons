//
// Copyright (c) 2009-2013 Mart Haitjema, Charlie Wiseman, Jyoti Parwatikar, John DeHart 
// and Washington University in St. Louis
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
//    limitations under the License.
//
//

#include <algorithm>
#include <boost/shared_ptr.hpp>
#include "nmd_includes.h"

vlan::vlan()
{
  // initialize lock
  pthread_mutex_init(&lock, NULL);
}

vlan::~vlan()
{
  pthread_mutex_destroy(&lock);
}

void vlan::set_vlan_id(switch_vlan vlan_id)
{
  vlanId = vlan_id;
}

bool vlan::set_in_use(bool in_use, std::string sid)
{
  autoLock vlock(lock);
  if (in_use && inUse) return false;
  else
    {
      inUse = in_use;
      sessionID = sid;
      return true;
    } 
}

port_list vlan::get_switch_ports(string switch_id)
{
  // unfortunately when we configure a vlan on a switch
  // we have to specify all ports at time
  // Here we simply iterate through all ports on this vlan
  // and extract those that match the switch for
  // the port we are adding
  port_list switch_ports;
  for (port_iter iter = ports.begin();
       iter != ports.end(); iter++) {
    if (iter->getSwitchId() == switch_id) {
      switch_ports.push_back(*iter);
    }
  }

  return switch_ports;
}

pthread_mutex_t&
vlan::get_switch_lock(string switch_id)
{
  if (switch_locks.find(switch_id) == switch_locks.end())
    {
      //this lock has not been created but needs to be
      pthread_mutex_init(&switch_locks[switch_id], NULL);
    }
  return (switch_locks[switch_id]);
}

bool vlan::add_port(switch_port p)
{
  autoLock vlock(lock);//lock the vlan to update state
  bool status = true;

  ports.push_back(p);
  vlock.unlock();//unlock the vlan

  return status;
}

bool vlan::get_add_ports_cmd(string switch_id, ostringstream& cmd)
{
  autoLock slock(get_switch_lock(switch_id)); //lock the switch for this vlan
  autoLock vlock(lock);//lock the vlan to update state
  bool status = true;
  
  // for this switch, get the list of ports on this vlan
  port_list switch_ports = get_switch_ports(switch_id);

  vlock.unlock();//unlock the vlan


  // for this switch, update the set of ports on this vlan
  status &= set_switch_vlan_membership(switch_ports, switch_id, vlanId, cmd);

  port_iter p;
  for (p = switch_ports.begin(); p != switch_ports.end(); ++p)
    {
      // set the PVID so that untagged packets will be tagged
      // with this VLAN ID
      if (!(p->getPassTag()))
	status &= set_switch_pvid(*p, p->getSwitchId(), vlanId, cmd);
    }

  return status;
}

bool vlan::delete_port(switch_port p)
{
  autoLock slock(get_switch_lock(p.getSwitchId()));
  autoLock vlock(lock);
  bool status = true;
 
  ports.remove(p);

  // for this switch, get the list of ports on this vlan
  port_list switch_ports = get_switch_ports(p.getSwitchId());

  vlock.unlock();
  // for this switch, update the set of ports on this vlan
  ostringstream cmd;
  start_command(p.getSwitchId(), cmd);
  status &= set_switch_vlan_membership(switch_ports, p.getSwitchId(), vlanId, cmd);

  // Set the PVID back to the default VLAN for no experiment
  // This shouldn't be necessary but we do it to be safe
  status &= set_switch_pvid(p, p.getSwitchId(), default_vlan, cmd);
  status &= send_command(p.getSwitchId(), cmd);
 

  return status;
}

void vlan::initialize()
{
  ports.clear();
  inUse = false;
}

bool vlan::clear_vlan()
{
  autoLock vlock(lock);
  // iterate through ports and build list of switches to clear vlan state from
  bool status = true;
  list<string> switch_list;
  list<string>::iterator siter;
  for (port_iter iter = ports.begin();
       iter != ports.end(); iter++) {
    siter = find(switch_list.begin(), switch_list.end(), iter->getSwitchId());
    if (siter == switch_list.end()) {
      switch_list.push_back(iter->getSwitchId());
    } 
  }
 
  port_list no_ports; // create empty list of ports
  for (siter = switch_list.begin();
       siter != switch_list.end(); siter++) {
    // for this switch, the ports on this vlan need to have their
    // PVIDs set back to the default vlan for no experiment
    port_list pvidPorts = get_switch_ports(*siter);
    ostringstream cmd;
    start_command(*siter, cmd);
    status &= set_switch_pvids(pvidPorts, *siter, default_vlan, cmd);
    // no ports on this switch should be a member of this vlan
    status &= set_switch_vlan_membership(no_ports, *siter, vlanId, cmd);
    send_command(*siter, cmd);
  }

  ports.clear();

  std::map<std::string, pthread_mutex_t>::iterator sliter;
  for (sliter = switch_locks.begin(); sliter != switch_locks.end(); ++sliter)
    pthread_mutex_destroy(&(sliter->second));
  switch_locks.clear();

  inUse = false;
  sessionID = "";

  return status;
}

//generates clear vlan cmd for the given switch and returns number of ports
//removed from the vlan
int vlan::get_clear_vlan_cmd(string switch_id, ostringstream& cmd)
{
  autoLock vlock(lock);
  // iterate through ports and build list of switches to clear vlan state from
  bool status = true;
  port_list removals;
  for (port_iter iter = ports.begin();
       iter != ports.end(); iter++) {
    if (iter->getSwitchId() == switch_id){
      removals.push_back(*iter);
    } 
  }
 
  if (removals.empty()) return 0; //this switch isn't in this vlan

  port_list no_ports; // create empty list of ports
  // for this switch, the ports on this vlan need to have their
  // PVIDs set back to the default vlan for no experiment
  port_list pvidPorts = get_switch_ports(switch_id);
  status &= set_switch_pvids(pvidPorts, switch_id, default_vlan, cmd);
  // no ports on this switch should be a member of this vlan
  status &= set_switch_vlan_membership(no_ports, switch_id, vlanId, cmd);
  
  for (port_iter iter = removals.begin(); iter != removals.end(); ++iter)
    ports.remove(*iter);

  if (ports.empty())
    {
      switch_locks.clear();
      
      inUse = false;
      sessionID = "";
    }

  return pvidPorts.size();
}

void vlan::print_ports()
{
  autoLock vlock(lock);
  ostringstream msg;
  msg << "vlan " << vlanId << " for session: " << sessionID << " has ports:" << endl;
  port_iter iter;
  for (iter = ports.begin(); iter != ports.end(); iter++)
  {
    msg << *iter << " ";
  }
  write_log(msg.str());
}

vlan_set::vlan_set(uint32_t num_vlans_) : num_vlans(num_vlans_)
{
  uint32_t i;
  switch_vlan vlan_num = 2;
  vlans = new vlan[num_vlans_];
  for (i = 0; i < num_vlans; i++) {
    vlans[i].set_vlan_id(vlan_num + i);
    vlans[i].set_in_use(false, "");   
  }

  // initialize lock
  pthread_mutex_init(&lock, NULL);
}

vlan_set::~vlan_set()
{
  // is this right?
  delete vlans;
}

void vlan_set::initialize_vlans()
{
  uint32_t i;
  for (i = 0; i < num_vlans; i++) {
    vlans[i].initialize();   
  }
  sessions.clear();
}


vlan *vlan_set::get_vlan(switch_vlan vlan_id)
{
  uint32_t i = 0;
  for (i = 0; i < num_vlans; i++) {
    if (vlans[i].vlan_id() == vlan_id) {
      return &vlans[i];
    }
  }
  return NULL;
}

uint32_t vlan_set::get_vlan_index(switch_vlan vlan_id)
{
  uint32_t i = 0;
  for (i = 0; i < num_vlans; i++) {
    if (vlans[i].vlan_id() == vlan_id) {
      return i;
    }
  }
  return VLAN_NOT_FOUND;
}

vlan *vlan_set::get_vlan_by_index(uint32_t index)
{
  if (index >= num_vlans) return NULL;
  return &vlans[index];
}


bool vlan_set::add_vlan(switch_vlan &vlan_id, std::string sid)
{
  //autoLock slock(lock);
  vlan_id = alloc_vlan(sid);
  if (vlan_id == NO_FREE_VLANS) {
    write_log("add_link_req::handle() no free vlans!");
    return false;
  }
 return true;
}


session_ptr vlan_set::get_session(std::string sid)
{
  autoLock slock(lock);//lock the set so no changes to the sessions happen
  std::list<session_ptr>::iterator siter;
  for (siter = sessions.begin(); siter != sessions.end(); ++siter)
    {
      if ((*siter)->get_session_id() == sid)
	{
	  return *siter;
	}
    }
  session_ptr no_ptr;
  return no_ptr;
}

void vlan_set::remove_session(session_ptr sptr)
{
  autoLock slock(lock);
  if (find(sessions.begin(), sessions.end(), sptr) != sessions.end()) sessions.remove(sptr);
}

bool vlan_set::delete_vlan(switch_vlan vlan_id, std::string sid)
{
  //autoLock slock(lock);
  session_ptr sptr = get_session(sid);
  bool rtn = true;
  if (sptr)
    {
      rtn = sptr->remove_vlan(vlan_id);
      if (sptr->is_cleared()) remove_session(sptr);
    }
  //uint32_t vlan_index = get_vlan_index(vlan_id);
  //vlans[vlan_index].clear_vlan(); 
  return rtn;
}
/*
bool vlan_set::add_to_vlan(switch_vlan vlan_id, switch_port p, std::string sid)//add_to_vlan_req* const req, std::string sid)
{
  session_ptr sptr = get_session(sid);
  if (sptr)
    {
      vlan* v = get_vlan(vlan_id);
      if (v != NULL)
	{
	  v->add_port(p);
	  return (sptr->add_switch(p.getSwitchId()));
	}
    }
  return false;
}
*/
bool vlan_set::create_session_vlans(std::string sid)
{
  session_ptr sptr = get_session(sid);
  if (sptr)
    {
      return (sptr->create_vlans());
    }
  return false;
}

bool vlan_set::add_to_vlan(switch_vlan vlan_id, switch_port port, std::string sid)
{
  //autoLock slock(lock);
  ostringstream msg;
  session_ptr sptr = get_session(sid);
  if (sptr)
    {
      vlan *curVlan = get_vlan(vlan_id);
      if (!curVlan->is_in_use()) 
	{ 
	  msg << "add_to_vlan() could not find vlan " << vlan_id;
	  write_log(msg.str());
	  return false;
	}

      if (curVlan->get_session_id() == sid)
	{
	  curVlan->add_port(port);
	  sptr->add_switch(port.getSwitchId());
	  //curVlan->print_ports();
	}
      else
	{ 
	  msg << "add_to_vlan() vlan " << vlan_id << " failed for session " << sid << " current session:" << curVlan->get_session_id();
	  write_log(msg.str());
	  return false;
	}
    }
  else
    {
      msg << "add_to_vlan() vlan " << vlan_id << " failed for session " << sid << " current session: session invalid";
      write_log(msg.str());
      return false;
    }
  return true;
}

 //this shouldn't get called anymore
bool vlan_set::delete_from_vlan(switch_vlan vlan_id, switch_port port, std::string sid)
{
  //autoLock slock(lock);
  ostringstream msg;
  vlan *curVlan = get_vlan(vlan_id);
  if (!curVlan->is_in_use()) 
  { 
    msg << "delete_from_vlan() could not find vlan " << vlan_id;
    write_log(msg.str());
    return false;
  }

  if (curVlan->get_session_id() == sid)
    {
      curVlan->delete_port(port);
      curVlan->print_ports();  
    }
  else
    { 
      msg << "delete_from_vlan() vlan " << vlan_id << " failed for session " << sid << " current session:" << curVlan->get_session_id();
      write_log(msg.str());
      return false;
    }

  return true;
}


// Private members

switch_vlan vlan_set::alloc_vlan(std::string sid)
{
  //autoLock slock(lock);
  uint32_t i = 0;
  for (i = 0; i < num_vlans; i++) {
    if (vlans[i].set_in_use(true, sid))
      {
	session_ptr sptr = get_session(sid);
	if (!sptr)//create the session if it hasn't already been
	  {
	    session_ptr new_sptr(new session(sid));
	    sptr = new_sptr;
	    autoLock slock(lock);//lock to add new session
	    sessions.push_back(sptr);
	    slock.unlock();
	  }
	sptr->add_vlan(vlans[i].vlan_id());
	return vlans[i].vlan_id();
      }
  }
  return NO_FREE_VLANS;
}


switch_info_set::switch_info_set()
{
}

switch_info_set::~switch_info_set()
{
}

void switch_info_set::set_switches(list<switch_info> switches_)
{
  switches = switches_;
  initialize_ssh_locks(switches);
}

switch_info switch_info_set::get_switch(string switch_id)
{
  // return a copy of the switch_info object specified by 
  // switch_id
  // if it doesn't exist, return a switch_info object
  // with switch_info not found
  list<switch_info>::iterator iter;
  for (iter = switches.begin();
       iter != switches.end(); iter++) {
    if (iter->getSwitchId() == switch_id) return *iter;
  }
  return switch_info("not_found", 0);
}

uint32_t switch_info_set::get_num_ports(string switch_id)
{
  ostringstream msg;
  switch_info eth_switch = get_switch(switch_id);
  if (eth_switch.getSwitchId() != switch_id) {
    msg << endl << "ERROR: Could not find switch "
        << switch_id << "!" << endl;
    write_log(msg.str());
    return 0 ;
  }
  return eth_switch.getNumPorts();
}

uint32_t switch_info_set::get_management_port(string switch_id)
{
  ostringstream msg;
  switch_info eth_switch = get_switch(switch_id);
  if (eth_switch.getSwitchId() != switch_id) {
    msg << endl << "ERROR: Could not find switch "
        << switch_id << "!" << endl;
    write_log(msg.str());
    return 0 ;
  }
  return eth_switch.getManagementPort();
}


session::session(std::string str)
{
  sessionID = str;
  cleared = false;
  
  // initialize lock
  pthread_mutex_init(&lock, NULL);
}


session::~session()
{
  if (!cleared) clear_session();
  pthread_mutex_destroy(&lock);
}

bool session::add_switch(std::string swid)
{
  autoLock slock(lock);
  if (cleared) return false;
  if (find(switches.begin(), switches.end(), swid) == switches.end())
    switches.push_back(swid);
  write_log("session(" + sessionID + ")::add_switch " + swid);
  return true;
}

/*
bool session::add_request(nmd::add_to_vlan_req* const req)
{
  autoLock slock(lock);
  if (cleared) return false;
  switch_requests[req->get_port().getSwitchId()].push_back(req);
  return true;
}
*/
bool session::create_vlans()
{
  autoLock slock(lock);
  if (cleared) 
    {
      write_log("session(" + sessionID + ")::create_vlans failed session was cleared");
      return false; //this session has already be closed
    }
  std::list<std::string>::iterator sw_iter;
  std::list<switch_vlan>::iterator viter;
  bool rtn = true;

  write_log("session(" + sessionID + "):create_vlans");
  for (viter = active_vlans.begin(); viter != active_vlans.end(); ++viter)
    {
      vlan* v = vlans->get_vlan(*viter);
      v->print_ports();
    }

  for (sw_iter = switches.begin(); sw_iter != switches.end(); ++sw_iter)
    {
      ostringstream cmd;
      start_command(*sw_iter, cmd);
      for (viter = active_vlans.begin(); viter != active_vlans.end(); ++viter)
	{
	  vlan* v = vlans->get_vlan(*viter);
	  v->get_add_ports_cmd(*sw_iter, cmd);
	}
      bool r = send_command(*sw_iter, cmd);
      if (!r) 
	{
	  rtn = false;
	  write_log("session(" + sessionID +"):create_vlans failed to send cmd for switch(" + *sw_iter + ") " + cmd.str());
	}
      else 
	write_log("session(" + sessionID +"):create_vlans sent cmd for switch(" + *sw_iter + ") " + cmd.str());
    }
  /*
  //while loop to wait until all commands had returned
  bool finished = false;
  while(!finished)
  {
    finished = true;
    std::map<std::string, int>::iterator cmd_it;
    for (cmd_it = switch_commands.begin(); cmd_it != switch_commands.end(); ++cmd_it)
      {
	if (cmd_it->second == CMD_SENT)
	  {
	    finished = false;
	    break;
	  }
      }
      }*/
  return rtn;
}


/*
bool session::create_vlans()
{
  autoLock slock(lock);
  if (cleared) 
    {
      write_log("session(" + sessionID + ")::create_vlans failed session was cleared");
      return false; //this session has already be closed
    }
  std::map<std::string, std::list<nmd::add_to_vlan_req *> >::iterator sw_iter;
  std::list<switch_vlan>::iterator viter;
  bool rtn = true;

  for (sw_iter = switch_requests.begin(); sw_iter != switch_requests.end(); ++sw_iter)
    {
      ostringstream cmd;
      start_command(sw_iter->first, cmd);
      for (viter = active_vlans.begin(); viter != active_vlans.end(); ++viter)
	{
	  vlan* v = vlans->get_vlan(*viter);
	  v->get_add_ports_cmd(sw_iter->first, cmd);
	}
      bool r = send_command(sw_iter->first, cmd);
      if (!r) 
	{
	  rtn = false;
	}
      add_to_vlan_req* req_ptr;
      while (!sw_iter->second.empty())//clear list requests deleting requests and sending responses
	{
	  req_ptr = sw_iter->second.front();
	  sw_iter->second.pop_front();
	  req_ptr->send_response(r);
	  delete req_ptr;
	}
    }
  return rtn;
}*/

void session::add_vlan(switch_vlan vlan_id)//adds id to active list
{  
  autoLock slock(lock);
  if (find(active_vlans.begin(), active_vlans.end(), vlan_id) == active_vlans.end())
    active_vlans.push_back(vlan_id);
}

bool session::remove_vlan(switch_vlan vlan_id) //removes vlan from active to removal when the active list is empty it clears the session.
{  
  autoLock slock(lock);
  bool rtn = true;
  if (find(active_vlans.begin(), active_vlans.end(), vlan_id) != active_vlans.end())
    {
      active_vlans.remove(vlan_id);
      removed_vlans.push_back(vlan_id);
    }
  if (active_vlans.empty()) 
    {
      slock.unlock();
      rtn = clear_session();
    }
  return rtn;
}


bool session::clear_session() //called when last vlan is cleared sends vlan clears at once 
{ 
  autoLock slock(lock);
  if (cleared) return true;
  bool rtn = true;
  cleared = true;
  //first clear any requests that may not have been taken care of
  //std::map<std::string, std::list<nmd::add_to_vlan_req *> >::iterator sw_iter;
  std::list<std::string >::iterator sw_iter;
  std::list<switch_vlan>::iterator viter;
  for (sw_iter = switches.begin(); sw_iter != switches.end(); ++sw_iter)
    {
      //now clear any vlans
      //remove any active_vlans
      for (viter = active_vlans.begin(); viter != active_vlans.end(); ++viter)
	{
	  if (find(removed_vlans.begin(), removed_vlans.end(), *viter) == removed_vlans.end()) removed_vlans.push_back(*viter);
	}
      ostringstream cmd;
      int num_rm = 0;
      start_command(*sw_iter, cmd);
      for (viter = removed_vlans.begin(); viter != removed_vlans.end(); ++viter)
	{
	  num_rm += vlans->get_vlan(*viter)->get_clear_vlan_cmd(*sw_iter, cmd);
	}
      if (num_rm > 0) 
	{
	  if (!send_command(*sw_iter, cmd))
	    {
	      write_log("session("  + sessionID + ")::clear_session failed to clear vlans for switch " + *sw_iter);
	      rtn =  false;
	    }
	  else
	    {
	      write_log("session("  + sessionID + ")::clear_session cleared vlans for switch " + *sw_iter);
	    }
	}
    }
  return rtn;
}
