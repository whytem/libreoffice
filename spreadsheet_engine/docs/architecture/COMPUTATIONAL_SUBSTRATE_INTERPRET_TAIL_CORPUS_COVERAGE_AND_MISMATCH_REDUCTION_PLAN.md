# Computational Substrate InterpretTail Corpus Coverage And Mismatch Reduction Plan

Status: active next-pass plan for increasing promoted-family corpus coverage
and reducing retained fallback hotspots

## Purpose

This plan defines the next focused evaluator-migration pass after the
completed first non-zero authoritative replay-corpus usage wave.

The immediate goal is not another broad function-family expansion.

The immediate goal is to make the already-promoted evaluator cluster matter
more on the Calc-backed corpus by:

- increasing `interpret_tail_probe_formula_cells`
- converting more of those probe attempts into authoritative routes
- debugging and reducing the retained
  `unsupported_formula_shape=295` surface
- debugging and reducing the retained `shadow_mismatch=85` surface

## Why This Pass Next

The previous pass removed the old source-bridge blocker and proved the
already-landed function families do achieve real authoritative corpus usage.

But the new baseline also made the remaining costs visible:

- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=996`
- `interpret_tail_authoritative_fallback_total=395`
- `interpret_tail_fallback_unsupported_formula_shape=295`
- `interpret_tail_fallback_shadow_mismatch=85`
- `interpret_tail_fallback_unsupported_host_surface=15`

That means the immediate blocker is no longer “can the live seam reach the
engine parser?” It can.

The immediate blocker is now:

- too little of the promoted-family surface is being counted in the focused
  corpus probe
- too much of the counted surface still closes as unsupported-shape or
  mismatch

This is the highest-leverage next pass because it improves real migration
signal on the families that are already live before the program spends effort
on new capability families.

## Frozen Starting Point

The frozen baseline for this pass is the completed authoritative corpus
snapshot from:

- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_PLAN.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_PLAN.md)

The frozen metrics are:

- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=996`
- `interpret_tail_authoritative_fallback_total=395`
- `interpret_tail_fallback_unsupported_formula_shape=295`
- `interpret_tail_fallback_shadow_mismatch=85`
- `interpret_tail_fallback_unsupported_host_surface=15`

Per-family authoritative support currently closes at:

- `VALUE`: `13 / 15`
- `DATEVALUE`: `4 / 31`
- `TIMEVALUE`: `8 / 8`
- `NUMBERVALUE`: `9 / 9`
- `MATCH`: `91 / 117`
- `XMATCH`: `22 / 33`
- `LOOKUP`: `520 / 815`
- `VLOOKUP`: `291 / 313`
- `HLOOKUP`: `36 / 39`
- `INDEX`: `2 / 11`

The most obvious retained hotspots are:

- `LOOKUP` on raw count
- `DATEVALUE` on support rate
- `INDEX` on support rate
- `XMATCH` on support rate

## Strategic Objective

Turn the new authoritative corpus probe from a proof-of-life metric into a
sharper migration guide for the current evaluator cluster.

This pass should improve both:

- coverage:
  more promoted-family cells should be recognized and counted by the probe
- quality:
  more counted cells should close authoritatively instead of as unsupported
  shape or mismatch

## Success Criteria

This pass is complete only if all of the following are true:

- `interpret_tail_probe_formula_cells > 1391`
- `interpret_tail_authoritative_total > 996`
- `interpret_tail_fallback_unsupported_formula_shape < 295`
- `interpret_tail_fallback_shadow_mismatch < 85`
- at least two of the current weak hotspots show measurable improvement:
  - `DATEVALUE`
  - `INDEX`
  - `XMATCH`
  - `LOOKUP`
- the standing standalone replay guardrail remains exact:
  - `500` workbooks
  - `50,661` formula cells
  - `50,652` parsed formulas
  - `0` cached-fallback cells
  - `0` cached-fallback rate

Stretch outcome:

- `interpret_tail_probe_formula_cells >= 1600`
- `interpret_tail_authoritative_total >= 1100`
- the retained unsupported-shape and mismatch buckets both fall by at least
  twenty percent

## Non-Goals

Out of scope for this pass:

- broad new function-family promotion such as `XLOOKUP`
- matrix, spill, slice-valued, or multi-cell result surfaces
- external references, add-ins, macros, DDE, and environment-sensitive
  families
- broad default-on `InterpretTail` delegation
- substrate widening unrelated to the live evaluator migration surface

## Preferred Strategy

Prefer hotspot reduction on the current promoted families over breadth
expansion.

The best outcome is not “add one more supported function.”
The best outcome is:

- recognize more of the current promoted-family surface
- explain the remaining unsupported-shape and mismatch fallout precisely
- fix the highest-volume, lowest-risk contributors first

That should create a better basis for the next capability-wave expansion than
adding a new family on top of noisy metrics.

## Workstreams

### 1. Freeze Contributor Inventory

Make the retained fallback surface more legible before changing behavior.

Add bounded corpus accounting that can answer:

- which promoted families contribute most to `unsupported_formula_shape`
- which promoted families contribute most to `shadow_mismatch`
- which concrete normalized top-level shapes recur most often
- which workbook files contribute repeated examples

The accounting should stay bounded and opt-in.

Required result:

- one frozen breakdown of the top unsupported-shape and mismatch
  contributors, not just the aggregate totals

### 2. Increase Probe-Covered Surface

The probe should count more real promoted-family cells without broadening the
actual authority contract yet.

Likely directions:

- classify from the live Calc formula source when workbook-model formula text
  is too lossy
- accept bounded wrapper forms around promoted roots:
  - harmless parentheses
  - bounded array wrappers
  - namespace aliases
  - bounded unary wrappers that still collapse to the same promoted family
- normalize the probe's pre-classification path the same way the live seam
  normalizes authoritative attempts

Required result:

- more promoted-family corpus cells are measured in
  `interpret_tail_probe_formula_cells`

### 3. Reduce Unsupported-Shape Fallout

Attack the retained `unsupported_formula_shape` bucket as a current-family
problem rather than a generic parser problem.

Priority targets:

- `LOOKUP`-class wrapper shapes
- `DATEVALUE` wrapper or coercion shapes
- `INDEX` root-shape variants
- `XMATCH` namespaced or compatibility aliases that still fit the bounded
  current authority surface

Required result:

- the retained unsupported-shape bucket falls below the frozen `295`
  baseline

### 4. Triage Shadow Mismatch By Semantic Class

Make `shadow_mismatch` actionable rather than opaque.

Add bounded samples or buckets that can separate likely mismatch families,
for example:

- date serial or text-parsing coercion differences
- lookup-match-mode differences
- approximate-match ordering differences
- string-vs-number projection differences
- error-propagation differences

Priority triage order:

1. `LOOKUP`
2. `DATEVALUE`
3. `INDEX`
4. `XMATCH`

Required result:

- at least one concrete mismatch contributor is explained and reduced, not
  merely reclassified

### 5. Add Focused Live Proof

Add or widen tests that prove both the coverage increase and the semantic
fixes on real Calc-backed examples.

Minimum proof set:

- one helper-side test for new probe/source normalization
- one corpus-runner assertion on increased probe coverage
- one or more targeted live tests for repaired mismatch or unsupported-shape
  examples
- one retained-fallback test proving out-of-contract shapes still fall back
  cleanly

Required result:

- the live proof covers both positive reductions and retained boundaries

### 6. Re-Run The Corpus And Freeze The New Hotspot Baseline

Re-run:

- `CppunitTest_sc_interpret_tail_corpus` with
  `SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_STATS=1`

Close the pass only if the rerun shows both:

- more probe-covered promoted-family cells
- lower unsupported-shape and mismatch fallout

Required result:

- a new frozen baseline that is strictly better than the current one on both
  coverage and at least one retained hotspot bucket

## Recommended Sequencing

### Phase 0. Baseline Freeze

- preserve the current `1391 / 996 / 295 / 85` starting point

### Phase 1. Contributor Inventory

- freeze the top unsupported-shape and mismatch contributors

### Phase 2. Probe Coverage Increase

- widen the bounded promoted-family detection path so more relevant cells are
  counted

### Phase 3. Unsupported-Shape Reduction

- remove the highest-volume bounded wrapper or root-shape contributors

### Phase 4. Mismatch Reduction

- fix the clearest semantic mismatch contributor on the current cluster

### Phase 5. Corpus Rerun And Closeout

- re-run the corpus
- freeze the new metrics
- update roadmap docs

## Completion Bar

This plan should not close as “better diagnostics” or “more samples.”

It closes only when the completed corpus rerun shows:

- more measured promoted-family cells
- more authoritative routes
- less unsupported-shape fallout
- less mismatch fallout on at least one real promoted-family hotspot
