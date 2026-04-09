# Computational Substrate InterpretTail Lookup Residual Conversion Decision Record

Status: completed closeout for the focused InterpretTail lookup-residual
conversion pass

## Decision

Accept this pass as completed with a strong mixed-result runtime closeout.

Do record it as a substantial live evaluator improvement pass.
Do not record it as a full success against every original cleanup target.

Specifically:

- accept the large authoritative-usage increase on the same probe surface
- accept the large total-fallback reduction
- accept the near-elimination of the retained `LOOKUP`
  unsupported-shape bucket
- do not claim that the remaining `LOOKUP` mismatch and lookup-family
  host-surface cleanup is finished

## Why

The pass started from the completed hotspot-conversion snapshot:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1156`
- `interpret_tail_authoritative_fallback_total=332`
- `interpret_tail_fallback_unsupported_formula_shape=241`
- `interpret_tail_fallback_shadow_mismatch=72`
- `interpret_tail_fallback_unsupported_host_surface=19`

The completed runtime work then moved the same seam materially:

- widened bounded referenced-formula materialization beyond one-off special
  cases into delegated function and scalar-node rescue
- added recursion-safe referenced-formula materialization guards
- widened lookup input normalization so `1x1` matrices scalarize cleanly
- rejected scalar-error lookup inputs earlier instead of carrying bad
  projections deeper into runtime lookup resolution
- materialized single-cell lookup results through the referenced-formula
  host bridge instead of raw host reads only

That is enough runtime movement to count as a real conversion pass.

## Exact Outcome

The completed Calc-backed corpus rerun now freezes:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1379`
- `interpret_tail_authoritative_fallback_total=109`
- `interpret_tail_fallback_unsupported_formula_shape=26`
- `interpret_tail_fallback_shadow_mismatch=64`
- `interpret_tail_fallback_unsupported_host_surface=19`

The accepted interpretation is:

- the measured probe surface stayed flat
- authoritative routes rose by `223`
- total fallback dropped by `223`
- unsupported-shape fallout dropped by `215`
- shadow mismatch dropped by `8`
- unsupported-host-surface fallout stayed flat

## What Landed

The runtime and proof surfaces for the completed pass are:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
- [LookupExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/LookupExecution.hxx)
- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)
- [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)

The key landed capability changes are:

- general bounded referenced-formula source rescue on the live seam
- recursion-safe formula-backed source and result-cell materialization
- scalarization of `1x1` lookup inputs
- earlier rejection of scalar-error lookup inputs
- exact projection of formula-backed single-cell lookup results

## What Closed Well

The strongest win in this pass is the former `LOOKUP` unsupported-shape
bucket.

That family moved from:

- authoritative `555 / 815`
- unsupported-shape fallback `215`
- shadow mismatch `45`

to:

- authoritative `777 / 815`
- unsupported-shape fallback `0`
- shadow mismatch `38`

That is enough to treat the old `LOOKUP` shape-conversion blocker as
substantially cleared.

The whole seam also improved materially:

- authoritative total: `1156 -> 1379`
- total fallback: `332 -> 109`
- unsupported-shape: `241 -> 26`

## What Did Not Close

The remaining work is now narrower and more specific.

The completed corpus snapshot still records:

- `LOOKUP` shadow mismatch `38`
- `VLOOKUP` unsupported-host-surface `8`
- `VLOOKUP` shadow mismatch `7`
- `XLOOKUP` unsupported-host-surface `6`
- `XLOOKUP` shadow mismatch `10`
- `INDEX` unsupported-host-surface `5`

Those numbers mean the dominant remaining problem is no longer shape
recognition.
It is scalar-projection and host-surface cleanup on already-promoted lookup
families.

## Next Recommendation

The next evaluator pass should stay narrow and stay on the same lookup
projection seam.

The highest-value next work is:

1. residual `LOOKUP` scalar-projection parity cleanup
2. residual `VLOOKUP` / `XLOOKUP` scalar host-surface cleanup
3. bounded `INDEX` host-surface cleanup only if it closes on the same seam

The key point is that the large lookup-shape blocker is now gone.
The remaining work is smaller, more exact, and more host-surface-specific.

## Validation Summary

The supporting proof is summarized in
[COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_LOOKUP_RESIDUAL_CONVERSION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_LOOKUP_RESIDUAL_CONVERSION_EVIDENCE.md).
