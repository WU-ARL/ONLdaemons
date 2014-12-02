#!/bin/bash
if [ $# -ne 8 ]
then
  echo "Usage: $0 <portnum> <iface> <vlanNum> <ifaceIP> <ifaceMask> <ifaceRate> <ifaceDefaultMin> <iptMark>"
  echo "Example: $0 1 data0 241 192.168.82.1 255.255.255.0 1000000 241"
  exit 0
else
  MAINDIR=/usr/local
  portNum=$(($1+1))
  iface=$2   # data0 or data1
  vlanNum=$3 # 241 or similar
  ifaceIP=$4 # 192.168.81.1 or similar
  ifaceMask=$5 # 255.255.255.0 or similar
  ifaceRate=$6 # in kbit/s
  ifaceDefRate=$7 # in kbit/s
  iptMark=$8 # perhaps same as vlan?

  # make sure that reverse path filtering is turned off
  echo  0 > /proc/sys/net/ipv4/conf/default/rp_filter 

  rate=${ifaceRate}kbit
  echo "rate = $rate"
  
  echo "$MAINDIR/bin/onl_cfgvlan.sh $iface $ifaceIP $ifaceMask $vlanNum" 

  $MAINDIR/bin/onl_cfgvlan.sh $iface $ifaceIP $ifaceMask $vlanNum
  RTN=$?
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

