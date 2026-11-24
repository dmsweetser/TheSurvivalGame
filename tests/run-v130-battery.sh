#!/bin/bash
# v1.30 ship battery — canonical v1.29 list (wartest twice). Strictly
# sequential, one log per suite, tests/v130-logs/. Gate vs v1.29.
cd ~/workspace/survival-game/tests || exit 1
mkdir -p v130-logs
SUITES="arcadetest audioprobe banktest beanjettest bosstest bowlpad bowltest bstest buddybwloss buddycontinue buddycoop buddyloss buddyquit buddyquit2 budtest bwloss bwtest cartcoop carttest castletest comforttest craft2test craft3test craftsearchtest crafttest ctltest eddietest elitetest energytest entrancetest entrytest firewalltest firstshift gatetest hatbuddy hatsavetest hudtest huttest intensitytest kbmobiletest kbtest leaguetest loadtest mallfix maptest medaltest musicprobe musictest nametest oceantest pacingtest pagetest platformtest portraittest questtest regiontest revivetest scaletest shieldtest shuffletest skirmishpathtest slottest speedfloor speedtest splitbw splittest story2test story3test storytest switchtest tanktest tapbattle taptest tilltest tilltest2 toppertest touchaudit trophycasetest wallpooltest walltest warcoop warpadtest wartest wartest2 weathertest weeklytest whaletest woftest srctermtest livedatatest bwstarttest srctouchtest wartouchtest mordortest"
for s in $SUITES; do
  f="$s.js"
  [ "$s" = "wartest2" ] && f="wartest.js"
  echo "=== $s $(date +%H:%M:%S) ==="
  timeout 900 node "$f" > "v130-logs/$s.log" 2>&1
  echo "$s exit=$? last: $(tail -1 "v130-logs/$s.log" | head -c 120)"
done
echo "BATTERY DONE $(date +%H:%M:%S)"
