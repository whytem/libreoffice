# Computational Substrate InterpretTail Authoritative Usage Expansion Plan

Status: completed closeout for the ambitious live
`InterpretTail -> engine` authoritative-usage expansion pass

## Purpose

This plan defines the next ambitious evaluator-migration wave after the
completed corpus coverage and mismatch-reduction pass.

The goal is no longer just to shave a few fallback counts off the current
cluster.

The goal is to create a visible step-change in real live delegated usage by
combining three things in one pass:

- repair the highest-volume retained hotspots on the current promoted
  families
- promote the next high-leverage bounded evaluator family
- widen the live seam from root-only delegation into bounded
  wrapper-aware compound delegation

This is intended to be the first evaluator pass whose success is measured as
"substantially more real authoritative usage," not merely "better metrics on
the same lane."

## Why This Pass Next

The current live seam is real and already useful, but the latest corpus
snapshot shows the remaining ceiling clearly:

- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=1045`
- `interpret_tail_authoritative_fallback_total=346`
- `interpret_tail_fallback_unsupported_formula_shape=240`
- `interpret_tail_fallback_shadow_mismatch=90`
- `interpret_tail_fallback_unsupported_host_surface=16`

The retained hotspots are also clear:

- `DATEVALUE`: `27` shadow mismatches
- `LOOKUP`: `217` unsupported-shape fallbacks and `45` shadow mismatches
- `VLOOKUP`: `8` unsupported-host-surface fallbacks and `9` shadow
  mismatches
- `INDEX`: `4` unsupported-shape fallbacks and `5`
  unsupported-host-surface fallbacks

At the same time, the standalone engine already owns more nearby capability
than the live seam currently exposes:

- `XLOOKUP` exists on the engine side
- lookup and reference planning are already richer than the current live
  seam surface
- compiler lowering already recognizes bounded wrapper families such as
  `IFERROR` and `IFNA`

So the best next move is not another narrow cleanup pass and not another
tiny function-only promotion.

The best next move is one combined authority-usage expansion wave that:

- fixes the current hotspot debt
- adds at least one new high-value family
- and turns wrapper-contained promoted roots into real live authority,
  not just top-level roots

## Frozen Starting Point

This pass starts from the completed corpus coverage and mismatch-reduction
snapshot:

- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=1045`
- `interpret_tail_authoritative_fallback_total=346`
- `interpret_tail_fallback_unsupported_formula_shape=240`
- `interpret_tail_fallback_shadow_mismatch=90`
- `interpret_tail_fallback_unsupported_host_surface=16`

Per promoted family, the current completed snapshot is:

- `VALUE`: `14 / 15`
- `DATEVALUE`: `4 / 31`
- `TIMEVALUE`: `8 / 8`
- `NUMBERVALUE`: `9 / 9`
- `MATCH`: `101 / 117`
- `XMATCH`: `24 / 33`
- `LOOKUP`: `550 / 815`
- `VLOOKUP`: `294 / 313`
- `HLOOKUP`: `39 / 39`
- `INDEX`: `2 / 11`

This baseline is frozen in:

- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_COVERAGE_AND_MISMATCH_REDUCTION_PLAN.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_COVERAGE_AND_MISMATCH_REDUCTION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_COVERAGE_AND_MISMATCH_REDUCTION_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_COVERAGE_AND_MISMATCH_REDUCTION_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_COVERAGE_AND_MISMATCH_REDUCTION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_COVERAGE_AND_MISMATCH_REDUCTION_EVIDENCE.md)

## Closeout Result

This pass is now complete.

It widened the live seam materially, but it closed as a mixed-result pass
rather than a full success against the original ambitious targets.

The completed rerun freezes:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1126`
- `interpret_tail_authoritative_fallback_total=362`
- `interpret_tail_fallback_unsupported_formula_shape=243`
- `interpret_tail_fallback_shadow_mismatch=97`
- `interpret_tail_fallback_unsupported_host_surface=22`

The main accepted wins are:

- bounded live `XLOOKUP` is now authoritative on the corpus
- bounded wrapper-aware delegation is now live and unit-proven
- the live probe-covered promoted-family surface grew beyond the old `1391`
  ceiling
- authoritative usage rose materially from `1045` to `1126`

The main retained misses are:

- total fallback did not fall to the planned target
- `LOOKUP` unsupported-shape fallout remains the dominant residual hotspot
- `DATEVALUE` mismatch remains the dominant residual mismatch hotspot

## Strategic Objective

Turn the current bounded live evaluator seam from "useful but still narrow"
into a materially broader authoritative lane that can plausibly retire more
real `ScInterpreter` traffic.

This pass should succeed only if it expands both:

- the number of corpus formulas that the live seam can meaningfully attempt
- the number of those attempts that close authoritatively rather than as
  fallback

## Target Expansion Envelope

This pass is intentionally ambitious and should be treated as a combined
capability wave, not a one-hotspot patch.

### 1. Hotspot Repair On Current Families

Repair the highest-volume retained blockers on the current promoted set:

- bounded `DATEVALUE` mismatch repair
- residual `LOOKUP` unsupported-shape reduction
- residual `LOOKUP` / `VLOOKUP` mismatch cleanup
- bounded `INDEX` host-surface cleanup where the result remains scalar

### 2. New Promoted Family: `XLOOKUP`

Promote bounded live `XLOOKUP` delegation on the same live seam.

Recommended admitted boundary:

- same workbook only
- single-area lookup and result ranges only
- scalar lookup key only
- scalar or single-cell result only
- no spill, slice, or matrix-return surfaces
- bounded `if_not_found`, match mode, and search mode support only where the
  existing engine runtime already closes cleanly

Stretch if the same runtime closes without widening the result surface:

- bounded nested `XLOOKUP`
- bounded text lookup and regex modes already covered by engine tests

### 3. Wrapper-Aware Compound Delegation

Add bounded authority for formulas whose promoted evaluator root is wrapped
inside a small stable scalar wrapper family.

Recommended wrapper boundary:

- `IFERROR`
- `IFNA`
- harmless parentheses
- bounded unary scalar wrappers

The target is not broad expression evaluation.
The target is formulas where the engine-owned promoted root remains the real
semantic center and the wrapper only chooses between a scalar primary result
and a scalar fallback.

This is the main route for growing `interpret_tail_probe_formula_cells`
beyond the flat `1391` top-level-root ceiling.

## Success Criteria

This pass is complete only if all of the following are true:

- `interpret_tail_probe_formula_cells >= 1800`
- `interpret_tail_authoritative_total >= 1400`
- `interpret_tail_authoritative_fallback_total <= 250`
- `interpret_tail_fallback_unsupported_formula_shape <= 125`
- `interpret_tail_fallback_shadow_mismatch <= 55`
- `interpret_tail_fallback_unsupported_host_surface <= 10`
- `DATEVALUE` shadow mismatch falls from `27` to `10` or below
- `LOOKUP` unsupported-shape fallout falls from `217` to `120` or below
- at least one newly promoted family shows non-zero authoritative corpus
  usage:
  - `XLOOKUP`
  - or a bounded wrapper-aware delegated family such as `IFERROR` / `IFNA`
- the standing standalone replay guardrail remains exact:
  - `500` workbooks
  - `50,661` formula cells
  - `50,652` parsed formulas
  - `0` cached-fallback cells
  - `0` cached-fallback rate

Stretch outcome:

- `interpret_tail_probe_formula_cells >= 2000`
- `interpret_tail_authoritative_total >= 1550`
- both `XLOOKUP` and wrapper-aware delegation show non-zero authoritative
  corpus usage

## Success Criteria Outcome

The pass did not satisfy the full original completion bar.

What did close:

- `interpret_tail_probe_formula_cells` rose from `1391` to `1488`
- `interpret_tail_authoritative_total` rose from `1045` to `1126`
- `XLOOKUP` now shows non-zero authoritative corpus usage:
  authoritative `78`, fallback `19`, attempts `97`
- wrapper-aware delegation is now live and unit-proven on the widened seam
- the standing standalone replay guardrail remained exact

What did not close:

- `interpret_tail_probe_formula_cells >= 1800`
- `interpret_tail_authoritative_total >= 1400`
- `interpret_tail_authoritative_fallback_total <= 250`
- `interpret_tail_fallback_unsupported_formula_shape <= 125`
- `interpret_tail_fallback_shadow_mismatch <= 55`
- `interpret_tail_fallback_unsupported_host_surface <= 10`
- `DATEVALUE` mismatch `<= 10`
- `LOOKUP` unsupported-shape `<= 120`

So the correct read is: real widening success, incomplete hotspot cleanup.

## Non-Goals

Still out of scope for this wave:

- broad default-on AutoCalc authority
- external references, DDE, macros, add-ins, or environment-sensitive
  execution
- matrix, spill, slice-valued, or broad multi-cell result projection
- broad workbook-wide evaluator replacement
- unrelated computational-substrate widening

## Final Closeout

The final closeout set for this pass is now:

- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_EVIDENCE.md)

## Primary Engineering Surfaces

- [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
- [LookupExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx)
- [TextParsingExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx)
- [WorkbookCompilerLowering.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/WorkbookCompilerLowering.hxx)
- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
- [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)
- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)

## Workstreams

### 1. Freeze A Combined Authority Contract

Freeze one contract for the whole wave, not three unrelated mini-passes.

That contract should define:

- the bounded `DATEVALUE` parity surface
- the bounded residual lookup and index cleanup surface
- the bounded live `XLOOKUP` surface
- the bounded wrapper-aware delegation surface
- explicit fallback reasons for each retained boundary

Required result:

- one execution contract for a combined authoritative-usage expansion wave

### 2. Repair `DATEVALUE` Live Semantics

Attack the largest current mismatch hotspot directly.

Priority surfaces:

- month-name and punctuation normalization
- text materialization from referenced cells and names
- whitespace and empty-text normalization
- bounded serial conversion parity against Calc on the admitted live seam

Required result:

- `DATEVALUE` mismatch becomes a bounded residual edge case rather than the
  dominant retained mismatch family

### 3. Generalize Lookup And Index Shape Support

Reduce the remaining unsupported-shape and host-surface fallout on the
current lookup cluster.

Priority shapes:

- additional bounded vector and matrix source forms
- bounded row/column coercions that still project to one scalar result
- scalar-projected `INDEX` variants that already close in standalone engine
- residual approximate-match and projection shapes that currently fall back
  even though the engine runtime already knows the answer

Required result:

- `LOOKUP`, `VLOOKUP`, and `INDEX` all show materially cleaner corpus usage
  on the current promoted surface

### 4. Promote Bounded `XLOOKUP`

Turn the existing standalone `XLOOKUP` capability into a real live
`InterpretTail` authority family.

Priority scope:

- exact and bounded approximate modes
- text and numeric keys
- bounded `if_not_found`
- no slice or spill return

Required result:

- non-zero authoritative corpus usage for live `XLOOKUP`

### 5. Add Wrapper-Aware Compound Delegation

Teach the live seam to keep authority when a promoted root sits inside a
small stable scalar wrapper family.

Priority wrappers:

- `IFERROR`
- `IFNA`
- harmless parentheses
- bounded unary wrappers

Required result:

- the probe can measure wrapper-contained promoted roots
- at least one wrapper-contained delegated family closes authoritatively on
  the corpus or live focused tests

### 6. Strengthen Corpus Accounting

The next pass needs better visibility than simple family totals.

Add bounded accounting for:

- top wrapper contributors
- top newly promoted-family contributors
- per-family unsupported-shape and mismatch leaders after the new wave lands

Required result:

- the rerun can explain where the step-change came from

### 7. Add Focused Live Proof

Minimum proof set:

- helper-side parity proof for repaired `DATEVALUE`
- live authority proof for widened lookup and index shapes
- live authority proof for bounded `XLOOKUP`
- live authority or shadow proof for wrapper-aware delegation
- retained-fallback proof for still-deferred spill, slice, or external
  shapes

Required result:

- the live evaluator wave is backed by direct positive and retained-boundary
  proof, not just corpus totals

## Recommended Sequencing

### Phase 0. Baseline Freeze

- preserve the current `1391 / 1045 / 346 / 240 / 90 / 16` starting point

### Phase 1. Contract And Inventory

- freeze the combined wave contract
- freeze the dominant retained contributors by family and reason

### Phase 2. `DATEVALUE` Parity Repair

- clear the biggest live mismatch hotspot first

### Phase 3. Lookup/Index Cleanup

- reduce unsupported-shape and host-surface fallout on the current cluster

### Phase 4. `XLOOKUP` Promotion

- add the first new high-value live family in this wave

### Phase 5. Wrapper-Aware Delegation

- widen beyond top-level promoted roots

### Phase 6. Corpus Rerun And Closeout

- rerun the Calc-backed corpus
- freeze the new totals
- update the roadmap surfaces

## Completion Bar

This plan should not close as "better diagnostics" or "a little less
fallback on the same roots."

It closes only when the pass delivers a real step-change in live
authoritative usage through some combination of:

- materially more probe-covered promoted-family formulas
- materially more authoritative routes
- materially fewer unsupported-shape fallbacks
- materially fewer mismatches
- and at least one new or wrapper-aware delegated family on the board
