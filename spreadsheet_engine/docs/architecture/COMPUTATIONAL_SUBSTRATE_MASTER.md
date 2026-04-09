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
- `interpret_tail_live_supported_total=0`
- `interpret_tail_live_fallback_total=303`
- `interpret_tail_live_seen_total=303`
- `interpret_tail_live_unseen_formula_cells=50358`
- `interpret_tail_live_supported_rate=0.00`
- `interpret_tail_live_seen_rate=0.60`

Dominant ambient fallback reasons:

- `unsupported_formula_shape=253`
- `parse_failure=48`
- `unsupported_function=2`

### Promoted-Family Probe

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1379`
- `interpret_tail_authoritative_fallback_total=109`
- promoted-family authoritative rate: `92.67%`

Dominant promoted-family fallback reasons:

- `unsupported_formula_shape=26`
- `shadow_mismatch=64`
- `unsupported_host_surface=19`

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

## Engine-Owned Today

The engine broadly owns:

- compiler and token infrastructure
- standalone workbook loading and evaluation
- dependency snapshots, invalidation planning, and recalc planning
- substantial shared runtime already consumed by Calc

On the live migration track, the engine now also owns bounded delegated
evaluation for:

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
   the full replay corpus still shows only `303` seen formulas out of
   `50,661`
2. ambient unsupported-shape fallout:
   the biggest full-corpus fallback class is still
   `unsupported_formula_shape=253`
3. promoted-family residual parity:
   `shadow_mismatch=64`
4. promoted-family residual host access:
   `unsupported_host_surface=19`
5. first real Calc-path retirement:
   one family is hard-routed, but no `ScInterpreter` subroutine has been
   deleted yet

## Recommended Next Pass

The next pass should:

1. increase the full-corpus `interpret_tail_live_seen_total`
2. convert current ambient `unsupported_formula_shape` fallout
3. reduce retained lookup/index `shadow_mismatch` and
   `unsupported_host_surface`
4. only then expand to the next evaluator capability class

## Navigation

Use these documents in order:

1. [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
2. [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
3. [../PROJECT_STATUS.md](../PROJECT_STATUS.md)
4. [../archive/interpret_tail/](../archive/interpret_tail/)
