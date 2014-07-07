#!/bin/bash

vm_num=$1
host_cores=$(cat /etc/hostname | grep -o "pc[0-9]\+" | grep -o "[0-9]\+" ) 
host_num=$(cat /etc/hostname | grep -o "core[0-9]\+" | grep -o "[0-9]\+" )
count=1

if [ $vm_num -gt $(cat ../var/max_vms) ]
then
 echo "Error: Max VM number is $(cat ../var/max_vms) User entered: $(echo $vm_num)"
 exit 1
fi

if [ $vm_num -le 9 ]
then
 extra_zero="0"
else
 extra_zero=""
fi

vm_status=$(head -n $vm_num ../var/cur_vms | tail -n 1)
if [ $vm_status -eq 1 ]
then
 echo "Error: vm$(echo $host_cores)c$(echo $host_num)v$(echo $extra_zero)$(echo $vm_num) already in use."
 exit 1
fi

cp ../var/cur_vms ../var/cur_vms.old
chmod 666 ../var/cur_vms
head -n $(( $vm_num - 1 )) ../var/cur_vms.old > ../var/cur_vms
echo "1" >> ../var/cur_vms
tail -n $(( $(wc -l < ../var/cur_vms.old) - $vm_num )) ../var/cur_vms.old >> ../var/cur_vms

echo "vm$(echo $host_cores)c$(echo $host_num)v$(echo $extra_zero)$(echo $vm_num)"
