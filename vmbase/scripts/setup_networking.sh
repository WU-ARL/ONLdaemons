#!/bin/bash
# Copyright (c) 2015  Jason Barnes, Jyoti Parwatikar, and John DeHart
# and Washington University in St. Louis
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
#    limitations under the License.
#

if [ "$(id -u)" != "0" ]
then
 echo "You must be root to run $0."
  exit 1
  fi

  if [ $# -ne 1 ] 
  then
   echo "Usage: $0 <network config script>"
    exit 1
    fi


#Check to see if control bridge has been created:
br_control_exists=$(ifconfig -a|grep -c "br_control")
if [ $br_control_exists -eq 0 ]
then
	#Control bridge has not been created yet
	cp interfaces.vm /etc/network/interfaces
	brctl addbr br_control
	ifconfig br_control up
	echo "1" > /proc/sys/net/ipv4/ip_forward #enable ip forwarding between interfaces
	#following 2 commands set up proxy arp so that br_control answers for vms and packets come to it for the br_control
	echo "1" > /proc/sys/net/ipv4/conf/control/proxy_arp
	echo "1" > /proc/sys/net/ipv4/conf/br_control/proxy_arp
	#adds a route to ip routing tables for each of the
	#pre-assigned control ips for this host to interface br_control
	./add_control_routes.sh $(cat /etc/hostname)_control_routes
fi

#Check to see if the alternate routing tables have been created:
alt_rt_tables=$(grep -c "vm" /etc/iproute2/rt_tables)
if [ $alt_rt_tables -eq 0 ]
then
        #Alternate  routing tables for control have not been setup.
        #adds routing tables 1 vm_out, 2 vm_in
	cp rt_tables.vm /etc/iproute2/rt_tables
fi

#Check to see if IP rules have been setup to use alternate routes:
alt_rule_exists=$(ip rule show | grep -c "vm")
if [ $alt_rule_exists -eq 0 ]
then
        #Alternate rules have not been setup:
        # add iptables rule to use route table vm_out if coming from br_control
	ip rule add iif br_control table vm_out
        # add iptables rule to use route table vm_in if coming from control to
	#one of the pre-assigned vm subnets
	ip rule add to 10.0.24.0/24 iif control table vm_in
	ip rule add to 10.0.25.0/24 iif control table vm_in
	ip rule add to 10.0.26.0/24 iif control table vm_in
        #add route to table vm_in to route all control traffic out through br_control
	ip route add 10.0.0.0/8 dev br_control table vm_in
fi

ifconfig data0 up

lines=$(wc -l < $1)
count=1
#for each vlan/port assigned for this experiment run setup_vlan.sh
while [ $count -le $lines ]
do
	./setup_vlan.sh $(cat $1|head -n $count|tail -n 1)
	if [ $? -eq 1 ]
	then
	 echo "Error: ./setup_vlan.sh failed"
	 exit 1
	fi
	count=$(( $count + 1 ))
done

#All bridges and vlans setup, let's check the route table to see it's correct
exit 0
