#!/bin/bash
source /users/onl/pc1core/ccnx/scripts/set_prefix.sh
source ${CMD_DIR}/setenv.sh
${CMD_DIR}/ccndelphi "$1" 2>&1
