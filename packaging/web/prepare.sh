#!/bin/sh
# Prepare the web build: download the two libraries Emscripten has no port
# for (SDL2 and its image, mixer, ttf and net libraries, libpng and zlib come
# from Emscripten's ports). Then build:
#   packaging/web/prepare.sh
#   emcmake cmake -S packaging/web -B packaging/web/build -DCMAKE_BUILD_TYPE=Release
#   cmake --build packaging/web/build -j
# Everything it creates is ignored by git (see .gitignore here).
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
DEPS="$HERE/deps"
CACHE="${WARMUX_WEB_CACHE:-$HERE/.downloads}"
mkdir -p "$DEPS" "$CACHE"

# name  version  url  sha256 (the same versions as the Android build)
LIBS="
SDL2_gfx   1.0.4   https://downloads.sourceforge.net/project/sdl2gfx/SDL2_gfx-1.0.4.tar.gz 63e0e01addedc9df2f85b93a248f06e8a04affa014a835c2ea34bfe34e576262
libxml2    2.14.6  https://download.gnome.org/sources/libxml2/2.14/libxml2-2.14.6.tar.xz 7ce458a0affeb83f0b55f1f4f9e0e55735dbfc1a9de124ee86fb4a66b597203a
"

echo "$LIBS" | while read -r name version url sha; do
  [ -n "$name" ] || continue
  [ -f "$DEPS/$name/.version" ] && [ "$(cat "$DEPS/$name/.version")" = "$version" ] && continue
  file="$CACHE/$(basename "$url")"
  [ -f "$file" ] || curl -fsSL -o "$file" "$url"
  echo "$sha  $file" | sha256sum -c --quiet
  rm -rf "${DEPS:?}/$name" && mkdir -p "$DEPS/$name"
  tar -xf "$file" -C "$DEPS/$name" --strip-components=1
  echo "$version" > "$DEPS/$name/.version"
  echo "Fetched $name $version"
done
