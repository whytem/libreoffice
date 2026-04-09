# Computational Substrate InterpretTail Hotspot Conversion Plan

Status: active next-pass plan for the focused post-expansion
`InterpretTail -> engine` hotspot-conversion wave

## Purpose

This plan defines the next evaluator pass after the completed
authoritative-usage expansion wave.

The previous pass widened the live seam materially:

- bounded `XLOOKUP` is now authoritative on the Calc-backed corpus
- wrapper-aware delegation for `IFERROR` / `IFNA` is now live and
  unit-proven
- the probe-covered promoted-family surface rose beyond the old `1391`
  ceiling

But it did not convert the dominant retained hotspots into a lower-fallback
lane.

So this pass is intentionally narrower.

It is not another broad capability-wave plan.
It is a targeted hotspot-conversion pass whose goal is to turn more of the
already-measured live seam into exact authoritative routes.

## Why This Pass Next

The current completed rerun now freezes:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1126`
- `interpret_tail_authoritative_fallback_total=362`
- `interpret_tail_fallback_unsupported_formula_shape=243`
- `interpret_tail_fallback_shadow_mismatch=97`
- `interpret_tail_fallback_unsupported_host_surface=22`

The retained hotspots are now explicit and concentrated:

- `DATEVALUE`: `27` shadow mismatches
- `LOOKUP`: `217` unsupported-shape fallbacks and `45` shadow mismatches
- `VLOOKUP`: `8` unsupported-host-surface fallbacks and `7` shadow
  mismatches
- `INDEX`: `4` unsupported-shape fallbacks and `5`
  unsupported-host-surface fallbacks

This means the best next move is not "add another new family."

The best next move is to convert the remaining high-volume fallout on the
existing promoted families.

## Strategic Objective

Raise authoritative usage on the current probe-covered surface by reducing
the dominant retained failure classes:

- `DATEVALUE` mismatch
- `LOOKUP` unsupported shape
- `LOOKUP` / `VLOOKUP` mismatch and host-surface fallout

The key idea is:

- keep the same live seam
- keep the same promoted-family cluster
- make more of the existing `1488` measured probe-covered cells close
  authoritatively

## Frozen Starting Point

This pass starts from the completed authoritative-usage expansion snapshot:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1126`
- `interpret_tail_authoritative_fallback_total=362`
- `interpret_tail_fallback_unsupported_formula_shape=243`
- `interpret_tail_fallback_shadow_mismatch=97`
- `interpret_tail_fallback_unsupported_host_surface=22`

Per promoted family, the starting snapshot is:

- `VALUE`: `14 / 15`
- `DATEVALUE`: `4 / 31`
- `TIMEVALUE`: `8 / 8`
- `NUMBERVALUE`: `9 / 9`
- `MATCH`: `102 / 117`
- `XMATCH`: `24 / 33`
- `LOOKUP`: `550 / 815`
- `VLOOKUP`: `296 / 313`
- `HLOOKUP`: `39 / 39`
- `XLOOKUP`: `78 / 97`
- `INDEX`: `2 / 11`

This baseline is frozen in:

- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_PLAN.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_EVIDENCE.md)

## Recommended Scope

### 1. `DATEVALUE` Live-Corpus Parity

Focus first on the currently measured mismatch rows, not on generic parsing
theory.

Bounded target surface:

- referenced date/time cells whose host cell type carries date or datetime
  meaning
- referenced text cells that contain ISO-style datetime strings
- month-name and punctuation shapes that are explicitly present in the
  current corpus diagnostics
- no broad locale-generalization promise beyond the diagnosed corpus rows

Exit condition for this lane:

- convert the current `DATEVALUE` retained mismatch rows into a smaller,
  explicit residual class

### 2. `LOOKUP` Unsupported-Shape Reduction

Attack the largest single retained bucket directly.

Bounded target surface:

- vector and matrix inputs that still collapse to one scalar result
- additional row/column coercions already exercised by the current corpus
- bounded array-constant and single-area source variants that the engine
  runtime already knows how to answer
- no spill, slice, or broad multi-cell result projection

Exit condition for this lane:

- materially reduce the `LOOKUP` unsupported-shape bucket without widening
  into matrix-return surfaces

### 3. `LOOKUP` / `VLOOKUP` Mismatch And Host-Surface Cleanup

After the unsupported-shape bucket, the next leverage is reducing supported
attempts that still fall back.

Bounded target surface:

- scalar result projection parity
- bounded text-vs-number result normalization
- bounded search-type / wildcard parity on shapes already seen in corpus
  diagnostics
- host-surface cases where Calc exposes a scalar answer and the engine is
  already semantically close

Exit condition for this lane:

- convert a meaningful fraction of supported-but-mismatching lookup cells
  into authoritative routes

### 4. Optional `INDEX` Scalar Cleanup

This is intentionally secondary and should only land if it closes on the
same runtime surface without opening a new matrix or spill class.

Bounded target surface:

- scalar `INDEX` matrix selection already visible on the current corpus
- no multi-cell result widening

## Success Criteria

This pass is complete only if all of the following are true:

- `interpret_tail_authoritative_total >= 1225`
- `interpret_tail_authoritative_fallback_total <= 300`
- `interpret_tail_fallback_unsupported_formula_shape <= 170`
- `interpret_tail_fallback_shadow_mismatch <= 75`
- `interpret_tail_fallback_unsupported_host_surface <= 15`
- `DATEVALUE` shadow mismatch falls from `27` to `10` or below
- `LOOKUP` unsupported-shape fallout falls from `217` to `150` or below
- `LOOKUP` shadow mismatch falls from `45` to `30` or below
- the standing standalone replay guardrail remains exact:
  - `500` workbooks
  - `50,661` formula cells
  - `50,652` parsed formulas
  - `0` cached-fallback cells
  - `0` cached-fallback rate

Stretch outcome:

- `interpret_tail_authoritative_total >= 1275`
- `interpret_tail_authoritative_fallback_total <= 275`
- `DATEVALUE` mismatch becomes a minor residual instead of the dominant
  mismatch family

## Non-Goals

Still out of scope for this wave:

- broad new family promotion beyond optional bounded `INDEX` cleanup
- broad `XLOOKUP` surface widening beyond the already-admitted bounded lane
- ambient AutoCalc-traffic claims
- spill, matrix-return, or slice-valued projection
- external references, macros, add-ins, or environment-sensitive execution
- broad workbook-wide evaluator replacement

## Primary Engineering Surfaces

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
- [TextParsingExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/TextParsingExecution.hxx)
- [LookupExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx)
- [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)
- [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)

## Workstreams

### 1. Freeze Diagnostic Inventory

Before touching runtime behavior, freeze the exact current contributor list
for:

- `DATEVALUE` mismatch rows
- `LOOKUP` unsupported-shape rows
- `LOOKUP` / `VLOOKUP` mismatch rows
- `LOOKUP` / `VLOOKUP` host-surface rows

Required result:

- one concrete retained-hotspot inventory tied to the current corpus logs

### 2. `DATEVALUE` Parity Pass

Implement only the bounded live-corpus shapes confirmed by the frozen
inventory.

Required result:

- unit proof for each new `DATEVALUE` live helper
- corpus rerun showing real mismatch reduction on the diagnosed rows

### 3. `LOOKUP` Shape Conversion Pass

Implement the highest-volume scalar-preserving `LOOKUP` shape conversions
from the current unsupported-shape bucket.

Required result:

- unit proof for each newly-authoritative `LOOKUP` shape
- measurable reduction in the `LOOKUP` unsupported-shape bucket

### 4. Lookup Mismatch And Host-Surface Cleanup

After shape conversion, close the highest-volume retained mismatch and
host-surface rows for `LOOKUP` / `VLOOKUP`.

Required result:

- per-family fallback-reason totals move down on corpus rerun

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

1. [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_DECISION_RECORD.md)
2. [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_EVIDENCE.md)
3. this plan
