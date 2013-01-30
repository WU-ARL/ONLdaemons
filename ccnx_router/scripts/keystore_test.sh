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
