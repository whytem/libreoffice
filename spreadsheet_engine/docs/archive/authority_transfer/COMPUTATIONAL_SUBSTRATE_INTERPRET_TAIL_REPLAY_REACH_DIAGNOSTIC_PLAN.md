# InterpretTail Replay Reach Diagnostic

Status: completed

## Goal

Measure why promoted evaluator families still do not appear on the full replay
corpus live surface even after the ambient residual-blocker cleanup.

## Starting Point

The migration started this slice with:

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=606`
- `interpret_tail_live_fallback_total=0`
- `interpret_tail_live_promoted_function_supported_total=0`
- promoted-family probe still healthy at `1812 / 1698 / 114`

That meant the ambient replay denominator was real, but promoted families still
did not reach it.

## Implemented Work

- added a bounded observe/shadow bridge on the formula-group path in
  `sc/source/core/data/formulacell.cxx`
- added a targeted shared-group `LOOKUP` observe/shadow proof in
  `sc/qa/unit/ucalc_formula2.cxx`
- extended the replay corpus runner to report a second full-corpus denominator:
  `interpret_tail_forced_interpret_*`

## Outcome

The slice closed with a useful negative result:

- the shared-group bridge works on targeted live proof
- the full replay corpus still records
  `interpret_tail_live_promoted_function_supported_total=0`
- even the full forced-interpret replay surface records
  `interpret_tail_forced_interpret_promoted_function_supported_total=0`

The measured forced-interpret denominator is:

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=303`
- `interpret_tail_forced_interpret_fallback_total=0`
- `interpret_tail_forced_interpret_seen_total=303`
- `interpret_tail_forced_interpret_unseen_formula_cells=50358`

Interpretation:

- the blocker is not just incidental access pattern
- the blocker is earlier in the live path, likely import-state, dirty-state, or
  pre-Interpret eligibility on the replay corpus

## Next Target

The next pass should focus on replay-corpus live eligibility, not more function
surface widening:

1. inventory which promoted-family replay cells never become live seen
2. compare probe addresses against live / forced-interpret seen addresses
3. identify whether the blocker is dirty-state, shared-group entry, or another
   pre-tail routing condition
4. only after that resume promoted-family ambient conversion work
