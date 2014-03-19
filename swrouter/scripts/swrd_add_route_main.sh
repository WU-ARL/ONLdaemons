#!/bin/bash
if [ $# -eq 4 ]
then
  PREFIX=$1    # 
  MASK=$2      # 
  DEV=$3       # 
  VLAN=$4       # 
  ip route add table main ${PREFIX}/${MASK} dev $DEV.$VLAN
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:ip route add failed $RTN"
    exit $RTN
  fi
  ip route flush cache
elif [ $# -eq 5 ]
then
  PREFIX=$1    # 
  MASK=$2      # 
  DEV=$3       # 
  VLAN=$4       # 
  GW=$5
  if [ $GW = "0.0.0.0" ]
  then
    echo "no gateway"
    ip route add table main ${PREFIX}/${MASK} dev $DEV.$VLAN via $GW
    RTN=$?
    if [ $RTN -ne 0 ]
    then
      echo "Error:ip route add failed $RTN"
      exit $RTN
    fi
    ip route flush cache
  else
    echo "gateway detected"
    ip route add table main $GW dev $DEV.$VLAN 
    ip route add table main ${PREFIX}/${MASK} via $GW
    RTN=$?
    if [ $RTN -ne 0 ]
    then
      echo "Error:ip route add failed $RTN"
      exit $RTN
    fi
    ip route flush cache
  fi
  #ip route add table main ${PREFIX}/${MASK} dev $DEV.$VLAN via $GW
  #ip route flush cache
else
  echo "Usage: $0 <prefix> <mask> <dev> <vlan> [<gw>]"
  echo "Example: $0 192.168.81.0 255.255.255.0 data0 241"
  echo "  OR"
  echo "Example: $0 192.168.81.0 255.255.255.0 data0 241 192.168.81.249"
  exit 0
fi



