# Computational Substrate InterpretTail Authoritative Usage Expansion Decision Record

Status: completed closeout for the ambitious InterpretTail authoritative
usage expansion pass

## Decision

Accept this pass as completed with a mixed-result widening closeout.

Do record it as a real runtime expansion wave.
Do not record it as a full success against the original ambitious targets.

Specifically:

- accept the widened probe surface and authoritative usage on the live
  `InterpretTail -> engine` seam
- accept bounded live `XLOOKUP` promotion as a real newly-authoritative
  family
- accept bounded wrapper-aware delegation for `IFERROR` / `IFNA` around
  promoted roots as landed live runtime behavior
- do not claim the hotspot-cleanup targets were met, because total fallback,
  unsupported-host-surface, and shadow-mismatch fallout all remained above
  the planned completion bar

## Why

The pass started from a stronger but still narrow corpus-backed live seam:

- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=1045`
- `interpret_tail_authoritative_fallback_total=346`
- `interpret_tail_fallback_unsupported_formula_shape=240`
- `interpret_tail_fallback_shadow_mismatch=90`
- `interpret_tail_fallback_unsupported_host_surface=16`

The completed runtime work then materially changed the seam:

- promoted bounded live `XLOOKUP`
- widened live lookup requests onto the engine-owned search-type surface
- added bounded wrapper-aware delegation for `IFERROR` / `IFNA`
- widened supported probe classification to recognize those delegated roots
- landed additional bounded text-parsing and referenced-date helper support
  on the live seam

That is enough runtime movement to count as a real migration wave, not just
an instrumentation pass.

## Exact Outcome

The completed Calc-backed corpus rerun now freezes:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1126`
- `interpret_tail_authoritative_fallback_total=362`
- `interpret_tail_fallback_unsupported_formula_shape=243`
- `interpret_tail_fallback_shadow_mismatch=97`
- `interpret_tail_fallback_unsupported_host_surface=22`

The accepted interpretation is:

- the live probe-covered surface is larger than before
- authoritative usage is materially higher than before
- bounded `XLOOKUP` is now real on the live corpus surface
- wrapper-aware delegated shapes are now live and unit-proven
- the cleanup side of the pass underperformed the ambition of the plan

## What Landed

The runtime and proof surfaces for the completed pass are:

- [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
- [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
- [ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx)
- [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)

The key landed capability changes are:

- bounded `XLOOKUP` authority on the live seam
- bounded delegated fallback handling through `if_not_found`
- wrapper-aware delegated evaluation for `IFERROR` / `IFNA`
- promoted-family probe recognition beyond top-level bare roots
- additional bounded scalar and referenced-date helper support

## What Did Not Close

The ambitious plan targets were not met in full.

The main retained hotspots are still:

- `DATEVALUE` shadow mismatch: `27`
- `LOOKUP` unsupported-shape fallout: `217`
- `LOOKUP` shadow mismatch: `45`
- `VLOOKUP` host-surface plus mismatch fallout: `15`
- `INDEX` unsupported-shape plus host-surface fallout: `9`

So the pass widened authority, but it did not yet convert the dominant
retained hotspots into a clean low-fallback lane.

## Next Recommendation

The next evaluator pass should not be another broad capability wave.

It should be a targeted hotspot-conversion pass focused on:

1. `DATEVALUE` live-corpus parity on retained mismatch rows
2. residual `LOOKUP` unsupported-shape reduction on the current corpus
3. residual `LOOKUP` / `VLOOKUP` shadow-mismatch and host-surface cleanup
4. bounded `INDEX` scalar-projection cleanup only if it closes on the same
   runtime surface

That is the highest-leverage next move after this widening wave.

The scoped execution plan for that follow-on is now:

- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_PLAN.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_HOTSPOT_CONVERSION_PLAN.md)

## Validation Summary

The supporting proof is summarized in
[COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_EVIDENCE.md).
