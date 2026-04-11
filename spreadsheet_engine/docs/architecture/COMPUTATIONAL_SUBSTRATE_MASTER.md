# Computational Substrate Master

Status: canonical current-state, scope, and roadmap reference

## Executive Summary

The first-stage extraction objective is materially achieved.
The live authority-transfer objective is still in progress.

Today:

- `spreadsheet_engine/` is a real shared computation layer used by both
  standalone evaluation and Calc-backed execution paths
- the promoted FODS replay baseline remains exact at zero cached fallback
- the old computational-substrate widening program proved a bounded admitted
  slice
- the live migration program now runs through a real `InterpretTail` seam in
  production Calc code
- debug and CI-style builds default that seam to `observe`
- one tiny family is now engine-first even with rollout explicitly `off`

The active program is no longer “prove more substrate slices.”
The active program is “use the substrate to underwrite live evaluator
delegation.”

## Live Migration Dashboard

### Replay Guardrail

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

### Full Replay Corpus: Ambient Live Observe

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=4182`
- `interpret_tail_live_fallback_total=44`
- `interpret_tail_live_seen_total=4226`
- `interpret_tail_live_unseen_formula_cells=46435`
- `interpret_tail_live_promoted_function_supported_total=2932`
- `interpret_tail_live_supported_rate=8.25`
- `interpret_tail_live_seen_rate=8.34`

Dominant ambient fallback reasons:

- `unsupported_formula_shape=44`
- `unsupported_host_surface=0`
- `parse_failure=0`

### Full Replay Corpus: Forced Interpret Observe

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=2091`
- `interpret_tail_forced_interpret_fallback_total=22`
- `interpret_tail_forced_interpret_seen_total=2113`
- `interpret_tail_forced_interpret_unseen_formula_cells=48548`
- `interpret_tail_forced_interpret_promoted_function_supported_total=1466`
- `interpret_tail_forced_interpret_supported_rate=4.13`
- `interpret_tail_forced_interpret_seen_rate=4.17`

### Promoted-Family Probe

- `interpret_tail_probe_formula_cells=1812`
- `interpret_tail_authoritative_total=1765`
- `interpret_tail_authoritative_fallback_total=47`
- promoted-family authoritative rate: `97.41%`

Dominant promoted-family fallback reasons:

- `unsupported_formula_shape=22`
- `shadow_mismatch=25`
- `unsupported_host_surface=0`

### Hard-Quarantined Calc Paths

- `1` env-independent engine-first family:
  - literal-only `NUMBERVALUE`

## Strategic Position

The program now has three settled layers:

1. shared-engine extraction
2. bounded substrate proof
3. live `InterpretTail -> engine` migration

Only the third layer is the active authority-transfer roadmap.

That means:

- shared-engine extraction remains a success
- substrate widening is now secondary and conditional
- live evaluator delegation is the primary measure of progress

The strategy reset is recorded in
[COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md).
The active current-state ledger is
[COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md).
The latest replay eligibility closeout is now folded into that migration ledger.

## Engine-Owned Today

The engine broadly owns:

- compiler and token infrastructure
- standalone workbook loading and evaluation
- dependency snapshots, invalidation planning, and recalc planning
- substantial shared runtime already consumed by Calc

On the live migration track, the engine now also owns bounded delegated
evaluation for:

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

## Calc-Retained Today

Calc still intentionally owns:

- broad document storage and mutation
- broad listener and broadcaster residency
- broad formula evaluation outside the promoted delegated family
- token-container construction and Calc-local token plumbing
- UI, UNO, import/export, rendering, persistence, and shell integration
- external-reference and environment-sensitive host behavior

That retained shell is expected. The migration goal is targeted delegation,
not immediate whole-document replacement.

## Scope Policy

The active roadmap is evaluator migration.

New computational-substrate widening is out of scope unless it directly:

- removes a live `InterpretTail` fallback reason
- removes a live `InterpretTail` mismatch class
- unlocks host access needed by a promoted evaluator family

Historical substrate frontier items that do not meet one of those bars
remain archived reference material only.

## Biggest Remaining Blockers

The highest-value remaining blockers are now:

1. ambient live-routing reach:
   the full replay corpus now shows `4,226` seen formulas out of `50,661`, with
   `2,932` promoted-family live supported routes
2. promoted replay residual fallback:
   the reach blocker is cleared for promoted replay formulas, but the replay
   imported surface still retains `22` direct promoted fallback cells and
   ambient replay still covers only a bounded minority of formulas
3. promoted-family residual parity:
   `shadow_mismatch=25`
4. promoted-family residual formula shape:
   replay-live `unsupported_formula_shape=44`
5. first real Calc-path retirement:
   one family is hard-routed, but no `ScInterpreter` subroutine has been
   deleted yet

The latest descending-binary `XLOOKUP` parity slice improved promoted-family
authority from `1756 / 56` to `1765 / 47`, cut `shadow_mismatch` from `34`
to `25`, and narrowed the top blocker to `VLOOKUP` parity and
`MATCH` / `XMATCH` shape cleanup.

## Recommended Next Pass

The next pass should:

1. convert the remaining `XLOOKUP` and `VLOOKUP` mismatch rows on the
   replay-promoted surface
2. then clean up the remaining replay-imported `MATCH` / `XMATCH`
   `unsupported_formula_shape` fallout
3. convert residual `LOOKUP` mismatch rows and smaller `INDEX` shape fallout
4. move the first replay-imported promoted family from observe-only reach
   toward a broader authority candidate once mismatch and shape fallback
   shrink materially

## Navigation

Use these documents in order:

1. [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
2. [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
3. [../PROJECT_STATUS.md](../PROJECT_STATUS.md)
4. [../archive/interpret_tail/](../archive/interpret_tail/)
5. [../archive/pre_pivot_substrate/](../archive/pre_pivot_substrate/)
