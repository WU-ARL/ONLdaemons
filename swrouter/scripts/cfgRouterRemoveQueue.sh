#!/bin/bash
if [ $# -ne 4 ]
then
  echo "Usage: $0 <portnum> <iface> <vlanNum> <qid> "
  echo "Example: $0 1 data0 241 57"
  exit 0
else
  portNum=$1
  iface=$2   # data0 or data1
  vlanNum=$3 # 241 or similar
  qid=$4

  #
  sudo tc class del dev $iface.$vlanNum parent ${portNum}:0 classid ${portNum}:$qid  


fi

