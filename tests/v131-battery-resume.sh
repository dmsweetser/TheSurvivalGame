#!/bin/bash
# v1.31 battery RESUME after runtime restart (first segment: 84 suites done,
# trophycasetest last green; v131craftpad was killed at start, 0-byte log).
cd ~/workspace/survival-game || exit 1
LOGDIR=tests/v131-logs
REMAIN="v131craftpad v131test wallpooltest walltest warcoop warpadtest wartest wartouchtest weathertest weeklytest whaletest woftest"
for s in $REMAIN; do
  echo "RUN $s $(date +%H:%M:%S)" >> "$LOGDIR/battery-progress.txt"
  node "tests/$s.js" > "$LOGDIR/$s.log" 2>&1
  echo "DONE $s exit=$? $(date +%H:%M:%S)" >> "$LOGDIR/battery-progress.txt"
done
echo "RUN wartest2 $(date +%H:%M:%S)" >> "$LOGDIR/battery-progress.txt"
node tests/wartest.js > "$LOGDIR/wartest2.log" 2>&1
echo "DONE wartest2 exit=$? $(date +%H:%M:%S)" >> "$LOGDIR/battery-progress.txt"
# Solo re-runs of the five battery reds (flake classification, from the logs)
for s in bwtest entrancetest livedatatest mordortest scaletest; do
  echo "RUN $s-solo $(date +%H:%M:%S)" >> "$LOGDIR/battery-progress.txt"
  node "tests/$s.js" > "$LOGDIR/$s-solo.log" 2>&1
  echo "DONE $s-solo exit=$? $(date +%H:%M:%S)" >> "$LOGDIR/battery-progress.txt"
done
echo "BATTERY COMPLETE" >> "$LOGDIR/battery-progress.txt"
