#!/bin/bash
# MAH: based on code in http://www.mjmwired.net/kernel/Documentation/networking/ixgb.txt


function usage {
   echo "usage: <interface> <runpath>"
   echo "This script tunes the linux parameters for 10GbE card according to Intel's recommendations."
   echo "It must be run as root on the host with the 10Gb NIC." 
   exit
}

if [ $# -lt 2 ]
    then
    usage
fi
iface=$1
runpath=$2

function ethtool_dump_device {
  echo "-------------- start $iface configuration ----------------"
  sudo ethtool -a $iface
  sudo ethtool -c $iface
  sudo ethtool -g $iface
  sudo ethtool -k $iface
  sudo ethtool -u $iface
  sudo ethtool -T $iface
  sudo ethtool -x $iface
  sudo ethtool -l $iface
  echo "-------------- end $iface configuration ----------------"

}

#echo "Dumping device $iface configuration before tuning"
#ethtool_dump_device

# Need to find the correct device ID
devnum=0
if [ "$iface" == "data1" ]
then
    devnum=1
fi
# Assume lspci reports the following entries for data0 and data1:
#23:00.0 Ethernet controller [0200]: Intel Corporation 82599EB 10-Gigabit SFI/SFP+ Network Connection [8086:10fb] (rev 01)
#23:00.1 Ethernet controller [0200]: Intel Corporation 82599EB 10-Gigabit SFI/SFP+ Network Connection [8086:10fb] (rev 01)
# XXX Huh... they appear to be the same  for data0 and data1... should they be?
devid=`lspci -vv -nn | grep 8086 | grep "$devnum Ethernet controller" | awk '{print $12}' | awk -F'[' '{print $2}' | awk -F']' '{print $1}'`

#echo "configuring network performance , edit this file to change the interface or device ID of 10GbE card"
#echo "set mmrbc to 4k reads, modify only Intel 10GbE device IDs"
# replace 1a48 with appropriate 10GbE device's ID installed on the system,
# if needed.
#devid=8086:1a48
#echo "sudo setpci -d $devid e6.b=2e"
#sudo setpci -d $devid e6.b=2e

# set the MTU (max transmission unit) - it requires your switch and clients
# to change as well.
# set the txqueuelen
# your ixgb adapter should be loaded as eth1 for this to work, change if needed
#MTU=9000 # I think this will only benefit TCP
#MTU=1500 
#echo "sudo ifconfig $iface mtu $MTU txqueuelen 1000 up"
#sudo ifconfig $iface mtu $MTU txqueuelen 1000 up

sudo ifconfig $iface txqueuelen 10000 up


# Set the rx/tx ring size to their maximum values
cmd="sudo ethtool -G $iface rx 4096"
echo "$cmd" ; eval "$cmd"
cmd="sudo ethtool -G $iface tx 4096"
echo "$cmd" ; eval "$cmd"
#sudo ethtool -g $iface # print settings for verification


# XXX NOTE: it appears only one of the following 3 get set each time this script is run
# Disable ethernet pause frames
cmd="sudo ethtool -A $iface autoneg off"
echo "$cmd" ; eval "$cmd" 
sleep 2 # we need to sleep here to give the autoneg a chance to get turned off before we turn off rx and tx
cmd="sudo ethtool -A $iface rx off"
echo "$cmd" ; eval "$cmd" 
cmd="sudo ethtool -A $iface tx off"
echo "$cmd" ; eval "$cmd" 
sleep 2
cmd="sudo ethtool -A $iface rx off"
echo "$cmd" ; eval "$cmd" 
cmd="sudo ethtool -A $iface tx off"
echo "$cmd" ; eval "$cmd" 
#sleep 5 # we need to sleep here to give the rx and tx a chance to get turned off before we verify
#sudo ethtool -a $iface # print settings for verification

# Turn off irqbalance daemon
# recommended by among others: http://www.redhat.com/promo/summit/2008/downloads/pdf/Thursday/Mark_Wagner.pdf
#cmd="sudo service irqbalance stop"
#echo "$cmd" ; eval "$cmd"

# http://www.kernel.org/doc/ols/2009/ols2009-pages-169-184.pdf
# "Intel I/OAT is a feature that improves network application responsiveness by moving
# network data more ef- ficiently through Dual-Core and Quad-Core Intel Xeon 
# processor-based servers when using Intel network cards. 
# This feature improves the overall network speed."
# "It is not recommended to remove the ioatdma module after it was loaded, 
# once TCP holds a reference to the ioatdma driver when offloading receive traffic."
cmd="sudo modprobe ioatdma"
#echo "$cmd" ; eval "$cmd"
# Note: it seems ioatdma can be loaded on a pc48core despite being AMD... not sure whether it helps 

# Turn rx hashing off... probably a good idea when we add our own filters
cmd="sudo ethtool -K $iface rxhash off"
echo "$cmd" ; eval "$cmd"

# Generic receive offload
#cmd="sudo ethtool -K $iface gro off"
#echo "$cmd" ; eval "$cmd"

# Large receive offload
#cmd="sudo ethtool -K $iface lro off"
#echo "$cmd" ; eval "$cmd"

# Generic segmentation offload
#cmd="sudo ethtool -K $iface gso off"
#echo "$cmd" ; eval "$cmd"

# Scatter gather
#cmd="sudo ethtool -K $iface sg off"
#echo "$cmd" ; eval "$cmd"

# rx-frames-irq : max number of pkts to process per interrupt
#cmd="sudo ethtool -C $iface rx-frames-irq 512"
#echo "$cmd" ; eval "$cmd"

#cmd="sudo ethtool -C $iface rx-frames 512"
#echo "$cmd" ; eval "$cmd"

cmd="sudo ethtool -C $iface rx-usecs 512"
echo "$cmd" ; eval "$cmd"

# call the sysctl utility to modify /proc/sys entries
echo "sudo sysctl -p $runpath/sysctl_ixgb.conf"
sudo sysctl -p $runpath/sysctl_ixgb.conf


#echo "Dumping device $iface configuration after tuning"
#ethtool_dump_device

