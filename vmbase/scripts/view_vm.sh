#!/bin/bash

if [ $# -ne 1 ]
then
 echo "Usage: $0 <vm name>"
 exit 1
fi

cur_user=$USER
vm=$1

vm_check=$(echo "$vm" | grep -c "^vm[0-9]\+c[0-9]\+v[0-9][0-9]$")

if [ $vm_check -ne "1" ]
then
 echo "VM name format incorrect, should be vmXXcXXvXX"
 exit 1
fi

vm_owned=$(find "/KVM_Images/var/users" | grep "/$cur_user/" | grep -c "$(echo $vm)$")

if [ $vm_owned -ne "1" ]
then
 echo "You are not the current owner of $vm"
 exit 1
fi

sudo virt-viewer $vm
