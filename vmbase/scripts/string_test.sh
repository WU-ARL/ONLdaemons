#!/bin/bash
vlan_config_file=$1
count=1
vlan=$(echo $(cat $vlan_config_file|head -n $count|tail -n 1) | cut -d' ' -f 1)

echo $vlan

exit 1
