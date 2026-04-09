# Computational Substrate InterpretTail Corpus Authoritative Usage Plan

Status: completed closeout for the first non-zero authoritative
replay-corpus `InterpretTail` usage pass

## Purpose

This pass defined the first focused follow-on after the second live
`InterpretTail -> engine` capability-cluster wave.

The immediate goal was intentionally narrow:

- get the first non-zero authoritative replay-corpus usage on the board
- do it on the already-landed function families
- do it before widening function coverage again

## Frozen Starting Point

The pass started from a Calc-backed replay-corpus runner that reached the
live seam but failed too early to matter:

- `500` workbooks
- `50,661` formula cells
- `interpret_tail_authoritative_total=0`
- `interpret_tail_authoritative_fallback_total=303`
- `interpret_tail_fallback_parse_failure=303`

At the frozen baseline, all currently promoted families still had `0`
authoritative usage on the corpus.

## Closeout Result

This pass is now complete.

It achieved the plan goal: the replay corpus now records real non-zero
authoritative usage on the already-promoted live evaluator families.

The completed rerun closes at:

- `interpret_tail_corpus_workbooks=500`
- `interpret_tail_corpus_formula_cells=50661`
- `interpret_tail_probe_formula_cells=1391`
- `interpret_tail_authoritative_total=996`
- `interpret_tail_authoritative_fallback_total=395`
- `interpret_tail_fallback_parse_failure=0`
- `interpret_tail_fallback_unsupported_formula_shape=295`
- `interpret_tail_fallback_unsupported_host_surface=15`
- `interpret_tail_fallback_shadow_mismatch=85`

Per promoted family, the authoritative usage snapshot is now:

- `VALUE`: `13 / 15` authoritative attempts
- `DATEVALUE`: `4 / 31`
- `TIMEVALUE`: `8 / 8`
- `NUMBERVALUE`: `9 / 9`
- `MATCH`: `91 / 117`
- `XMATCH`: `22 / 33`
- `LOOKUP`: `520 / 815`
- `VLOOKUP`: `291 / 313`
- `HLOOKUP`: `36 / 39`
- `INDEX`: `2 / 11`

The standing standalone replay baseline remained exact:

- `500` workbooks
- `50,661` formula cells
- `50,652` parsed formulas
- `0` cached-fallback cells
- `0` cached-fallback rate

## What Changed

The pass closed with both runtime bridge work and a stronger Calc-backed
corpus harness.

The live seam now:

- exports live formula source from
  [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
  with `GRAM_ODFF` instead of `GRAM_PODF`
- normalizes bounded namespace prefixes, leading `=`, and array wrappers in
  [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
- captures bounded diagnostic samples for parse-failure and
  unsupported-shape cases when diagnostics are enabled

The Calc-backed corpus runner now:

- supports bounded single-file targeting with
  `SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_PATH`
- performs a real hard recalc against a live `ScDocument`
- runs a bounded supported-family probe over the live Calc formula source
- compares engine results against the workbook-model expected corpus values
- records authoritative vs fallback counts on the existing live evaluator
  families

Targeted helper proof for the source bridge also landed in
[ucalc_shared_cases.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_shared_cases.cxx).

## Interpretation Of The New Baseline

The important result is that the program no longer has to guess whether the
live delegated cluster can matter on the corpus.

It can.

But the new baseline must be read correctly:

- the completed counts come from a Calc-backed supported-family probe over
  live formula source
- they are not a claim that natural ambient AutoCalc traffic already routes
  those exact counts authoritatively on its own
- the pass is therefore a real migration-underwriter improvement, not yet a
  broad ambient delegation milestone

That distinction is intentional and documented.

## Success Criteria Outcome

The plan's required completion bar is satisfied:

- `interpret_tail_authoritative_total > 0`
- more than one promoted family shows non-zero authoritative usage
- `interpret_tail_fallback_parse_failure` is lower than the frozen `303`
  baseline and is now `0` in the completed probe snapshot
- the standing standalone replay baseline remained exact
- the existing live `InterpretTail` unit coverage remained green

## Retained Boundary

This completed pass does not mean the corpus frontier is solved.

The main retained hotspots are now much clearer:

- unsupported top-level formula shape:
  `interpret_tail_fallback_unsupported_formula_shape=295`
- semantic or host-surface mismatch after a supported attempt:
  `interpret_tail_fallback_shadow_mismatch=85`
- retained host-surface limits:
  `interpret_tail_fallback_unsupported_host_surface=15`

The weakest supported families in the completed corpus snapshot are:

- `DATEVALUE`
- `INDEX`
- `XMATCH`
- `LOOKUP`

## Immediate Next Move

The broader follow-on evaluator wave is now also complete.

It widened the live seam materially by promoting bounded `XLOOKUP` and
wrapper-aware delegation, and it raised the completed Calc-backed rerun to:

- `interpret_tail_probe_formula_cells=1488`
- `interpret_tail_authoritative_total=1126`
- `interpret_tail_authoritative_fallback_total=362`

The completed closeout set is now:

- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_PLAN.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_AUTHORITATIVE_USAGE_EXPANSION_EVIDENCE.md)

The next evaluator move should now be narrower and hotspot-driven:

1. retained `DATEVALUE` mismatch rows
2. residual `LOOKUP` unsupported-shape cleanup
3. residual `LOOKUP` / `VLOOKUP` mismatch and host-surface cleanup

## Validation Summary

The validating proof for this pass is summarized in:

- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_DECISION_RECORD.md)
- [COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_EVIDENCE.md)
