#!/bin/bash
if [ $# -eq 5 ]
then
  PREFIX=$1    # 
  MASK=$2      # 
  DEV=$3       # 
  VLAN=$4       # 
  TABLE=$5
  ip route del table $TABLE ${PREFIX}/${MASK} dev $DEV
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:ip route del failed $RTN"
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
  ip route del table $TABLE ${PREFIX}/${MASK} dev $DEV via $GW
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:ip route del failed $RTN"
    exit $RTN
  fi
  ip route flush cache
else
  echo "Usage: $0 <prefix> <mask> <dev> <vlan> [<gw>]"
  echo "Example: $0 192.168.81.0 255.255.255.0 data0 241"
  echo "  OR"
  echo "Example: $0 192.168.81.0 255.255.255.0 data0 241 192.168.81.249"
  exit 0
fi



