#!/bin/bash

# VLAN is manually extracted from onlsrv:/etc/ONL/nmd.log by searching for onlsw11 and looking for the
# VLAN(s) added to onlsw11:port35
VLAN_laptop=$1
ROUTING_TABLE_ID=$2
PREFIX=$3
MASK=$4
#IP_DATA_NEXT_HOP=$5
# Need to manage the entries in the /etc/iproute2/rt_tables file


# permanently added to /etc/iproute2/rt_tables
#echo "${ROUTING_TABLE_ID} ${ROUTING_TABLE_NAME}" >> /etc/iproute2/rt_tables

# we do NOT need the specific routes to the Next Hops that used to be here..

echo "ip route del ${PREFIX}/${MASK} dev data0.${VLAN_laptop} table ${ROUTING_TABLE_ID}"
ip route del ${PREFIX}/${MASK} dev data0.${VLAN_laptop} table ${ROUTING_TABLE_ID}


# Notes on ip rule commands:
# root@onlvpn:/users2/jdd/onlvpn/ExternalDevice_with_Gige_and_2SWR# ip rule show
#0:	from all lookup local
#32765:	from 192.168.252.3 lookup laptop1
#32766:	from all lookup main
#32767:	from all lookup default
#root@onlvpn:/users2/jdd/onlvpn/ExternalDevice_with_Gige_and_2SWR# ip rule del from 192.168.252.3 lookup laptop1
#root@onlvpn:/users2/jdd/onlvpn/ExternalDevice_with_Gige_and_2SWR# ip rule show
#0:	from all lookup local
#32766:	from all lookup main
#32767:	from all lookup default
#root@onlvpn:/users2/jdd/onlvpn/ExternalDevice_with_Gige_and_2SWR#

# Notes on ip route commands:
#jdd@onlvpn:~$ ip route show table laptop1
#192.168.1.0/24 dev data0.39 scope link
#192.168.2.0/24 via 192.168.1.4 dev data0.39
#192.168.3.0/24 via 192.168.1.4 dev data0.39
#192.168.4.0/24 via 192.168.1.5 dev data0.39
#192.168.5.0/24 via 192.168.1.5 dev data0.39
#jdd@onlvpn:~$ sudo ip route flush table laptop1
#jdd@onlvpn:~$ ip route show table laptop1
#jdd@onlvpn:~$
