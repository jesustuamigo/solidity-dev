#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Bash script to execute the Solidity tests.
#
# The documentation for solidity is hosted at:
#
#     https://solidity.readthedocs.org
#
# ------------------------------------------------------------------------------
# This file is part of solidity.
#
# solidity is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# solidity is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with solidity.  If not, see <http://www.gnu.org/licenses/>
#
# (c) 2017 solidity contributors.
#------------------------------------------------------------------------------

set -e

REPO_ROOT=$(cd $(dirname "$0")/.. && pwd)
SOLJSON="$REPO_ROOT/build/libsolc/soljson.js"

echo "Running solcjs tests...."
"$REPO_ROOT/test/solcjsTests.sh" "$SOLJSON"
echo "Running external tests...."
"$REPO_ROOT/test/externalTests.sh" "$SOLJSON"
