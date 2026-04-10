# Spreadsheet Engine: Project Status

This file is the concise current-state snapshot for `spreadsheet_engine/`.

Start here for the active migration story:

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)

## Objective

The long-term goal remains to make `spreadsheet_engine/` the home for Calc's
spreadsheet computation engine while Calc remains the document and
application host.

That splits into two tracks:

- shared-engine extraction: materially achieved
- live evaluation authority transfer inside Calc: in progress

## Live Evaluator Dashboard

### Replay Guardrail

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

### Full Replay Corpus: Ambient Live Observe

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=4158`
- `interpret_tail_live_fallback_total=68`
- `interpret_tail_live_seen_total=4226`
- `interpret_tail_live_unseen_formula_cells=46435`
- `interpret_tail_live_promoted_function_supported_total=2908`
- `interpret_tail_live_supported_rate=8.21`
- `interpret_tail_live_seen_rate=8.34`

Ambient live fallback reasons:

- `unsupported_formula_shape=46`
- `unsupported_host_surface=22`
- `parse_failure=0`

### Full Replay Corpus: Forced Interpret Observe

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=2079`
- `interpret_tail_forced_interpret_fallback_total=34`
- `interpret_tail_forced_interpret_seen_total=2113`
- `interpret_tail_forced_interpret_unseen_formula_cells=48548`
- `interpret_tail_forced_interpret_promoted_function_supported_total=1454`
- `interpret_tail_forced_interpret_supported_rate=4.10`
- `interpret_tail_forced_interpret_seen_rate=4.17`

### Promoted-Family Probe

- `interpret_tail_probe_formula_cells=1812`
- `interpret_tail_authoritative_total=1708`
- `interpret_tail_authoritative_fallback_total=104`
- promoted-family authoritative rate: `94.26%`

Promoted-family fallback reasons:

- `unsupported_formula_shape=23`
- `shadow_mismatch=70`
- `unsupported_host_surface=11`

### Promoted Replay Eligibility Inventory

- `interpret_tail_replay_promoted_formula_cells=1812`
- `interpret_tail_replay_promoted_direct_seen=1812`
- `interpret_tail_replay_promoted_direct_supported=1778`
- `interpret_tail_replay_promoted_direct_fallback=34`
- `interpret_tail_replay_promoted_direct_unseen=0`
- `interpret_tail_replay_promoted_shared_formula_cells=395`
- `interpret_tail_replay_promoted_non_shared_formula_cells=1417`
- `interpret_tail_replay_promoted_unseen_shared_member=0`
- `interpret_tail_replay_promoted_unseen_non_shared=0`
- `interpret_tail_replay_promoted_shared_member_seen_via_top=0`
- `interpret_tail_replay_promoted_needs_interpret_after_dirty=1812`
- `interpret_tail_replay_promoted_dirty_after_interpret=0`

### Current Hard-Routed Family Count

- `1` env-independent engine-first family:
  - literal-only `NUMBERVALUE`

## Current State

Today:

- the shared compiler, token model, workbook model, FODS loader, evaluator,
  dependency snapshot, invalidation planning, and recalc planning are
  engine-owned
- the `InterpretTail` seam is real production code, not a test-only oracle
- `DBG_UTIL` builds default to `observe` when the rollout env var is unset
- `authority` mode authoritatively bypasses `ScInterpreter` for supported
  promoted families
- the full replay corpus now has a true all-formula live-routing denominator
- replay-imported promoted formulas now reach the seam broadly, and bounded
  top-level `INDEX` / `XLOOKUP` slice results now stay inside it
- the latest `LOOKUP` semantics pass tightened scalar-text and range-backed
  behavior, but did not move the replay counters yet

Still not true:

- no broad default-on rollout exists
- no `ScInterpreter` subroutine has been deleted yet
- full replay-corpus live traffic is now material, but most supported ambient
  traffic still covers only a bounded minority of formulas
- the replay-promoted reach blocker is cleared, and the latest slice reduced
  replay-imported promoted fallback further
- the dominant retained promoted-family blocker is now `shadow_mismatch`,
  with residual `VLOOKUP` / `INDEX` host-surface fallout secondary
- the next highest-value replay blocker is still the bounded `LOOKUP` parity
  band, especially formula-backed `2D` result rows and remaining
  `OUT OF BOUND` range-backed rows

## Active Delegated Family

The live delegated evaluator family currently includes:

- `TRUE`
- `FALSE`
- `VALUE`
- `DATEVALUE`
- `TIMEVALUE`
- `NUMBERVALUE`
- `MATCH`
- `XMATCH`
- `LOOKUP`
- `VLOOKUP`
- `HLOOKUP`
- `XLOOKUP`
- `INDEX`
- bounded `IFERROR(...)` / `IFNA(...)` wrappers around promoted roots

## Scope Policy

The active roadmap is evaluator migration.

Further computational-substrate widening is out of scope unless it directly:

- removes an `InterpretTail` fallback reason
- removes an `InterpretTail` mismatch class
- unlocks required host access for a promoted evaluator family

Residual substrate frontier items that do not satisfy one of those bars are
historical reference material, not active roadmap.

## Recommended Next Pass

The next pass should stay on promoted-family parity cleanup:

1. convert the formula-backed `2D` `LOOKUP` replay rows that still return
   `error:0` instead of the live text result
2. convert the remaining range-backed short-result `LOOKUP` rows that still
   miss Calc's `OUT OF BOUND`-style behavior
3. then convert residual `XLOOKUP` and `VLOOKUP` mismatch rows
4. clean up the remaining replay-imported `VLOOKUP` / `INDEX`
   `unsupported_host_surface` and `unsupported_formula_shape` fallout
5. only return to broader reach work if the promoted replay surface regresses

## References

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md)
- [archive/interpret_tail/](archive/interpret_tail/)
- [archive/pre_pivot_substrate/](archive/pre_pivot_substrate/)
