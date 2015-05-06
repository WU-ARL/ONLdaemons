#!/usr/bin/python

#
#  Copyright (c) 2015 Jyoti Parwatikar and Washington University in St. Louis.
#  All rights reserved
#
#  Distributed under the terms of the GNU General Public License v3
#



import xml.etree.ElementTree as ET
#import sys
import argparse
import subprocess


aparser = argparse.ArgumentParser(description="using vmid find vnet of data interface")
#aparser.add_argument("vmid", help="vm id from virsh list", type=int, action="store_true")
aparser.add_argument("vmid", help="vm id from virsh list")
args = aparser.parse_args()


p = subprocess.Popen(["virsh", "dumpxml", args.vmid], stdout=subprocess.PIPE)
output, err = p.communicate()

root = ET.fromstring(output)

for ifc in root.iter('interface'):
     source = ifc.find('source').attrib.get('bridge')
     if source.startswith('br_vlan'):
          target = ifc.find('target').attrib.get('dev')
          print target
          break
