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
- one tiny family is now engine-first even with rollout set to `off`:
  literal-only `NUMBERVALUE`
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
- the latest bounded replay slice therefore moved the remaining imported
  named-range host-surface band to zero and tightened the residual frontier
  down to parity and shape cleanup inside already-promoted families

What is still not true:

- no broad default-on production rollout exists
- no `ScInterpreter` subroutine has been deleted yet
- the full replay corpus now shows material ambient live-seam traffic
- the full replay corpus still reaches only a bounded minority of formulas
- residual replay-live fallback is now concentrated in promoted-family
  `shadow_mismatch`, with residual `unsupported_formula_shape` secondary
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

### Full Replay Corpus: Ambient Live Observe

This is the honest all-formula live-routing surface from the standing replay
corpus after the completed lookup-value scalarization slice:

- `interpret_tail_live_formula_cells=50661`
- `interpret_tail_live_supported_total=4202`
- `interpret_tail_live_fallback_total=24`
- `interpret_tail_live_seen_total=4226`
- `interpret_tail_live_unseen_formula_cells=46435`
- `interpret_tail_live_promoted_function_supported_total=2952`
- `interpret_tail_live_supported_rate=8.29`
- `interpret_tail_live_seen_rate=8.34`

Current ambient fallback reasons:

- `unsupported_formula_shape=24`
- `unsupported_host_surface=0`
- `parse_failure=0`

Interpretation:

- the seam now measures the full replay corpus
- the ambient live-routing denominator is real, not curated
- the ambient replay surface now includes material promoted-family function
  traffic
- the pre-RPN observe bridge converted the replay-promoted reach problem into a
  residual family-fallback problem
- the latest slice held the ambient denominator steady while converting more
  replay lookup-family traffic inside it
- the main remaining ambient replay work is now parity cleanup inside
  already-promoted families

### Full Replay Corpus: Forced Interpret Observe

This is the new full-corpus measurement after explicitly dirtying and forcing
every replay formula cell through Calc's live `Interpret()` path:

- `interpret_tail_forced_interpret_formula_cells=50661`
- `interpret_tail_forced_interpret_supported_total=2101`
- `interpret_tail_forced_interpret_fallback_total=12`
- `interpret_tail_forced_interpret_seen_total=2113`
- `interpret_tail_forced_interpret_unseen_formula_cells=48548`
- `interpret_tail_forced_interpret_promoted_function_supported_total=1476`
- `interpret_tail_forced_interpret_supported_rate=4.15`
- `interpret_tail_forced_interpret_seen_rate=4.17`

Interpretation:

- the replay barrier is no longer “promoted families never reach live
  interpretation”
- forced interpret now shows real promoted-family reach on replay-imported
  formulas
- the remaining replay blocker is now narrower parity and residual
  formula-shape cleanup on that imported surface, not host-surface
  eligibility

### Promoted-Family Probe

This is the promoted-family Calc-backed probe over the same replay corpus:

- `interpret_tail_probe_formula_cells=1812`
- `interpret_tail_authoritative_total=1774`
- `interpret_tail_authoritative_fallback_total=38`
- promoted-family authoritative rate: `97.90%`

Current promoted-family fallback reasons:

- `unsupported_formula_shape=12`
- `shadow_mismatch=26`
- `unsupported_host_surface=0`

Interpretation:

- once the probe hits promoted-family cells, authority conversion is now
  strong
- the main remaining conversion work is no longer generic breadth
- the latest slice reduced promoted-family fallback from `47` to `38`
  while cutting `unsupported_formula_shape` from `22` to `12`
- the current hotspots are now `VLOOKUP` mismatch plus
  `MATCH` / `XMATCH` parity and shape cleanup, with smaller `INDEX`
  shape fallout behind them

### Replay Eligibility Inventory

This is the new per-cell replay inventory over the promoted-family replay
surface after forcing each promoted replay formula through direct live
`Interpret()`:

- `interpret_tail_replay_promoted_formula_cells=1812`
- `interpret_tail_replay_promoted_direct_seen=1812`
- `interpret_tail_replay_promoted_direct_supported=1800`
- `interpret_tail_replay_promoted_direct_fallback=12`
- `interpret_tail_replay_promoted_direct_unseen=0`
- `interpret_tail_replay_promoted_shared_formula_cells=395`
- `interpret_tail_replay_promoted_shared_top_formula_cells=83`
- `interpret_tail_replay_promoted_shared_member_formula_cells=312`
- `interpret_tail_replay_promoted_non_shared_formula_cells=1417`
- `interpret_tail_replay_promoted_unseen_shared_top=0`
- `interpret_tail_replay_promoted_unseen_shared_member=0`
- `interpret_tail_replay_promoted_unseen_non_shared=0`
- `interpret_tail_replay_promoted_shared_member_seen_via_top=0`
- `interpret_tail_replay_promoted_needs_interpret_after_dirty=1812`
- `interpret_tail_replay_promoted_dirty_after_interpret=0`

Interpretation:

- the pre-tail replay eligibility blocker is cleared for the promoted replay
  surface
- the latest slice improved parity and shape coverage inside that surface
  rather than widening its denominator
- both shared and non-shared promoted replay formulas now reach the live seam
- the residual replay work is now the `12` direct fallback cells on this
  imported promoted-family surface
- the dominant next targets are now `VLOOKUP` parity first, then
  `MATCH` / `XMATCH` parity and `unsupported_formula_shape`, with
  smaller `INDEX` shape fallout and residual `LOOKUP` / `XLOOKUP`
  mismatch rows behind them

## Hard-Routed Family

The first env-independent engine-first family is now live:

- literal-only `NUMBERVALUE`

Current meaning:

- the formula is classified and attempted through the engine even when the
  rollout env var is explicitly `off`
- supported results bypass the normal rollout gate and are applied
  authoritatively
- unsupported or projection-failure cases still fall back safely

This is the first hard-quarantined Calc path on the migration track.

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

The next high-value pass should now stay on promoted-family parity cleanup,
starting with the still-dominant `LOOKUP` replay rows:

1. convert the formula-backed `2D` `LOOKUP` replay rows that still surface
   `error:0` instead of the live text result
2. convert the remaining range-backed short-result `LOOKUP` rows that still
   mismatch on `OUT OF BOUND`-style semantics
3. then convert residual `XLOOKUP` and `VLOOKUP` mismatch rows
4. clean up the remaining replay-imported `VLOOKUP` / `INDEX`
   `unsupported_host_surface` and `unsupported_formula_shape` fallout
5. only return to broader reach work if the promoted replay surface regresses

## Historical Archive

The retired pass-by-pass ledger lives in:

- [../archive/interpret_tail/](../archive/interpret_tail/)
