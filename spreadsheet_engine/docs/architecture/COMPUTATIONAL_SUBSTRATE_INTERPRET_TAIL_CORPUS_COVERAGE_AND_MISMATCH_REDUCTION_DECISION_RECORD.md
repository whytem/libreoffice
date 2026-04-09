# Computational Substrate InterpretTail Corpus Coverage And Mismatch Reduction Decision Record

Status: completed closeout for the focused InterpretTail corpus coverage
and mismatch-reduction pass

## Decision

Accept this pass as completed with a mixed-result closeout.

Do record it as a real runtime-improvement wave.
Do not record it as a full success against every original stretch target.

Specifically:

- accept the widened authoritative usage on the already-promoted live
  evaluator families
- accept the material unsupported-shape reduction on the current corpus
  surface
- do not claim probe-surface growth that the rerun did not show
- do not claim mismatch reduction as a settled win, because the retained
  mismatch bucket rose slightly overall even though some bounded live cases
  were repaired

## Why

The pass started from a useful but noisy first authoritative corpus baseline:

- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=996`
- `interpret_tail_authoritative_fallback_total=395`
- `interpret_tail_fallback_unsupported_formula_shape=295`
- `interpret_tail_fallback_shadow_mismatch=85`
- `interpret_tail_fallback_unsupported_host_surface=15`

The completed runtime work then did three meaningful things:

- widened lookup-family source materialization on the live seam so current
  promoted families can consume bounded array constants and scalar-shaped
  sources more often
- widened scalar result projection so bounded `1x1` matrix results no longer
  fail gratuitously on the current seam
- corrected `VALUE(empty-cell)` on the live seam to match Calc's scalar
  behavior

The pass also proved that live-source-normalized probe classification does
not uncover additional top-level promoted-family roots on the current
replay corpus.

That last result matters because it means the flat `1391` probe count is
itself a useful answer. The current corpus surface is already being measured
on the right top-level roots for the promoted evaluator families.

## Exact Outcome

The completed corpus rerun freezes:

- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=1045`
- `interpret_tail_authoritative_fallback_total=346`
- `interpret_tail_fallback_unsupported_formula_shape=240`
- `interpret_tail_fallback_shadow_mismatch=90`
- `interpret_tail_fallback_unsupported_host_surface=16`

The accepted interpretation is:

- authoritative usage improved materially
- unsupported-shape fallout improved materially
- probe-surface coverage did not grow
- mismatch fallout is now more precisely inventoried, not broadly solved

## Current Corpus Frontier

The retained evaluator hotspots are now concrete enough to target directly.

The dominant remaining contributors are:

- `DATEVALUE` shadow mismatch: `27`
- `LOOKUP` unsupported shape: `217`
- `LOOKUP` shadow mismatch: `45`
- `VLOOKUP` unsupported host surface: `8`
- `VLOOKUP` shadow mismatch: `9`
- `INDEX` unsupported shape: `4`
- `INDEX` unsupported host surface: `5`

This is a much sharper next-step surface than the generic old blockers
"increase probe coverage" and "reduce mismatch."

## Next Recommendation

The next evaluator follow-on should stay on the current promoted families and
target the now-explicit retained hotspots:

1. bounded `DATEVALUE` mismatch reduction
2. bounded lookup-family residual unsupported-shape reduction
3. bounded lookup-family and `VLOOKUP` mismatch or host-surface cleanup

That is a better next move than another broad function-family promotion,
because the current live seam already has more unexploited value inside the
existing evaluator cluster.

## Validation Summary

The supporting proof is summarized in
[COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_COVERAGE_AND_MISMATCH_REDUCTION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_COVERAGE_AND_MISMATCH_REDUCTION_EVIDENCE.md).
