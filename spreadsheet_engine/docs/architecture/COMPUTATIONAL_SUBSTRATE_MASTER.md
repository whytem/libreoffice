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
- an eighty-five-slice env-independent logical/text/match/xmatch/lookup/index
  cluster is now engine-first even with rollout explicitly `off`
- the first real legacy deletion milestone has landed:
  `ScInterpreter::ScTrue()` / `ScFalse()` are retired behind an explicit
  family-local default-on logical-constant path
- hard-route widening is now frozen unless it removes a live fallback reason
  or live mismatch bucket
- the deletion-gating live authoritative-match north-star is currently only
  `2 / 50,661` (`0.0039%`) on the standing replay corpus

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

### North-Star Live Authoritative Match

This is the deletion-gating number for the standing replay corpus:

- `interpret_tail_live_authoritative_corpus_formula_cells=50661`
- `interpret_tail_live_authoritative_probe_formula_cells=7063`
- `interpret_tail_live_authoritative_match_total=2`
- `interpret_tail_live_authoritative_fallback_total=7061`
- live authoritative-match rate over the corpus: `0.0039%`
- live authoritative-match rate over the current promoted probe: `0.0283%`

Everything below is diagnostic context for improving that number.

### Full Replay Corpus: Ambient Live Observe

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=14700`
- `interpret_tail_live_fallback_total=702`
- `interpret_tail_live_seen_total=15402`
- `interpret_tail_live_unseen_formula_cells=35259`
- `interpret_tail_live_promoted_function_supported_total=13450`
- `interpret_tail_live_supported_rate=29.02`
- `interpret_tail_live_seen_rate=30.40`

Dominant ambient fallback reasons:

- `unsupported_formula_shape=78`
- `unsupported_host_surface=12`
- `parse_failure=4`
- `unsupported_function=608`

### Full Replay Corpus: Forced Interpret Observe

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=47150`
- `interpret_tail_forced_interpret_fallback_total=351`
- `interpret_tail_forced_interpret_seen_total=47501`
- `interpret_tail_forced_interpret_unseen_formula_cells=3160`
- `interpret_tail_forced_interpret_promoted_function_supported_total=46525`
- `interpret_tail_forced_interpret_supported_rate=93.07`
- `interpret_tail_forced_interpret_seen_rate=93.76`

### Promoted-Family Probe

- `interpret_tail_probe_formula_cells=7063`
- `interpret_tail_authoritative_total=6742`
- `interpret_tail_authoritative_fallback_total=321`
- promoted-family authoritative rate: `95.45%`

Dominant promoted-family fallback reasons:

- `shadow_mismatch=272`
- `unsupported_function=14`
- `unsupported_formula_shape=29`
- `unsupported_host_surface=6`

### Live-Target Filtered Promoted Probe

- `interpret_tail_live_target_probe_formula_cells=0`
- `interpret_tail_probe_host_truth_artifact_formula_cells=7063`
- `interpret_tail_live_target_authoritative_total=0`
- `interpret_tail_live_target_authoritative_fallback_total=0`

### Hard-Quarantined Calc Paths

- `85` env-independent engine-first slices:
  - `TRUE()`
  - `FALSE()`
  - string-literal `VALUE`
- string-literal `DATEVALUE`
- string-literal `TIMEVALUE`
- literal-only `NUMBERVALUE`
- bounded scalar `RATE`
- exact `MATCH(<literal>; <1D literal array>; 0)`
  - default-approximate `MATCH(<literal>; <ascending numeric 1D literal array>)`
  - approximate-ascending `MATCH(<literal>; <ascending numeric 1D literal array>; 1)`
  - approximate-descending `MATCH(<literal>; <descending numeric 1D literal array>; -1)`
  - default-exact `XMATCH(<literal>; <1D literal array>)`
  - exact `XMATCH(<literal>; <1D literal array>; 0)`
  - exact-forward `XMATCH(<literal>; <1D literal array>; 0; 1)`
  - exact-reverse `XMATCH(<literal>; <1D literal array>; 0; -1)`
  - exact-binary-ascending `XMATCH(<literal>; <ascending numeric 1D literal array>; 0; 2)`
  - exact-binary-descending `XMATCH(<literal>; <descending numeric 1D literal array>; 0; -2)`
  - default-exact-forward `XMATCH(<literal>; <1D literal array> ;; 1)`
  - default-exact-reverse `XMATCH(<literal>; <1D literal array> ;; -1)`
  - default-exact-binary-ascending `XMATCH(<literal>; <ascending numeric 1D literal array> ;; 2)`
  - default-exact-binary-descending `XMATCH(<literal>; <descending numeric 1D literal array> ;; -2)`
  - next-larger `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; 1)`
  - next-smaller `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; -1)`
  - next-larger-forward `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; 1; 1)`
  - next-smaller-forward `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; -1; 1)`
  - next-larger-reverse `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; 1; -1)`
  - next-smaller-reverse `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; -1; -1)`
  - next-larger-binary-ascending `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; 1; 2)`
  - next-smaller-binary-ascending `XMATCH(<numeric literal>; <ascending numeric 1D literal array>; -1; 2)`
  - next-larger-binary-descending `XMATCH(<numeric literal>; <descending numeric 1D literal array>; 1; -2)`
  - next-smaller-binary-descending `XMATCH(<numeric literal>; <descending numeric 1D literal array>; -1; -2)`
  - `LOOKUP(<literal>; <1D literal vector>)`
  - `LOOKUP(<literal>; <1D literal vector>; <1D literal result vector>)`
  - `LOOKUP(<literal>; <2D literal matrix>)`
  - `VLOOKUP(<literal>; <2D literal array>; <positive whole>; 0)`
  - `VLOOKUP(<literal>; <2D literal array>; <positive whole>; FALSE())`
  - `VLOOKUP(<literal>; <ascending numeric 2D literal array>; <positive whole>)`
  - `VLOOKUP(<literal>; <ascending numeric 2D literal array>; <positive whole>; TRUE())`
  - `HLOOKUP(<literal>; <2D literal array>; <positive whole>; 0)`
  - `HLOOKUP(<literal>; <2D literal array>; <positive whole>; FALSE())`
  - `HLOOKUP(<literal>; <ascending numeric 2D literal array>; <positive whole>)`
  - `HLOOKUP(<literal>; <ascending numeric 2D literal array>; <positive whole>; TRUE())`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found>)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;; 0)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found>; 0)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;; 0; 1)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found>; 0; 1)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;; 0; -1)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found>; 0; -1)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;;; 1)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found> ;; 1)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector> ;;; -1)`
  - `XLOOKUP(<literal>; <1D literal array>; <1D literal result vector>; <literal if_not_found> ;; -1)`
  - `XLOOKUP(<literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; 0; 2)`
  - `XLOOKUP(<literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 0; 2)`
  - `XLOOKUP(<literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;;; 2)`
  - `XLOOKUP(<literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found> ;; 2)`
  - `XLOOKUP(<literal>; <descending numeric 1D literal array>; <1D literal result vector> ;; 0; -2)`
  - `XLOOKUP(<literal>; <descending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 0; -2)`
  - `XLOOKUP(<literal>; <descending numeric 1D literal array>; <1D literal result vector> ;;; -2)`
  - `XLOOKUP(<literal>; <descending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found> ;; -2)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; 1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; -1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; -1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; 1; 1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 1; 1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; -1; 1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; -1; 1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; 1; -1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 1; -1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; -1; -1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; -1; -1)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; 1; 2)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 1; 2)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector> ;; -1; 2)`
  - `XLOOKUP(<numeric literal>; <ascending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; -1; 2)`
  - `XLOOKUP(<numeric literal>; <descending numeric 1D literal array>; <1D literal result vector> ;; 1; -2)`
  - `XLOOKUP(<numeric literal>; <descending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; 1; -2)`
  - `XLOOKUP(<numeric literal>; <descending numeric 1D literal array>; <1D literal result vector> ;; -1; -2)`
  - `XLOOKUP(<numeric literal>; <descending numeric 1D literal array>; <1D literal result vector>; <literal if_not_found>; -1; -2)`
  - `INDEX(<2D literal array>; <positive whole>)`
  - `INDEX(<2D literal array>; <positive whole>; <positive whole>)`
  - `INDEX(<2D literal array>; 0; <positive whole>)`
  - `INDEX(<2D literal array>; <positive whole>; 0)`
- `TRUE()` / `FALSE()` now also have an explicit family-local default-on
  rollout path, and their dedicated `ScInterpreter` subroutines are deleted
- the corresponding legacy interpreter entries now warn on normal reach for
  those narrow slices:
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
- `RATE`
- `ROUND`
- `ROUNDUP`
- `ROUNDDOWN`
- `ISERROR`
- `ISERR`
- `ISNUMBER`
- `ISNA`
- `ISTEXT`
- `ISNONTEXT`
- `ISBLANK`
- `AND`
- `OR`
- `XOR`
- `NOT`
- `MATCH`
- `XMATCH`
- `LOOKUP`
- `VLOOKUP`
- `HLOOKUP`
- `XLOOKUP`
- `INDEX`
- bounded scalar-math helpers under comparison ranges:
  `ABS`, `PI`, trig / inverse-trig / hyperbolic variants, scalar rounding
  variants, bitwise helpers, `POWER`, `LOG`, `EXP`, `MOD`, `TRUNC`,
  `GCD`, `LCM`, and related aliases
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

1. quality inside the new ambient live traffic:
   the full replay corpus now shows `15,402` seen formulas out of `50,661`,
   but `702` of those seen routes still fall back
2. logical-fold support quality:
   promoted probe `2021 authoritative / 169 fallback`, still led by
   `shadow_mismatch`
3. scalar-math comparison-feeder parity:
   promoted probe `848 authoritative / 172 fallback`, still mostly
   `shadow_mismatch`
4. promoted-family residual parity and shape:
   `shadow_mismatch=272`, `unsupported_function=14`,
   `unsupported_formula_shape=29`
5. smaller raw mismatch bands:
   `information_predicate=37` fallback and `lookup=0`
6. first real Calc-path retirement:
   the logical-constant pair now has explicit family-local default-on routing
   and its dedicated `ScInterpreter` subroutines are deleted, but broader
   interpreter retirement is still ahead
7. imported replay denominator honesty:
   the raw promoted replay probe is now confirmed to be a cached imported
   correctness surface, not a live seam-off retirement denominator

The live authoritative-match north-star on the standing replay corpus is now
explicitly measured at only `2 / 50,661` (`0.0039%`), even though ambient
live reach is `15,402 / 50,661` seen formulas (`30.40%`) and
`14,700 / 50,661` supported (`29.02%`). That is now the only number that
should gate deletion claims.

Inside the current families, the semantically distinct env-independent
literal-array hard-route frontier is now frozen. New widening is out of scope
unless it removes a live fallback reason or live mismatch bucket.

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

1. do not add new hard-route slices unless they remove a live fallback reason
   or live mismatch bucket
2. treat live authoritative-match as the single north-star metric for
   retirement progress
3. treat the raw promoted replay probe as a cached imported correctness
   surface, not as the live retirement denominator
4. reduce residual fallback inside the admitted ambient traffic, led by
   `math_scalar` and `logical_fold` shadow mismatch
5. use the logical-constant deletion milestone as the template for the next
   narrow retirement only after `math_scalar` / `logical_fold` mismatch
   reduction pays down more live fallback
6. only return to imported replay parity if we intentionally decide to
   rehabilitate legacy seam-off imported-formula execution

## Navigation

Use these documents in order:

1. [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
2. [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
3. [../PROJECT_STATUS.md](../PROJECT_STATUS.md)
4. [../archive/interpret_tail/](../archive/interpret_tail/)
5. [../archive/pre_pivot_substrate/](../archive/pre_pivot_substrate/)
