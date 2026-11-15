#!/bin/sh
# Builds the testing servers from a commit (default: the dev branch on
# GitHub) on the server and (re)starts them:
#   warmux-index-testing  TCP 9998  lists games of testing builds
#   warmux-game-testing   TCP 3900  "Testing Battle", built --enable-testing
#
#   tools/servers/deploy/deploy-testing.sh [git-ref] [ssh-host]
#
# Runs from a checkout of the repository; the host defaults to the
# warmux-vps alias in ~/.ssh/config. Stable servers are not touched.
# Testing games in progress are dropped when the servers restart.
set -eu

REF=${1:-origin/dev}
HOST=${2:-warmux-vps}
cd "$(git rev-parse --show-toplevel)"

git fetch -q origin
COMMIT=$(git rev-parse --verify "$REF^{commit}")
echo "Deploying $(git log -1 --format='%h %s' "$COMMIT") to $HOST"

git archive --prefix=warmux/ "$COMMIT" | ssh "$HOST" '
  set -e
  rm -rf /root/build-testing && mkdir -p /root/build-testing
  tar -x -C /root/build-testing'

ssh "$HOST" COMMIT="$COMMIT" sh -s <<'EOF'
set -eu
cd /root/build-testing/warmux
# The archive gives every file the same time; make the generated build
# files newer than their sources so make does not try to regenerate them
touch aclocal.m4; sleep 1
find . -name Makefile.in -exec touch {} +
touch configure lib/warmux/include/config_autotools.h.in

./configure -q --prefix=/opt/warmux/testing --enable-servers --enable-testing >/dev/null
make -s -j2 -C lib >/dev/null
make -s -j2 -C tools/servers >/dev/null

D=tools/servers/deploy/testing
install -d /opt/warmux/testing/bin /etc/warmux/testing
install -m755 tools/servers/index_server/warmux-index-server \
  tools/servers/game_server/warmux-server /opt/warmux/testing/bin/
install -m644 $D/index_server.conf $D/game_server.conf /etc/warmux/testing/
install -m644 $D/warmux-index-testing.service $D/warmux-game-testing.service /etc/systemd/system/
install -m644 $D/warmux-testing-tmpfiles.conf /etc/tmpfiles.d/warmux-testing.conf
echo "$COMMIT" > /opt/warmux/testing/COMMIT

ufw allow 9998/tcp >/dev/null
ufw allow 3900/tcp >/dev/null
systemctl daemon-reload
systemctl enable -q warmux-index-testing warmux-game-testing
systemctl restart warmux-index-testing
sleep 1
systemctl restart warmux-game-testing
sleep 3
systemctl is-active warmux-index-testing warmux-game-testing
journalctl -u warmux-game-testing -n 50 --no-pager -o cat | grep -m1 "game server version" || true
EOF
echo "Done: testing servers run $(git rev-parse --short "$COMMIT")"
