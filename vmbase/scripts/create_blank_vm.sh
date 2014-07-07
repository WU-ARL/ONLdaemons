#!/bin/bash
if [ "$(id -u)" != "0" ]
then
 echo "You must be root to run $0."
  exit 1
  fi

  if [ $# -ne 6 ] 
  then
   echo "Usage: $0 <vm_name> <template path> <num cores> <memory> <number of vm interfaces> <list of vlans>"
    exit 1
    fi

vm_name=$1
template_dir=$2
num_cores=$3
memory=$4
num_interfaces=$5
vlan_config_file=$6
count=1
network_string=""
while [ $count -le $num_interfaces ]
do
	network_string="$(echo $network_string) --network=bridge=\"br_vlan$(cat $vlan_config_file|head -n $count|tail -n 1)\",mac=54:$(./to_mac $count):$(./to_mac $(echo $vm_name|grep -o "vm[0-9]\+"|grep -o "[0-9]\+")):$(./to_mac $(echo $vm_name|grep -o "c[0-9]\+"|grep -o "[0-9]\+")):$(./to_mac $(echo $vm_name|grep -o "v[0-9]\+"|grep -o "[0-9]\+")):0,model=virtio"
	count=$(( $count + 1 ))
done
#sudo qemu-img create -f qcow2 -b $template_dir "$(dirname $template_dir)/$vm_name.img" 20G
cp $template_dir "$(dirname $template_dir)/$vm_name.img"

sudo virt-install --name="$vm_name" --ram="$memory" --vcpus="$num_cores" --import --os-type=linux --disk=path="$(dirname $template_dir)/$vm_name.img",format="raw" --network=bridge="br_control",mac=52:$(./to_mac $(echo $vm_name|grep -o "vm[0-9]\+"|grep -o "[0-9]\+")):$(./to_mac $(echo $vm_name|grep -o "c[0-9]\+"|grep -o "[0-9]\+")):$(./to_mac $(echo $vm_name|grep -o "v[0-9]\+"|grep -o "[0-9]\+")):0:0 $network_string --noreboot
