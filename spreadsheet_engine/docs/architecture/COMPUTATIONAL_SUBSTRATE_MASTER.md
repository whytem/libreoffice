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
- `interpret_tail_live_supported_total=4326`
- `interpret_tail_live_fallback_total=622`
- `interpret_tail_live_seen_total=4948`
- `interpret_tail_live_unseen_formula_cells=45713`
- `interpret_tail_live_promoted_function_supported_total=3076`
- `interpret_tail_live_supported_rate=8.54`
- `interpret_tail_live_seen_rate=9.77`

Dominant ambient fallback reasons:

- `unsupported_formula_shape=32`
- `unsupported_host_surface=0`
- `parse_failure=4`
- `unsupported_function=588`

### Full Replay Corpus: Forced Interpret Observe

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=2163`
- `interpret_tail_forced_interpret_fallback_total=311`
- `interpret_tail_forced_interpret_seen_total=2474`
- `interpret_tail_forced_interpret_unseen_formula_cells=48187`
- `interpret_tail_forced_interpret_promoted_function_supported_total=1538`
- `interpret_tail_forced_interpret_supported_rate=4.27`
- `interpret_tail_forced_interpret_seen_rate=4.88`

### Promoted-Family Probe

- `interpret_tail_probe_formula_cells=1812`
- `interpret_tail_authoritative_total=1794`
- `interpret_tail_authoritative_fallback_total=18`
- promoted-family authoritative rate: `99.01%`

Dominant promoted-family fallback reasons:

- `unsupported_formula_shape=5`
- `shadow_mismatch=13`
- `unsupported_host_surface=0`

### Live-Target Filtered Promoted Probe

- `interpret_tail_live_target_probe_formula_cells=0`
- `interpret_tail_probe_host_truth_artifact_formula_cells=1812`
- `interpret_tail_live_target_authoritative_total=0`
- `interpret_tail_live_target_authoritative_fallback_total=0`

### Hard-Quarantined Calc Paths

- `30` env-independent engine-first slices:
  - `TRUE()`
  - `FALSE()`
  - string-literal `VALUE`
  - string-literal `DATEVALUE`
  - string-literal `TIMEVALUE`
  - literal-only `NUMBERVALUE`
  - exact `MATCH(<literal>; <1D literal array>; 0)`
  - default-exact `XMATCH(<literal>; <1D literal array>)`
  - exact `XMATCH(<literal>; <1D literal array>; 0)`
  - exact-forward `XMATCH(<literal>; <1D literal array>; 0; 1)`
  - exact-reverse `XMATCH(<literal>; <1D literal array>; 0; -1)`
  - exact-binary-ascending `XMATCH(<literal>; <ascending numeric 1D literal array>; 0; 2)`
  - exact-binary-descending `XMATCH(<literal>; <descending numeric 1D literal array>; 0; -2)`
  - `LOOKUP(<literal>; <1D literal vector>)`
  - `LOOKUP(<literal>; <1D literal vector>; <1D literal result vector>)`
  - `LOOKUP(<literal>; <2D literal matrix>)`
  - `VLOOKUP(<literal>; <2D literal array>; <positive whole>; 0)`
  - `VLOOKUP(<literal>; <2D literal array>; <positive whole>; FALSE())`
  - `HLOOKUP(<literal>; <2D literal array>; <positive whole>; 0)`
  - `HLOOKUP(<literal>; <2D literal array>; <positive whole>; FALSE())`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;; 0)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;; 0; 1)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;; 0; -1)`
  - `XLOOKUP(<literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; 0; 2)`
  - `XLOOKUP(<literal>; <descending numeric 1D literal array>; <1D literal result vector> ;; 0; -2)`
  - `INDEX(<2D literal array>; <positive whole>)`
  - `INDEX(<2D literal array>; <positive whole>; <positive whole>)`
  - `INDEX(<2D literal array>; 0; <positive whole>)`
  - `INDEX(<2D literal array>; <positive whole>; 0)`
- the corresponding legacy interpreter entries now warn on normal reach for
  those narrow slices:
  - `ScInterpreter::ScTrue()`
  - `ScInterpreter::ScFalse()`
  - `ScInterpreter::ScValue()`
  - `ScInterpreter::ScGetDateValue()`
  - `ScInterpreter::ScGetTimeValue()`
  - `ScInterpreter::ScNumberValue()`
  - `ScInterpreter::ScMatch()`
  - `ScInterpreter::ScXMatch()`
  - `ScInterpreter::ScLookup()`
  - `ScInterpreter::ScVLookup()`
  - `ScInterpreter::ScHLookup()`
  - `ScInterpreter::ScXLookup()`
  - `ScInterpreter::ScIndex()`

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
   the full replay corpus now shows `4,948` seen formulas out of `50,661`, with
   `3,076` promoted-family live supported routes
2. promoted-family residual parity:
   `shadow_mismatch=13`
3. promoted-family residual formula shape:
   replay-live `unsupported_formula_shape=6`
4. first real Calc-path retirement:
   one narrow logical/text/lookup/index cluster is now hard-routed and its
   legacy interpreter entries are quarantined for those slices, but no whole
   `ScInterpreter` subroutine has been deleted yet
5. imported replay denominator honesty:
   the raw promoted replay probe is now confirmed to be a cached imported
   correctness surface, not a live seam-off retirement denominator

The latest bounded exact-search-mode plus literal-array lookup/index slice
raised the env-independent hard-route cluster from `20` to `30` while
keeping the expansion inside explicit-exact and literal-array semantics.

A focused live-host check now shows the replay-imported whole-row
`MATCH([.$B$150]; [.$150:.$150]; -1)` row evaluates to
`FormulaError::VariableExpected` in Calc with the seam forced off, so that
row is no longer treated as a confirmed live parity blocker even though the
replay workbook stores a non-error expected value.

A bounded live-host-truth pass now also shows the replay-imported exact
`VLOOKUP([.P6]; [.$L$2:.$M$8]; 2; 0)` and
`VLOOKUP(21; [.$AM$2:.$AN$4]; 2; 0)` rows both evaluate to
`FormulaError::VariableExpected` in Calc with the seam forced off, so those
cached non-error workbook values are likewise no longer treated as runtime
conversion targets.

A bounded live-host-truth pass now also shows the replay-imported
collation-sensitive exact `VLOOKUP([.M22]; [.$L$11:.$M$32]; 1; 0)` row
evaluates to `FormulaError::VariableExpected` in Calc with the seam forced
off, so the residual replay `VLOOKUP` band is no longer treated as a real
runtime conversion target either.

A bounded live-host-truth pass now also shows the residual replay-imported
`XLOOKUP("Ireland"; [.$H$2:.$H$11]; [.$J$2:.$J$11]; "")` and
`XLOOKUP([.$G$14]; [.$I$14:.$R$14]; [.$I$15:.$R$16])` rows evaluate to
`FormulaError::VariableExpected` in Calc with the seam forced off, so the
residual replay `XLOOKUP` band is no longer treated as a real runtime
conversion target either.

A bounded live-host-truth pass now also shows the replay-imported
`INDEX([.H13:.J19]; XMATCH([.G10]; [.G13:.G19]); XMATCH([.H10]; [.H12:.J12]))`
row evaluates to `FormulaError::VariableExpected` in Calc with the seam
forced off, so that cached numeric workbook value is not treated as a real
runtime conversion target either.

A bounded live-host-truth pass now also shows the replay-imported
`INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 1)`,
`INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 2)`, and
`INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 0)` rows all
evaluate to `FormulaError::VariableExpected` in Calc with the seam forced
off, so that imported `INDEX` matrix-function band is not treated as a real
runtime conversion target either.

A bounded live-host-truth pass now also shows the replay-imported
`DATEVALUE("Jan1, 2015")` rows evaluate to `FormulaError::VariableExpected`
in Calc with the seam forced off, so those cached non-numeric workbook rows
are not treated as real runtime conversion targets either.

A bounded live-host-truth pass now also shows the replay-imported
`MATCH(1; FREQUENCY([.I126]; [.H129:.M129]); 0)` row evaluates to
`FormulaError::VariableExpected` in Calc with the seam forced off, so that
cached workbook non-error row is likewise not treated as a real runtime
conversion target.

With the new host-truth filtered probe, all `1,812` promoted replay probe
rows now classify as imported host-truth artifacts under seam-off direct
legacy interpretation. So that probe remains useful as a cached imported
correctness surface, but not as the live retirement denominator.

## Recommended Next Pass

The next pass should:

1. treat the raw promoted replay probe as a cached imported correctness
   surface, not as the live retirement denominator
2. drive the next real runtime win on broader ambient live reach or another
   narrow hard-route / quarantine slice inside `ScInterpreter`
3. only return to imported replay parity if we intentionally decide to
   rehabilitate legacy seam-off imported-formula execution

## Navigation

Use these documents in order:

1. [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
2. [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
3. [../PROJECT_STATUS.md](../PROJECT_STATUS.md)
4. [../archive/interpret_tail/](../archive/interpret_tail/)
5. [../archive/pre_pivot_substrate/](../archive/pre_pivot_substrate/)
