#!/bin/sh
# Random input everywhere: runs the game, built with AddressSanitizer and
# UndefinedBehaviorSanitizer, with
# WARMUX_TEST_MONKEY tapping, swiping, scrolling, touching and typing at
# random through the menus and into battles, a number of rounds with
# different seeds and window sizes (phone-shaped too). Fails on any memory
# error or crash, and prints how to replay it.
#
#   MONKEY_ROUNDS   rounds (default 6)
#   MONKEY_SECONDS  seconds per round (default 60)
#   MONKEY_SEED     first seed (default: from the date)
#
# Builds in _asantest/ (shared with NETTEST_SANITIZE=1 ci-network-test.sh).
set -eu

TOP=$(cd "$(dirname "$0")/.." && pwd)
NETTEST_SANITIZE=1 NETTEST_BUILD_ONLY=1 sh "$TOP/.github/ci-network-test.sh"
W=$TOP/_asantest
cd "$W"
SUFFIX=$(sed -n 's/^#define WARMUX_VERSION_SUFFIX "\(.*\)"$/\1/p' lib/warmux/include/WARMUX_net_defaults.h)

ROUNDS=${MONKEY_ROUNDS:-6}
SECONDS_PER_ROUND=${MONKEY_SECONDS:-60}
SEED=${MONKEY_SEED:-$(date +%Y%m%d%H)}
export ASAN_OPTIONS=detect_leaks=0:halt_on_error=1:handle_abort=1:print_summary=1
export UBSAN_OPTIONS=print_stacktrace=1

rm -rf monkey && mkdir monkey
failed=0
i=0
while [ $i -lt "$ROUNDS" ]; do
  seed=$((SEED + i))
  case $((i % 4)) in
    0) size=2400x1080 ;;  # a phone, sideways
    1) size=1920x1080 ;;
    2) size=1280x720 ;;
    3) size=1000x700 ;;
  esac
  H=$W/monkey/home_$seed
  mkdir -p "$H/config" "$H/data" "$H/cache"
  log=$W/monkey/$seed.log
  env HOME="$H" XDG_CONFIG_HOME="$H/config" XDG_DATA_HOME="$H/data" XDG_CACHE_HOME="$H/cache" \
      SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy \
      WARMUX_INDEX_SERVER=127.0.0.1:1 WARMUX_TEST_MONKEY=$seed \
      "$W/inst/bin/warmux" --size $size > "$log" 2>&1 &
  pid=$!
  end=$(( $(date +%s) + SECONDS_PER_ROUND ))
  while kill -0 $pid 2>/dev/null && [ "$(date +%s)" -lt $end ]; do sleep 1; done
  if kill -0 $pid 2>/dev/null; then
    { kill -9 $pid; wait $pid; } 2>/dev/null || true
    status=timeout
  else
    if wait $pid; then status=0; else status=$?; fi
  fi
  actions=$(grep -ac "^monkey: " "$log" || true)
  if grep -aq "ERROR: AddressSanitizer\|runtime error:" "$log" || { [ "$status" != timeout ] && [ "$status" != 0 ]; }; then
    failed=$((failed + 1))
    echo "FAILED: seed $seed, $size, after $actions actions (exit $status)" >&2
    if grep -aq "ERROR: AddressSanitizer\|runtime error:" "$log"; then
      grep -a -A40 "ERROR: AddressSanitizer\|runtime error:" "$log" | grep -av "^monkey: " | head -70 >&2
    else
      grep -av "^monkey: " "$log" | tail -n 30 >&2
    fi
    echo "  last actions:" >&2
    grep -a "^monkey: " "$log" | tail -n 25 | sed 's/^/    /' >&2
    echo "  replay: WARMUX_TEST_MONKEY=$seed warmux --size $size (AddressSanitizer build)" >&2
  else
    how=$([ "$status" = timeout ] && echo "ran ${SECONDS_PER_ROUND}s" || echo "quit by itself")
    echo "ok: seed $seed, $size, $actions actions, $how"
  fi
  i=$((i + 1))
done

if [ $failed -gt 0 ]; then
  echo "Monkey test FAILED: $failed of $ROUNDS rounds (logs in $W/monkey)" >&2
  exit 1
fi
echo "Monkey test passed ($ROUNDS rounds)"
