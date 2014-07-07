#!/bin/bash
if [ "$(id -u)" != "0" ]
then
 echo "You must be root to run $0."
  exit 1
  fi

  if [ $# -ne 9 ] 
  then
   echo "Usage: $0 <username> <public key file> <template_path> <num cores> <memory> <number of data interfaces> <vlan list> <data ip list> <vm number>" 
    exit 1
    fi

    username=$1
    public_key_file=$2
    template_dir=$3
    num_cores=$4
    memory=$5
    number_of_interfaces=$6
    network_confile=$7
    data_ip_list=$8
    vm_number=$9

    echo "Starting new VM for user $username with key $public_key_file, machine config $machine_confile, network config $network_confile, vm number $vm_number"

KVM_SCRIPT_DIR="/KVM_Images/scripts"
cd $KVM_SCRIPT_DIR
$KVM_SCRIPT_DIR/setup_networking.sh $network_confile
if [ $? -eq 1 ]
then
 echo "Error: setup_networking.sh failed"
 exit 1
fi
vm_name=$($KVM_SCRIPT_DIR/get_vm_name.sh $vm_number)
if [ $? -eq 1 ]
then
 echo "Error: get_vm_name.sh failed"
 exit 1
fi
$KVM_SCRIPT_DIR/create_blank_vm.sh $vm_name $template_dir $num_cores $memory $number_of_interfaces $network_confile

count=0
touch prepare.log
rm prepare.log
./prepare_vm_test.sh $vm_name $username $public_key_file $data_ip_list
virsh start $vm_name
exit 0
