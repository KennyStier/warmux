#!/bin/sh
# Network games with nobody at the keyboard, against a local index server
# and game servers (see nettest-lib.sh for the build and its options):
#
#  1. The lobby: two players join a game and wait; the game server goes
#     away, so both leave the lobby. Passes if neither crashes (leaving a
#     lobby whose team list had several cards per row used to crash).
#  2. A whole game, one worm each on a small map by default. Passes when
#     the battle starts, is reported to the index server as in progress,
#     and ends.
set -eu
. "$(dirname "$0")/nettest-lib.sh"

start_index

# --- 1. Joining and leaving a lobby
start_game_server lobby 3828 "Lobby Test"; LOBBY=$LAST
player l1 tux 3828 lobby; L1=$LAST
sleep 3
player l2 wilber 3828 lobby; L2=$LAST
if ! wait_for "$RUN/lobby.log" "total: 2" 120; then
  echo "Network test FAILED: the players didn't both join the lobby" >&2
  tail -n 20 "$RUN/lobby.log" "$RUN/l1.log" "$RUN/l2.log" >&2
  exit 1
fi
echo "ok: two players in the lobby"
sleep 5  # let them settle in (teams, AI)
stop $LOBBY
sleep 10
lobby_ok=yes
alive "l1 left the lobby and is still running" $L1 "$RUN/l1.log" || lobby_ok=no
alive "l2 left the lobby and is still running" $L2 "$RUN/l2.log" || lobby_ok=no
asan_check "$RUN/l1.log" "$RUN/l2.log" || lobby_ok=no
stop $L1 $L2
if [ $lobby_ok = no ]; then
  echo "Network test FAILED (lobby)" >&2
  exit 1
fi

# --- 2. A whole game
start_game_server gs 3826 "Public Battle"
player p1 beastie 3826 1
sleep 3
player p2 gnu 3826 1

started=no reported=no finished=no
deadline=$(( $(date +%s) + ${NETTEST_TIMEOUT:-600} ))
while [ "$(date +%s)" -lt "$deadline" ]; do
  [ $started = no ] && check "the battle started" "$RUN/gs.log" "started with 2 players" && started=yes
  [ $reported = no ] && check "the index server lists it as in progress" "$RUN/idx.log" "1 battles in progress" && reported=yes
  [ $finished = no ] && check "the battle ended" "$RUN/gs.log" "finished with" && finished=yes
  if [ $started = yes ] && [ $reported = yes ] && [ $finished = yes ]; then
    sleep 5  # past the results screen
    if asan_check "$RUN"/*.log; then
      echo "Network test passed"
      exit 0
    fi
    echo "Network test FAILED (memory error)" >&2
    exit 1
  fi
  # A client that died stops the game: report it now
  if ! asan_check "$RUN/p1.log" "$RUN/p2.log" "$RUN/gs.log" 2>/dev/null; then
    asan_check "$RUN/p1.log" "$RUN/p2.log" "$RUN/gs.log" || true
    echo "Network test FAILED (memory error)" >&2
    exit 1
  fi
  sleep 5
done

echo "Network test FAILED (started=$started reported=$reported finished=$finished)" >&2
for f in "$RUN/gs.log" "$RUN/p1.log" "$RUN/p2.log"; do
  echo "=== $f" >&2
  tail -n 25 "$f" >&2
done
grep -a "battles in progress" "$RUN/idx.log" | tail -n 5 >&2 || true
exit 1
