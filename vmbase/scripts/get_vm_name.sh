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
