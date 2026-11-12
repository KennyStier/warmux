#!/bin/sh
# Cross-compile a portable Windows (x86_64) build of Warmux on Fedora.
#   packaging/windows/build-windows.sh <version> [output-dir]
#
# Needs Fedora with the MinGW toolchain and libraries:
#   dnf install mingw64-gcc-c++ mingw64-sdl2-compat mingw64-SDL2_image \
#     mingw64-SDL2_mixer mingw64-SDL2_ttf mingw64-curl mingw64-libxml2 \
#     mingw64-libpng mingw64-gettext mingw64-zlib autoconf automake \
#     gettext-devel make git zip curl findutils mingw32-nsis
# SDL2_net and SDL2_gfx are not packaged by Fedora and are built here.
#
# Produces:
#   warmux-<version>-windows-x64-setup.exe  installer (packaging/windows/warmux.nsi)
#   warmux-<version>-windows-x64.zip        portable: warmux.exe, DLLs, data/
#                                           and locale/, runnable from any folder
set -eu

VERSION=${1:?usage: build-windows.sh <version> [output-dir]}
OUT=$(realpath "${2:-.}")
SRC=$(cd "$(dirname "$0")/../.." && pwd)
HOST=x86_64-w64-mingw32
SYSROOT=/usr/$HOST/sys-root/mingw
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
JOBS=$(nproc)

# --- Libraries Fedora doesn't package -----------------------------------
fetch() {  # url sha256 [mirror]
  f="$WORK/$(basename "$1")"
  curl -fsSL -o "$f" "$1" || { [ -n "${3:-}" ] && curl -fsSL -o "$f" "$3"; }
  echo "$2  $f" | sha256sum -c --quiet
  tar -xzf "$f" -C "$WORK"
}
build_lib() {  # dir configure-args...
  d=$1; shift
  (cd "$WORK/$d" && mingw64-configure --disable-static "$@" >/dev/null \
     && make -j"$JOBS" >/dev/null && make install >/dev/null)
}

if [ ! -e "$SYSROOT/lib/pkgconfig/SDL2_net.pc" ]; then
  fetch https://github.com/libsdl-org/SDL_net/releases/download/release-2.4.0/SDL2_net-2.4.0.tar.gz \
        9cbca2527feb3f1a622d48ba65cc7dee9b1e3f2c55ceafb7d7720bb058aafb30
  build_lib SDL2_net-2.4.0
fi
if [ ! -e "$SYSROOT/lib/pkgconfig/SDL2_gfx.pc" ]; then
  fetch https://www.ferzkopp.net/Software/SDL2_gfx/SDL2_gfx-1.0.4.tar.gz \
        63e0e01addedc9df2f85b93a248f06e8a04affa014a835c2ea34bfe34e576262 \
        https://downloads.sourceforge.net/project/sdl2gfx/SDL2_gfx-1.0.4.tar.gz
  build_lib SDL2_gfx-1.0.4 --disable-mmx
fi

# --- Warmux ---------------------------------------------------------------
cp -a "$SRC" "$WORK/src"
cd "$WORK/src"
git clean -dfxq 2>/dev/null || true
autoreconf -fi
mkdir -p _win && cd _win
mingw64-configure --disable-servers
make -j"$JOBS"

STAGE="$WORK/warmux-$VERSION-windows-x64"
mkdir -p "$STAGE"
make install program_transform_name= bindir="$STAGE" pkgdatadir="$STAGE/data" \
     localedir="$STAGE/locale" datadir="$WORK/unused" mandir="$WORK/unused"
$HOST-strip "$STAGE/warmux.exe"

# --- DLLs: follow the import tables, copy everything found in the sysroot --
copy_deps() {
  for dll in $($HOST-objdump -p "$1" | sed -n 's/^\s*DLL Name: //p'); do
    [ -e "$STAGE/$dll" ] && continue
    src=$(find "$SYSROOT/bin" -maxdepth 1 -iname "$dll" | head -1)
    [ -n "$src" ] || continue          # Windows system DLL
    cp "$src" "$STAGE/"
    copy_deps "$STAGE/$(basename "$src")"
  done
}
copy_deps "$STAGE/warmux.exe"
# Loaded at runtime, not linked: SDL3 (sdl2-compat is a wrapper around it)
# and codec libraries for SDL2_mixer / SDL2_image
for dll in "$SYSROOT"/bin/SDL3.dll \
           "$SYSROOT"/bin/libogg*.dll "$SYSROOT"/bin/libvorbis*.dll \
           "$SYSROOT"/bin/libjpeg*.dll "$SYSROOT"/bin/libwebp*.dll; do
  [ -e "$dll" ] && [ ! -e "$STAGE/$(basename "$dll")" ] && cp "$dll" "$STAGE/" \
    && copy_deps "$STAGE/$(basename "$dll")"
done

$HOST-strip --strip-unneeded "$STAGE"/*.dll

cp "$SRC/COPYING" "$STAGE/COPYING.txt"
cat > "$STAGE/README.txt" <<EOF
Warmux $VERSION for Windows (64-bit)
https://github.com/KennyStier/warmux

Run warmux.exe. The folder can live anywhere; settings and saved teams
are kept in your user profile.
EOF

mkdir -p "$OUT"
(cd "$WORK" && zip -qr9 "$OUT/warmux-$VERSION-windows-x64.zip" "$(basename "$STAGE")")
echo "Built $OUT/warmux-$VERSION-windows-x64.zip"

# --- Installer -------------------------------------------------------------
# Uninstall list: every staged file, then folders deepest first (RMDir only
# removes them once empty)
UNLIST="$WORK/uninstall.nsh"
(cd "$STAGE"
 find . -type f | sed 's|^\./||; s|/|\\|g; s|.*|  Delete "$INSTDIR\\&"|'
 find . -mindepth 1 -type d | awk '{ print length, $0 }' | sort -rn | cut -d' ' -f2- \
   | sed 's|^\./||; s|/|\\|g; s|.*|  RMDir "$INSTDIR\\&"|') > "$UNLIST"
# NSIS needs a numeric x.y.z version
NSIS_VERSION=$(echo "$VERSION" | sed -n 's/^\([0-9]*\.[0-9]*\.[0-9]*\).*/\1/p')
makensis -V2 -DVERSION="${NSIS_VERSION:-0.0.0}" -DSTAGE="$STAGE" \
  -DUNINSTALL_LIST="$UNLIST" \
  -DOUTFILE="$OUT/warmux-$VERSION-windows-x64-setup.exe" \
  "$SRC/packaging/windows/warmux.nsi"
echo "Built $OUT/warmux-$VERSION-windows-x64-setup.exe"
