#!/bin/bash
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
