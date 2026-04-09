# Computational Substrate Primitive Execution Host Evidence

Status: frozen primitive-execution evidence

## Purpose

This note records the bounded evidence for
[COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md)
after the admitted primitive-execution surface was implemented.

The question for this proof cycle is narrow:

- does the admitted slice still close exactly after introducing an explicit
  engine-authored primitive execution plan
- and did the retained host shell actually get smaller without widening the
  admitted slice

## Result Summary

The bounded proof stayed green.

On the admitted slice:

- applied mutation-entry lanes now carry an explicit primitive execution
  plan
- dirty-baseline rollback lanes now also carry an explicit primitive
  execution plan
- admitted primitive execution closes with
  `PrimitiveExecutionObservationKind::Exact` on the exact proof lanes
- the earlier admitted raw document mutation, primitive realization,
  primitive rollback, live apply, and final verification results remain
  green

This is strong evidence that the retained primitive execution shell is now a
smaller and more explicit host surface than it was before this cycle.

## Exact Apply And Rollback Outcomes

The exact admitted proof lanes now show:

- applied admitted mutation-entry results carry
  `moPrimitiveExecutionPlan`
- applied admitted mutation-entry results classify
  `moPrimitiveExecutionObservation` as `Exact`
- dirty-baseline rollback admitted mutation-entry results carry
  `moPrimitiveExecutionPlan`
- dirty-baseline rollback admitted mutation-entry results classify
  `moPrimitiveExecutionObservation` as `Exact`

That means the admitted primitive host shell is no longer only inferred from
the earlier raw document mutation, primitive realization or rollback, and
final verification observations. It is now represented by one explicit
engine-authored primitive execution plan and one explicit composite
observation.

## Meaningful Host-Shell Reduction

The host-owned primitive shell is meaningfully smaller in this proof cycle
because mutation entry now carries:

- an explicit primitive execution plan layered over the admitted raw
  document mutation record plus primitive realization or primitive rollback
  identity
- an explicit primitive execution observation layered over the admitted raw
  document mutation, primitive realization or rollback, and final
  verification observations

Calc still performs the retained low-level host calls, but it no longer
quietly supplies all of the admitted primitive execution identity and stage
sequencing through local implicit control flow.

## Deferred And Out-Of-Contract Classes

The proof cycle stayed intentionally bounded.

It does not justify widening into:

- shared-group-sensitive primitive execution
- named-range-sensitive primitive execution
- off-sheet or sheet-wide structural primitive execution
- workbook or mutation classes outside the admitted scalar and narrow
  structural slice

Those classes remain explicit defer or out-of-contract surfaces rather than
silent widening through the new primitive execution plan.

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

- `dependency_shadow elapsed=6.95`
- `dependency_shadow rss_kb=248624`

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

- the admitted primitive execution plan is stable enough to carry exact
  apply and rollback lanes
- the admitted primitive execution observation is strong enough to classify
  the combined primitive host shell explicitly instead of only through the
  narrower upstream and downstream observations
- queue, computational, graph, replay, realization, rollback, and final
  verification proof remain exact on the admitted slice after the
  primitive-execution layer was added

## What The Evidence Does Not Support Yet

This evidence does not justify:

- widening beyond the admitted scalar and narrow structural slice
- broad primitive host-call replacement outside the admitted slice
- shared-group-sensitive, named-range-sensitive, off-sheet, or sheet-wide
  primitive execution migration
- broad `ScDocument` host independence
