#!/bin/bash
if [ $# -lt 3 ]
then
  echo "Usage: $0 <mgroup> [<iface> <originIpAddr>]*  "
  echo "Example: $0 224.10.10.10 data0.241 192.168.81.2 data0.242 192.168.82.2 data0.243 192.168.83.2"
  exit 0
else
  mgroup=$1
  shift
  InterfacesString=""
  #echo "InterfacesString=$InterfacesString"
  #echo "Args left: $#"
  num=0
  while [ $# -gt 0 ]
  do
    Interfaces[$num]=$1
    InterfacesString="$InterfacesString $1"
    OriginIpAddresses[$num]=$2
    shift 2
    #echo "InterfacesString=$InterfacesString"
    #echo "OriginIpAddresses=$OriginIpAddresses"
    #echo "Args left: $#"
    num=$(($num+1))

  done

  i=0
  while ( [ $i -lt $num ] )
  do
    #echo "i=$i num=$num"
    #echo "InterfacesString=$InterfacesString"
    #echo "Interfaces[i]=${Interfaces[i]}"
    cmd="smcroute -a ${Interfaces[i]} ${OriginIpAddresses[i]}  $InterfacesString"
    echo "$cmd"
    i=$(($i+1))
  done
  #smcroute -a data0.241 192.168.81.2 224.10.10.10 data0.242 data0.243
fi

