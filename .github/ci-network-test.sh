#!/bin/sh
# Plays a whole network game with nobody at the keyboard: builds the game
# and servers, starts a local index server and game server, and two
# computer-played clients (WARMUX_TEST_AUTOPLAY) with one worm each. Passes
# when the battle starts, is reported to the index server as in progress,
# and ends.
set -eu

TOP=$(cd "$(dirname "$0")/.." && pwd)
cd "$TOP"
[ -x configure ] || autoreconf -fi
mkdir -p _nettest
cd _nettest
W=$PWD
if [ ! -x inst/bin/warmux ]; then
  ../configure -q --prefix="$W/inst" --enable-servers
  make -s -j"$(nproc)" >/dev/null
  make -s install >/dev/null
fi
VERSION=$(sed -n "s/^PACKAGE_VERSION='\(.*\)'$/\1/p" config.log | head -1)
SUFFIX=$(sed -n 's/^#define WARMUX_VERSION_SUFFIX "\(.*\)"$/\1/p' lib/warmux/include/WARMUX_net_defaults.h)
echo "Testing version $VERSION$SUFFIX"

rm -rf run && mkdir -p run/idx run/gs
cat > run/idx.conf <<EOF
port=9997
my_hostname=127.0.0.1
connexion_max=-2
working_dir=$W/run/idx/
chroot=false
local=true
versions=$VERSION$SUFFIX
hidden_versions=svn
verbose=true
EOF
cat > run/gs.conf <<EOF
port=3826
connexion_max=-2
working_dir=$W/run/gs/
chroot=false
max_nb_clients=20
max_nb_games=5
game_name=Public Battle
password=
public=true
EOF

PIDS=""
cleanup() { [ -n "$PIDS" ] && kill -9 $PIDS 2>/dev/null || true; }
trap cleanup EXIT

(cd run/idx && exec "$W/inst/bin/warmux-index-server" -f "$W/run/idx.conf") > run/idx.log 2>&1 &
PIDS="$PIDS $!"
sleep 1
(cd run/gs && WARMUX_INDEX_SERVER=127.0.0.1 exec "$W/inst/bin/warmux-server" -f "$W/run/gs.conf") > run/gs.log 2>&1 &
PIDS="$PIDS $!"
sleep 2

for c in p1 p2; do
  H=$W/run/home_$c
  mkdir -p "$H/config/warmux" "$H/data" "$H/cache"
  team=beastie; [ $c = p2 ] && team=gnu
  # One worm each, so the battle is short
  printf '<?xml version="1.0" encoding="utf-8"?>\n<config><network><local_teams><team_0><id>%s</id><player_name>%s</player_name><nb_characters>1</nb_characters><ai>default</ai></team_0></local_teams></network></config>\n' \
    $team $c > "$H/config/warmux/config.xml"
  [ -n "$SUFFIX" ] && cp -r "$H/config/warmux" "$H/config/warmux$SUFFIX"
  env HOME="$H" XDG_CONFIG_HOME="$H/config" XDG_DATA_HOME="$H/data" XDG_CACHE_HOME="$H/cache" \
      SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \
      WARMUX_INDEX_SERVER=127.0.0.1 WARMUX_TEST_AUTOPLAY=1 \
      "$W/inst/bin/warmux" -c127.0.0.1 > "run/$c.log" 2>&1 &
  PIDS="$PIDS $!"
  sleep 3
done

check() {  # description file pattern
  if grep -aq "$3" "$2"; then echo "ok: $1"; return 0; fi
  return 1
}

started=no reported=no finished=no
deadline=$(( $(date +%s) + ${NETTEST_TIMEOUT:-600} ))
while [ "$(date +%s)" -lt "$deadline" ]; do
  [ $started = no ] && check "the battle started" run/gs.log "started with 2 players" && started=yes
  [ $reported = no ] && check "the index server lists it as in progress" run/idx.log "1 battles in progress" && reported=yes
  [ $finished = no ] && check "the battle ended" run/gs.log "finished with" && finished=yes
  if [ $started = yes ] && [ $reported = yes ] && [ $finished = yes ]; then
    echo "Network test passed"
    exit 0
  fi
  sleep 5
done

echo "Network test FAILED (started=$started reported=$reported finished=$finished)" >&2
for f in run/gs.log run/p1.log run/p2.log; do
  echo "=== $f" >&2
  tail -25 "$f" >&2
done
grep -a "battles in progress" run/idx.log | tail -5 >&2 || true
exit 1
