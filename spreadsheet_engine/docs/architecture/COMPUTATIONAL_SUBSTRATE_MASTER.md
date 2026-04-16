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
- an eighty-three-slice env-independent hard-route cluster is still
  engine-first even with rollout explicitly `off`, and logical constants,
  formula text, conversion, significant rounding, and bitwise now also have
  family-local default-on rollout paths
- eight real narrow legacy deletion milestones have landed:
  `ScInterpreter::ScTrue()` / `ScFalse()` are retired behind an explicit
  family-local default-on logical-constant path, and the dedicated
  `ScGetDateValue()` / `ScGetTimeValue()` wrapper pair is retired while
  preserving inline nested legacy execution, and the dedicated
  `ScInterpreter::ScFormula()` wrapper is now retired behind a family-local
  default-on `FORMULA(...)` path while preserving inline nested/array legacy
  execution at opcode dispatch, and now the dedicated
  `ScInterpreter::ScConvertOOo()` wrapper is retired behind a family-local
  default-on `CONVERT(...)` path while preserving inline nested legacy
  execution at opcode dispatch, and now the dedicated
  `ScInterpreter::ScRoundSignificant()` wrapper is retired behind a narrow
  family-local default-on `ROUNDSIG(...)` / `ORG.LIBREOFFICE.ROUNDSIG(...)`
  path while preserving inline nested legacy execution at opcode dispatch,
  and now the dedicated `ScInterpreter::ScEuroConvert()` wrapper is retired
  behind the family-local default-on conversion path while preserving inline
  nested legacy execution at opcode dispatch, and now the dedicated
  `ScInterpreter::ScBase()` / `ScDecimal()` / `ScRoman()` / `ScArabic()`
  wrappers are retired behind that same conversion family-local default-on
  path while preserving inline nested legacy execution at opcode dispatch,
  and now the dedicated `ScInterpreter::ScBitAnd()` / `ScBitOr()` /
  `ScBitXor()` / `ScBitLshift()` / `ScBitRshift()` wrappers are retired
  behind a narrow family-local default-on bitwise math slice while
  preserving inline nested legacy execution at opcode dispatch
- hard-route widening is now frozen unless it removes a live fallback reason
  or live mismatch bucket
- the deletion-gating live authoritative-match north-star has now moved to
  `20762 / 50,661` (`40.9822%`) on the standing replay corpus
- the broad corpus lane now completes again with `BusinessDay` admitted on the
  default ambient surface after rejecting zero-workday `WORKDAY` weekend masks
  before they enter the shared runtime, and the next `BusinessDay` slice has
  now admitted cheap local reference, holiday-range, weekend-range,
  weekend-code-ref, and named-ref shapes, with the latest bounded ambient wall
  move now also admitting `CONVERT`, the numeral-conversion roots
  `BASE` / `DECIMAL` / `ROMAN` / `ARABIC`, and `AGGREGATE` while pinning
  focused `ROUNDSIG` / `ORG.LIBREOFFICE.ROUNDSIG` coverage

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
- `interpret_tail_live_authoritative_probe_formula_cells=26996`
- `interpret_tail_live_authoritative_match_total=20762`
- `interpret_tail_live_authoritative_fallback_total=6234`
- live authoritative-match rate over the corpus: `40.9822%`
- live authoritative-match rate over the current promoted probe: `76.9077%`

Everything below is diagnostic context for improving that number.

### Full Replay Corpus: Ambient Live Observe Attempts

These are attempt totals from live observe, not unique-cell coverage.

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=92032`
- `interpret_tail_live_fallback_total=354`
- `interpret_tail_live_seen_total=92386`
- `interpret_tail_live_unseen_formula_cells=0`
- `interpret_tail_live_promoted_function_supported_total=91430`
- `interpret_tail_live_supported_rate=181.66`
- `interpret_tail_live_seen_rate=182.36`

Dominant ambient fallback reasons:

- `unsupported_formula_shape=80`
- `unsupported_host_surface=0`
- `parse_failure=4`
- `unsupported_function=270`

### Full Replay Corpus: Live Unique-Cell Surface

This is the honest per-formula-cell live-routing surface from the standing
replay corpus. It counts whether each formula cell was actually seen and
supported during the bulk live observe run.

- `interpret_tail_live_unique_formula_cells=50661`
- `interpret_tail_live_unique_seen_formula_cells=27417`
- `interpret_tail_live_unique_supported_formula_cells=27245`
- `interpret_tail_live_unique_fallback_formula_cells=172`
- `interpret_tail_live_unique_unsupported_function_formula_cells=135`
- `interpret_tail_live_unique_unseen_formula_cells=23284`
- `interpret_tail_live_unique_seen_rate=54.12`
- `interpret_tail_live_unique_supported_rate=53.78`

### Live Unique Unsupported-Function Top-N

- `unknown`: `108` unique unsupported-function cells
- `text_utility`: `19` unique unsupported-function cells
- `conditional`: `8` unique unsupported-function cells

Routing policy:

- first keep burning down the smaller `unknown` bucket by taking the highest-
  count bounded roots directly
- then target `text_utility`
- then target `conditional`

Unknown root split:

- `MDETERM`: `7`
- `COM.MICROSOFT.UNIQUE`: `7`
- `COM.MICROSOFT.SORT`: `6`
- `COM.MICROSOFT.SORTBY`: `6`
- `PROB`: `6`
- `COM.MICROSOFT.TEXTSPLIT`: `4`
- `COM.MICROSOFT.HSTACK`: `4`
- `GROWTH`: `4`

That now sharpens the next routing policy:

- the bounded selector cluster is now admitted and tracked as
  `selector=62` live unique cells, `58` supported / `4` fallback
- then the classic scalar/matrix cluster: `MDETERM`, `PROB`, `GROWTH`
- only then the spill-heavy dynamic-array cluster:
  `UNIQUE`, `SORT`, `SORTBY`, `TEXTSPLIT`, `HSTACK`

### Full Replay Corpus: Forced Interpret Observe Attempts

These are also attempt totals. The new forced-direct inventory below is the
honest unique-cell surface for direct routing.

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=1681397`
- `interpret_tail_forced_interpret_fallback_total=193`
- `interpret_tail_forced_interpret_seen_total=1681590`
- `interpret_tail_forced_interpret_unseen_formula_cells=0`
- `interpret_tail_forced_interpret_promoted_function_supported_total=1681096`
- `interpret_tail_forced_interpret_supported_rate=3318.92`
- `interpret_tail_forced_interpret_seen_rate=3319.30`

### Full Replay Corpus: Forced Direct Unique-Cell Surface

This is the direct-routing comparison surface after explicitly dirtying and
forcing each replay formula cell once.

- `interpret_tail_forced_direct_formula_cells=50661`
- `interpret_tail_forced_direct_seen_formula_cells=27361`
- `interpret_tail_forced_direct_supported_formula_cells=27189`
- `interpret_tail_forced_direct_fallback_formula_cells=172`
- `interpret_tail_forced_direct_unseen_formula_cells=23284`
- `interpret_tail_forced_direct_seen_rate=54.01`
- `interpret_tail_forced_direct_supported_rate=53.67`

### Raw Cached-Workbook Promoted Probe

- `interpret_tail_probe_formula_cells=26996`
- `interpret_tail_authoritative_total=5414`
- `interpret_tail_authoritative_fallback_total=21582`
- raw promoted authoritative rate: `20.05%`

Dominant promoted-family fallback reasons:

- `shadow_mismatch=21530`
- `unsupported_function=27`
- `unsupported_formula_shape=25`
- `unsupported_host_surface=0`

### Live-Reachable vs Imported-Artifact Promoted Probe

- `interpret_tail_probe_live_reachable_formula_cells=0`
- `interpret_tail_probe_imported_artifact_formula_cells=26996`
- live-reachable promoted rate: `0.00%`
- imported-artifact-only promoted rate: `100.00%`

Interpretation:

- the promoted probe is now diagnostic-only and only meaningful when split
  into live-reachable vs imported-artifact-only buckets
- the `shadow_mismatch=21530` wall is overwhelmingly imported cached-workbook
  debt, not a live parity denominator

### Engine-Authoritative Families

Default-on families:

- logical constants
- formula text
- conversion family
- significant rounding
- bitwise family

Hard-routed env-independent slices:

- `83` slices remain in the explicit hard-route quarantine surface
- they are concentrated in text parsing, bounded `RATE`, and the literal-array
  lookup/match/index families

Legacy quarantine policy:

- `interpr4.cxx` now routes all family-local default-on warnings through a
  shared helper instead of duplicating the same guard boilerplate per opcode
- string-literal `DATEVALUE` / `TIMEVALUE` wrapper bodies remain deleted even
  though their root slices are still env-`off` hard-routes rather than
  default-on families

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
- `CONCATENATE`
- `CONCAT`
- `CLEAN`
- `CHAR`
- `CODE`
- `UNICHAR`
- `UNICODE`
- `UPPER`
- `LOWER`
- `PROPER`
- `ASC`
- `JIS`
- `LEN`
- `LEFT`
- `RIGHT`
- `T`
- `EXACT`
- `FISHER`
- `FISHERINV`
- `POISSON`
- `POISSON.DIST`
- `BINOMDIST`
- `BINOM.DIST`
- `BINOM.DIST.RANGE`
- `B`
- `BETADIST`
- `BETA.DIST`
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
- bounded numeric aggregates:
  `SUM`, `PRODUCT`, `SUMSQ`, `AVERAGE`, `DEVSQ`, `MULTINOMIAL`,
  `SUMX2MY2`, `SUMX2PY2`, `SUMXMY2`
- bounded ranked statistical helpers:
  `QUARTILE`, `QUARTILE.INC`, `QUARTILE.EXC`,
  `PERCENTRANK`, `PERCENTRANK.INC`, `PERCENTRANK.EXC`
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
   the full replay corpus still has a broad live attempt wall, with
   `unsupported_function=270`
2. live-authority headroom:
   the north-star is now `20762 / 50,661` (`40.9822%`), which is a real jump
   but still well short of deletion-comfortable territory
3. imported live-host-truth residuals:
   the next likely north-star movers are now the broader ambient
   `unsupported_function` wall rather than the tiny imported residue buckets
4. raw promoted diagnostic debt:
   `logical_fold=2157` fallback and `math_scalar=626` fallback now reflect
   cached-workbook disagreement after the live-host parity pivot
5. smaller live and raw residue:
   ambient live now has `logical_fold=6`, `INDEX=2`, while the raw promoted
   diagnostic surface still carries
   `logical_constant=5`
6. first real Calc-path retirement:
   the logical-constant pair now has explicit family-local default-on routing
   and its dedicated `ScInterpreter` subroutines are deleted, but broader
   interpreter retirement is still ahead
7. imported replay denominator honesty:
   the raw promoted replay probe is now confirmed to be a cached imported
   correctness surface, not a live seam-off retirement denominator

The live authoritative-match north-star on the standing replay corpus is now
`20762 / 50,661` (`40.9822%`). The honest live unique-cell inventory now
shows `27417 / 50,661` formula cells seen (`54.12%`) and
`27245 / 50,661` supported (`53.78%`) during the bulk live observe run, while
the forced-direct comparison surface now sits at
`27361 / 50,661` seen (`54.01%`) and `27189 / 50,661` supported (`53.67%`).
Those are the coverage-style numbers we should currently use alongside the
north-star; the broader live and forced-interpret counters are still attempt
telemetry rather than a deletion denominator.

That gain came first from aligning imported live host truth on
reference-driven `logical_fold` / `math_scalar` roots, then from extending
that same policy into imported direct `information_predicate` residue plus the
new bounded selector cluster `CHOOSECOLS` / `CHOOSEROWS`,
follow-on imported direct `logical_fold` and nested-`XMATCH` `INDEX`
residue, then from admitting a bounded ranked-statistical cluster for
`QUARTILE*` and `PERCENTRANK*`, then bounded calendar/date utility helpers,
then from fixing the zero-workday `WORKDAY` weekend-mask hang so
`BusinessDay` can stay on the default ambient surface without blowing out the
broad corpus lane, followed by widening that same `BusinessDay` family to
cheap local reference, holiday-range, weekend-range, weekend-code-ref, and
named-ref shapes, then by admitting a bounded statistical aggregate slice for
`MAX` / `MIN`, `MEDIAN`, `GEOMEAN` / `HARMEAN`, `VAR*` / `STDEV*`, plus the
ranked extension for `LARGE` / `SMALL` / `RANK*`, then a bounded scalar
`IF(...)` family, then a bounded `text_utility` family covering
`CONCATENATE` / `CONCAT`, `CLEAN`, `CHAR` / `CODE`, `UNICHAR` / `UNICODE`,
case conversion, width conversion, `LEN`, `LEFT` / `RIGHT`, `T`, and `EXACT`,
and now a bounded `statistical_distribution` family covering `FISHER` /
`FISHERINV`, `POISSON*`, `BINOMDIST` / `BINOM.DIST*`, and `BETADIST` /
`BETA.DIST`, then from imported `FORMULA(...)` live-host-truth alignment, and
now further from a bounded `CONVERT` slice that contributes
`308 / 308 / 0` on the live unique surface plus a bounded `AGGREGATE` slice
that contributes `212 / 211 / 1`, while focused `ROUNDSIG` /
`ORG.LIBREOFFICE.ROUNDSIG` helper and authority coverage is now pinned in the
validation suite. The imported `FORMULA(...)` slice remains the biggest single
north-star mover: imported reference-target `FORMULA(...)` roots now match
live Calc `FormulaError::VariableExpected` instead of replay-cached workbook
strings, which added `14278 / 14278` supported cells on the live unique
surface and moved the deletion-gating metric decisively above the `25%`
milestone. The raw promoted replay probe now sits at `5414 / 26996` and still
carries `14278` `formula_text` shadow mismatches, so it remains purely
diagnostic rather than a retirement denominator.

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

With the new host-truth filtered probe, all `26,319` promoted replay probe
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
4. keep targeting slices that increase live authoritative-match directly,
   led by the remaining ambient `unsupported_function` wall rather than by
   replay-probe cleanup
5. keep `NETWORKDAYS`, `WORKDAY`, `NETWORKDAYS.INTL`, and `WORKDAY.INTL`
   behind explicit opt-in until the ambient evaluator path is cheap enough for
   broad corpus measurement
6. keep attacking the remaining ambient `unsupported_function` wall with cheap,
   measurable clusters after calendar/date utility admission
7. use the logical-constant and date/time wrapper deletion milestones as the
   template for the next narrow retirement only after that ambient wall pays
   down further
8. only return to imported replay parity if we intentionally decide to
   rehabilitate legacy seam-off imported-formula execution

## Navigation

Use these documents in order:

1. [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
2. [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
3. [../PROJECT_STATUS.md](../PROJECT_STATUS.md)
4. [../archive/interpret_tail/](../archive/interpret_tail/)
5. [../archive/pre_pivot_substrate/](../archive/pre_pivot_substrate/)
