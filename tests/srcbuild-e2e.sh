#!/bin/bash
# srcbuild-e2e.sh — THE SOURCE ROOM host compile, end to end (v1.26):
# the Linux binary under Xvfb with a sandbox HOME; the compiler must be
# found via ~/workspace/tools/zig-*/zig (TSG_ZIG unset). Stage every
# source file over HTTP with one visible edit, build, verify the
# served page changed and its wasm carries the edit; then stage a
# broken edit and verify the build fails and the last good page is
# still served. Run: bash tests/srcbuild-e2e.sh > tests/srcbuild-e2e.log 2>&1
set -u
cd "$(dirname "$0")/.."
ROOT="$PWD"
E2EHOME="$ROOT/tests/e2e-home"
BIN="$HOME/workspace/your_files/TheSurvivalGame-linux"
FAIL=0
say() { echo "[e2e] $*"; }
bad() { echo "[e2e] FAIL: $*"; FAIL=1; }

rm -rf "$E2EHOME"
mkdir -p "$E2EHOME/workspace"
ln -s "$HOME/workspace/tools" "$E2EHOME/workspace/tools"

Xvfb :99 -screen 0 1024x768x24 >/dev/null 2>&1 &
XVFB_PID=$!
sleep 1
env -u TSG_ZIG HOME="$E2EHOME" DISPLAY=:99 "$BIN" >/dev/null 2>&1 &
GAME_PID=$!

cleanup() {
  fuser -k 8379/tcp >/dev/null 2>&1
  kill $XVFB_PID >/dev/null 2>&1
}
trap cleanup EXIT

# wait for the server
UP=0
for i in $(seq 1 60); do
  if curl -s -o /dev/null "http://localhost:8379/"; then UP=1; break; fi
  sleep 1
done
[ "$UP" = 1 ] || { bad "server never came up"; exit 1; }
say "server up"

curl -s "http://localhost:8379/" -o /tmp/e2e-stock.html
SZ=$(stat -c%s /tmp/e2e-stock.html)
say "stock page: $SZ bytes"
grep -q "TSG_HOSTED" /tmp/e2e-stock.html && say "hosted flag present in stock page" || bad "no hosted flag in stock page"

# stage every source file; design.c carries the visible edit
python3 - "$ROOT" <<'PYEOF'
import sys
s = open(sys.argv[1] + '/src/data/design.c').read()
assert '"MECHANICAL KEYBOARD"' in s
open('/tmp/e2e-design.c', 'w').write(s.replace('"MECHANICAL KEYBOARD"', '"MECHANICAL KEYBOARD ZQ"', 1))
PYEOF
N=0
while IFS= read -r f; do
  rel="${f#$ROOT/src/}"
  if [ "$rel" = "data/design.c" ]; then SRC=/tmp/e2e-design.c; else SRC="$f"; fi
  R=$(curl -s -X POST --data-binary @"$SRC" "http://localhost:8379/src/put?path=$rel")
  [ "$R" = "OK" ] || bad "PUT $rel -> $R"
  N=$((N+1))
done < <(find "$ROOT/src" -name '*.c' | sort)
say "staged $N files"

curl -s -X POST "http://localhost:8379/src/build" >/dev/null
STATE=""
for i in $(seq 1 60); do
  STATE=$(curl -s "http://localhost:8379/src/status" | head -1)
  [ "$STATE" = "done-ok" ] && break
  [ "$STATE" = "done-failed" ] && break
  sleep 5
done
say "build state: $STATE"
[ "$STATE" = "done-ok" ] || bad "edited build did not succeed"

curl -s "http://localhost:8379/" -o /tmp/e2e-custom.html
if cmp -s /tmp/e2e-stock.html /tmp/e2e-custom.html; then bad "served page identical to stock"; else say "served page differs from stock"; fi
python3 - <<'PYEOF' || bad "wasm does not carry the edit"
import re, base64, gzip, sys
html = open('/tmp/e2e-custom.html', 'rb').read().decode('utf-8', 'replace')
runs = re.findall(r'[A-Za-z0-9+/=]{1000,}', html)
blob = base64.b64decode(max(runs, key=len))
wasm = gzip.decompress(blob)
assert b'MECHANICAL KEYBOARD ZQ' in wasm, 'edit string missing from wasm'
print('[e2e] decoded wasm carries MECHANICAL KEYBOARD ZQ (%d bytes)' % len(wasm))
PYEOF

# a broken edit must fail and leave the last good page served
printf '\nthis is not C at all {{{ ;;;\n' >> /tmp/e2e-design.c
R=$(curl -s -X POST --data-binary @/tmp/e2e-design.c "http://localhost:8379/src/put?path=data/design.c")
[ "$R" = "OK" ] || bad "PUT broken design.c -> $R"
curl -s -X POST "http://localhost:8379/src/build" >/dev/null
STATE=""
for i in $(seq 1 60); do
  STATE=$(curl -s "http://localhost:8379/src/status" | head -1)
  [ "$STATE" = "done-failed" ] && break
  [ "$STATE" = "done-ok" ] && break
  sleep 5
done
say "broken build state: $STATE"
[ "$STATE" = "done-failed" ] || bad "broken build was not reported failed"
curl -s "http://localhost:8379/" -o /tmp/e2e-after.html
if cmp -s /tmp/e2e-custom.html /tmp/e2e-after.html; then say "last good page still served after the failed build"; else bad "served page changed after a failed build"; fi

if [ "$FAIL" = 0 ]; then say "SRCBUILD E2E: PASS"; else say "SRCBUILD E2E: FAIL"; fi
exit $FAIL
