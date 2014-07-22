#!/bin/bash
if [ "$(id -u)" != "0" ]
then
 echo "You must be root to run $0."
  exit 1
  fi

  if [ $# -ne 1 ] 
  then
   echo "Usage: $0 <data_ip_list>"
    exit 1
  fi

final_string=""
while read a
do
 first_octet=$(echo $a|grep -o "^[0-9]\+")
 second_octet=$(echo $a|grep -o "^[0-9]\+\.[0-9]\+"|grep -o "[0-9]\+$")
 third_octet=$(echo $a|grep -o "^[0-9]\+\.[0-9]\+\.[0-9]\+"|grep -o "[0-9]\+$")
 fourth_octet=$(echo $a|grep -o "[0-9]\+$")
 final_string="$(echo $final_string)$(./to_mac $first_octet)$(./to_mac $second_octet)$(./to_mac $third_octet)$(./to_mac $fourth_octet)"
done < $1
echo $final_string
