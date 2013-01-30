#!/bin/bash
source /users/onl/ccnx/scripts/set_prefix.sh
source /users/onl/ccnx/scripts/keystore_test.sh
#source ${CMD_DIR}/setenv.sh
${CMD_DIR}/ccndstart 2>&1
