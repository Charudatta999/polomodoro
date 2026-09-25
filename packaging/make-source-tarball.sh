#!/usr/bin/env bash
# Usage: packaging/make-source-tarball.sh <version> [outdir]
# Writes <outdir>/polomodoro-<version>.tar.gz from the committed tree (git archive),
# so the package never contains build/, Renders/ or other untracked clutter.
set -euo pipefail
ver="${1:?version required}"
out="${2:-packaging/arch}"
mkdir -p "$out"
git archive --format=tar.gz --prefix="polomodoro-${ver}/" \
  -o "${out}/polomodoro-${ver}.tar.gz" HEAD
echo "${out}/polomodoro-${ver}.tar.gz"
