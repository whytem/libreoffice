# Computational Substrate Final Verification Host-Shell Evidence

Status: frozen final-verification evidence

## Purpose

This note records the bounded evidence for
[COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md)
after the admitted final-verification surface was implemented.

The question for this proof cycle is narrow:

- does the admitted slice still close exactly after introducing an explicit
  engine-authored final verification record
- and did the retained host shell actually get smaller without widening the
  admitted slice

## Result Summary

The bounded proof stayed green.

On the admitted slice:

- applied mutation-entry lanes now carry an explicit final verification
  record
- dirty-baseline rollback lanes now carry an explicit final verification
  record
- admitted final verification closes with `FinalVerificationObservationKind::Exact`
  on the exact proof lanes
- the previous admitted queue, computational, graph, live-apply, and
  primitive realization or rollback results remain green

This is strong evidence that the retained final verification shell is now a
smaller and more explicit host surface than it was before this cycle.

## Validation Lanes

The evidence run stayed green on:

- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- `CppunitTest_sc_ucalc_compile_diff`
- `spreadsheetengine_computational_graph_tests`
- `spreadsheetengine_computational_substrate_tests`
- `spreadsheetengine_workbook_facade_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Timed Sample

Recorded bounded sample:

- `dependency_shadow elapsed=6.73`
- `dependency_shadow rss_kb=248720`

## Replay Baseline

The standing replay baseline remains exact:

- `workbooks=500`
- `formula_cells=50661`
- `parsed_formulas=50652`
- `cached_fallback_cells=0`
- `cached_fallback_rate=0`

Representative replay summary lines from the evidence run:

- `cell_reference_nodes=72217`
- `range_reference_nodes=6219`
- `named_reference_nodes=398`
- `function_call_nodes=57239`
- `literal_nodes=37853`
- `top_functions=FORMULA:14278,ROUND:11189,ORG.LIBREOFFICE.ROUNDSIG:4867,ISERROR:2570,IF:2465,AND:2166,ABS:1509,FALSE:1082,TRUE:993,LOOKUP:815`

## What The Evidence Supports

The evidence supports these bounded conclusions:

- the admitted final-verification record is stable enough to carry exact
  apply and rollback lanes
- rollback lanes now carry explicit comparison state through the
  final-verification seam instead of collapsing immediately to a single
  rollback observation
- execution-IR remains observational on this admitted slice without
  destabilizing the exact queue, computational, graph, replay, and
  primitive proof lanes

## What The Evidence Does Not Support Yet

This evidence does not justify:

- widening beyond the admitted scalar and narrow structural slice
- broad final verification replacement outside the admitted slice
- shared-group-sensitive, named-range-sensitive, off-sheet, or sheet-wide
  verification migration
- broad `ScDocument` host independence
