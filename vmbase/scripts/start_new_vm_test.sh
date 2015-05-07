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

  if [ $# -ne 8 ] 
  then
   echo "Usage: $0 <username> <public key file> <template_path> <num cores> <memory> <number of data interfaces> <vlan list> <data ip list> <vm number>" 
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

    echo "Starting new VM for user $username with key $public_key_file, machine config $machine_confile, network config $network_confile, vm name $vm_name"

KVM_SCRIPT_DIR="/KVM_Images/scripts"
cd $KVM_SCRIPT_DIR
#$KVM_SCRIPT_DIR/setup_networking.sh $network_confile
#if [ $? -eq 1 ]
#then
# echo "Error: setup_networking.sh failed"
# exit 1
#fi
#vm_name=$($KVM_SCRIPT_DIR/get_vm_name.sh $vm_number)
#if [ $? -eq 1 ]
#then
# echo "Error: get_vm_name.sh failed"
# exit 1
#fi
$KVM_SCRIPT_DIR/create_blank_vm.sh $vm_name $template_dir $num_cores $memory $number_of_interfaces $network_confile

count=0
touch prepare.log
rm prepare.log
./prepare_vm_test.sh $vm_name $username $data_ip_list
virsh start $vm_name
exit 0
