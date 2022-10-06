#! /usr/bin/python
#
#  Copyright (c) 2015 Jyoti Parwatikar and Washington University in St. Louis.
#  All rights reserved
#
#  Distributed under the terms of the GNU General Public License v3
#


import subprocess
import argparse

aparser = argparse.ArgumentParser(description="return bytes or packet counter for iptables filter with the specified mark. returns bytes by default")
aparser.add_argument("-m", help="mark", nargs='+')
aparser.add_argument("-b", help="return byte counter", action="store_true", default=False)
aparser.add_argument("-p", help="return packet counter", action="store_true", default=False)

args = aparser.parse_args()


is_bytes = True

if args.p:
    is_bytes = False

filter_mark = int(args.m[0])


pipe = subprocess.Popen("iptables -nvL FORWARD -t filter", shell=True, stdout=subprocess.PIPE).stdout

#skip the first two lines which are not important to us 
packets = 0
fbytes = 0
pipe.readline()
pipe.readline()
for line in pipe:
    #parse for target = MARK and mark = filter_mark
    fields = line.split()
    if fields[2] == 'MARK':
        if fields[11] == hex(filter_mark):#should make some kind of hex conversion
             packets = fields[0]
             fbytes = fields[1]
#        print "mark:" + fields[11]
#	print "packets:" + packets
#	print "bytes:" + fbytes

if is_bytes:
    print fbytes.rstrip('KMGkmg')
else:
    print packets
