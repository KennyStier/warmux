#!/bin/sh
# Native build used by CI on every distro: regenerate autotools, build out
# of tree with the index/game servers, install into a staging directory.
set -eu

autoreconf -fi
mkdir -p _build
cd _build
../configure --prefix=/usr --enable-servers
make -j"$(nproc)"
make DESTDIR="$PWD/stage" install

# The game and all three tools must have been installed
for bin in warmux warmux-server warmux-index-server warmux-list-games; do
  test -x "stage/usr/bin/$bin" || { echo "missing $bin" >&2; exit 1; }
done
echo "Build OK"
