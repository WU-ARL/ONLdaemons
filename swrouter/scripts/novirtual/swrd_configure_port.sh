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

  rate=${ifaceRate}kbit
  echo "rate = $rate"
  
  TXQUEUELEN=10000
  echo "/sbin/ifconfig $iface $ifaceIP netmask $ifaceMask"
  /sbin/ifconfig $iface $ifaceIP netmask $ifaceMask txqueuelen $TXQUEUELEN
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:ifconfig failed $RTN"
    exit $RTN
  fi

  echo "/sbin/ifconfig $NIC up"
  /sbin/ifconfig $iface up
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:ifconfig failed $RTN"
    exit $RTN
  fi

  # Remove routes added by default, we will have RLI add all the routes explicitly:
  netstat -nr
  netPrefix=`echo "$ifaceIP" | cut -d'.' -f 1,2,3 `
  netAddr="$netPrefix.0"
  echo "ip route del table main $netAddr/$ifaceMask"
  ip route del table main $netAddr/$ifaceMask

  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:ip route del failed $RTN"
    exit $RTN
  fi

  # Mark packets that are destined for this interface/port via iptables
  iptables -t filter -A FORWARD -o $iface -j MARK --set-mark $iptMark
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:iptables failed $RTN"
    exit $RTN
  fi

  # Associate queueing discipline (qdisc) htb (Hierarchical Token Bucket) with interface $iface.$vlanNum and give it handle $portNum:
  tc qdisc add dev $iface root handle ${portNum}: htb default 1 #${portNum}
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:tc failed $RTN"
    exit $RTN
  fi

  # Create a root class $portNum:0 under qdiscs $portNum: and set it to the link rate. Any classes created with this as a parent
  #    can borrow from others under this parent.
  tc class add dev $iface parent ${portNum}: classid ${portNum}:0  htb rate ${ifaceRate}kbit ceil ${ifaceRate}kbit prio 1 mtu 1500
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:tc failed $RTN"
    exit $RTN
  fi

  # Create a default class. Anything that does not match a filter and get directed to a specific class.
  tc class add dev $iface parent ${portNum}:0 classid ${portNum}:1  htb rate ${ifaceDefRate}kbit ceil ${ifaceRate}kbit prio 2 mtu 1500
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:tc failed $RTN"
    exit $RTN
  fi

  #
  tc filter add dev $iface parent ${portNum}:0 protocol ip prio 2 handle $iptMark fw flowid ${portNum}:1
  RTN=$?
  if [ $RTN -ne 0 ]
  then
    echo "Error:tc failed $RTN"
    exit $RTN
  fi


fi

