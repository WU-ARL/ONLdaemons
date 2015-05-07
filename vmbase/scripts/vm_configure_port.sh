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

if [ $# -ne 9 ]
then
  echo "Usage: $0 <portnum> <iface> <vlanNum> <ifaceIP> <ifaceMask> <ifaceRate> <ifaceDefaultMin> <iptMark> <vmname>"
  echo "Example: $0 1 data0 241 192.168.82.1 255.255.255.0 1000000 241 vm12c01v01"
  exit 0
else
  MAINDIR=/KVM_Images/scripts
  portNum=$(($1+1))
  iface=$2   # data0 or data1
  vlanNum=$3 # 241 or similar
  ifaceIP=$4 # 192.168.81.1 or similar
  ifaceMask=$5 # 255.255.255.0 or similar
  ifaceRate=$6 # in kbit/s
  ifaceDefRate=$7 # in kbit/s
  iptMark=$8 # perhaps same as vlan?
  vmname=$9

  rate=${ifaceRate}kbit
  echo "rate = $rate"
  
  echo "$MAINDIR/onl_cfgvlan.sh $iface $ifaceIP $ifaceMask $vlanNum" 

  $MAINDIR/onl_cfgvlan.sh $iface $ifaceIP $ifaceMask $vlanNum
  RTN=$?

#set up ingress queueing run this no matter what. onl_cfgvlan will fail when multiple nodes are
#added to a vlan but this needs to run for every node
#first get the id
vmid="$(virsh list | grep $vmname | grep -o "[0-9]\+" | head -n 1)"
#get name of tap vnetX
vmvnet="$($MAINDIR/get_vnet.py $vmid)"
echo "$vmname has tap $vmvnet"

#setup ingress queue
echo "tc qdisc add dev $vmvnet handle ffff: ingress"
tc qdisc add dev $vmvnet handle ffff: ingress

#add filter to restrict bandwidth leaving the vm and entering the host
echo "tc filter add dev $vmvnet parent ffff: protocol ip u32 match u32 0 0 police rate ${ifaceRate}kbit burst 70M mtu 100k drop flowid :1"
tc filter add dev $vmvnet parent ffff: protocol ip u32 match u32 0 0 police rate ${ifaceRate}kbit burst 70M mtu 100k drop flowid :1
  if [ $RTN -ne 0 ]
  then
    echo "Error:onl_cfgvlan failed $RTN"
    exit $RTN
  fi

  # Mark packets that are destined for this interface/port via iptables
  iptables -t filter -A FORWARD -o $iface.$vlanNum -j MARK --set-mark $iptMark 
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:iptables failed $RTN"
    exit $RTN
  fi

  # Associate queueing discipline (qdisc) htb (Hierarchical Token Bucket) with interface $iface.$vlanNum and give it handle $portNum:
  tc qdisc add dev $iface.$vlanNum root handle ${portNum}: htb default ${portNum}
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:tc qdisc add failed $RTN"
    exit $RTN
  fi

  # Create a root class $portNum:0 under qdiscs $portNum: and set it to the link rate. Any classes created with this as a parent
  #    can borrow from others under this parent.
  tc class add dev $iface.$vlanNum parent ${portNum}: classid ${portNum}:0  htb rate ${ifaceRate}kbit ceil ${ifaceRate}kbit prio 1 mtu 1500
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:tc class add failed $RTN"
    exit $RTN
  fi

  # Create a default class. Anything that does not match a filter and get directed to a specific class.
  tc class add dev $iface.$vlanNum parent ${portNum}:0 classid ${portNum}:1  htb rate ${ifaceDefRate}kbit ceil ${ifaceRate}kbit prio 2 mtu 1500
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:tc class add failed $RTN"
    exit $RTN
  fi

  #
  tc filter add dev $iface.$vlanNum parent ${portNum}:0 protocol ip prio 2 handle $iptMark fw flowid ${portNum}:1
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:tc filter add failed $RTN"
    exit $RTN
  fi
fi

exit 0
