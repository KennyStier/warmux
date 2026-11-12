#!/bin/sh
# Prepare the Android build: download the native libraries, copy SDL's Java
# glue and stage the game data as APK assets. Run before Gradle:
#   packaging/android/prepare.sh && cd packaging/android && ./gradlew assembleDebug
# Everything it creates is ignored by git (see .gitignore here).
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
SRC=$(cd "$HERE/../.." && pwd)
DEPS="$HERE/app/jni/deps"
CACHE="${WARMUX_ANDROID_CACHE:-$HERE/.downloads}"
mkdir -p "$DEPS" "$CACHE"

# name  version  url  sha256
LIBS="
SDL2       2.32.10 https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz 5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165
SDL2_image 2.8.12  https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.12/SDL2_image-2.8.12.tar.gz 393f5efb50536ec13ca4f4affb69cc9966d3c3f969e6c5e701faddf9f9785381
SDL2_mixer 2.8.2   https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.2/SDL2_mixer-2.8.2.tar.gz 938dff531d00ace2296557a6599abe6f34599e2f34f0a4a08a397e2ccac8b8f7
SDL2_ttf   2.24.0  https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.24.0/SDL2_ttf-2.24.0.tar.gz 0b2bf1e7b6568adbdbc9bb924643f79d9dedafe061fa1ed687d1d9ac4e453bfd
SDL2_net   2.4.0   https://github.com/libsdl-org/SDL_net/releases/download/release-2.4.0/SDL2_net-2.4.0.tar.gz 9cbca2527feb3f1a622d48ba65cc7dee9b1e3f2c55ceafb7d7720bb058aafb30
SDL2_gfx   1.0.4   https://downloads.sourceforge.net/project/sdl2gfx/SDL2_gfx-1.0.4.tar.gz 63e0e01addedc9df2f85b93a248f06e8a04affa014a835c2ea34bfe34e576262
libpng     1.6.50  https://github.com/pnggroup/libpng/archive/refs/tags/v1.6.50.tar.gz 71158e53cfdf2877bc99bcab33641d78df3f48e6e0daad030afe9cb8c031aa46
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

# SDL's Java side must match the native SDL version
JAVA="$HERE/app/src/main/java/org/libsdl/app"
rm -rf "$JAVA" && mkdir -p "$JAVA"
cp "$DEPS/SDL2/android-project/app/src/main/java/org/libsdl/app/"*.java "$JAVA/"

# Game data as assets, plus the list of files the game copies out of the
# APK on first start (see src/android_data.cpp). Hidden files are left out:
# the APK packager drops them.
ASSETS="$HERE/app/src/main/assets"
rm -rf "$ASSETS" && mkdir -p "$ASSETS"
(cd "$SRC" && find data -type f ! -name '.*' ! -name 'Makefile*' ! -name '*.desktop' \
   ! -name '*.in' ! -name '*.svg' ! -name '*.icns' ! -name '*.ico' ! -name '*.mbm' \
   ! -name '*.mif' ! -name '*.xpm' | LC_ALL=C sort) > "$ASSETS/filelist.txt"
(cd "$SRC" && tar -cf - -T "$ASSETS/filelist.txt") | tar -xf - -C "$ASSETS"
# A new stamp makes the game refresh its copy of the data after an update
(cd "$SRC" && { git rev-parse HEAD 2>/dev/null || date +%s; } ; sha256sum "$ASSETS/filelist.txt" | cut -c1-16) \
  | tr '\n' ' ' > "$ASSETS/data_stamp.txt"
echo "Staged $(wc -l < "$ASSETS/filelist.txt") data files"
