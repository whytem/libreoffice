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

### North-Star Live Authoritative Match

This is the deletion-gating number for the standing replay corpus:

- `interpret_tail_live_authoritative_corpus_formula_cells=50661`
- `interpret_tail_live_authoritative_probe_formula_cells=43961`
- `interpret_tail_live_authoritative_match_total=43957`
- `interpret_tail_live_authoritative_fallback_total=4`
- `legacy_interpreter_subroutine_count=220`
- live authoritative-match rate over the corpus: `86.7669%`
- live authoritative-match rate over the current promoted probe: `99.9909%`

The north-star measures live authority transfer. `legacy_interpreter_subroutine_count`
is the blunt retirement-progress companion metric, derived from the remaining
`void Sc*()` declarations in [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx).
Lower is better.

Everything below is diagnostic context for improving that number.

### Full Replay Corpus: Ambient Live Observe Attempts

These are attempt totals from live observe, not unique-cell coverage. They
remain useful for hotspot steering, but they should not be read as the
deletion denominator.

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=90300`
- `interpret_tail_live_fallback_total=0`
- `interpret_tail_live_seen_total=90300`
- `interpret_tail_live_unseen_formula_cells=0`
- `interpret_tail_live_promoted_function_supported_total=90170`
- `interpret_tail_live_supported_rate=178.24`
- `interpret_tail_live_seen_rate=178.24`

Ambient live fallback reasons:

- `unsupported_formula_shape=0`
- `unsupported_host_surface=0`
- `parse_failure=0`
- `unsupported_function=0`

### Full Replay Corpus: Live Unique-Cell Surface

This is the honest per-formula-cell live-routing surface from the standing
replay corpus. It counts whether each formula cell was actually seen and
supported during the bulk live observe run.

- `interpret_tail_live_unique_formula_cells=50661`
- `interpret_tail_live_unique_seen_formula_cells=44026`
- `interpret_tail_live_unique_supported_formula_cells=44026`
- `interpret_tail_live_unique_fallback_formula_cells=0`
- `interpret_tail_live_unique_unsupported_function_formula_cells=0`
- `interpret_tail_live_unique_unseen_formula_cells=6635`
- `interpret_tail_live_unique_seen_rate=86.90`
- `interpret_tail_live_unique_supported_rate=86.90`

### Live Unique Unsupported-Function Top-N

This table is now intentionally empty on the live unique surface. It remains
the right place to publish any new regression if one appears.

- `interpret_tail_live_unique_unsupported_function_formula_cells=0`

Next routing policy:

- keep `unsupported_function=0` as an explicit regression guard
- steer the next phase off unseen live surface and wrapper-retirement
  opportunities rather than unsupported-function admission

### Unknown Bucket Root Split

There is no remaining live unique `FunctionKind::Unknown` residue on the
validated standing corpus.

Updated next routing policy:

- the next big value is retirement and unseen-surface reduction, not more
  unsupported-function widening
- the current live routing surface is broad enough that the deletion metric
  and the north-star should lead decision-making

### Full Replay Corpus: Forced Interpret Observe Attempts

These are also attempt totals. The new unique-cell direct surface is the
honest coverage metric below.

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=2649510`
- `interpret_tail_forced_interpret_fallback_total=0`
- `interpret_tail_forced_interpret_seen_total=2649510`
- `interpret_tail_forced_interpret_unseen_formula_cells=0`
- `interpret_tail_forced_interpret_promoted_function_supported_total=2649445`
- `interpret_tail_forced_interpret_supported_rate=5229.88`
- `interpret_tail_forced_interpret_seen_rate=5229.88`

### Full Replay Corpus: Forced Direct Unique-Cell Surface

This is the direct-routing comparison surface after dirtying and forcing each
replay formula cell once, then classifying whether that formula cell was
actually seen and supported by the seam.

- `interpret_tail_forced_direct_formula_cells=50661`
- `interpret_tail_forced_direct_seen_formula_cells=44023`
- `interpret_tail_forced_direct_supported_formula_cells=44023`
- `interpret_tail_forced_direct_fallback_formula_cells=0`
- `interpret_tail_forced_direct_unseen_formula_cells=6638`
- `interpret_tail_forced_direct_seen_rate=86.90`
- `interpret_tail_forced_direct_supported_rate=86.90`

### Raw Cached-Workbook Promoted Probe

- `interpret_tail_probe_formula_cells=43961`
- `interpret_tail_authoritative_total=300`
- `interpret_tail_authoritative_fallback_total=43661`
- raw promoted authoritative rate: `0.68%`

Raw promoted fallback reasons:

- `shadow_mismatch=43661`
- `unsupported_function=0`
- `unsupported_formula_shape=0`
- `unsupported_host_surface=0`

### Live-Reachable vs Imported-Artifact Promoted Probe Split

- `interpret_tail_probe_live_reachable_formula_cells=300`
- `interpret_tail_live_target_authoritative_total=300`
- `interpret_tail_live_target_authoritative_fallback_total=0`
- `interpret_tail_probe_live_reachable_rate=0.68%`
- `interpret_tail_probe_imported_artifact_formula_cells=43661`
- `interpret_tail_probe_host_truth_artifact_formula_cells=43661`
- `interpret_tail_probe_imported_artifact_rate=99.32%`

Interpretation:

- the raw promoted probe is now overwhelmingly imported cached-workbook debt,
  not a live parity denominator
- the promoted probe is only interpretable when split into live-reachable vs
  imported-artifact-only buckets
- the `shadow_mismatch=43661` wall is real diagnostic debt, but it is almost
  entirely on imported-artifact-only rows rather than live-reachable parity rows

### Promoted Replay Eligibility Inventory

- `interpret_tail_replay_promoted_formula_cells=43961`
- `interpret_tail_replay_promoted_direct_seen=43958`
- `interpret_tail_replay_promoted_direct_supported=43958`
- `interpret_tail_replay_promoted_direct_fallback=0`
- `interpret_tail_replay_promoted_direct_unseen=3`
- `interpret_tail_replay_promoted_shared_formula_cells=31605`
- `interpret_tail_replay_promoted_non_shared_formula_cells=6411`
- `interpret_tail_replay_promoted_unseen_shared_member=0`
- `interpret_tail_replay_promoted_unseen_non_shared=3`
- `interpret_tail_replay_promoted_shared_member_seen_via_top=0`
- `interpret_tail_replay_promoted_needs_interpret_after_dirty=43961`
- `interpret_tail_replay_promoted_dirty_after_interpret=3`

### Engine-Authoritative Families

Default-on families:

- logical constants: `TRUE()` / `FALSE()`
- scalar-root formulas: comparisons, arithmetic roots, unary roots, and
  percent roots
- formula text: `FORMULA(...)`
- conversion family: `CONVERT(...)`, `ORG.OPENOFFICE.CONVERT(...)`,
  `EUROCONVERT(...)`, `BASE(...)`, `DECIMAL(...)`, `ROMAN(...)`,
  `ARABIC(...)`
- round family: `ROUND(...)`, `ROUNDUP(...)`, `ROUNDDOWN(...)`
- significant rounding: `ROUNDSIG(...)`, `ORG.LIBREOFFICE.ROUNDSIG(...)`
- math-scalar family: bounded scalar trig, inverse-trig, hyperbolic,
  logarithmic, modular, combinatoric, and helper roots
- information predicates, logical folds, `NOT`, and conditionals
- bitwise family: `BITAND`, `BITOR`, `BITXOR`, `BITLSHIFT`, `BITRSHIFT`
- aggregate wrapper: `AGGREGATE(...)`, `COM.MICROSOFT.AGGREGATE(...)`
- matrix determinant: `MDETERM(...)`
- narrow probability slice: `PROB(...)`

Hard-routed env-independent slices:

- `83` engine-first slices remain in the explicit hard-route quarantine
  surface
- those slices are concentrated in:
  - string-literal text parsing: `VALUE`, `DATEVALUE`, `TIMEVALUE`,
    `NUMBERVALUE`
  - bounded scalar financial: `RATE`
  - literal-array lookup/match/index families: `MATCH`, `XMATCH`, `LOOKUP`,
    `VLOOKUP`, `HLOOKUP`, `XLOOKUP`, `INDEX`

Legacy quarantine policy:

- default-on families now route through the shared helper in
  `interpr4.cxx`, so future retirements add one warning call instead of
  another hand-copied guard block
- string-literal `DATEVALUE` / `TIMEVALUE` wrapper bodies remain deleted even
  though their root slices are still env-`off` hard-routes rather than
  default-on families

## Current State

Today:

- the shared compiler, token model, workbook model, FODS loader, evaluator,
  dependency snapshot, invalidation planning, and recalc planning are
  engine-owned
- the `InterpretTail` seam is real production code, not a test-only oracle
- `DBG_UTIL` builds default to `observe` when the rollout env var is unset
- `authority` mode authoritatively bypasses `ScInterpreter` for supported
  promoted families
- the env-`off` hard-route boundary now covers an eighty-three-slice
  quarantine cluster, while logical constants, logical folds, `NOT`,
  conditionals, scalar-root formulas, text utility, formula text,
  conversion, information predicates, round-family roots, significant
  rounding, math-scalar roots, bitwise, aggregate, matrix determinant,
  and `PROB` now also have family-local default-on rollout paths
- the full replay corpus now has a true all-formula live-routing denominator
- replay-imported promoted formulas now reach the seam broadly, and bounded
  top-level `INDEX` / `XLOOKUP` slice results now stay inside it
- the bounded selector cluster `CHOOSECOLS` / `CHOOSEROWS` is now admitted on
  the live unique surface, reducing the dominant `unknown` unsupported-function
  bucket from `130` to `108`
- the new `matrix_math` / `MDETERM` and narrow `PROB(...)` slices now reduce
  that same `unknown` bucket further from `108` to `95`
- the latest deliberate underlying math-feeder expansion now delegates a
  bounded scalar-math family beneath comparison-helper ranges
- the earlier bounded default/approximate `MATCH` plus omitted/approximate
  extended-match work finished the hard-route frontier, and later cleanup
  converted part of that older surface into default-on families
- within the current families, the semantically distinct env-independent
  literal-array hard-route surface is now effectively exhausted
- new hard-route widening is now frozen unless it removes a live fallback
  reason or a live mismatch bucket

Still not true:

- broad wrapper retirement is now meaningfully underway, not just a handful
  of narrow milestones: after the earlier logical-constant, date/time-value,
  formula-text, conversion, numeral-conversion, `ROUNDSIG`, bitwise,
  `MDETERM`, `AGGREGATE`, `PROB`, text-utility, information-predicate, and
  logical/conditional retirements, the latest push also retires the
  scalar-root, `ERROR.TYPE`, round-family, and broad math-scalar wrapper
  clusters
- multiple interpreter hard-route milestones have landed, but full legacy
  opcode retirement has not
- the dominant retained live blocker is no longer unsupported function or
  live fallback on the validated standing corpus: both are now at `0`, so
  the next ceiling is the unseen live surface plus further Calc-path
  retirement
- the live authoritative-match north-star has now moved to
  `43957 / 50,661` (`86.7669%`) on the replay corpus
- that gain now includes the earlier imported-root host-truth alignment work,
  plus the latest broad retirement push that makes scalar-root formulas,
  `ERROR.TYPE`, round-family roots, and bounded math-scalar roots default-on
  engine-owned families while deleting their dedicated legacy wrappers
- the raw promoted replay probe is now unambiguously a diagnostic surface, not
  the retirement denominator: it now sits at `300 / 43961`, with the
  remaining `43661` fallback rows entirely dominated by imported host-truth
  artifacts after the live-host-truth pivot
- a focused live-host check now shows the replay-imported whole-row
  `MATCH([.$B$150];[.$150:.$150];-1)` row evaluates to
  `FormulaError::VariableExpected`
  in Calc itself, so it is no longer treated as a confirmed live parity
  blocker
- a bounded live-host-truth pass now shows the residual replay-imported
  logical-constant band is genuine live Calc error behavior, not a stale
  cached-value artifact
- a bounded live-host-truth pass now also shows the replay-imported exact
  `VLOOKUP([.P6];[.$L$2:.$M$8];2;0)` and
  `VLOOKUP(21;[.$AM$2:.$AN$4];2;0)` rows are genuine live Calc
  `FormulaError::VariableExpected` rows, not real non-error parity blockers
- a bounded live-host-truth pass now also shows the replay-imported
  collation-sensitive exact `VLOOKUP([.M22]; [.L$11:.M$32]; 1; 0)` row is a
  genuine live Calc `FormulaError::VariableExpected` row, not a real runtime
  parity blocker
- a bounded live-host-truth pass now also shows the residual replay-imported
  `XLOOKUP("Ireland"; [.H2:.H11]; [.J2:.J11]; "")` and
  `XLOOKUP([.G14]; [.I14:.R14]; [.I15:.R16])` rows are genuine live Calc
  `FormulaError::VariableExpected` rows, not real runtime parity blockers
- a bounded live-host-truth pass now also shows the replay-imported
  `INDEX([.H13:.J19]; XMATCH([.G10]; [.G13:.G19]); XMATCH([.H10]; [.H12:.J12]))`
  row is also a genuine live Calc `FormulaError::VariableExpected` row, not a
  real runtime parity blocker
- a bounded live-host-truth pass now also shows the replay-imported
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 1)`,
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 2)`, and
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 0)` rows are
  likewise genuine live Calc `FormulaError::VariableExpected` rows, not real
  runtime parity blockers
- a bounded live-host-truth pass now also shows the replay-imported
  `DATEVALUE("Jan1, 2015")` rows are genuine live Calc
  `FormulaError::VariableExpected` rows, not real runtime parity blockers
- a bounded live-host-truth pass now also shows the replay-imported
  `MATCH(1; FREQUENCY([.I126]; [.H129:.M129]); 0)` row is also a genuine live
  Calc `FormulaError::VariableExpected` row, not a real runtime parity blocker
- the promoted replay probe is now explicitly split into raw cached-workbook
  parity and live-target filtered parity: `300` rows are live-reachable and
  authoritative under seam-off direct legacy interpretation, while the
  remaining `43661` rows are imported host-truth artifacts
- the dominant retained live bucket is now unsupported shape rather than
  unsupported function; the raw promoted buckets remain diagnostic debt, not
  the deletion-gating story
- the next runtime milestone therefore should not be defined by the imported
  replay probe anymore; it should move to quality inside the new ambient live
  traffic and the remaining raw shadow-mismatch buckets, or to additional
  Calc-path quarantine / retirement slices

## Active Delegated Family

The live delegated evaluator family currently includes:

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

## Scope Policy

The active roadmap is evaluator migration.

Further computational-substrate widening is out of scope unless it directly:

- removes an `InterpretTail` fallback reason
- removes an `InterpretTail` mismatch class
- unlocks required host access for a promoted evaluator family

Residual substrate frontier items that do not satisfy one of those bars are
historical reference material, not active roadmap.

## Recommended Next Pass

The next pass is now constrained by the scope gate:

1. do not add new hard-route slices unless they remove a live fallback reason
   or a live mismatch bucket
2. treat live authoritative-match as the single north-star metric for
   retirement progress
3. treat the raw promoted replay probe as a cached imported correctness
   surface, not as the live retirement denominator
4. keep targeting slices that increase live authoritative-match directly,
   starting now with unseen live surface and broader wrapper retirement
   rather than reopening replay-probe cleanup
5. keep `NETWORKDAYS`, `WORKDAY`, `NETWORKDAYS.INTL`, and `WORKDAY.INTL`
   behind explicit opt-in until the ambient evaluator path is cheap enough for
   broad corpus measurement
6. treat `unsupported_function=0` as a regression guard and focus the next
   measurable push on unseen-surface reduction
7. use the broadened scalar-root / round / math-scalar retirement pass as the
   template for the next large Calc wrapper-deletion batch
8. only return to imported replay parity if we intentionally decide to
   rehabilitate legacy seam-off imported-formula execution

## References

- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_STRATEGY_MEMO.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_MIGRATION.md)
- [architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_REPLAY_REACH_DIAGNOSTIC_PLAN.md)
- [archive/interpret_tail/](archive/interpret_tail/)
- [archive/pre_pivot_substrate/](archive/pre_pivot_substrate/)
