#!/bin/bash
if [ $# -ne 1 ]
then
  echo "Usage: $0 runpath"
  exit 0
else
  echo "cd $1"
  cd $1
  pwd
  source ./hosts

  ../bin/cfgRouterBase.sh

  #Usage: ../bin/cfgRouterPort.sh <portNum> <iface> <ifaceIP> <ifaceMask> <ifaceRate> <ifaceDefRate> <vlanNum> <iptMark>


  ../bin/cfgRouterPort.sh 1 data0 241 $rtr_subnet1 255.255.255.0 1000000 1000 241
  ../bin/cfgRouterPort.sh 2 data0 242 $rtr_subnet2 255.255.255.0 1000000 1000 242
  ../bin/cfgRouterPort.sh 3 data0 243 $rtr_subnet3 255.255.255.0 1000000 1000 243
  ../bin/cfgRouterPort.sh 4 data0 244 $rtr_subnet4 255.255.255.0 1000000 1000 244
  ../bin/cfgRouterPort.sh 5 data1 245 $rtr_subnet5 255.255.255.0 1000000 1000 245
  ../bin/cfgRouterPort.sh 6 data1 246 $rtr_subnet6 255.255.255.0 1000000 1000 246
  ../bin/cfgRouterPort.sh 7 data1 247 $rtr_subnet7 255.255.255.0 1000000 1000 247
  ../bin/cfgRouterPort.sh 8 data1 248 $rtr_subnet8 255.255.255.0 1000000 1000 248

  ../bin/cfgRouterAddRoute.sh  192.168.81.0 255.255.255.0 data0 241
  ../bin/cfgRouterAddRoute.sh  192.168.82.0 255.255.255.0 data0 242
  ../bin/cfgRouterAddRoute.sh  192.168.83.0 255.255.255.0 data0 243
  ../bin/cfgRouterAddRoute.sh  192.168.84.0 255.255.255.0 data0 244
  ../bin/cfgRouterAddRoute.sh  192.168.85.0 255.255.255.0 data1 245
  ../bin/cfgRouterAddRoute.sh  192.168.86.0 255.255.255.0 data1 246
  ../bin/cfgRouterAddRoute.sh  192.168.87.0 255.255.255.0 data1 247
  ../bin/cfgRouterAddRoute.sh  192.168.88.0 255.255.255.0 data1 248
fi


echo "Done"

