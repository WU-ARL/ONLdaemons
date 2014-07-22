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

cd "/KVM_Images/scripts"

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
