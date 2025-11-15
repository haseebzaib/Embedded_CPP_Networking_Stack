#!/usr/bin/env bash
# Simple, robust packer: archive repo excluding build + VCS clutter.

set -euo pipefail

# 1) sanity: must run from a directory that has some source files
if ! ls *.cpp *.c *.hpp *.h  >/dev/null 2>&1 && ! find . -maxdepth 2 -name CMakeLists.txt | grep -q . ; then
  echo "[!] Run this from your project root (where your sources/CMakeLists.txt live)."
fi

STAMP="$(date +%Y%m%d_%H%M%S)"
ARCHIVE="netstack_bundle_${STAMP}.tar.gz"

echo "[+] Creating $ARCHIVE ..."
# Pack everything except noisy dirs/files. This is intentionally simple.
tar -czf "$ARCHIVE" \
  --exclude-vcs \
  --exclude='./build' \
  --exclude='./build-*' \
  --exclude='./.vscode' \
  --exclude='./.idea' \
  --exclude='./.cache' \
  --exclude='./node_modules' \
  --exclude='./venv' \
  --exclude='./.venv' \
  --exclude='*.o' \
  --exclude='*.obj' \
  --exclude='*.a' \
  --exclude='*.so' \
  --exclude='*.exe' \
  --exclude='*.out' \
  .

# quick contents check
COUNT=$(tar -tzf "$ARCHIVE" | wc -l | tr -d ' ')
echo "[+] Files in archive: $COUNT"
echo "[✓] Done: $ARCHIVE"
