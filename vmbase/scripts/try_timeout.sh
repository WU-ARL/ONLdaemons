#!/bin/bash
if [ "$(id -u)" != "0" ]
then
 echo "You must be root to run $0."
  exit 1
  fi

  if [ $# -ne 2 ] 
  then
   echo "Usage: $0 <command to try> <seconds to wait>"
    exit 1
    fi

$1 &
last_pid=$!
sleep $2
kill $last_pid &> /dev/null
exit 0
