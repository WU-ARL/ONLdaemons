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
   echo "Usage: $0 <vlan_num>"
    exit 1
    fi

vlan_num=$1
#I'm particular about this script running correctly, so this one
#does a lot of error checking to avoid the machine crashing.


is_numeric=$(echo $vlan_num | grep -c "[^0-9]")
if [ $is_numeric -gt 0 ]
then
	echo "Argument should be integer only"
	exit 1
fi
ifconfig | grep -o "^br_vlan[0-9]\+\s" > cur_vlans.txt
vlan_exists=$(grep -c "^br_vlan$vlan_num\s$" cur_vlans.txt)
if [ $vlan_exists -eq 0 ]
then
	echo "br_vlan$vlan_num does not exist"
	exit 0
fi

#Let's remove the vlan interface
vlan_interface=$(ifconfig -a |grep -o "^data[0-9]\+\.$vlan_num\s")

ifconfig $vlan_interface down
ifconfig br_vlan$vlan_num down
brctl delbr br_vlan$vlan_num
vconfig rem $vlan_interface
