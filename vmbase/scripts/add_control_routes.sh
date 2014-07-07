#!/bin/bash
if [ "$(id -u)" != "0" ]
then
 echo "You must be root to run $0."
  exit 1
  fi

  if [ $# -ne 1 ] 
  then
   echo "Usage: $0 <pcXXcoreXX_control_routes>"
    exit 1
    fi

route_file=$1

while read a
do
 #echo "route add -host $a dev br_control"
 route add -host $a dev br_control
done < $route_file
