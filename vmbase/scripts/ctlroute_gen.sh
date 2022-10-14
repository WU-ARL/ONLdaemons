#! /bin/sh

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
