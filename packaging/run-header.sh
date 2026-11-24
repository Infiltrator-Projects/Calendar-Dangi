#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Network-only native installer. No source or binaries are embedded.
set -Eeuo pipefail
readonly repository='@REPOSITORY@'
readonly version='@VERSION@'
readonly source_commit='@SOURCE_COMMIT@'
readonly common_commit='@COMMON_COMMIT@'
[[ "$repository" =~ ^Calendar-[A-Za-z0-9-]+$ && "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ &&
   "$source_commit" =~ ^[0-9a-f]{40}$ && "$common_commit" =~ ^[0-9a-f]{40}$ ]] || exit 1
(( $(wc -c < "$0") < 16384 )) || exit 1
if [[ "${1:-}" == --help || "${1:-}" == -h ]]; then
  echo 'Native calendar installer: --profile aggressive|native|portable, --build-only --output DIR, --system-package-mode, --jobs N'
  exit 0
fi
for cmd in git python3; do command -v "$cmd" >/dev/null || { echo "Missing bootstrap utility: $cmd" >&2; exit 1; }; done
work=$(mktemp -d "${TMPDIR:-/tmp}/infiltrator-provider.XXXXXXXX")
trap 'rm -rf -- "$work"' EXIT
trap 'exit 1' HUP INT TERM
git init --quiet "$work/source"
git -C "$work/source" remote add origin "https://github.com/Infiltrator-Projects/$repository.git"
git -C "$work/source" fetch --quiet --depth=1 origin "$source_commit"
git -C "$work/source" checkout --quiet --detach FETCH_HEAD
[[ $(git -C "$work/source" rev-parse HEAD) == "$source_commit" ]]
[[ $(git -C "$work/source" ls-tree HEAD -- common | awk '$1 == "160000" {print $3}') == "$common_commit" ]]
[[ $(git -C "$work/source" config -f .gitmodules --get submodule.common.url) == https://github.com/Infiltrator-Projects/Libraries.git ]]
git -C "$work/source" submodule update --init --depth 1 -- common
[[ $(git -C "$work/source/common" rev-parse HEAD) == "$common_commit" ]]
[[ $(cat "$work/source/VERSION") == "$version" ]]
python3 "$work/source/packaging/release.py" install "$@"
