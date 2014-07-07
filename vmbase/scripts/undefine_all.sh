#!/bin/bash

if [ "$(id -u)" != "0" ]
then
 echo "You must be root to run $0."
 exit 1
fi

count=1
host_cores=$(cat /etc/hostname | grep -o "pc[0-9]\+" | grep -o "[0-9]\+" ) 
host_num=$(cat /etc/hostname | grep -o "core[0-9]\+" | grep -o "[0-9]\+" )

while [ $count -le $(cat ../var/max_vms) ]
do
	if [ $count -le 9 ]
	then
		extra_zero="0"
	else
		extra_zero=""
	fi
	./undefine_vm.sh "vm$(echo $host_cores)c$(echo $host_num)v$(echo $extra_zero)$(echo $count)"
	count=$(( $count + 1 ))
done
