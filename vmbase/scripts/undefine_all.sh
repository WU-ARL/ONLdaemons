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

count=1
host_cores=$(cat /etc/hostname | grep -o "pc[0-9]\+" | grep -o "[0-9]\+" ) 
host_num=$(cat /etc/hostname | grep -o "core[0-9]\+" | grep -o "[0-9]\+" )

while [ $count -le $(cat ../var/max_vms) ]
do
	if [ $count -le 9 ]
	then
		extra_zero="0"
	else
		extra_zero=""
	fi
	./undefine_vm.sh "vm$(echo $host_cores)c$(echo $host_num)v$(echo $extra_zero)$(echo $count)"
	count=$(( $count + 1 ))
done
