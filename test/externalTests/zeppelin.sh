#!/usr/bin/env bash

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
# (c) 2019 solidity contributors.
#------------------------------------------------------------------------------

set -e

source scripts/common.sh
source test/externalTests/common.sh

verify_input "$@"
BINARY_TYPE="$1"
BINARY_PATH="$2"
SELECTED_PRESETS="$3"

function compile_fn { npm run compile; }
function test_fn { npm test; }

function zeppelin_test
{
    local repo="https://github.com/OpenZeppelin/openzeppelin-contracts.git"
    local ref_type=branch
    local ref="master"
    local config_file="hardhat.config.js"

    local compile_only_presets=(
        ir-optimize-evm+yul       # Compiles but tests fail. See https://github.com/nomiclabs/hardhat/issues/2115
    )
    local settings_presets=(
        "${compile_only_presets[@]}"
        #ir-no-optimize           # Compilation fails with "YulException: Variable var_account_852 is 4 slot(s) too deep inside the stack."
        #ir-optimize-evm-only     # Compilation fails with "YulException: Variable var_account_852 is 4 slot(s) too deep inside the stack."
        legacy-no-optimize
        legacy-optimize-evm-only
        legacy-optimize-evm+yul
    )

    [[ $SELECTED_PRESETS != "" ]] || SELECTED_PRESETS=$(circleci_select_steps_multiarg "${settings_presets[@]}")
    print_presets_or_exit "$SELECTED_PRESETS"

    setup_solc "$DIR" "$BINARY_TYPE" "$BINARY_PATH"
    download_project "$repo" "$ref_type" "$ref" "$DIR"

    neutralize_package_json_hooks
    force_hardhat_compiler_binary "$config_file" "$BINARY_TYPE" "$BINARY_PATH"
    force_hardhat_compiler_settings "$config_file" "$(first_word "$SELECTED_PRESETS")"
    npm install

    replace_version_pragmas

    for preset in $SELECTED_PRESETS; do
        hardhat_run_test "$config_file" "$preset" "${compile_only_presets[*]}" compile_fn test_fn
    done
}

external_test Zeppelin zeppelin_test
