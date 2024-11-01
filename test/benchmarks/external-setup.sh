#!/usr/bin/env bash

#------------------------------------------------------------------------------
# Downloads and configures external projects used for benchmarking by external.sh.
#
# By default the download location is the benchmarks/ dir at the repository root.
# A different directory can be provided via the BENCHMARK_DIR variable.
#
# Dependencies: foundry, git.
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
# (c) 2024 solidity contributors.
#------------------------------------------------------------------------------

set -euo pipefail

repo_root=$(cd "$(dirname "$0")/../../" && pwd)
BENCHMARK_DIR="${BENCHMARK_DIR:-${repo_root}/benchmarks}"

function neutralize_version_pragmas {
    find . -name '*.sol' -type f -print0 | xargs -0 \
        sed -i -E -e 's/pragma solidity [^;]+;/pragma solidity *;/'
}

function neutralize_via_ir {
    sed -i '/^via_ir\s*=.*$/d' foundry.toml
}

function setup_foundry_project {
    local subdir="$1"
    local ref_type="$2"
    local ref="$3"
    local repo_url="$4"
    local install_function="${5:-}"

    printf ">>> %-22s | " "$subdir"

    [[ $ref_type == commit || $ref_type == tag ]] || assertFail

    [[ ! -e "$subdir" ]] || { printf "already exists\n"; return; }
    printf "downloading...\n\n"

    if [[ $ref_type == tag ]]; then
        git clone --depth=1 "$repo_url" "$subdir" --branch "$ref"
        pushd "$subdir"
    else
        git clone "$repo_url" "$subdir"
        pushd "$subdir"
        git checkout "$ref"
    fi
    if [[ -z $install_function ]]; then
        forge install
    else
        "$install_function"
    fi

    neutralize_via_ir
    neutralize_version_pragmas
    popd
    echo
}

function install_sablier {
    # NOTE: To avoid hard-coding dependency versions here we'd have to install them from npm
    forge install --no-commit \
        foundry-rs/forge-std@v1.5.6 \
        OpenZeppelin/openzeppelin-contracts@v4.9.2 \
        PaulRBerg/prb-math@v4.0.2 \
        PaulRBerg/prb-test@v0.6.4 \
        evmcheb/solarray@a547630 \
        Vectorized/solady@v0.0.129
   cat <<EOF > remappings.txt
@openzeppelin/contracts/=lib/openzeppelin-contracts/contracts/
forge-std/=lib/forge-std/
@prb/math/=lib/prb-math/
@prb/test/=lib/prb-test/
solarray/=lib/solarray/
solady/=lib/solady/
EOF
}

mkdir -p "$BENCHMARK_DIR"
cd "$BENCHMARK_DIR"

setup_foundry_project openzeppelin/ tag v5.0.2 https://github.com/OpenZeppelin/openzeppelin-contracts
setup_foundry_project uniswap-v4/ commit ae86975b058d386c9be24e8994236f662affacdb https://github.com/Uniswap/v4-core
# NOTE: Can't select the tag with `git clone` because a branch of the same name exists.
setup_foundry_project seaport/ commit tags/1.6 https://github.com/ProjectOpenSea/seaport
setup_foundry_project eigenlayer/ tag v0.3.0-holesky-rewards https://github.com/Layr-Labs/eigenlayer-contracts
setup_foundry_project sablier-v2/ tag v1.1.2 https://github.com/sablier-labs/v2-core install_sablier
