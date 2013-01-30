#!/bin/bash
source /users/onl/pc1core/ccnx/scripts/set_prefix.sh
HOST="$2"
COMMAND="$3"
OUTFILE="$1"
shift; shift; shift;
ssh -f -q -o StrictHostKeyChecking=no "${HOST}" "${SCRIPT_DIR}/${COMMAND}" "$@" 2>&1 > "${OUTFILE}"
