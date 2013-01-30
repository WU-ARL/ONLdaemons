#!/bin/bash
source /users/onl/pc1core/ccnx/scripts/set_prefix.sh
source ${CMD_DIR}/setenv.sh
DEST_PORT=9695
${CMD_DIR}/ccndc del "$1" udp "${2}" "${DEST_PORT}" 2>&1
