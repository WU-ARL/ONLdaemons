#! /bin/sh
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


HOST_NDX=1
IPBYTE2=24
IPBYTE1=1

while [ $HOST_NDX -lt 21 ]
do
#set the current file to write to
    if [ $HOST_NDX -lt 10 ]
    then
	CURR_FILE="pc12core0"$HOST_NDX"_control_routes"
    else
	CURR_FILE="pc12core"$HOST_NDX"_control_routes"
    fi
    echo "current file: $CURR_FILE"
#write 24 ips to host file
    COUNT=0
    while [ $COUNT -lt 24 ]
    do
#if IPBYTE1 is 256 increase IPBYTE2 and reset IPBYTE1
	if [ $IPBYTE1 -gt 255 ]
	then
	    IPBYTE2=$(( $IPBYTE2 + 1 ))
	    IPBYTE1=1
	fi
#write line to file
	echo "10.0.$IPBYTE2.$IPBYTE1" >> $CURR_FILE
	#echo "10.0.$IPBYTE2.$IPBYTE1"
	IPBYTE1=$(( $IPBYTE1 + 1 ))
        COUNT=$(( $COUNT + 1 ))
    done
    HOST_NDX=$(( $HOST_NDX + 1 ))
done
