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
#!/bin/bash
source /users/onl/pc1core/ccnx/scripts/set_prefix.sh
HOST="$2"
COMMAND="$3"
OUTFILE="$1"
shift; shift; shift;
ssh -f -q -o StrictHostKeyChecking=no "${HOST}" "${SCRIPT_DIR}/${COMMAND}" "$@" 2>&1 > "${OUTFILE}"
