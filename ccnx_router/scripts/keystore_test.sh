#
# ONL Notice
#
# Copyright (c) 2009-2013 Pierluigi Rolando, Charlie Wiseman, Jyoti Parwatikar, John DeHart
# and Washington University in St. Louis
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
#   limitations under the License.

include ../../Makefile.inc
#! /bin/sh 

CCNX=/users/onl/ccnx/ccnx-head

if [ -f $HOME/.ccnx/.ccnx_keystore ]
then
  echo "$HOME/.ccnx/.ccnx_keystore already there. You should be ready to go."
else
  echo "$HOME/.ccnx/.ccnx_keystore not there. Creating one..."
  rm -r $HOME/.ccnx
  cd $HOME
  $CCNX/csrc/lib/ccn_initkeystore.sh
fi
