#!/bin/bash
if [ "$(id -u)" != "0" ]
then
 echo "You must be root to run $0."
  exit 1
  fi

  if [ $# -ne 8 ] 
  then
   echo "Usage: $0 <username> <template_path> <num cores> <memory> <number of data interfaces> <vlan list> <data ip list> <vm name>" 
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

    echo "Starting new VM for user $username, machine config $machine_confile, network config $network_confile, vm name $vm_name"

cd "/KVM_Images/scripts"
{
 flock -x -w 300 200 || exit 1
 ./setup_networking.sh $network_confile
 if [ $? -eq 1 ]
 then
  echo "Error: setup_networking.sh failed"
  exit 1
 fi
} 200>/KVM_Images/var/setup_networking.lck

mkdir -p /KVM_Images/var/users/$username/$vm_name

{
 flock -x -w 300 202 || exit 1
./create_blank_vm.sh $vm_name $template_dir $num_cores $memory $number_of_interfaces $network_confile $username $data_ip_list
} 202>/KVM_Images/var/create_blank_vm.lck

{
 flock -x -w 300 203 || exit 1
 virsh start $vm_name
} 203>/KVM_Images/var/virst_start.lck
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
