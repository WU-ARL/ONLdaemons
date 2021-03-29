#! /bin/sh

if [ $# -ne 6 ]
then
  echo "Usage: $0 <portnum> <iface> <ifaceRate> <ifaceDefaultMin> <iptMark>"
  echo "Example: $0 1 data0 1000000 241"
  exit 0
else
  MAINDIR=/usr/local
  portNum=$(($1+1))
  iface=$2   # data0 or data1
  ifaceRate=$3 # in kbit/s
  ifaceDefRate=$4 # in kbit/s
  iptMark=$5 # perhaps same as vlan?

  rate=${ifaceRate}kbit
  echo "rate = $rate"
fi


NIC=$iface
IPADDR=$ifaceIP
MASK=$ifaceMask


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

