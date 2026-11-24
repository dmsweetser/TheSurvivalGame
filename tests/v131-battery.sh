#!/bin/bash
# v1.31 full battery — suites STRICTLY sequential, logs to v131-logs/.
cd ~/workspace/survival-game || exit 1
LOGDIR=tests/v131-logs
mkdir -p "$LOGDIR"
SUITES="arcadetest audioprobe banktest beanjettest bosstest bowlpad bowltest bstest buddybwloss buddycontinue buddycoop buddyloss buddyquit buddyquit2 budtest bwloss bwstarttest bwtest cartcoop carttest castletest comforttest craft2test craft3test craftsearchtest crafttest ctltest eddietest elitetest energytest entrancetest entrytest firewalltest firstshift gatetest hatbuddy hatsavetest hudtest huttest intensitytest kbmobiletest kbtest leaguetest livedatatest loadtest mallfix maptest medaltest mordortest musicprobe musictest nametest oceantest pacingtest pagetest platformtest portraittest questtest regiontest revivetest scaletest shieldtest shuffletest skirmishpathtest slottest speedfloor speedtest splitbw splittest srctermtest srctouchtest story2test story3test storytest switchtest tanktest tapbattle taptest tilltest tilltest2 toppertest touchaudit trophycasetest v131craftpad v131test wallpooltest walltest warcoop warpadtest wartest wartouchtest weathertest weeklytest whaletest woftest"
: > "$LOGDIR/battery-progress.txt"
for s in $SUITES; do
  echo "RUN $s $(date +%H:%M:%S)" >> "$LOGDIR/battery-progress.txt"
  node "tests/$s.js" > "$LOGDIR/$s.log" 2>&1
  echo "DONE $s exit=$? $(date +%H:%M:%S)" >> "$LOGDIR/battery-progress.txt"
done
# wartest second clean run (ship gate: clean TWICE)
echo "RUN wartest2 $(date +%H:%M:%S)" >> "$LOGDIR/battery-progress.txt"
node tests/wartest.js > "$LOGDIR/wartest2.log" 2>&1
echo "DONE wartest2 exit=$? $(date +%H:%M:%S)" >> "$LOGDIR/battery-progress.txt"
echo "BATTERY COMPLETE" >> "$LOGDIR/battery-progress.txt"
