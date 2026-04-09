# Computational Substrate InterpretTail Hotspot Conversion Decision Record

Status: completed closeout for the focused InterpretTail hotspot-conversion
pass

## Decision

Accept this pass as completed with a mixed-result runtime closeout.

Do record it as a real live evaluator improvement pass.
Do not record it as a full success against the original ambitious hotspot
targets.

Specifically:

- accept the bounded runtime improvements on the live `InterpretTail ->
  engine` seam
- accept the strong `DATEVALUE` parity win on the Calc-backed corpus
- accept the overall reduction in total fallback and total shadow mismatch
- do not claim that the `LOOKUP` unsupported-shape and mismatch targets were
  fully converted

## Why

The pass started from the completed authoritative-usage expansion snapshot:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1126`
- `interpret_tail_authoritative_fallback_total=362`
- `interpret_tail_fallback_unsupported_formula_shape=243`
- `interpret_tail_fallback_shadow_mismatch=97`
- `interpret_tail_fallback_unsupported_host_surface=22`

The completed runtime work then materially improved the same live seam:

- added bounded standalone live parsing fallback for `VALUE`,
  `DATEVALUE`, and `TIMEVALUE`
- added a bounded referenced-formula source bridge for `BASISODATETIME(...)`
  shaped inputs
- widened matrix and range materialization for lookup-family sources
- added bounded matrix arithmetic materialization for scalar-preserving
  lookup requests
- removed one residual empty-result lookup host-surface fallback path

That is enough runtime movement to count as a real migration-improvement
wave, not just another instrumentation pass.

## Exact Outcome

The completed Calc-backed corpus rerun now freezes:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1156`
- `interpret_tail_authoritative_fallback_total=332`
- `interpret_tail_fallback_unsupported_formula_shape=241`
- `interpret_tail_fallback_shadow_mismatch=72`
- `interpret_tail_fallback_unsupported_host_surface=19`

The accepted interpretation is:

- the measured probe surface stayed flat
- authoritative routes rose by `30`
- total fallback dropped by `30`
- total shadow mismatch dropped by `25`
- unsupported-host-surface fallout dropped by `3`
- unsupported-shape fallout dropped only slightly, by `2`

## What Landed

The runtime and proof surfaces for the completed pass are:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)
- [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)
- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)

The key landed capability changes are:

- bounded standalone text-parsing rescue on the live seam
- bounded referenced-formula source materialization for date-shaped text
- bounded lookup source materialization from ranges, names, and matrix
  binary operations
- bounded scalar projection for empty lookup results that still close
  exactly

## What Closed Well

The strongest win in this pass is `DATEVALUE`.

That family moved from:

- authoritative `4 / 31`
- shadow mismatch `27`

to:

- authoritative `29 / 31`
- shadow mismatch `2`

That is enough to treat `DATEVALUE` as substantially converted from a
dominant retained hotspot into a minor residual.

The whole seam also improved materially:

- authoritative total: `1126 -> 1156`
- total fallback: `362 -> 332`
- shadow mismatch: `97 -> 72`

## What Did Not Close

The main retained hotspot is still `LOOKUP`.

The completed corpus snapshot still records:

- `LOOKUP`: authoritative `555 / 815`
- `LOOKUP` unsupported-shape fallback: `215`
- `LOOKUP` shadow mismatch: `45`

Those numbers mean the pass did not convert the biggest residual
two-dimensional data-only and scalar-projection mismatch classes.

Residual smaller hotspots also remain:

- `VLOOKUP`: unsupported-host-surface `8`, shadow mismatch `7`
- `INDEX`: unsupported-shape `4`, unsupported-host-surface `5`
- `XLOOKUP`: unsupported-host-surface `6`, shadow mismatch `10`

## Next Recommendation

The next evaluator pass should stay narrow and stay on the same live seam.

The highest-value next work is:

1. bounded `LOOKUP` `2D` data-only unsupported-shape conversion
2. bounded `LOOKUP` scalar-projection parity on the retained mismatch rows
3. residual `VLOOKUP` host-surface cleanup on the already-promoted scalar
   lane
4. bounded `INDEX` cleanup only if it closes on the same lookup projection
   surface

The key point is that the next hotspot is no longer `DATEVALUE`.
It is now the residual lookup-family matrix-shape and scalar-projection
surface.

## Validation Summary

The supporting proof is summarized in
[COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_EVIDENCE.md).
