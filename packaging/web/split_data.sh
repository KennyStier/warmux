#!/bin/sh
# Split the game data for the web build:
#   <out>/core  everything needed to start, packed into warmux.data
#   <out>/lazy  files downloaded when first used: each map's images (all but
#               config.xml and the small previews), the battle songs, and the fonts
#               only needed for Japanese and Chinese
# <out>/core/lazy.txt lists the lazy files, after a first line with a version
# that changes whenever they do, so browsers never keep a stale copy.
# Usage: split_data.sh <source data folder> <out>
set -eu
SRC=$1
OUT=$2

rm -rf "$OUT/core" "$OUT/lazy"
mkdir -p "$OUT/core" "$OUT/lazy"

cd "$SRC"
find . -type f ! -name '.*' ! -name 'Makefile*' ! -name '*.svg' ! -name '*.desktop' \
  ! -name '*.in' ! -name '*.icns' ! -name '*.ico' ! -name '*.mbm' ! -name '*.mif' \
  | sed 's|^\./||' | LC_ALL=C sort > "$OUT/all.txt"

grep -E '^map/[^/]+/[^/]+$' "$OUT/all.txt" | grep -vE '/(config\.xml|preview\.[a-z]+|mini\.[a-z]+)$' > "$OUT/lazy.list" || true
grep -E '^music/ingame/.*\.ogg$' "$OUT/all.txt" >> "$OUT/lazy.list" || true
grep -E '^font/(VL-Gothic-Regular\.ttf|wqy-microhei-lite\.ttc)$' "$OUT/all.txt" >> "$OUT/lazy.list" || true
LC_ALL=C sort -o "$OUT/lazy.list" "$OUT/lazy.list"
LC_ALL=C comm -23 "$OUT/all.txt" "$OUT/lazy.list" > "$OUT/core.list"

tar -cf - -T "$OUT/core.list" | tar -xf - -C "$OUT/core"
tar -cf - -T "$OUT/lazy.list" | tar -xf - -C "$OUT/lazy"

version=$(tar -cf - -T "$OUT/lazy.list" | sha256sum | cut -c1-12)
{ echo "$version"; cat "$OUT/lazy.list"; } > "$OUT/core/lazy.txt"

echo "Web data: $(wc -l < "$OUT/core.list") files at start ($(du -sh "$OUT/core" | cut -f1)), $(wc -l < "$OUT/lazy.list") when needed ($(du -sh "$OUT/lazy" | cut -f1))"
