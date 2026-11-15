#!/bin/sh
# Network games with nobody at the keyboard. Builds the game and servers,
# starts a local index server and game servers, and runs computer-played
# clients (WARMUX_TEST_AUTOPLAY):
#
#  1. The lobby: two players join a game and wait; the game server goes
#     away, so both leave the lobby. Passes if neither crashes. The window
#     is wide, so the team list lays out several cards per row, as on
#     phones (leaving such a lobby used to crash).
#  2. A whole game, one worm each. Passes when the battle starts, is
#     reported to the index server as in progress, and ends.
#
# NETTEST_SANITIZE=1 builds everything with AddressSanitizer (in
# _asantest/ instead of _nettest/), and any memory error it reports fails
# the test.
set -eu

TOP=$(cd "$(dirname "$0")/.." && pwd)
cd "$TOP"
[ -x configure ] || autoreconf -fi
SANITIZE=${NETTEST_SANITIZE:-0}
if [ "$SANITIZE" = 1 ]; then
  BUILD=_asantest
  FLAGS="-g -O1 -fsanitize=address -fno-omit-frame-pointer"
  # The old code leaks a lot on exit; only look for memory errors
  export ASAN_OPTIONS=detect_leaks=0:halt_on_error=1:print_summary=1
else
  BUILD=_nettest
  FLAGS=""
fi
mkdir -p $BUILD
cd $BUILD
W=$PWD
if [ ! -x inst/bin/warmux ]; then
  if [ -n "$FLAGS" ]; then
    ../configure -q --prefix="$W/inst" --enable-servers \
      CFLAGS="$FLAGS" CXXFLAGS="$FLAGS" LDFLAGS="-fsanitize=address"
  else
    ../configure -q --prefix="$W/inst" --enable-servers
  fi
  make -s -j"$(nproc)" >/dev/null
  make -s install >/dev/null
fi
VERSION=$(sed -n "s/^PACKAGE_VERSION='\(.*\)'$/\1/p" config.log | head -1)
SUFFIX=$(sed -n 's/^#define WARMUX_VERSION_SUFFIX "\(.*\)"$/\1/p' lib/warmux/include/WARMUX_net_defaults.h)
echo "Testing version $VERSION$SUFFIX$([ "$SANITIZE" = 1 ] && echo ", with AddressSanitizer")"

rm -rf run && mkdir -p run/idx run/gs run/lobby
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
game_server_conf() {  # port name dir
  cat <<EOF
port=$1
connexion_max=-2
working_dir=$3/
chroot=false
max_nb_clients=20
max_nb_games=5
game_name=$2
password=
public=true
EOF
}
game_server_conf 3826 "Public Battle" "$W/run/gs" > run/gs.conf
game_server_conf 3828 "Lobby Test" "$W/run/lobby" > run/lobby.conf

PIDS=""
cleanup() { [ -n "$PIDS" ] && kill -9 $PIDS 2>/dev/null || true; }
trap cleanup EXIT

# Any memory error AddressSanitizer reported in these logs fails the test
asan_check() {
  bad=no
  for f in "$@"; do
    if grep -aq "ERROR: AddressSanitizer" "$f"; then
      echo "MEMORY ERROR in $f:" >&2
      sed -n '/ERROR: AddressSanitizer/,/SUMMARY: AddressSanitizer/p' "$f" | head -60 >&2
      bad=yes
    fi
  done
  [ $bad = no ]
}

player() {  # name team port autoplay log
  H=$W/run/home_$1
  rm -rf "$H"
  mkdir -p "$H/config/warmux" "$H/data" "$H/cache"
  # One worm each on a small map, and a game mode where death mode (which
  # takes everyone down to 1 HP) starts after a minute, so the battle ends
  # soon. On a random map the computer players can miss each other for
  # longer than the test allows.
  printf '<?xml version="1.0" encoding="utf-8"?>\n<config><map>pumpkins</map><game_mode>ci_quick</game_mode><network><as_client><host>127.0.0.1</host><port>%s</port></as_client><local_teams><team_0><id>%s</id><player_name>%s</player_name><nb_characters>1</nb_characters><ai>default</ai></team_0></local_teams></network></config>\n' \
    "$3" "$2" "$1" > "$H/config/warmux/config.xml"
  mkdir -p "$H/data/warmux/game_mode"
  sed -e 's|<duration_before_death_mode>[0-9]*<|<duration_before_death_mode>60<|' \
      -e 's|<damage_per_turn_during_death_mode>[0-9]*<|<damage_per_turn_during_death_mode>30<|' \
      "$W/inst/share/warmux/game_mode/classic.xml" > "$H/data/warmux/game_mode/ci_quick.xml"
  if [ -n "$SUFFIX" ]; then
    cp -r "$H/config/warmux" "$H/config/warmux$SUFFIX"
    cp -r "$H/data/warmux" "$H/data/warmux$SUFFIX"
  fi
  env HOME="$H" XDG_CONFIG_HOME="$H/config" XDG_DATA_HOME="$H/data" XDG_CACHE_HOME="$H/cache" \
      SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \
      WARMUX_INDEX_SERVER=127.0.0.1 WARMUX_TEST_AUTOPLAY="$4" \
      "$W/inst/bin/warmux" --size 1920x1080 -c127.0.0.1 > "$5" 2>&1 &
  LAST=$!
  PIDS="$PIDS $LAST"
}

check() {  # description file pattern
  if grep -aq "$3" "$2"; then echo "ok: $1"; return 0; fi
  return 1
}

(cd run/idx && exec "$W/inst/bin/warmux-index-server" -f "$W/run/idx.conf") > run/idx.log 2>&1 &
PIDS="$PIDS $!"
sleep 1

# --- 1. Joining and leaving a lobby
(cd run/lobby && WARMUX_INDEX_SERVER=127.0.0.1 exec "$W/inst/bin/warmux-server" -f "$W/run/lobby.conf") > run/lobby.log 2>&1 &
LOBBY=$!
PIDS="$PIDS $LOBBY"
sleep 2
player l1 tux 3828 lobby run/l1.log; L1=$LAST
sleep 3
player l2 wilber 3828 lobby run/l2.log; L2=$LAST
for i in $(seq 1 120); do grep -aq "total: 2" run/lobby.log && break; sleep 1; done
if ! check "two players in the lobby" run/lobby.log "total: 2"; then
  echo "Network test FAILED: the players didn't both join the lobby" >&2
  tail -20 run/lobby.log run/l1.log run/l2.log >&2
  exit 1
fi
sleep 5  # let them settle in (teams, AI)
{ kill -9 $LOBBY; wait $LOBBY; } 2>/dev/null || true
sleep 10
lobby_ok=yes
for p in "l1 $L1" "l2 $L2"; do
  set -- $p
  if kill -0 "$2" 2>/dev/null; then
    echo "ok: $1 left the lobby and is still running"
  else
    echo "FAILED: $1 crashed leaving the lobby" >&2
    tail -30 "run/$1.log" >&2
    lobby_ok=no
  fi
done
asan_check run/l1.log run/l2.log || lobby_ok=no
{ kill -9 $L1 $L2; wait $L1 $L2; } 2>/dev/null || true
if [ $lobby_ok = no ]; then
  echo "Network test FAILED (lobby)" >&2
  exit 1
fi

# --- 2. A whole game
(cd run/gs && WARMUX_INDEX_SERVER=127.0.0.1 exec "$W/inst/bin/warmux-server" -f "$W/run/gs.conf") > run/gs.log 2>&1 &
PIDS="$PIDS $!"
sleep 2
player p1 beastie 3826 1 run/p1.log
sleep 3
player p2 gnu 3826 1 run/p2.log

started=no reported=no finished=no
deadline=$(( $(date +%s) + ${NETTEST_TIMEOUT:-600} ))
while [ "$(date +%s)" -lt "$deadline" ]; do
  [ $started = no ] && check "the battle started" run/gs.log "started with 2 players" && started=yes
  [ $reported = no ] && check "the index server lists it as in progress" run/idx.log "1 battles in progress" && reported=yes
  [ $finished = no ] && check "the battle ended" run/gs.log "finished with" && finished=yes
  if [ $started = yes ] && [ $reported = yes ] && [ $finished = yes ]; then
    sleep 5  # past the results screen
    if asan_check run/p1.log run/p2.log run/gs.log run/lobby.log run/idx.log; then
      echo "Network test passed"
      exit 0
    fi
    echo "Network test FAILED (memory error)" >&2
    exit 1
  fi
  # A client that died stops the game: report it now
  if ! asan_check run/p1.log run/p2.log run/gs.log 2>/dev/null; then
    asan_check run/p1.log run/p2.log run/gs.log || true
    echo "Network test FAILED (memory error)" >&2
    exit 1
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
