#!/bin/bash

if [ "$(id -u)" != "0" ]
then
 echo "You must be root to run $0."
 exit 1
fi

if [ $# -ne 1 ]
then
 echo "Usage: $0 <vm_name>"
 exit 1
fi

cd /KVM_Images/scripts

vm_zero_num=$(echo "$1" | grep -o "v[0-9][0-9]" | grep -o "[0-9]*" | grep -o "[1-9][0-9]*")
vm_num=$vm_zero_num

virsh destroy $1
virsh undefine $1
rm /KVM_Images/img/$1.img

touch ../var/cur_vms.old
rm ../var/cur_vms.old
cp ../var/cur_vms ../var/cur_vms.old
head -n $(( $vm_num - 1 )) ../var/cur_vms.old > ../var/cur_vms
echo "0" >> ../var/cur_vms
tail -n $(( $(wc -l < ../var/cur_vms.old) - $vm_num )) ../var/cur_vms.old >> ../var/cur_vms
