# Shared by the network tests (ci-network-test.sh, ci-disrupt-test.sh):
# sourced, not run. Builds the game and servers if needed, then provides
# helpers to start a local index server, game servers and computer-played
# clients (WARMUX_TEST_AUTOPLAY), and to check their logs.
#
#   NETTEST_SANITIZE=1   build with AddressSanitizer and
#                        UndefinedBehaviorSanitizer (in _asantest/ instead
#                        of _nettest/); any report fails the test
#   NETTEST_BUILD_ONLY=1 only build
#   NETTEST_MAP, NETTEST_WORMS, NETTEST_DEATH
#                        the game: map, worms per team, seconds before death
#                        mode (which takes everyone down to 1 HP)

TOP=$(cd "$(dirname "$0")/.." && pwd)
cd "$TOP"
[ -x configure ] || autoreconf -fi
SANITIZE=${NETTEST_SANITIZE:-0}
if [ "$SANITIZE" = 1 ]; then
  BUILD=_asantest
  # AddressSanitizer and UndefinedBehaviorSanitizer. Not "vptr" (the game
  # is built without RTTI), "alignment" (SDL_net's own macros write
  # unaligned on purpose) nor "shift-base" (the fixed-point maths shifts
  # negative numbers left by design: defined since C++20, and the same on
  # every compiler; rewriting it risks desyncs between versions). Any other
  # undefined behaviour stops the program.
  SAN="-fsanitize=address,undefined -fno-sanitize=vptr,alignment,shift-base -fno-sanitize-recover=undefined"
  FLAGS="-g -O1 $SAN -fno-omit-frame-pointer"
  # The old code leaks a lot on exit; only look for memory errors
  export ASAN_OPTIONS=detect_leaks=0:halt_on_error=1:print_summary=1
  export UBSAN_OPTIONS=print_stacktrace=1
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
      CFLAGS="$FLAGS" CXXFLAGS="$FLAGS" LDFLAGS="$SAN"
  else
    ../configure -q --prefix="$W/inst" --enable-servers
  fi
  make -s -j"$(nproc)" >/dev/null
  make -s install >/dev/null
fi
[ "${NETTEST_BUILD_ONLY:-0}" = 1 ] && exit 0
VERSION=$(sed -n "s/^PACKAGE_VERSION='\(.*\)'$/\1/p" config.log | head -1)
SUFFIX=$(sed -n 's/^#define WARMUX_VERSION_SUFFIX "\(.*\)"$/\1/p' lib/warmux/include/WARMUX_net_defaults.h)
echo "Testing version $VERSION$SUFFIX$([ "$SANITIZE" = 1 ] && echo ", with sanitizers")"

# Everything a test writes goes in $RUN (logs: $RUN/*.log)
RUN=$W/${RUN_DIR:-run}
rm -rf "$RUN" && mkdir -p "$RUN"

MAP=${NETTEST_MAP:-pumpkins}
WORMS=${NETTEST_WORMS:-1}
DEATH=${NETTEST_DEATH:-60}

PIDS=""
cleanup() { [ -n "$PIDS" ] && kill -9 $PIDS 2>/dev/null || true; }
trap cleanup EXIT

# Stop processes without the shell reporting them as killed
stop() { { kill -9 "$@"; wait "$@"; } 2>/dev/null || true; }

# Any memory error, undefined behaviour or desync in these logs fails
asan_check() {
  bad=no
  for f in "$@"; do
    [ -f "$f" ] || continue
    if grep -aq "ERROR: AddressSanitizer\|runtime error:" "$f"; then
      echo "MEMORY ERROR or undefined behaviour in $f:" >&2
      grep -a -A30 "ERROR: AddressSanitizer\|runtime error:" "$f" | head -60 >&2
      bad=yes
    fi
    if grep -aq "out of sync" "$f"; then
      echo "DESYNC in $f:" >&2
      grep -a -B5 "out of sync" "$f" | head -20 >&2
      bad=yes
    fi
  done
  [ $bad = no ]
}

check() {  # description file pattern
  if grep -aq "$3" "$2"; then echo "ok: $1"; return 0; fi
  return 1
}

wait_for() {  # file pattern seconds [count]: until pattern is there (count times)
  n=0
  while [ $n -lt "$3" ]; do
    [ "$(grep -ac "$2" "$1" 2>/dev/null || true)" -ge "${4:-1}" ] && return 0
    sleep 1
    n=$((n + 1))
  done
  return 1
}

alive() {  # description pid log: passes if the process is still running
  if kill -0 "$2" 2>/dev/null; then
    echo "ok: $1"
    return 0
  fi
  echo "FAILED: $1: the process died" >&2
  tail -n 30 "$3" >&2
  return 1
}

start_index() {
  mkdir -p "$RUN/idx"
  cat > "$RUN/idx.conf" <<EOF
port=9997
my_hostname=127.0.0.1
connexion_max=-2
working_dir=$RUN/idx/
chroot=false
local=true
versions=$VERSION$SUFFIX
hidden_versions=svn
verbose=true
EOF
  (cd "$RUN/idx" && exec "$W/inst/bin/warmux-index-server" -f "$RUN/idx.conf") > "$RUN/idx.log" 2>&1 &
  PIDS="$PIDS $!"
  sleep 1
}

start_game_server() {  # name port "game name": log in $RUN/<name>.log, pid in LAST
  mkdir -p "$RUN/$1"
  cat > "$RUN/$1.conf" <<EOF
port=$2
connexion_max=-2
working_dir=$RUN/$1/
chroot=false
max_nb_clients=20
max_nb_games=5
game_name=$3
password=
public=true
EOF
  (cd "$RUN/$1" && WARMUX_INDEX_SERVER=127.0.0.1 exec "$W/inst/bin/warmux-server" -f "$RUN/$1.conf") > "$RUN/$1.log" 2>&1 &
  LAST=$!
  PIDS="$PIDS $LAST"
  sleep 2
}

player() {  # name team port autoplay: log in $RUN/<name>.log, pid in LAST
  H=$RUN/home_$1
  rm -rf "$H"
  mkdir -p "$H/config/warmux" "$H/data" "$H/cache"
  # A small map by default, and a game mode where death mode starts after
  # $DEATH seconds, so the battle ends soon: on a random map the computer
  # players can miss each other for longer than a test allows. The wide
  # window lays the team list out several cards per row, as on phones.
  printf '<?xml version="1.0" encoding="utf-8"?>\n<config><map>%s</map><game_mode>ci_quick</game_mode><network><as_client><host>127.0.0.1</host><port>%s</port></as_client><local_teams><team_0><id>%s</id><player_name>%s</player_name><nb_characters>%s</nb_characters><ai>default</ai></team_0></local_teams></network></config>\n' \
    "$MAP" "$3" "$2" "$1" "$WORMS" > "$H/config/warmux/config.xml"
  mkdir -p "$H/data/warmux/game_mode"
  sed -e "s|<duration_before_death_mode>[0-9]*<|<duration_before_death_mode>$DEATH<|" \
      -e 's|<damage_per_turn_during_death_mode>[0-9]*<|<damage_per_turn_during_death_mode>30<|' \
      "$W/inst/share/warmux/game_mode/classic.xml" > "$H/data/warmux/game_mode/ci_quick.xml"
  if [ -n "$SUFFIX" ]; then
    cp -r "$H/config/warmux" "$H/config/warmux$SUFFIX"
    cp -r "$H/data/warmux" "$H/data/warmux$SUFFIX"
  fi
  env HOME="$H" XDG_CONFIG_HOME="$H/config" XDG_DATA_HOME="$H/data" XDG_CACHE_HOME="$H/cache" \
      SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \
      WARMUX_INDEX_SERVER=127.0.0.1 WARMUX_TEST_AUTOPLAY="$4" \
      "$W/inst/bin/warmux" --size 1920x1080 -c127.0.0.1 > "$RUN/$1.log" 2>&1 &
  LAST=$!
  PIDS="$PIDS $LAST"
}
