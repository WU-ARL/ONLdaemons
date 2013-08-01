#!/bin/bash
if [ $# -ne 6 ]
then
  echo "Usage: $0 <portnum> <iface> <vlanNum> <qid> <maxRate> <minRate> "
  echo "Example: $0 1 data0 241 57 1000000"
  exit 0
else
  portNum=$1
  iface=$2   # data0 or data1
  vlanNum=$3 # 241 or similar
  qid=$4
  maxRate=$5 # in kbit/s
  minRate=$6 # in kbit/s

  #
  sudo tc class change dev $iface.$vlanNum parent ${portNum}:0 classid ${portNum}:$qid  htb rate ${minRate}kbit ceil ${maxRate}kbit prio 1 mtu 1500


fi

