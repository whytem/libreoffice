# Computational Substrate InterpretTail Corpus Authoritative Usage Decision Record

Status: completed closeout for the first non-zero authoritative
replay-corpus `InterpretTail` usage pass

## Decision Summary

This pass is accepted as completed.

It did what it was supposed to do:

- it removed the old `0 authoritative / 303 parse_failure` corpus blocker
- it put real authoritative usage on the board for the already-promoted
  live evaluator families
- it converted the replay corpus from a pure fallback signal into a usable
  migration metric

## What Changed

The completed pass introduced three meaningful changes:

- a better live formula-source bridge:
  [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
  now exports `GRAM_ODFF`, and
  [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
  now normalizes the bounded namespace and wrapper shapes that actually occur
  on the live seam
- bounded seam diagnostics:
  the live evaluator can now capture representative parse-failure and
  unsupported-shape samples without turning the seam into a broad logging
  surface
- a stronger Calc-backed corpus harness:
  [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
  now hard-recalcs a real `ScDocument`, probes the already-promoted function
  families against live formula source, and freezes authoritative vs fallback
  counts against corpus-expected results

## Accepted Outcome

The completed rerun proves first non-zero authoritative corpus usage:

- `interpret_tail_authoritative_total=996`
- `interpret_tail_authoritative_fallback_total=395`
- `interpret_tail_probe_formula_cells=1391`

That is a real program improvement because it replaces “we think the seam
could matter” with “we can now measure where it does matter.”

## What This Does Not Mean

This is not yet the same thing as broad ambient delegation.

The accepted interpretation is narrower:

- the counts come from a focused Calc-backed supported-family probe over live
  formula source
- they do not claim that natural ambient AutoCalc traffic already produces
  the same authoritative usage totals by default
- the pass is therefore accepted as a migration-underwriter and
  measurement-clarity win, not as a broad new authority-transfer wave

## Remaining Boundary

The corpus runner now makes the remaining blockers concrete.

The dominant retained buckets are:

- unsupported formula shape
- shadow mismatch
- unsupported host surface

The highest-value remaining evaluator hotspots are now clearly visible on:

- `LOOKUP`
- `DATEVALUE`
- `INDEX`
- `XMATCH`

## Next Recommendation

The next evaluator pass should use this new corpus baseline to improve the
already-promoted families before adding a broad new capability wave:

- reduce unsupported-shape fallout on promoted families
- reduce `LOOKUP`-class mismatch behavior
- improve how much natural live seam traffic reaches the same promoted-family
  authority path without probe-only help

## Validation Summary

The supporting proof is summarized in
[COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_INTERPRET_TAIL_CORPUS_AUTHORITATIVE_USAGE_EVIDENCE.md).
