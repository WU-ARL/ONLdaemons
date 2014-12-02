#! /bin/bash

sudo ip rule add unicast priority 32760 iif data0 table 252
sudo ip rule add unicast priority 32761 iif data1 table 250
sudo ip rule add unicast priority 32762 iif data2 table 248
sudo ip rule add unicast priority 32763 iif data3 table 246
sudo ip rule add unicast priority 32764 iif data4 table 244
