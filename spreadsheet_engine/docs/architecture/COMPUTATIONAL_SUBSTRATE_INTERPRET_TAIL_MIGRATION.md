# InterpretTail Engine Migration

Status: active migration ledger for `ScFormulaCell::InterpretTail -> engine evaluator`

## Purpose

This is the active current-state document for the live evaluator migration.

Use it for:

- the current delegated family
- the current routing modes
- the current live metrics
- the current hard-routed or quarantined paths
- the current next target
- the active next-slice plan

The older per-pass plan, decision, and evidence documents are retained in
[../archive/interpret_tail/](../archive/interpret_tail/) for history only.

## Current State

The migration is real, but still early.

Today:

- the seam is wired into `ScFormulaCell::InterpretTail`
- `DBG_UTIL` builds default to `observe` when the rollout env var is unset
- release-style builds still default to `off` unless explicitly opted in
- `observe`, `shadowcompare`, and `authority` are all live on the production
  Calc path
- an eighty-five-slice env-independent logical/text/match/xmatch/lookup/index
  cluster is now engine-first even with rollout set to `off`; see
  [Hard-Routed Family](#hard-routed-family) below
- direct scalar-root formulas and a bounded scalar utility cluster now also
  ride the live seam: arithmetic/reference/concat/comparison roots plus
  `ROUND`, `ROUNDUP`, `ROUNDDOWN`, information predicates, logical folds, and
  `NOT`
- replay-imported promoted formulas now reach the live seam on both the
  ambient and forced-interpret replay surfaces through a bounded pre-RPN
  observe bridge
- nested delegated `XLOOKUP` and `INDEX` matrix-return sources now stay inside
  the engine seam on the replay-imported promoted surface
- top-level replay-imported `INDEX` and `XLOOKUP` slice results now scalarize
  through the engine seam instead of falling out as generic host-surface
  fallout
- replay-imported named-range `VLOOKUP` and `INDEX` rows now resolve through
  the engine seam using the stored name-definition base position instead of
  falling out as generic host-surface fallout
- the latest deliberate family expansion materially raised the ambient live
  denominator instead of only refining replay-imported parity inside already-
  promoted families

What is still not true:

- no broad default-on production rollout exists beyond logical constants
- two narrow legacy deletion milestones have landed:
  `ScInterpreter::ScTrue()` / `ScFalse()` and the dedicated
  `ScGetDateValue()` / `ScGetTimeValue()` wrapper pair; broader legacy
  retirement has not started
- the full replay corpus now shows strong ambient live-seam traffic, but most
  retained fallback now lives inside the newly admitted scalar utility traffic
- the next dominant live blocker is no longer simple reach; it is the quality
  of logical-fold, round, and related scalar utility support
- no promoted family has moved from observe-only replay reach into a broader
  replay authority lane yet

## Routing Modes

The seam is controlled by
`SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR`.

Supported values:

- `observe`
- `shadow` or `shadowcompare`
- `authority`

When the env var is unset:

- `DBG_UTIL` builds use `observe`
- non-`DBG_UTIL` builds use `off`

This means developer and CI-style debug builds accumulate real seam signal by
default without changing shipping behavior.

## Delegated Family

The currently promoted live evaluator family includes:

- logical literals:
  - `TRUE`
  - `FALSE`
- text parsing:
  - `VALUE`
  - `DATEVALUE`
  - `TIMEVALUE`
  - `NUMBERVALUE`
- scalar utilities:
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
- lookup and index:
  - `MATCH`
  - `XMATCH`
  - `LOOKUP`
  - `VLOOKUP`
  - `HLOOKUP`
  - `XLOOKUP`
  - `INDEX`
- bounded wrappers:
  - `IFERROR(...)` around already-promoted roots
  - `IFNA(...)` around already-promoted roots

The admitted live input surface includes:

- literals
- single-cell references
- single-cell workbook-global names
- single-cell sheet-local names
- bounded scalar expression trees
- bounded single-area workbook-local lookup or index reads whose result stays
  scalar or single-cell

## Live Metrics

Two different denominators matter, and both are now reported.

### North-Star Live Authoritative Match

This is the deletion-gating number for the standing replay corpus:

- `interpret_tail_live_authoritative_corpus_formula_cells=50661`
- `interpret_tail_live_authoritative_probe_formula_cells=7063`
- `interpret_tail_live_authoritative_match_total=4385`
- `interpret_tail_live_authoritative_fallback_total=2678`
- live authoritative-match rate over the corpus: `8.6556%`
- live authoritative-match rate over the current promoted probe: `62.0841%`

Everything below is diagnostic context for improving that number.

### Full Replay Corpus: Ambient Live Observe

This is the honest all-formula live-routing surface from the standing replay
corpus:

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=19190`
- `interpret_tail_live_fallback_total=612`
- `interpret_tail_live_seen_total=19802`
- `interpret_tail_live_unseen_formula_cells=30859`
- `interpret_tail_live_promoted_function_supported_total=17940`
- `interpret_tail_live_supported_rate=37.88`
- `interpret_tail_live_seen_rate=39.09`

Current ambient fallback reasons:

- `unsupported_formula_shape=22`
- `unsupported_host_surface=0`
- `parse_failure=4`
- `unsupported_function=586`

Interpretation:

- the seam now measures the full replay corpus
- the ambient live-routing denominator is real, not curated
- the ambient replay surface now includes large real traffic from
  information predicates, logical folds, round-family formulas, scalar-math
  helpers, and bounded financial feeders
- the imported direct information-predicate host-truth residue is now closed
  on the live surface
- the main remaining ambient work is now parity quality inside those newly
  admitted families, not simple denominator reach
- the live authoritative-match north-star has now moved decisively above the
  first milestone, but these ambient reach numbers are still supporting
  diagnostics rather than a retirement claim

### Full Replay Corpus: Forced Interpret Observe

This is the new full-corpus measurement after explicitly dirtying and forcing
every replay formula cell through Calc's live `Interpret()` path:

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=47189`
- `interpret_tail_forced_interpret_fallback_total=306`
- `interpret_tail_forced_interpret_seen_total=47495`
- `interpret_tail_forced_interpret_unseen_formula_cells=3160`
- `interpret_tail_forced_interpret_promoted_function_supported_total=46562`
- `interpret_tail_forced_interpret_supported_rate=93.15`
- `interpret_tail_forced_interpret_seen_rate=93.75`

Interpretation:

- forced interpret now confirms that a much larger fraction of the corpus
  reaches the seam when explicitly dirtied and interpreted
- the retained forced-interpret fallback is now dominated by formulas outside
  the promoted family plus unsupported utility-family subshapes, not by simple
  replay reach failure

### Promoted-Family Probe

This is the promoted-family Calc-backed probe over the same replay corpus:

- `interpret_tail_probe_formula_cells=7063`
- `interpret_tail_authoritative_total=2628`
- `interpret_tail_authoritative_fallback_total=4435`
- promoted-family authoritative rate: `37.21%`

Current promoted-family fallback reasons:

- `shadow_mismatch=2815`
- `unsupported_function=14`
- `unsupported_formula_shape=10`
- `unsupported_host_surface=6`

### Live-Target Filtered Promoted Probe

This is the same promoted replay probe after excluding rows where live Calc
with the seam forced `off` already disagrees with the imported cached workbook
result:

- `interpret_tail_live_target_probe_formula_cells=0`
- `interpret_tail_probe_host_truth_artifact_formula_cells=7063`
- `interpret_tail_live_target_authoritative_total=0`
- `interpret_tail_live_target_authoritative_fallback_total=0`

Interpretation:

- the promoted-family denominator is now much broader because scalar roots,
  round-family formulas, information predicates, logical folds, `NOT`, and
  bounded scalar-math feeder formulas are part of the delegated family
- the raw promoted probe is now explicitly trading cached-workbook parity for
  live-host parity on imported reference-driven logical/math roots, so a lower
  raw authoritative rate is now compatible with a much stronger north-star
- the raw promoted replay probe is now diagnostic, not the deletion
  denominator
- the dominant remaining raw mismatch buckets are now `math_scalar=172`,
  `logical_fold=53`, `information_predicate=37`, and `lookup=0`, with
  round-family fallback still at `0`
- a focused host-truth test now shows the replay-imported whole-row
  `MATCH([.$B$150];[.$150:.$150];-1)` row evaluates to
  `FormulaError::VariableExpected`
  in live Calc with the seam forced off, so it is not a proven live parity
  blocker even though the replay workbook stores a non-error expected value
- a bounded live-host-truth pass now shows the residual replay-imported
  logical-constant rows are genuine live Calc error rows, not stale cached
  workbook artifacts
- a bounded live-host-truth pass now also shows the replay-imported exact
  `VLOOKUP([.P6];[.$L$2:.$M$8];2;0)` and
  `VLOOKUP(21;[.$AM$2:.$AN$4];2;0)` rows both evaluate to
  `FormulaError::VariableExpected` in Calc with the seam forced off, so those
  cached non-error workbook values are not treated as runtime conversion
  targets
- a bounded live-host-truth pass now also shows the replay-imported
  collation-sensitive exact `VLOOKUP([.M22]; [.L$11:.M$32]; 1; 0)` row
  evaluates to `FormulaError::VariableExpected` in Calc with the seam forced
  off, so the residual replay `VLOOKUP` band is no longer treated as a real
  runtime conversion target
- a bounded live-host-truth pass now also shows the residual replay-imported
  `XLOOKUP("Ireland"; [.H2:.H11]; [.J2:.J11]; "")` and
  `XLOOKUP([.G14]; [.I14:.R14]; [.I15:.R16])` rows both evaluate to
  `FormulaError::VariableExpected` in Calc with the seam forced off, so the
  residual replay `XLOOKUP` band is likewise no longer treated as a real
  runtime conversion target
- a bounded live-host-truth pass now also shows the replay-imported
  `INDEX([.H13:.J19]; XMATCH([.G10]; [.G13:.G19]); XMATCH([.H10]; [.H12:.J12]))`
  row evaluates to `FormulaError::VariableExpected` in Calc with the seam
  forced off, so that cached numeric workbook value is not treated as a real
  runtime conversion target either
- a bounded live-host-truth pass now also shows the replay-imported
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 1)`,
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 2)`, and
  `INDEX(LOGEST([.K11:.O11]; [.K12:.O12]; TRUE(); TRUE()); 2; 0)` rows all
  evaluate to `FormulaError::VariableExpected` in Calc with the seam forced
  off, so that imported `INDEX` matrix-function band is likewise no longer
  treated as a real runtime conversion target
- a bounded live-host-truth pass now also shows the replay-imported
  `DATEVALUE("Jan1, 2015")` rows evaluate to `FormulaError::VariableExpected`
  in Calc with the seam forced off, so those cached non-numeric workbook rows
  are not treated as real live runtime conversion targets either
- a bounded live-host-truth pass now also shows the replay-imported
  `MATCH(1; FREQUENCY([.I126]; [.H129:.M129]); 0)` row evaluates to
  `FormulaError::VariableExpected` in Calc with the seam forced off, so that
  cached workbook non-error row is likewise not treated as a real live runtime
  conversion target
- once the probe is filtered by live host truth, there is currently no
  remaining imported promoted replay surface that behaves like a real seam-off
  legacy runtime target
- the raw promoted replay probe is therefore useful as a cached imported
  correctness surface, but not as the live retirement denominator

### Replay Eligibility Inventory

This is the new per-cell replay inventory over the promoted-family replay
surface after forcing each promoted replay formula through direct live
`Interpret()`:

- `interpret_tail_replay_promoted_formula_cells=7063`
- `interpret_tail_replay_promoted_direct_seen=7063`
- `interpret_tail_replay_promoted_direct_supported=7033`
- `interpret_tail_replay_promoted_direct_fallback=30`
- `interpret_tail_replay_promoted_direct_unseen=0`
- `interpret_tail_replay_promoted_shared_formula_cells=3331`
- `interpret_tail_replay_promoted_shared_top_formula_cells=739`
- `interpret_tail_replay_promoted_shared_member_formula_cells=2592`
- `interpret_tail_replay_promoted_non_shared_formula_cells=3732`
- `interpret_tail_replay_promoted_unseen_shared_top=0`
- `interpret_tail_replay_promoted_unseen_shared_member=0`
- `interpret_tail_replay_promoted_unseen_non_shared=0`
- `interpret_tail_replay_promoted_shared_member_seen_via_top=0`
- `interpret_tail_replay_promoted_needs_interpret_after_dirty=7063`
- `interpret_tail_replay_promoted_dirty_after_interpret=0`

Interpretation:

- the pre-tail replay eligibility blocker is cleared for the promoted replay
  surface
- the latest slice widened the replay-promoted denominator materially by
  promoting bounded scalar-math feeder traffic beneath comparison helpers and
  adding bounded scalar `RATE`
- both shared and non-shared promoted replay formulas now reach the live seam
- the residual replay work is now the `30` direct fallback cells on this
  broader imported promoted-family surface
- the dominant next targets are the remaining logical-fold and math-scalar
  `shadow_mismatch` rows plus the smaller information-predicate and lookup
  raw mismatch bands

## Hard-Routed Family

The env-independent engine-first cluster now includes:

- `TRUE()`
- `FALSE()`
- string-literal `VALUE`
- string-literal `DATEVALUE`
- string-literal `TIMEVALUE`
- literal-only `NUMBERVALUE`
- exact `MATCH(<literal>; <1D literal array>; 0)`
- approximate-ascending `MATCH(<literal>; <ascending numeric 1D literal array>; 1)`
- default-approximate `MATCH(<literal>; <ascending numeric 1D literal array>)`
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

Current meaning:

- the formula is classified and attempted through the engine even when the
  rollout env var is explicitly `off`
- supported results bypass the normal rollout gate and are applied
  authoritatively
- unsupported or projection-failure cases still fall back safely
- `TRUE()` / `FALSE()` now also have an explicit family-local default-on
  rollout path, and their dedicated `ScInterpreter` subroutines are deleted
- the dedicated `ScInterpreter` wrapper pair for string-literal `DATEVALUE` /
  `TIMEVALUE` is also deleted; nested legacy evaluation stays inline at
  dispatch while the existing env-`off` engine-first root slice remains intact
- `ScInterpreter::ScValue()`, `ScInterpreter::ScNumberValue()`, and
  `ScInterpreter::ScMatch()`, `ScInterpreter::ScXMatch()`,
  `ScInterpreter::ScLookup()`, `ScInterpreter::ScVLookup()`,
  `ScInterpreter::ScHLookup()`, `ScInterpreter::ScXLookup()`, and
  `ScInterpreter::ScIndex()` now treat those narrow slices as quarantined
  legacy paths and emit a debug warning if normal interpreter execution
  reaches them

This latest hard-route milestone extends the adjacent default/approximate
`MATCH` plus omitted/approximate extended-match hard-quarantined Calc path
cluster on the migration track and brings the env-independent engine-first
total to `85` slices.

## Scope Policy

The active roadmap is now evaluator migration, not broad substrate widening.

New computational-substrate widening work is out of scope unless it does one
of these things:

- removes an `InterpretTail` live fallback reason
- removes an `InterpretTail` shadow mismatch class
- unlocks required host access for a promoted evaluator family

Residual substrate frontier items that do not satisfy one of those bars are
historical reference material, not active roadmap work.

## Current Next Target

The latest replay closeout is now folded into this migration ledger.

The next high-value pass is now constrained by the scope gate:

1. treat live authoritative-match as the single north-star metric for
   deletion progress
2. treat the raw promoted replay probe as a cached imported correctness
   surface, not as the live retirement denominator
3. do not add new hard-route slices unless they remove a live fallback reason
   or live mismatch bucket
4. keep targeting slices that increase live authoritative-match directly,
   led by the remaining imported live-host-truth buckets in
   `information_predicate`, residual `logical_constant`, and smaller lookup /
   structural residue
5. use the logical-constant and date/time wrapper deletion milestones as the
   template for the next narrow retirement only after `math_scalar` /
   `logical_fold` mismatch reduction pays down more live fallback
6. only return to imported replay parity if we intentionally decide to
   rehabilitate legacy seam-off imported-formula execution

## Historical Archive

The retired pass-by-pass ledger lives in:

- [../archive/interpret_tail/](../archive/interpret_tail/)
