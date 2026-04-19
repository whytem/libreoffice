#!/bin/bash
# Run the sc_ucalc_formula2 cppunit test and verify the failure set matches
# the documented known-regressions baseline in
# spreadsheet_engine/docs/architecture/CALC_TEST_KNOWN_REGRESSIONS.md.
#
# Exit codes:
#   0 - failure set matches the known list (no NEW regressions slipped in,
#       and no UNEXPECTED green tests we forgot to remove from the list)
#   1 - new regression introduced (failing test not in the known list)
#   2 - test in the known list now passes; remove it from the list
#   3 - test infrastructure error (build failed, etc.)

set -u

REPO_ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
KNOWN_LIST="$REPO_ROOT/spreadsheet_engine/docs/architecture/CALC_TEST_KNOWN_REGRESSIONS.md"
TEST_LOG="$REPO_ROOT/workdir/CppunitTest/sc_ucalc_formula2.test.log"

cd "$REPO_ROOT"

echo "Running CppunitTest_sc_ucalc_formula2..."
make CppunitTest_sc_ucalc_formula2 >/dev/null 2>&1
# We expect non-zero exit because some tests fail; that's the whole point.

if [[ ! -f "$TEST_LOG" ]]; then
    echo "ERROR: test log not produced" >&2
    exit 3
fi

# Extract the actually-failing test names from the log.
ACTUAL=$(grep -B1 "assertion failed\|equality assertion" "$TEST_LOG" \
    | grep "^Test name:" \
    | LC_ALL=C sort -u \
    | sed 's/Test name: //; s/::TestBody$//')

# Extract the documented known-regression names (lines that look like
# `- \`testFooBar\``) from the doc.
KNOWN=$(grep -oE "^- \`test[A-Za-z0-9_]+\`" "$KNOWN_LIST" \
    | sed 's/^- `//; s/`$//' \
    | LC_ALL=C sort -u)

# Diff the two sets.
NEW=$(comm -23 <(printf '%s\n' "$ACTUAL" | LC_ALL=C sort -u) <(printf '%s\n' "$KNOWN" | LC_ALL=C sort -u))
FIXED=$(comm -13 <(printf '%s\n' "$ACTUAL" | LC_ALL=C sort -u) <(printf '%s\n' "$KNOWN" | LC_ALL=C sort -u))

EXIT=0

if [[ -n "$NEW" ]]; then
    echo ""
    echo "ERROR: NEW regressions detected (not in known-regressions list):"
    echo "$NEW" | sed 's/^/  - /'
    EXIT=1
fi

if [[ -n "$FIXED" ]]; then
    echo ""
    echo "GOOD NEWS: tests in the known-regressions list now PASS."
    echo "Remove them from $KNOWN_LIST:"
    echo "$FIXED" | sed 's/^/  - /'
    if [[ $EXIT -eq 0 ]]; then
        EXIT=2
    fi
fi

if [[ $EXIT -eq 0 ]]; then
    echo ""
    echo "OK: failure set matches the known-regressions baseline ($(echo "$ACTUAL" | wc -l) failures)."
fi

exit $EXIT
