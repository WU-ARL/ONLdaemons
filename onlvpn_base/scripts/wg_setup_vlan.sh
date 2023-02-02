#!/bin/bash

# VLAN is manually extracted from onlsrv:/etc/ONL/nmd.log by searching for onlsw11 and looking for the
# VLAN(s) added to onlsw11:port35
VLAN_extdev=$1
IP_extdev=$2
IP_DATA_extdev=$3
IP_DATA_SUBNET_extdev=$4
# Need to manage the entries in the /etc/iproute2/rt_tables file
ROUTING_TABLE_ID=$5
ROUTING_TABLE_NAME=extdev${ROUTING_TABLE_ID}

MAINDIR=/usr/local

echo "$MAINDIR/bin/onl_cfgvlan.sh data0 $IP_DATA_extdev 255.255.255.0 $VLAN_extdev"
$MAINDIR/bin/onl_cfgvlan.sh data0 $IP_DATA_extdev 255.255.255.0 $VLAN_extdev

#table is now added when device is registered
# permanently added to /etc/iproute2/rt_tables
#echo "${ROUTING_TABLE_ID} ${ROUTING_TABLE_NAME}" >> /etc/iproute2/rt_tables

# we do NOT need the specific routes to the Next Hops that used to be here..

echo "ip route add ${IP_DATA_SUBNET_extdev}/24 dev data0.${VLAN_extdev} table ${ROUTING_TABLE_ID}"
ip route add ${IP_DATA_SUBNET_extdev}/24 dev data0.${VLAN_extdev} table ${ROUTING_TABLE_ID}

echo "ip rule add from ${IP_extdev} lookup ${ROUTING_TABLE_ID}"
ip rule add from ${IP_extdev} lookup ${ROUTING_TABLE_ID}

echo "iptables -A FORWARD -i data0.${VLAN_extdev} -o wg0 -j ACCEPT"
iptables -A FORWARD -i data0.${VLAN_extdev} -o wg0 -j ACCEPT
echo "iptables -t nat -A POSTROUTING -s ${IP_extdev}  -o data0.${VLAN_extdev} -j SNAT --to-source ${IP_DATA_extdev}"
iptables -t nat -A POSTROUTING -s ${IP_extdev}  -o data0.${VLAN_extdev} -j SNAT --to-source ${IP_DATA_extdev}
echo "iptables -t nat -A PREROUTING -d ${IP_DATA_extdev} -i data0.${VLAN_extdev} -j DNAT --to-destination ${IP_extdev}"
iptables -t nat -A PREROUTING -d ${IP_DATA_extdev} -i data0.${VLAN_extdev} -j DNAT --to-destination ${IP_extdev}

# Notes on ip rule commands:
# root@onlvpn:/users2/jdd/onlvpn/ExternalDevice_with_Gige_and_2SWR# ip rule show
#0:	from all lookup local
#32765:	from 192.168.252.3 lookup extdev1
#32766:	from all lookup main
#32767:	from all lookup default
#root@onlvpn:/users2/jdd/onlvpn/ExternalDevice_with_Gige_and_2SWR# ip rule del from 192.168.252.3 lookup extdev1
#root@onlvpn:/users2/jdd/onlvpn/ExternalDevice_with_Gige_and_2SWR# ip rule show
#0:	from all lookup local
#32766:	from all lookup main
#32767:	from all lookup default
#root@onlvpn:/users2/jdd/onlvpn/ExternalDevice_with_Gige_and_2SWR#

# Notes on ip route commands:
#jdd@onlvpn:~$ ip route show table extdev1
#192.168.1.0/24 dev data0.39 scope link
#192.168.2.0/24 via 192.168.1.4 dev data0.39
#192.168.3.0/24 via 192.168.1.4 dev data0.39
#192.168.4.0/24 via 192.168.1.5 dev data0.39
#192.168.5.0/24 via 192.168.1.5 dev data0.39
#jdd@onlvpn:~$ sudo ip route flush table extdev1
#jdd@onlvpn:~$ ip route show table extdev1
#jdd@onlvpn:~$
