#!/bin/sh
# Smoke test for the pattern loader: runs the command-line program on the bundled sample patterns.
# Usage: smoke_test.sh path/to/stitchly2d
set -eu
BIN=${1:?usage: smoke_test.sh path/to/stitchly2d}
DATA=$(dirname "$0")/data
fail() { echo "FAIL: $1" >&2; exit 1; }

out=$("$BIN" --cli "$DATA/patterns/male_shirt.sm2d" 2>/dev/null)
echo "$out" | grep -q "^17 piece(s)" || fail "shirt should have 17 pieces"
echo "$out" | grep -q "== FrontPanel ==" || fail "shirt should contain FrontPanel"
echo "$out" | grep -q "^Cast on" || fail "no instructions written"

# a visible grainline at 45 degrees must turn the first trouser piece
upright=$("$BIN" --cli "$DATA/patterns/trousers_gl.sm2d" 2>/dev/null | grep -A1 "== Front_no_pocket ==" | tail -1)
echo "$upright" | grep -q "no grainline" && fail "grainline of Front_no_pocket was not used"
echo "$upright" | grep -q "^256 rows" || fail "unexpected rotated size: $upright"

# the measurement file can be replaced from the command line, and errors are reported
"$BIN" --cli "$DATA/patterns/male_shirt.sm2d" -m "$DATA/measurements/individual/male_shirt.smis" >/dev/null 2>&1 || fail "--measurements"
if "$BIN" --cli "$DATA/patterns/missing.sm2d" >/dev/null 2>&1; then fail "missing file must fail"; fi
if "$BIN" --cli "$DATA/patterns/male_shirt.sm2d" -w 0 >/dev/null 2>&1; then fail "stitch width 0 must fail"; fi
echo "OK"
