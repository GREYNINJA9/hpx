#!/bin/bash

# Copyright (c) 2026 Hartmut Kaiser
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Prints the master commits (the first parents, oldest first) after <since>
# that are not in runs.tsv of the clone of backfill.sh, as commits.txt wants
# them. <since> defaults to the commit of the last run in runs.tsv.
#
# Usage: missing_commits.sh [<since> [<master, default origin/master>]]
#
# With PERFTEST_BACKFILL_DIR as for backfill.sh. See README.md.
set -euo pipefail
base="${PERFTEST_BACKFILL_DIR:?the work directory}"
runs=${base}/upload/data/runs.tsv
hpx="$(cd "$(dirname "$0")/../../.." && pwd)"
since="${1:-$(tail -1 "${runs}" 2> /dev/null | cut -f2 || true)}"
if [[ -z "${since}" || "${since}" == commit ]]; then
    echo "no <since> commit given and no run in ${runs}" >&2
    exit 1
fi
master="${2:-origin/master}"
git -C "${hpx}" log --first-parent --reverse --format=%H "${since}..${master}" |
    while read -r commit; do
        grep -q "	${commit}	" "${runs}" || echo "${commit}"
    done
