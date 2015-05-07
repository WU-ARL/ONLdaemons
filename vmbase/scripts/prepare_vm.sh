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

  if [ $# -ne 4 ] 
  then
   echo "Usage: $0 <vm_name> <username> <public_key_file> <data_interface_ip_file>" 
   echo "Saw $# args:"
    exit 1
    fi

#TODO this script changes the new VM's image to have the correct hostname and appends a script to rc.local that will correctly add a user and put thier public key in thier ~/.ssh/authorized_users
losetup /dev/loop7 /KVM_Images/img/$1.img
touch disk_image.info
touch sector_size
touch sector_start
chmod 666 disk_image.info
chmod 666 sector_size
chmod 666 sector_start
fdisk -lu /dev/loop7 > disk_image.info
grep Units disk_image.info |grep -o "[0-9]* bytes"|grep -o "[0-9]*" > sector_size
grep loop7p1 disk_image.info|grep -o "\*[[:space:]]*[0-9]*"|grep -o [0-9]* > sector_start
offset=$(( $(cat sector_size) * $(cat sector_start) ))
rm disk_image.info
rm sector_size
rm sector_start
losetup -d /dev/loop7
losetup -o $offset /dev/loop7 /KVM_Images/img/$1.img
mkdir -p /media/tempvm
mount /dev/loop7 /media/tempvm
chmod 666 /media/tempvm/etc/hosts 
chmod 666 /media/tempvm/etc/hostname
touch temp.txt
chmod 666 temp.txt
sed "s/ubuntu-12-04-template/$1/g" /media/tempvm/etc/hosts > temp.txt
cat temp.txt > /media/tempvm/etc/hosts
sed "s/ubuntu-12-04-template/$1/g" /media/tempvm/etc/hostname > temp.txt
cat temp.txt > /media/tempvm/etc/hostname
sudo cp $3 /media/tempvm/etc/user_key.pub
sudo cp $4 /media/tempvm/etc/data_config
sudo echo "$2" > /media/tempvm/etc/vm_username
sudo echo "password" > /media/tempvm/etc/vm_password
umount /media/tempvm
losetup -d /dev/loop7
