#!/bin/bash

host_cores=$(cat /etc/hostname | grep -o "pc[0-9]\+" | grep -o "[0-9]\+" ) 
host_num=$(cat /etc/hostname | grep -o "core[0-9]\+" | grep -o "[0-9]\+" )
count=1
while [ $count -le $(wc -l < ../var/cur_vms) ]
do
	vm_status=$(head -n $count ../var/cur_vms | tail -n 1)
	if [ $vm_status -eq 0 ]
	then
		vm_num=$count
		cp ../var/cur_vms ../var/cur_vms.old
		chmod 666 ../var/cur_vms
		head -n $(( $count - 1 )) ../var/cur_vms.old > ../var/cur_vms
		echo "1" >> ../var/cur_vms
		tail -n $(( $(wc -l < ../var/cur_vms.old) - $count )) ../var/cur_vms.old >> ../var/cur_vms
		break
	fi
	count=$(( $count + 1 ))
done
if [ $count -gt $(wc -l < ../var/cur_vms) ]
then
	echo "Error: Maximum number of VMs ($(cat ../var/max_vms)) reached for this machine."
	exit 1
fi
if [ $vm_num -le 9 ]
then
	extra_zero="0"
else
	extra_zero=""
fi
echo "vm$(echo $host_cores)c$(echo $host_num)v$(echo $extra_zero)$(echo $vm_num)"
