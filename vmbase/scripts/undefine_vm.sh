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


rm -r $(find /KVM_Images/var/users/ | grep "$(echo $1)$")
