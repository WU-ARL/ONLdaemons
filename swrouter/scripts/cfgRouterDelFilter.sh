#!/bin/bash
if [ $# -ne 23 ]
then
  echo "Usage: $0 <portnum> <iface> <vlanNum> <filterId> <dstAddr> <dstMask> <srcAddr> <srcMask> <proto> <dport> <sport> <tcpSyn> <tcpAck> <tcpFin> <tcpRst> <tcpUrg> <tcpPsh> <qid> <drop> <outputPortNum> <outputDev> <outputVlan> <gw>"
  echo "Example: $0 1 data0 241 27 192.168.82.0 24 0.0.0.0 0 tcp '*' '*' '*'  '*'  '*'  '*'  '*'  '*'  100 '*' 2 data0 242 192.168.82.249"
  exit 0
else
  inPortNum=$1
  iface=$2   # data0 or data1
  vlanNum=$3 # 241 or similar

  filterId=$4

  MIN_FILTERID=1
  MAX_FILTERID=30
  if [ $filterId -lt $MIN_FILTERID -o $filterId -gt $MAX_FILTERID ]
  then
    echo "filterId must be between $MIN_FILTERID and $MAX_FILTERID, given value was $filterId"
    exit
  fi

  dstAddr=$5 # 
  dstMask=$6 #  0 implies no dstAddr included. 32 implies full match (no mask needed)
  srcAddr=$7 # 
  srcMask=$8 #  0 implies no srcAddr included. 32 implies full match (no mask needed)
  proto=$9   #  *,tcp,udp,icmp, ... (0 is don't care)
  dport=${10}   # * is don't care
  sport=${11}   # * is don't care
  # TCP Flags: 0,1,* (unset, set, don't care)
  tcpFin=${12} #
  tcpSyn=${13} #
  tcpRst=${14} #
  tcpPsh=${15} #
  tcpAck=${16} #
  tcpUrg=${17} #
  qid=${18}
  drop=${19}  # 0,1 (forward, drop)
  # output
  outPortNum=${20}
  DEV=${21}       # 
  VLAN=${22}       # 
  GW=${23}   # * means leave gw out



  #sudo ip route add table main ${PREFIX}/${MASK} dev $DEV.$VLAN via $GW
  #sudo ip route flush cache

  # Add iptables rule with mark set to filterId
  if [ "$drop" = "1" ]
  then
    ACTION="-j DROP"
    markSpec=""
  else
    ACTION="-j MARK"
    markSpec="--set-mark $filterId"
  fi
  if [ $dstMask -eq 0 ]
  then
    dstAddrSpec=""
  elif [ $dstMask -eq 32 ]
  then
    dstAddrSpec="--dst $dstAddr"
  else
    dstAddrSpec="--dst ${dstAddr}/${dstMask}"
  fi
  if [ $srcMask -eq 0 ]
  then
    srcAddrSpec=""
  elif [ $srcMask -eq 32 ]
  then
    srcAddrSpec="--src $srcAddr"
  else
    srcAddrSpec="--src ${srcAddr}/${srcMask}"
  fi
  if [ "$proto" = "*" ]
  then
    protoSpec=""
  else
    protoSpec="-p $proto"
  fi
  if [ "$dport" = "*" ]
  then
    dstPortSpec=""
  else
    dstPortSpec="-dport $dstPort"
  fi
  if [ "$sport" = "*" ]
  then
    srcPortSpec=""
  else
    srcPortSpec="-sport $srcPort"
  fi
  if [ "$tcpFin" = "*" -a "$tcpSyn" = "*"  -a "$tcpRst" = "*"  -a "$tcpPsh" = "*"  -a "$tcpAck" = "*"  -a "$tcpUrg" = "*"  ]
  then
    tcpFlagsSpec=""
  else
    # go through each tcp flag and add to spec and mask...
    tcpFlagsMask=""
    tcpFlagsSet=""
    firstSet="TRUE"
    firstMask="TRUE"
    if [ $tcpSyn = "1" ]
    then
      # add it to both Mask and Set
      if [ firstSet = "TRUE" ]
      then
        firstSet="FALSE"
      else
        tcpFlagsSet="${tcpFlagsSet},"
      fi
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}SYN"
      tcpFlagsSet="${tcpFlagsSet}SYN"

    elif [ $tcpSyn = "0" ]
    then
      # add it to Mask
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}SYN"
    fi
    if [ $tcpAck = "1" ]
    then
      # add it to both Mask and Set
      if [ firstSet = "TRUE" ]
      then
        firstSet="FALSE"
      else
        tcpFlagsSet="${tcpFlagsSet},"
      fi
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}ACK"
      tcpFlagsSet="${tcpFlagsSet}ACK"

    elif [ $tcpAck = "0" ]
    then
      # add it to Mask
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}ACK"
    fi
    if [ $tcpFin = "1" ]
    then
      # add it to both Mask and Set
      if [ firstSet = "TRUE" ]
      then
        firstSet="FALSE"
      else
        tcpFlagsSet="${tcpFlagsSet},"
      fi
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}FIN"
      tcpFlagsSet="${tcpFlagsSet}FIN"

    elif [ $tcpFin = "0" ]
    then
      # add it to Mask
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}FIN"
    fi
    if [ $tcpRst = "1" ]
    then
      # add it to both Mask and Set
      if [ firstSet = "TRUE" ]
      then
        firstSet="FALSE"
      else
        tcpFlagsSet="${tcpFlagsSet},"
      fi
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}RST"
      tcpFlagsSet="${tcpFlagsSet}RST"

    elif [ $tcpRst = "0" ]
    then
      # add it to Mask
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}RST"
    fi
    if [ $tcpUrg = "1" ]
    then
      # add it to both Mask and Set
      if [ firstSet = "TRUE" ]
      then
        firstSet="FALSE"
      else
        tcpFlagsSet="${tcpFlagsSet},"
      fi
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}URG"
      tcpFlagsSet="${tcpFlagsSet}URG"

    elif [ $tcpUrg = "0" ]
    then
      # add it to Mask
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}URG"
    fi
    if [ $tcpPsh = "1" ]
    then
      # add it to both Mask and Set
      if [ firstSet = "TRUE" ]
      then
        firstSet="FALSE"
      else
        tcpFlagsSet="${tcpFlagsSet},"
      fi
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}PSH"
      tcpFlagsSet="${tcpFlagsSet}PSH"

    elif [ $tcpPsh = "0" ]
    then
      # add it to Mask
      if [ firstMask = "TRUE" ]
      then
        firstMask="FALSE"
      else
        tcpFlagsMask="${tcpFlagsMask},"
      fi
      tcpFlagsMask="${tcpFlagsMask}PSH"
    fi
  fi
  # if we are dropping do it in PREROUTING/mangle 
  # if we are forwarding set a mark in PREROUTING/mangle to get it to the right route table and in FORWARD/filter to get it to the right output queue!
  if [ "$drop" = "1" ]
  then
    echo "iptables -t mangle -D PREROUTING -i $iface.$vlanNum $protoSpec $dstAddrSpec $srcAddrSpec $dstPortSpec $srcPortSpec $tcpFlagsSpec $ACTION $markSpec"
    sudo  iptables -t mangle -D PREROUTING -i $iface.$vlanNum $protoSpec $dstAddrSpec $srcAddrSpec $dstPortSpec $srcPortSpec $tcpFlagsSpec $ACTION $markSpec
  else
    echo "iptables -t mangle -D PREROUTING -i $iface.$vlanNum $protoSpec $dstAddrSpec $srcAddrSpec $dstPortSpec $srcPortSpec $tcpFlagsSpec $ACTION $markSpec"
    sudo  iptables -t mangle -D PREROUTING -i $iface.$vlanNum $protoSpec $dstAddrSpec $srcAddrSpec $dstPortSpec $srcPortSpec $tcpFlagsSpec $ACTION $markSpec
    echo "iptables -t filter -D FORWARD    -i $iface.$vlanNum $protoSpec $dstAddrSpec $srcAddrSpec $dstPortSpec $srcPortSpec $tcpFlagsSpec $ACTION $markSpec"
    sudo  iptables -t filter -D FORWARD    -i $iface.$vlanNum $protoSpec $dstAddrSpec $srcAddrSpec $dstPortSpec $srcPortSpec $tcpFlagsSpec $ACTION $markSpec
  fi

  # Add ip route to appropriate route table based on Port and Filter ID
  routeTable=port${inPortNum}Filter${filterId}
  #DEV=${20}       # 
  #VLAN=${21}       # 
  #GW=${22}
  if [ "$GW" = '*' ]
  then
    GW_SPEC=""
  else
    GW_SPEC="via ${GW}"
  fi
    dstAddrSpec="--dst ${dstAddr}/${dstMask}"
  echo "ip route del table $routeTable unicast ${dstAddr}/${dstMask}  dev ${DEV}.${VLAN} $GW_SPEC"
  sudo  ip route del table $routeTable unicast ${dstAddr}/${dstMask}  dev ${DEV}.${VLAN} $GW_SPEC

  # Delete ip rule to direct marked packets to route table
  echo "ip rule del fwmark $filterId table $routeTable"
  sudo  ip rule del fwmark $filterId table $routeTable

  # Delete a tc rule to direct to queue identified by qid
  echo "tc filter del dev ${DEV}.${VLAN} parent ${outPortNum}:0 protocol ip prio 1 handle $filterId fw flowid ${outPortNum}:$qid"
  sudo  tc filter del dev ${DEV}.${VLAN} parent ${outPortNum}:0 protocol ip prio 1 handle $filterId fw flowid ${outPortNum}:$qid


fi

