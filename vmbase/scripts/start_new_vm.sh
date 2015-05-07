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

  if [ $# -ne 9 ] 
  then
   echo "Usage: $0 <username> <template_path> <num cores> <memory> <number of data interfaces> <vlan list> <data ip list> <vm name> <password>" 
    exit 1
    fi

    username=$1
    template_dir=$2
    num_cores=$3
    memory=$4
    number_of_interfaces=$5
    network_confile=$6
    data_ip_list=$7
    vm_name=$8
    password=$9

    echo "Starting new VM for user $username, machine config $machine_confile, network config $network_confile, vm number $vm_name"

cd /KVM_Images/scripts

./setup_networking.sh $network_confile
if [ $? -eq 1 ]
then
 echo "Error: setup_networking.sh failed"
 exit 1
fi
mkdir -p /KVM_Images/var/users/$username/$vm_name

./create_blank_vm.sh $vm_name $template_dir $num_cores $memory $number_of_interfaces $network_confile $username $data_ip_list $password

virsh start $vm_name
exit 0

count=0
while [ $count -lt 10 ]
do
touch prepare.log
rm prepare.log
command="./prepare_vm.sh $vm_name $username blic_key_file $data_ip_list"
if [ $count -lt 9 ]
then
timeout=5
else
timeout=30
fi
./try_timeout.sh "$command" "$timeout" &> prepare.log
if [ $(wc -l < prepare.log) -eq 0 ]
then
break
fi
count=$(( $count + 1 ))
done

virsh start $vm_name
exit 0
