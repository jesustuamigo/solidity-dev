#!/usr/bin/env bash
#------------------------------------------------------------------------------
# Bash script to execute the Solidity tests by CircleCI.
#
# The documentation for solidity is hosted at:
#
#     https://docs.soliditylang.org
#
# ------------------------------------------------------------------------------
# Configuration Environment Variables:
#
#     EVM=version_string      Specifies EVM version to compile for (such as homestead, etc)
#     OPTIMIZE=1              Enables backend optimizer
#     ABI_ENCODER_V1=1        Forcibly enables ABI coder version 1
#     SOLTEST_FLAGS=<flags>   Appends <flags> to default SOLTEST_ARGS
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
# (c) 2016-2019 solidity contributors.
# ------------------------------------------------------------------------------
set -e

OPTIMIZE=${OPTIMIZE:-"0"}
EVM=${EVM:-"invalid"}
EOF_VERSION=${EOF_VERSION:-0}
CPUs=${CPUs:-3}
REPODIR="$(realpath "$(dirname "$0")/..")"

IFS=" " read -r -a BOOST_TEST_ARGS <<< "$BOOST_TEST_ARGS"
IFS=" " read -r -a SOLTEST_FLAGS <<< "$SOLTEST_FLAGS"

# TODO: [EOF] These won't pass on EOF yet. Reenable them when the implementation is complete.
EOF_EXCLUDES=(
    --run_test='!Assembler/all_assembly_items'
    --run_test='!Assembler/immutable'
    --run_test='!Assembler/immutables_and_its_source_maps'
    --run_test='!Optimiser/jumpdest_removal_subassemblies'
    --run_test='!Optimiser/jumpdest_removal_subassemblies/*'
    --run_test='!SolidityCompiler/does_not_include_creation_time_only_internal_functions'
    --run_test='!SolidityInlineAssembly/Analysis/create2'
    --run_test='!SolidityInlineAssembly/Analysis/inline_assembly_shadowed_instruction_declaration'
    --run_test='!SolidityInlineAssembly/Analysis/large_constant'
    --run_test='!SolidityInlineAssembly/Analysis/staticcall'
    --run_test='!ViewPureChecker/assembly_staticcall'
    --run_test='!functionSideEffects/otherImmovables'
    --run_test='!functionSideEffects/state'
    --run_test='!functionSideEffects/storage'
    --run_test='!gasTests/abiv2'
    --run_test='!gasTests/abiv2_optimised'
    --run_test='!gasTests/data_storage'
    --run_test='!gasTests/dispatch_large'
    --run_test='!gasTests/dispatch_large_optimised'
    --run_test='!gasTests/dispatch_medium'
    --run_test='!gasTests/dispatch_medium_optimised'
    --run_test='!gasTests/dispatch_small'
    --run_test='!gasTests/dispatch_small_optimised'
    --run_test='!gasTests/exp'
    --run_test='!gasTests/exp_optimized'
    --run_test='!gasTests/storage_costs'
    --run_test='!yulStackLayout/literal_loop'
)

# shellcheck source=scripts/common.sh
source "${REPODIR}/scripts/common.sh"
# Test result output directory (CircleCI is reading test results from here)
mkdir -p test_results

# in case we run with ASAN enabled, we must increase stack size.
ulimit -s 16384

get_logfile_basename() {
    local run="$1"
    local filename="${EVM}"
    test "${OPTIMIZE}" = "1" && filename="${filename}_opt"
    test "${ABI_ENCODER_V1}" = "1" && filename="${filename}_abiv1"
    (( EOF_VERSION != 0 )) && filename="${filename}_eofv${EOF_VERSION}"
    filename="${filename}_${run}"

    echo -ne "${filename}"
}

[ -z "$CIRCLE_NODE_TOTAL" ] || [ "$CIRCLE_NODE_TOTAL" = 0 ] && CIRCLE_NODE_TOTAL=1
[ -z "$CIRCLE_NODE_INDEX" ] && CIRCLE_NODE_INDEX=0
[ -z "$INDEX_SHIFT" ] && INDEX_SHIFT=0

# Multiply by a prime number to get better spread, just in case
# long-running test cases are next to each other.
CIRCLE_NODE_INDEX=$(((CIRCLE_NODE_INDEX + 23 * INDEX_SHIFT) % CIRCLE_NODE_TOTAL))

PIDs=()
for run in $(seq 0 $((CPUs - 1)))
do
    BOOST_TEST_ARGS_RUN=(
        "--color_output=no"
        "--show_progress=yes"
        "--logger=JUNIT,error,test_results/$(get_logfile_basename "$((CPUs * CIRCLE_NODE_INDEX + run))").xml"
        "--logger=HRF,error,stdout"
        "${BOOST_TEST_ARGS[@]}"
    )
    (( EOF_VERSION != 0 )) && BOOST_TEST_ARGS_RUN+=("${EOF_EXCLUDES[@]}")
    SOLTEST_ARGS=("--evm-version=$EVM" "${SOLTEST_FLAGS[@]}")

    test "${OPTIMIZE}" = "1" && SOLTEST_ARGS+=(--optimize)
    test "${ABI_ENCODER_V1}" = "1" && SOLTEST_ARGS+=(--abiencoderv1)
    (( EOF_VERSION != 0 )) && SOLTEST_ARGS+=(--eof-version "$EOF_VERSION")

    BATCH_ARGS=("--batches" "$((CPUs * CIRCLE_NODE_TOTAL))" "--selected-batch" "$((CPUs * CIRCLE_NODE_INDEX + run))")

    echo "Running ${REPODIR}/build/test/soltest ${BOOST_TEST_ARGS_RUN[*]} -- ${SOLTEST_ARGS[*]}"

    "${REPODIR}/build/test/soltest" -l test_suite "${BOOST_TEST_ARGS_RUN[@]}" -- "${SOLTEST_ARGS[@]}" "${BATCH_ARGS[@]}" &
    PIDs+=($!)
done

# wait for individual processes to get their exit status
for pid in "${PIDs[@]}"
do
    wait "$pid"
done
