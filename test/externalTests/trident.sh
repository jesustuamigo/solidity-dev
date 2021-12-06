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
# (c) 2021 solidity contributors.
#------------------------------------------------------------------------------

set -e

source scripts/common.sh
source test/externalTests/common.sh

verify_input "$@"
BINARY_TYPE="$1"
BINARY_PATH="$2"

function compile_fn { yarn build; }

function test_fn {
    # shellcheck disable=SC2046
    TS_NODE_TRANSPILE_ONLY=1 npx hardhat test --no-compile $(
        # TODO: We need to skip Migration.test.ts because it fails and makes other tests fail too.
        # Replace this with `yarn test` once https://github.com/sushiswap/trident/issues/283 is fixed.
        find test/ -name "*.test.ts" ! -path "test/Migration.test.ts"
    )
}

function trident_test
{
    # TODO: Switch to https://github.com/sushiswap/trident / master once
    # https://github.com/sushiswap/trident/pull/282 gets merged.
    local repo="https://github.com/solidity-external-tests/trident.git"
    local branch=master_080
    local config_file="hardhat.config.ts"
    local config_var=config

    local compile_only_presets=()
    local settings_presets=(
        "${compile_only_presets[@]}"
        #ir-no-optimize            # Compilation fails with: "YulException: Variable var_amount_165 is 9 slot(s) too deep inside the stack."
        #ir-optimize-evm-only      # Compilation fails with: "YulException: Variable var_amount_165 is 9 slot(s) too deep inside the stack."
        #ir-optimize-evm+yul       # Compilation fails with: "YulException: Cannot swap Variable var_nearestTick with Variable _4: too deep in the stack by 4 slots"
        legacy-no-optimize
        legacy-optimize-evm-only
        legacy-optimize-evm+yul
    )

    local selected_optimizer_presets
    selected_optimizer_presets=$(circleci_select_steps_multiarg "${settings_presets[@]}")
    print_optimizer_presets_or_exit "$selected_optimizer_presets"

    setup_solc "$DIR" "$BINARY_TYPE" "$BINARY_PATH"
    download_project "$repo" "$branch" "$DIR"

    # TODO: Currently tests work only with the exact versions from yarn.lock.
    # Re-enable this when https://github.com/sushiswap/trident/issues/284 is fixed.
    #neutralize_package_lock

    neutralize_package_json_hooks
    force_hardhat_compiler_binary "$config_file" "$BINARY_TYPE" "$BINARY_PATH"
    force_hardhat_compiler_settings "$config_file" "$(first_word "$selected_optimizer_presets")" "$config_var"
    yarn install

    replace_version_pragmas
    force_solc_modules "${DIR}/solc"

    # @sushiswap/core package contains contracts that get built with 0.6.12 and fail our compiler
    # version check. It's not used by tests so we can remove it.
    rm -r node_modules/@sushiswap/core/

    for preset in $selected_optimizer_presets; do
        hardhat_run_test "$config_file" "$preset" "${compile_only_presets[*]}" compile_fn test_fn "$config_var"
    done
}

external_test Trident trident_test
