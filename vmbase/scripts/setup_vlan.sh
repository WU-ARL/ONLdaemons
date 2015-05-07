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

  if [ $# -ne 2 ] 
  then
   echo "Usage: $0 <vlan_num> <port>"
    exit 1
    fi

vlan_num=$1
vlan_port=$2
#I'm particular about this script running correctly, so this one
#does a lot of error checking to avoid the machine crashing.


is_numeric=$(echo $vlan_num | grep -c "[^0-9]")
if [ $is_numeric -gt 0 ]
then
	echo "vlan should be integer only"
	exit 1
fi
is_numeric=$(echo $port | grep -c "[^0-9]")
if [ $is_numeric -gt 0 ]
then
	echo "port should be integer only"
	exit 1
fi
ifconfig | grep -o "vlan[0-9]\+" > cur_vlans.txt
vlan_exists=$(grep -c "^vlan$vlan_num$" cur_vlans.txt)
if [ $vlan_exists -gt 0 ]
then
	echo "br_vlan$vlan_num already exists"
	exit 0
fi

#Vlan does not exist, perform the setup
#vlan_port=$(./get_vlan_port.sh $vlan_num)
#if [ $? -eq 1 ]
#then
# echo "Error: vlan does not exist"
# exit 1
#fi
vlan_interface=$(echo "data$vlan_port")

modprobe 8021q
vconfig add $vlan_interface $vlan_num
ip addr add 192.168.0.0/16 dev $vlan_interface.$vlan_num
sudo ifconfig $vlan_interface.$vlan_num up
brctl addbr br_vlan$vlan_num
brctl addif br_vlan$vlan_num $vlan_interface.$vlan_num
ifconfig br_vlan$vlan_num up
exit 0
