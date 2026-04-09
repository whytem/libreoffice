# Computational Substrate InterpretTail Lookup Residual Conversion Plan

Status: active next-pass plan for the focused post-hotspot-conversion
`InterpretTail -> engine` lookup-residual wave

## Purpose

This plan defines the next evaluator pass after the completed
hotspot-conversion wave.

The previous pass moved the live seam in a real way:

- `DATEVALUE` stopped being a dominant mismatch family
- total fallback and total shadow mismatch both moved down materially
- the same measured probe surface now closes more authoritatively than
  before

But the pass also clarified that the main retained bottleneck is now narrow,
not broad.

It is no longer a generic "grow authority usage" problem.
It is now a lookup-residual conversion problem.

So this pass is intentionally focused on the highest-volume remaining
contributors on the already-promoted lookup/index lane.

## Why This Pass Next

The current completed rerun now freezes:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1156`
- `interpret_tail_authoritative_fallback_total=332`
- `interpret_tail_fallback_unsupported_formula_shape=241`
- `interpret_tail_fallback_shadow_mismatch=72`
- `interpret_tail_fallback_unsupported_host_surface=19`

The retained hotspot is now explicit:

- `LOOKUP`: `215` unsupported-shape fallbacks and `45` shadow mismatches

The next smaller residuals are also clear:

- `VLOOKUP`: `8` unsupported-host-surface fallbacks and `7` shadow
  mismatches
- `XLOOKUP`: `6` unsupported-host-surface fallbacks and `10` shadow
  mismatches
- `INDEX`: `4` unsupported-shape fallbacks and `5`
  unsupported-host-surface fallbacks

That means the next best move is not another new family promotion.
The next best move is to convert the remaining lookup-heavy fallout on the
existing promoted families.

## Strategic Objective

Raise authoritative usage on the current probe-covered surface by reducing
the largest retained lookup-family failure classes:

- `LOOKUP` `2D` data-only unsupported-shape fallout
- `LOOKUP` scalar-projection mismatch fallout
- residual `VLOOKUP` and `XLOOKUP` host-surface exclusions
- optional bounded `INDEX` scalar-projection cleanup if it closes on the
  same runtime seam

The key idea is:

- keep the same live seam
- keep the same promoted-family cluster
- convert more of the measured lookup/index surface into exact
  authoritative routes

## Frozen Starting Point

This pass starts from the completed hotspot-conversion snapshot:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1156`
- `interpret_tail_authoritative_fallback_total=332`
- `interpret_tail_fallback_unsupported_formula_shape=241`
- `interpret_tail_fallback_shadow_mismatch=72`
- `interpret_tail_fallback_unsupported_host_surface=19`

Per promoted family, the starting snapshot is:

- `VALUE`: `14 / 15`
- `DATEVALUE`: `29 / 31`
- `TIMEVALUE`: `8 / 8`
- `NUMBERVALUE`: `9 / 9`
- `MATCH`: `102 / 117`
- `XMATCH`: `24 / 33`
- `LOOKUP`: `555 / 815`
- `VLOOKUP`: `296 / 313`
- `HLOOKUP`: `39 / 39`
- `XLOOKUP`: `78 / 97`
- `INDEX`: `2 / 11`

This baseline is frozen in:

- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_PLAN.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_EVIDENCE.md)

## Recommended Scope

### 1. Freeze A Retained `LOOKUP` Inventory

Before touching runtime behavior, freeze the exact current contributor list
for:

- `LOOKUP` unsupported-shape rows
- `LOOKUP` shadow-mismatch rows
- `VLOOKUP` unsupported-host-surface rows
- `XLOOKUP` unsupported-host-surface rows
- optional `INDEX` residual rows if they can close on the same seam

Required result:

- one concrete retained-row inventory tied to the current corpus logs

### 2. `LOOKUP` `2D` Data-Only Shape Conversion

Attack the largest single retained bucket directly.

Bounded target surface:

- `LOOKUP(value; one-argument 2D data area)` and equivalent bounded source
  forms where Calc still returns one scalar answer
- scalar-preserving `2D` area reads that can be lowered onto the existing
  lookup execution model without spill or slice semantics
- no broad matrix-return or multi-cell projection promise

Exit condition for this lane:

- materially reduce the current `LOOKUP` unsupported-shape bucket

### 3. `LOOKUP` Scalar-Projection Parity

After shape conversion, focus on supported attempts that still mismatch.

Bounded target surface:

- retained scalar result projection on `LOOKUP` rows where Calc and the
  engine are already close
- bounded text-vs-number and empty-vs-error parity on the diagnosed rows
- no broad coercion-theory rewrite beyond the corpus-backed rows

Exit condition for this lane:

- convert a meaningful fraction of the current `LOOKUP` mismatch rows into
  authoritative routes

### 4. `VLOOKUP` And `XLOOKUP` Host-Surface Cleanup

The next leverage after `LOOKUP` is the residual host-surface exclusions on
already-promoted lookup families.

Bounded target surface:

- scalar result rows where Calc exposes one ordinary scalar answer and the
  engine already has the semantic answer
- bounded missing-value, empty-result, and typed-result projection rows from
  the current corpus diagnostics
- no spill, slice, or multi-area broadening

Exit condition for this lane:

- reduce the residual host-surface bucket for `VLOOKUP` and `XLOOKUP`

### 5. Optional `INDEX` Scalar Cleanup

Attempt only if it closes on the same lookup projection surface without
opening a new matrix or spill class.

Bounded target surface:

- scalar `INDEX` matrix selection and projection on rows already visible in
  the current corpus

Required result:

- either bounded scalar `INDEX` improvement with proof
- or an explicit retained reject recorded during closeout

## Success Criteria

This pass is complete only if all of the following are true:

- `interpret_tail_authoritative_total >= 1215`
- `interpret_tail_authoritative_fallback_total <= 275`
- `interpret_tail_fallback_unsupported_formula_shape <= 180`
- `interpret_tail_fallback_shadow_mismatch <= 58`
- `interpret_tail_fallback_unsupported_host_surface <= 12`
- `LOOKUP` unsupported-shape fallout falls from `215` to `150` or below
- `LOOKUP` shadow mismatch falls from `45` to `30` or below
- combined `VLOOKUP` + `XLOOKUP` unsupported-host-surface fallout falls from
  `14` to `6` or below
- the standing standalone replay guardrail remains exact:
  - `500` workbooks
  - `50,661` formula cells
  - `50,652` parsed formulas
  - `0` cached-fallback cells
  - `0` cached-fallback rate

Stretch outcome:

- `interpret_tail_authoritative_total >= 1250`
- `interpret_tail_authoritative_fallback_total <= 250`
- `LOOKUP` unsupported-shape fallout falls to `125` or below
- `LOOKUP` mismatch stops being the dominant live evaluator hotspot

## Non-Goals

Still out of scope for this wave:

- broad new family promotion beyond optional bounded `INDEX` cleanup
- broad matrix-return, spill, or slice-valued lookup/index surfaces
- ambient AutoCalc traffic claims
- external references, macros, add-ins, or environment-sensitive execution
- broad workbook-wide evaluator replacement
- broad coercion-generalization beyond the rows diagnosed on the corpus

## Primary Engineering Surfaces

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
- [LookupExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx)
- [FormulaInspectionExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/FormulaInspectionExecution.hxx)
- [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)
- [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)

## Workstreams

### 1. Freeze Diagnostic Inventory

Before touching runtime behavior, freeze the exact current contributor list
for the retained `LOOKUP`, `VLOOKUP`, `XLOOKUP`, and optional `INDEX`
rows.

Required result:

- one concrete retained-row inventory tied to the current corpus logs

### 2. `LOOKUP` `2D` Shape Pass

Implement only the highest-volume scalar-preserving `2D` `LOOKUP` rows
confirmed by the frozen inventory.

Required result:

- unit proof for each newly-authoritative `LOOKUP` shape
- measurable reduction in the `LOOKUP` unsupported-shape bucket

### 3. `LOOKUP` Projection Parity Pass

Close the highest-volume retained mismatch rows that remain after shape
conversion.

Required result:

- per-family mismatch totals move down on the corpus rerun

### 4. Residual Host-Surface Cleanup

Close the highest-volume retained `VLOOKUP` / `XLOOKUP` host-surface rows
that already sit on the promoted scalar lane.

Required result:

- per-family host-surface totals move down on the corpus rerun

### 5. Optional `INDEX` Cleanup

Attempt only if the same runtime surface closes cleanly.

Required result:

- either bounded scalar `INDEX` improvement with proof
- or an explicit retained reject recorded during closeout

### 6. Proof, Corpus Rerun, And Closeout

Validation must include:

- targeted `ucalc_shared_cases` helper proof
- targeted `ucalc_formula2` live authority proof
- Calc-backed corpus rerun with stats
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Deliverables

This pass should close with:

- this plan updated from active to completed
- one decision record
- one evidence summary
- refreshed metrics in [COMPUTATIONAL_SUBSTRATE_MASTER.md](COMPUTATIONAL_SUBSTRATE_MASTER.md)
- refreshed metrics in [../PROJECT_STATUS.md](../PROJECT_STATUS.md)

The desired shape is:

- one runtime commit
- one docs closeout commit

## Recommended Reading Order For The Pass

1. [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_DECISION_RECORD.md)
2. [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_EVIDENCE.md)
3. this plan
