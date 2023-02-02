#!/bin/bash
VLAN_extdev=$1
IP_DATA_extdev=$2
TID=$3
ROUTING_TABLE_NAME=extdev${TID}
IP_extdev=$4

# Remove ip rule
echo "ip rule show"
ip rule show
echo "ip rule del from ${IP_extdev} lookup ${ROUTING_TABLE_NAME}"
ip rule del from ${IP_extdev} lookup ${ROUTING_TABLE_NAME}
echo "ip rule show"
ip rule show

# Flush route table
echo "ip route show table ${ROUTING_TABLE_NAME}"
ip route show table ${ROUTING_TABLE_NAME}
echo "ip route flush table ${ROUTING_TABLE_NAME}"
ip route flush table ${ROUTING_TABLE_NAME}
echo "ip route show table ${ROUTING_TABLE_NAME}"
ip route show table ${ROUTING_TABLE_NAME}

echo "iptables -D FORWARD -i data0.${VLAN_extdev} -o wg0 -j ACCEPT"
iptables -D FORWARD -i data0.${VLAN_extdev} -o wg0 -j ACCEPT
echo "iptables -t nat -D POSTROUTING -s ${IP_extdev}  -o data0.${VLAN_extdev} -j SNAT --to-source ${IP_DATA_extdev}"
iptables -t nat -D POSTROUTING -s ${IP_extdev}  -o data0.${VLAN_extdev} -j SNAT --to-source ${IP_DATA_extdev}
echo "iptables -t nat -D PREROUTING -d ${IP_DATA_extdev} -i data0.${VLAN_extdev} -j DNAT --to-destination ${IP_extdev}"
iptables -t nat -D PREROUTING -d ${IP_DATA_extdev} -i data0.${VLAN_extdev} -j DNAT --to-destination ${IP_extdev}

if [ $VLAN_extdev -gt 0 ]
then
    # Remove VLAN device
    echo "ifconfig data0.${VLAN_extdev}"
    ifconfig data0.${VLAN_extdev}
    echo "vconfig rem data0.${VLAN_extdev}"
    vconfig rem data0.${VLAN_extdev}
    echo "ifconfig data0.${VLAN_extdev}"
    ifconfig data0.${VLAN_extdev}
fi
