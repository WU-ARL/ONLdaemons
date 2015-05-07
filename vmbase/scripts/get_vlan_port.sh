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

#Let's return the correct port for the requested vlan

vlan_num=$1
vlan_exists=$(cat /tmp/host.log | grep -c "port:[0-9]\+ vlan:$vlan_num$")
if [ $vlan_exists -eq 0 ]
then
	echo "vlan:$vlan_num not found in /tmp/host.log"
	exit 1
fi
cat /tmp/host.log | grep -o "port:[0-9]\+ vlan:$vlan_num" | grep -o "port:[0-9]\+" | grep -o "[0-9]\+"
exit 0
