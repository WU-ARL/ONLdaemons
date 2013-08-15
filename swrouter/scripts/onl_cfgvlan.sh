#! /bin/sh

if [ ! -n "$4" ]
then
   echo "Usage: $0  <nic> <ipaddr> <mask> <vlan>"
   echo "given: $*"
   exit 1
fi

NIC=$1
IPADDR=$2
MASK=$3
VLAN=$4
TXQUEUELEN=10000

#echo "/sbin/ifconfig $NIC down"
#/sbin/ifconfig $NIC down
#if [ ! 0 -eq $? ]
#then
#   exit $?
#fi

echo "modprobe 8021q"
modprobe 8021q

echo "vconfig add $NIC $VLAN"
vconfig add $NIC $VLAN

echo "/sbin/ifconfig $NIC up"
/sbin/ifconfig $NIC up

echo "/sbin/ifconfig $NIC.$VLAN $IPADDR netmask $MASK"
/sbin/ifconfig $NIC.$VLAN $IPADDR netmask $MASK txqueuelen $TXQUEUELEN

if [ ! 0 -eq $? ]
then
   exit $?
fi

echo "/sbin/ifconfig $NIC.$VLAN up"
/sbin/ifconfig $NIC.$VLAN up
if [ ! 0 -eq $? ]
then
   exit $?
fi

# Remove routes added by default, we will have RLI add all the routes explicitly:
netstat -nr
netPrefix=`echo "$IPADDR" | cut -d'.' -f 1,2,3 `
netAddr="$netPrefix.0"
echo "ip route del table main $netAddr/$MASK"
ip route del table main $netAddr/$MASK

#echo "echo 14400 > /proc/sys/net/ipv4/neigh/$NIC.$VLAN/gc_stale_time"
#echo 14400 > /proc/sys/net/ipv4/neigh/$NIC.$VLAN/gc_stale_time
#if [ ! 0 -eq $? ]
#then
#   exit $?
#fi
echo "arping -q -c 3 -A -I $NIC.$VLAN $IPADDR"
arping -q -c 3 -A -I $NIC.$VLAN $IPADDR
if [ ! 0 -eq $? ]
then
   exit $?
fi

exit 0
