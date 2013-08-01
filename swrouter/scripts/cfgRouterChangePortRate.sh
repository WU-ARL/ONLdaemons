#!/bin/bash
if [ $# -ne 4 ]
then
  echo "Usage: $0 <portnum> <iface> <vlanNum> <ifaceRate> "
  echo "Example: $0 1 data0 241 1000000"
  exit 0
else
  portNum=$1
  iface=$2   # data0 or data1
  vlanNum=$3 # 241 or similar
  ifaceRate=$4 # in kbit/s

  # Create a root class $portNum:0 under qdiscs $portNum: and set it to the link rate. Any classes created with this as a parent
  #    can borrow from others under this parent.
  sudo tc class change dev $iface.$vlanNum parent ${portNum}: classid ${portNum}:0  htb rate ${ifaceRate}kbit ceil ${ifaceRate}kbit prio 1 mtu 1500


fi

