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
# a project: its settings, bands and patterns are used (rib hem, then stripes), -w overrides the stitch width
proj=$("$BIN" --cli "$DATA/cuff.stitchly" --piece Cuff 2>/dev/null)
echo "$proj" | grep -q "^Project:" || fail "project header missing"
echo "$proj" | grep -q "and work in 2:2 rib" || fail "project band (2:2 rib) not used"
echo "$proj" | grep -q "A x" || fail "project color pattern not used"
echo "$proj" | grep -q "== FrontPanel ==" && fail "--piece must limit the output to one piece"
echo "$proj" | grep -q "default stitch 4 x row 3 mm" || fail "project stitch size not used"
over=$("$BIN" --cli "$DATA/cuff.stitchly" --piece Cuff -w 5 2>/dev/null)
echo "$over" | grep -q "default stitch 5 x row 3 mm" || fail "-w must override the project's stitch width"
if "$BIN" --cli "$DATA/cuff.stitchly" --piece Nope >/dev/null 2>&1; then fail "unknown --piece must fail"; fi
if "$BIN" --cli "$DATA/missing.stitchly" >/dev/null 2>&1; then fail "missing project must fail"; fi
# shaping on outside rows (default) or on every row; an unknown value is refused
every=$("$BIN" --cli "$DATA/patterns/male_shirt.sm2d" --piece FrontPanel --shape-on every 2>/dev/null)
outside=$("$BIN" --cli "$DATA/patterns/male_shirt.sm2d" --piece FrontPanel 2>/dev/null)
echo "$outside" | grep -q "Row 1 is an outside row" || fail "default must shape on outside rows"
echo "$every" | grep -q "outside row" && fail "--shape-on every must not mention outside rows"
[ "$every" != "$outside" ] || fail "--shape-on must change the instructions"
if "$BIN" --cli "$DATA/patterns/male_shirt.sm2d" --shape-on sideways >/dev/null 2>&1; then fail "bad --shape-on must fail"; fi
echo "OK"
