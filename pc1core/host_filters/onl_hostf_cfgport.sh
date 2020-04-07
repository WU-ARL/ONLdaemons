#! /bin/sh

if [ $# -ne 7 ]
then
  echo "Usage: $0 <portnum> <iface> <ifaceIP> <ifaceMask> <ifaceRate> <ifaceDefaultMin> <iptMark>"
  echo "Example: $0 1 data0 192.168.82.1 255.255.255.0 1000000 241"
  exit 0
else
  MAINDIR=/usr/local
  portNum=$(($1+1))
  iface=$2   # data0 or data1
  ifaceIP=$3 # 192.168.81.1 or similar
  ifaceMask=$4 # 255.255.255.0 or similar
  ifaceRate=$5 # in kbit/s
  ifaceDefRate=$6 # in kbit/s
  iptMark=$7 # perhaps same as vlan?

  rate=${ifaceRate}kbit
  echo "rate = $rate"
fi


NIC=$iface
IPADDR=$ifaceIP
MASK=$ifaceMask

if [ ! -n "$3" ]
then
   echo "Usage: $0  <nic> <ipaddr> <mask>"
   exit 1
fi

echo "start debug.log" > /tmp/debug.log
ifconfig data0 >> /tmp/debug.log

echo "/sbin/ifconfig $NIC down"
/sbin/ifconfig $NIC down
if [ ! 0 -eq $? ]
then
   exit $?
fi
ifconfig data0 >> /tmp/debug.log
echo "/sbin/ifconfig $NIC $IPADDR netmask $MASK"
/sbin/ifconfig $NIC $IPADDR netmask $MASK
if [ ! 0 -eq $? ]
then
   exit $?
fi
ifconfig data0 >> /tmp/debug.log
echo "/sbin/ifconfig $NIC up"
/sbin/ifconfig $NIC up
if [ ! 0 -eq $? ]
then
   exit $?
fi
echo "echo 14400 > /proc/sys/net/ipv4/neigh/$NIC/gc_stale_time"
echo 14400 > /proc/sys/net/ipv4/neigh/$NIC/gc_stale_time
if [ ! 0 -eq $? ]
then
   exit $?
fi
echo "arping -q -c 3 -A -I $NIC $IPADDR"
arping -q -c 3 -A -I $NIC $IPADDR
RTN=$?
if [ $RTN -ne 0 ]
#if [ ! 0 -eq $? ]
then
   echo "Error:arping failed $RTN"
   exit $?
fi
 
echo "iptables -t filter -A FORWARD -o $iface -j MARK --set-mark $iptMark"
iptables -t filter -A FORWARD -o $iface -j MARK --set-mark $iptMark
RTN=$?
if [ $RTN -ne 0 ]
then
    echo "Error:iptables failed $RTN"
    exit $RTN
fi

# Associate queueing discipline (qdisc) htb (Hierarchical Token Bucket) with interface $iface.$vlanNum and give it handle $portNum:
echo "tc qdisc add dev $iface root handle ${portNum}: htb default 1 #${portNum}"
tc qdisc add dev $iface root handle ${portNum}: htb default 1 #${portNum}
RTN=$?
if [ $RTN -ne 0 ]
then
    echo "Error:tc failed $RTN"
    exit $RTN
fi

# Create a root class $portNum:0 under qdiscs $portNum: and set it to the link rate. Any classes created with this as a parent
#    can borrow from others under this parent.
echo "tc class add dev $iface parent ${portNum}: classid ${portNum}:0  htb rate ${ifaceRate}kbit ceil ${ifaceRate}kbit prio 1 mtu 1500"
tc class add dev $iface parent ${portNum}: classid ${portNum}:0  htb rate ${ifaceRate}kbit ceil ${ifaceRate}kbit prio 1 mtu 1500
RTN=$?
if [ $RTN -ne 0 ]
then
    echo "Error:tc failed $RTN"
    exit $RTN
fi

# Create a default class. Anything that does not match a filter and get directed to a specific class.
echo "tc class add dev $iface parent ${portNum}:0 classid ${portNum}:1  htb rate ${ifaceDefRate}kbit ceil ${ifaceRate}kbit prio 2 mtu 1500"
tc class add dev $iface parent ${portNum}:0 classid ${portNum}:1  htb rate ${ifaceDefRate}kbit ceil ${ifaceRate}kbit prio 2 mtu 1500
RTN=$?
if [ $RTN -ne 0 ]
then
    echo "Error:tc failed $RTN"
    exit $RTN
fi

#
echo "tc filter add dev $iface parent ${portNum}:0 protocol ip prio 2 handle $iptMark fw flowid ${portNum}:1"
tc filter add dev $iface parent ${portNum}:0 protocol ip prio 2 handle $iptMark fw flowid ${portNum}:1
RTN=$?
if [ $RTN -ne 0 ]
then
    echo "Error:tc failed $RTN"
    exit $RTN
fi

exit 0

