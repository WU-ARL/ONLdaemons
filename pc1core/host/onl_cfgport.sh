#! /bin/sh

NIC=$1
IPADDR=$2
MASK=$3

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
if [ ! 0 -eq $? ]
then
   exit $?
fi

exit 0
