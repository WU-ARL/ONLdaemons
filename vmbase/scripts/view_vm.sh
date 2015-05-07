#!/bin/bash
# Copyright (c) 2015  Jason Barnes, Jyoti Parwatikar, and John DeHart
# and Washington University in St. Louis
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
#    limitations under the License.
#


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
