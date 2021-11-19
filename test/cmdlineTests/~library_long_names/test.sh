#!/usr/bin/env bash
set -euo pipefail

# shellcheck source=scripts/common.sh
source "${REPO_ROOT}/scripts/common.sh"

printTask "Testing long library names..."
echo '' | msg_on_error --no-stdout "$SOLC" - --link --libraries aveeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeerylonglibraryname=0x90f20564390eAe531E810af625A22f51385Cd222
