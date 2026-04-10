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
- `interpret_tail_live_supported_total=606`
- `interpret_tail_live_fallback_total=0`
- `interpret_tail_live_seen_total=606`
- `interpret_tail_live_unseen_formula_cells=50055`
- `interpret_tail_live_promoted_function_supported_total=0`
- `interpret_tail_live_supported_rate=1.20`
- `interpret_tail_live_seen_rate=1.20`

Ambient live fallback reasons:

- `unsupported_formula_shape=0`
- `unsupported_function=0`
- `parse_failure=0`

### Full Replay Corpus: Forced Interpret Observe

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=303`
- `interpret_tail_forced_interpret_fallback_total=0`
- `interpret_tail_forced_interpret_seen_total=303`
- `interpret_tail_forced_interpret_unseen_formula_cells=50358`
- `interpret_tail_forced_interpret_promoted_function_supported_total=0`
- `interpret_tail_forced_interpret_supported_rate=0.60`
- `interpret_tail_forced_interpret_seen_rate=0.60`

### Promoted-Family Probe

- `interpret_tail_probe_formula_cells=1812`
- `interpret_tail_authoritative_total=1698`
- `interpret_tail_authoritative_fallback_total=114`
- promoted-family authoritative rate: `93.71%`

Promoted-family fallback reasons:

- `unsupported_formula_shape=26`
- `shadow_mismatch=69`
- `unsupported_host_surface=19`

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
  traffic is still root-error or logical-literal traffic rather than promoted
  family function traffic
- even the new full forced-interpret replay denominator still surfaces zero
  promoted-family live traffic, which means the blocker is earlier than access
  pattern alone

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

The next pass should prioritize replay-corpus live eligibility:

1. inventory which promoted-family replay cells never become live seen
2. compare probe addresses against ambient and forced-interpret seen addresses
3. identify the dominant pre-tail barrier: dirty-state, shared-group entry, or
   another live routing condition
4. only then return to ambient promoted-family conversion work

The latest replay reach closeout is:

- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md)

## References

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md)
- [archive/interpret_tail/](archive/interpret_tail/)
- [archive/pre_pivot_substrate/](archive/pre_pivot_substrate/)
