#!/bin/bash
if [ $# -ne 6 ]
then
  echo "Usage: $0 <portnum> <iface> <vlanNum> <qid> <minRate> <maxRate> "
  echo "Example: $0 1 data0 241 57 1000000"
  exit 0
else
  portNum=$1
  iface=$2   # data0 or data1
  vlanNum=$3 # 241 or similar
  qid=$4
  minRate=$5 # in kbit/s
  maxRate=$6 # in kbit/s

  #
  sudo tc class add dev $iface.$vlanNum parent ${portNum}:0 classid ${portNum}:$qid  htb rate ${minRate}kbit ceil ${maxRate}kbit prio 1 mtu 1500


fi

