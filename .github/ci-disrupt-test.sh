#!/bin/sh
# Network games where things go wrong, as they do on phones over Wi-Fi
# (see nettest-lib.sh for the build and its options; run it with
# NETTEST_SANITIZE=1). Each scenario has its own game server:
#
#  1. A player drops out mid-battle and comes back: the others play on,
#     and the returning player waits in a new lobby until told the battle
#     is over.
#  2. The host (game master) drops out mid-battle: nobody else crashes.
#  3. The game server goes away mid-battle: nobody crashes.
#  4. Lag and packet loss (NETTEST_NETEM=1; needs sudo and tc): a whole
#     game still plays through, in sync.
#
# Anything crashing, a memory error, undefined behaviour or a desync fails.
# NETTEST_SCENARIOS picks some (default "1 2 3 4"). Without sudo, set the
# lag up in a network namespace of your own (the servers won't run as root,
# hence the second, nested namespace):
#   unshare -rn sh -c 'ip link set lo up &&
#     tc qdisc add dev lo root netem delay 80ms 30ms loss 2% &&
#     unshare -U --map-user=1000 env NETTEST_NETEM=preset sh .github/ci-disrupt-test.sh'
set -eu
. "$(dirname "$0")/nettest-lib.sh"

# Bigger teams than the default test, so there is a battle to drop out of
WORMS=${NETTEST_WORMS:-2}
DEATH=${NETTEST_DEATH:-90}
TIMEOUT=${NETTEST_TIMEOUT:-900}
SCENARIOS=${NETTEST_SCENARIOS:-1 2 3 4}
want() { case " $SCENARIOS " in *" $1 "*) return 0;; esac; return 1; }
failed=0
fail() { echo "FAILED: $1" >&2; failed=$((failed + 1)); }

start_index

battle_started() {  # server-log players
  wait_for "$1" "started with $2 players" "$TIMEOUT"
}

# --- 1. A player drops out mid-battle, and comes back
if want 1; then
echo "== 1. A player drops out mid-battle and comes back"
start_game_server drop 3830 "Drop Test"
player a1 beastie 3830 1; A1=$LAST; sleep 3
player a2 gnu 3830 1; A2=$LAST; sleep 3
player a3 tux 3830 1; A3=$LAST
if battle_started "$RUN/drop.log" 3; then
  echo "ok: the battle started with 3 players"
  sleep 20
  stop $A3
  echo "   (a3 dropped out)"
  sleep 5
  player a3 tux 3830 1; A3=$LAST
  echo "   (a3 came back)"
  if wait_for "$RUN/drop.log" "finished with" "$TIMEOUT"; then
    echo "ok: the others played the battle to the end"
  else
    fail "the battle didn't end after a player dropped out"
  fi
  # The battle being on, the returning player waits in a lobby of its own,
  # and the server tells it when the battle is over
  if grep -aq "Drop Test-2.*New client connected: a3" "$RUN/drop.log"; then
    echo "ok: the returning player waited in a new lobby"
  else
    fail "the returning player didn't get a lobby"
  fi
  if wait_for "$RUN/drop.log" "battle over, telling the waiting players" 30; then
    echo "ok: the server told the waiting player the battle was over"
  else
    fail "the server didn't tell the waiting player the battle was over"
  fi
  alive "a1 still running" $A1 "$RUN/a1.log" || failed=$((failed + 1))
  alive "a2 still running" $A2 "$RUN/a2.log" || failed=$((failed + 1))
  alive "a3 still running" $A3 "$RUN/a3.log" || failed=$((failed + 1))
else
  fail "the battle with 3 players never started"
fi
asan_check "$RUN"/a*.log "$RUN/drop.log" || failed=$((failed + 1))
stop $A1 $A2 $A3
fi

# --- 2. The host drops out mid-battle
if want 2; then
echo "== 2. The host drops out mid-battle"
start_game_server host 3831 "Host Test"
player h1 beastie 3831 1; H1=$LAST; sleep 3
player h2 gnu 3831 1; H2=$LAST; sleep 3
player h3 tux 3831 1; H3=$LAST
if battle_started "$RUN/host.log" 3; then
  echo "ok: the battle started with 3 players"
  sleep 20
  stop $H1
  echo "   (h1, the host, dropped out)"
  sleep 45
  alive "h2 still running" $H2 "$RUN/h2.log" || failed=$((failed + 1))
  alive "h3 still running" $H3 "$RUN/h3.log" || failed=$((failed + 1))
  # Someone else takes over as host and the battle goes on
  if grep -aq "New game master" "$RUN/host.log" &&
     sed -n '/h1 just disconnected/,$p' "$RUN/h2.log" | grep -aq "Playing team"; then
    echo "ok: another player took over as host and the battle went on"
  else
    fail "the battle didn't go on without the host"
  fi
else
  fail "the battle with 3 players never started"
fi
asan_check "$RUN"/h*.log "$RUN/host.log" || failed=$((failed + 1))
stop $H1 $H2 $H3
fi

# --- 3. The game server goes away mid-battle
if want 3; then
echo "== 3. The game server goes away mid-battle"
start_game_server gone 3832 "Server Test"; GONE=$LAST
player s1 beastie 3832 1; S1=$LAST; sleep 3
player s2 gnu 3832 1; S2=$LAST
if battle_started "$RUN/gone.log" 2; then
  echo "ok: the battle started with 2 players"
  sleep 20
  stop $GONE
  echo "   (the game server went away)"
  sleep 20
  alive "s1 still running" $S1 "$RUN/s1.log" || failed=$((failed + 1))
  alive "s2 still running" $S2 "$RUN/s2.log" || failed=$((failed + 1))
else
  fail "the battle with 2 players never started"
fi
asan_check "$RUN"/s*.log || failed=$((failed + 1))
stop $S1 $S2
fi

# --- 4. Lag and packet loss
if want 4 && [ "${NETTEST_NETEM:-0}" != 0 ]; then
  echo "== 4. A whole game with lag and packet loss"
  if [ "$NETTEST_NETEM" = preset ]; then
    netem_off() { :; }  # set up (and undone) by the caller
  else
    sudo tc qdisc add dev lo root netem delay 80ms 30ms loss 2%
    netem_off() { sudo tc qdisc del dev lo root 2>/dev/null || true; }
    trap 'netem_off; cleanup' EXIT
  fi
  start_game_server lag 3833 "Lag Test"
  player l1 beastie 3833 1; L1=$LAST; sleep 3
  player l2 gnu 3833 1; L2=$LAST
  if battle_started "$RUN/lag.log" 2 && wait_for "$RUN/lag.log" "finished with" "$TIMEOUT"; then
    echo "ok: the game played through with lag and packet loss"
  else
    fail "the game didn't play through with lag and packet loss"
    tail -n 20 "$RUN/lag.log" "$RUN/l1.log" >&2
  fi
  asan_check "$RUN"/l*.log || failed=$((failed + 1))
  stop $L1 $L2
  netem_off
fi

if [ $failed -gt 0 ]; then
  echo "Disruption test FAILED ($failed problems; logs in $RUN)" >&2
  exit 1
fi
echo "Disruption test passed"
