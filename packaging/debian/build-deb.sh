#!/bin/sh
# Build a .deb for Debian/Ubuntu from the source tree.
#   packaging/debian/build-deb.sh <version> [output-dir]
# Needs the build dependencies listed in README.md plus dpkg-dev.
# Library dependencies are computed from the binaries with dpkg-shlibdeps.
set -eu

VERSION=${1:?usage: build-deb.sh <version> [output-dir]}
OUT=$(realpath "${2:-.}")
SRC=$(cd "$(dirname "$0")/../.." && pwd)
ARCH=$(dpkg --print-architecture)
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# Build and stage
cp -a "$SRC" "$WORK/src"
cd "$WORK/src"
autoreconf -fi
mkdir -p _deb && cd _deb
../configure --prefix=/usr --enable-servers
make -j"$(nproc)"
PKG="$WORK/pkg"
make DESTDIR="$PKG" install
strip --strip-unneeded "$PKG"/usr/bin/* 2>/dev/null || true
install -Dm644 "$SRC/COPYING" "$PKG/usr/share/doc/warmux/copyright"
gzip -9n "$PKG"/usr/share/man/man6/warmux.6

# Work out shared-library dependencies (dpkg-shlibdeps wants debian/control)
mkdir -p "$WORK/shlib/debian"
printf 'Source: warmux\n\nPackage: warmux\nArchitecture: any\n' > "$WORK/shlib/debian/control"
LIBDEPS=$(cd "$WORK/shlib" && dpkg-shlibdeps -O "$PKG"/usr/bin/* 2>/dev/null \
          | sed -n 's/^shlibs:Depends=//p')

mkdir -p "$PKG/DEBIAN"
cat > "$PKG/DEBIAN/control" <<EOF
Package: warmux
Version: $VERSION
Architecture: $ARCH
Maintainer: KennyStier <https://github.com/KennyStier>
Installed-Size: $(du -sk "$PKG/usr" | cut -f1)
Depends: $LIBDEPS, hicolor-icon-theme
Section: games
Priority: optional
Homepage: https://github.com/KennyStier/warmux
Description: Turn-based artillery game with free-software mascots
 Teams of free-software mascots take turns blasting each other across
 destructible 2D maps with bazookas, grenades, baseball bats, ropes and
 plenty of other questionable equipment. Play hot-seat, against the AI or
 over the network. Includes the index and game servers.
EOF

mkdir -p "$OUT"
dpkg-deb --root-owner-group --build "$PKG" "$OUT/warmux_${VERSION}_${ARCH}.deb"
echo "Built $OUT/warmux_${VERSION}_${ARCH}.deb"
