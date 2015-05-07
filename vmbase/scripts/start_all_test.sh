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

date
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 1 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 2 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 3 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 4 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 5 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 6
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 7 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 8 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 9 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 10
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 11 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 12 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 13 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 14 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 15 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 16 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 17 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 18 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 19 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 20 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 21 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 22 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 23 
sudo ./start_new_vm.sh jason /KVM_Images/img/ubuntu_12.04_template.qcow2 1 1024 0 vlan_null.txt data_ip_null.txt 24 
echo "Starting connection test"
while ! ping -c1 vm12c22v01 &>/dev/null; do :; done
while ! ping -c1 vm12c22v02 &>/dev/null; do :; done
while ! ping -c1 vm12c22v03 &>/dev/null; do :; done
while ! ping -c1 vm12c22v04 &>/dev/null; do :; done
while ! ping -c1 vm12c22v05 &>/dev/null; do :; done
while ! ping -c1 vm12c22v06 &>/dev/null; do :; done
while ! ping -c1 vm12c22v07 &>/dev/null; do :; done
while ! ping -c1 vm12c22v08 &>/dev/null; do :; done
while ! ping -c1 vm12c22v09 &>/dev/null; do :; done
while ! ping -c1 vm12c22v10 &>/dev/null; do :; done
while ! ping -c1 vm12c22v11 &>/dev/null; do :; done
while ! ping -c1 vm12c22v12 &>/dev/null; do :; done
while ! ping -c1 vm12c22v13 &>/dev/null; do :; done
while ! ping -c1 vm12c22v14 &>/dev/null; do :; done
while ! ping -c1 vm12c22v15 &>/dev/null; do :; done
while ! ping -c1 vm12c22v16 &>/dev/null; do :; done
while ! ping -c1 vm12c22v17 &>/dev/null; do :; done
while ! ping -c1 vm12c22v18 &>/dev/null; do :; done
while ! ping -c1 vm12c22v19 &>/dev/null; do :; done
while ! ping -c1 vm12c22v20 &>/dev/null; do :; done
while ! ping -c1 vm12c22v21 &>/dev/null; do :; done
while ! ping -c1 vm12c22v22 &>/dev/null; do :; done
while ! ping -c1 vm12c22v23 &>/dev/null; do :; done
while ! ping -c1 vm12c22v24 &>/dev/null; do :; done
date
