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
- `interpret_tail_live_supported_total=4136`
- `interpret_tail_live_fallback_total=90`
- `interpret_tail_live_seen_total=4226`
- `interpret_tail_live_unseen_formula_cells=46435`
- `interpret_tail_live_promoted_function_supported_total=2886`
- `interpret_tail_live_supported_rate=8.16`
- `interpret_tail_live_seen_rate=8.34`

Ambient live fallback reasons:

- `unsupported_formula_shape=52`
- `unsupported_host_surface=38`
- `parse_failure=0`

### Full Replay Corpus: Forced Interpret Observe

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=2068`
- `interpret_tail_forced_interpret_fallback_total=45`
- `interpret_tail_forced_interpret_seen_total=2113`
- `interpret_tail_forced_interpret_unseen_formula_cells=48548`
- `interpret_tail_forced_interpret_promoted_function_supported_total=1443`
- `interpret_tail_forced_interpret_supported_rate=4.08`
- `interpret_tail_forced_interpret_seen_rate=4.17`

### Promoted-Family Probe

- `interpret_tail_probe_formula_cells=1812`
- `interpret_tail_authoritative_total=1698`
- `interpret_tail_authoritative_fallback_total=114`
- promoted-family authoritative rate: `93.71%`

Promoted-family fallback reasons:

- `unsupported_formula_shape=26`
- `shadow_mismatch=69`
- `unsupported_host_surface=19`

### Promoted Replay Eligibility Inventory

- `interpret_tail_replay_promoted_formula_cells=1812`
- `interpret_tail_replay_promoted_direct_seen=1812`
- `interpret_tail_replay_promoted_direct_supported=1767`
- `interpret_tail_replay_promoted_direct_fallback=45`
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

Still not true:

- no broad default-on rollout exists
- no `ScInterpreter` subroutine has been deleted yet
- full replay-corpus live traffic is now material, but most supported ambient
  traffic still covers only a bounded minority of formulas
- the replay-promoted reach blocker is cleared, but replay-imported promoted
  formulas still retain `unsupported_formula_shape` and
  `unsupported_host_surface` fallback

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

The next pass should target replay-imported promoted fallback conversion:

1. reduce replay-live `unsupported_formula_shape`
2. reduce replay-live `unsupported_host_surface`
3. prioritize retained replay-imported families by ambient impact:
   `VLOOKUP`, `XLOOKUP`, `MATCH`, `XMATCH`, `INDEX`, and bounded `VALUE`
4. only return to broader reach work if the promoted replay surface regresses

## References

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md)
- [archive/interpret_tail/](archive/interpret_tail/)
- [archive/pre_pivot_substrate/](archive/pre_pivot_substrate/)
