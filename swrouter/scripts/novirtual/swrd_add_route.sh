#!/bin/bash
if [ $# -eq 5 ]
then
  PREFIX=$1    # 
  MASK=$2      # 
  DEV=$3       # 
  VLAN=$4       # 
  TABLE=$5
  echo "ip route add table $TABLE ${PREFIX}/${MASK} dev $DEV"
  ip route add table $TABLE ${PREFIX}/${MASK} dev $DEV
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:ip route add failed $RTN"
    exit $RTN
  fi
  ip route flush cache
elif [ $# -eq 6 ]
then
  PREFIX=$1    # 
  MASK=$2      # 
  DEV=$3       # 
  VLAN=$4       # 
  GW=$5
  TABLE=$6
  echo "table $TABLE gateway $GW"
  if [ $GW = "0.0.0.0" ]
  then
    echo "no gateway"
    echo "ip route add table $TABLE ${PREFIX}/${MASK} dev $DEV via $GW"
    ip route add table $TABLE ${PREFIX}/${MASK} dev $DEV via $GW
    RTN=$?
    if [ $RTN -ne 0 ]
    then
      echo "Error:ip route add failed $RTN"
      exit $RTN
    fi
    ip route flush cache
  else
    echo "gateway detected"
    ip route add table main $GW dev $DEV 
    ip route add table $TABLE ${PREFIX}/${MASK} via $GW
    RTN=$?
    if [ $RTN -ne 0 ]
    then
      echo "Error:ip route add failed $RTN"
      exit $RTN
    fi
    ip route flush cache
  fi
else
  echo "Usage: $0 <prefix> <mask> <dev> <vlan> [<gw>] <table>"
  echo "Example: $0 192.168.81.0 255.255.255.0 data0 241 main"
  echo "  OR"
  echo "Example: $0 192.168.81.0 255.255.255.0 data0 241 192.168.81.249 main"
  exit 0
fi
