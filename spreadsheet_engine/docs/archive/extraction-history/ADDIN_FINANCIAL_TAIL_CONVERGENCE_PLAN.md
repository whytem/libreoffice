# Analysis Add-In Financial Tail Convergence Plan

Status: completed implementation and closeout record

## Purpose

The major extraction, replay-promotion, recalc, execution-shell, direct-entry,
host-boundary, token-boundary, production-boundary, and external-reference
facade streams are complete.

The retained Calc host boundary is now mostly explicit and stable. The
strongest remaining non-host-shaped residue is the final legacy pocket in the
Analysis add-in financial surface:

- most add-in financial callers already use shared engine runtime through
  [FinancialAddInExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/FinancialAddInExecution.hxx)
- but `ODDFPRICE` and `ODDFYIELD` still route through legacy helper calls in
  [financial.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/financial.cxx)
- those legacy helpers in
  [analysishelper.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysishelper.cxx)
  are currently just unconditional throwing stubs

This stream is for closing that residual semantic tail cleanly.

## Final Outcome

This stream is complete.

The final result was not a new engine-runtime adoption slice. A fresh inventory
and classification pass showed that there is no shared odd-first-period
`ODDFPRICE` / `ODDFYIELD` implementation anywhere in the current engine or Calc
tree.

So the closeout result is:

- the add-in callers in
  [financial.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/financial.cxx)
  now use one named explicit defer boundary instead of routing through a fake
  helper path
- the old unconditional-throw helper declarations and definitions were removed
  from
  [analysishelper.hxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysishelper.hxx)
  and
  [analysishelper.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysishelper.cxx)
- focused regression coverage now lives in
  [analysis.cxx](/home/ubuntu/repos/libreoffice/scaddins/qa/analysis.cxx)

That means the remaining Analysis add-in odd-first-period pair is now an
explicitly deferred semantic gap rather than an ambiguous legacy residue.

## What This Workstream Is For

This stream is successful when the remaining Analysis add-in financial tail is
easy to explain as one of:

- shared engine-owned spreadsheet semantics reached through the direct add-in
  adapter path
- an explicitly retained host-only add-in seam with a documented reason
- an explicit defer item, not an ambiguous legacy stub

In practical terms, the focus is:

- `ODDFPRICE`
- `ODDFYIELD`
- the local `GetOddfprice()` / `GetOddfyield()` helper tail
- any adjacent null-date/date-mode packaging that still exists only because of
  those two callers

## Out Of Scope

This plan does not:

- reopen the recalc boundary
- move add-in UNO/service ownership out of Calc
- move document null-date ownership out of the host
- broaden into a new general host-boundary cleanup stream
- attempt a large-scale rewrite of `analysishelper.cxx`
- change the zero-fallback replay contract

## Definition Of Done

This workstream is complete when all of the following are true:

1. the remaining Analysis add-in financial tail is explicitly inventoried and
   classified
2. `ODDFPRICE` and `ODDFYIELD` are either adopted onto a shared engine-backed
   path or explicitly retained/deferred with a clear rationale
3. the unconditional-throw helper tail is removed, isolated, or documented as
   an explicit retained boundary rather than legacy residue
4. the touched add-in callers use the smallest practical host-owned shell
   around the selected semantic implementation
5. Calc/add-in and standalone validation lanes remain green
6. the promoted replay baseline remains at zero fallback

## Boundary Rules

Every slice in this stream must respect the settled project boundary:

- keep add-in UNO/service ownership in Calc
- keep null-date acquisition and holiday-input ownership host-side
- prefer shared engine runtime where the semantics are spreadsheet-specific
- do not create new Calc-local semantic duplicates
- document any retained host-only or deferred outcome explicitly

## Workstreams

### 1. Freeze The Residual Add-In Financial Tail Inventory

Build an explicit inventory of the remaining Analysis add-in financial callers
that do not yet follow the direct shared-runtime path.

For each candidate, record:

- owning add-in caller
- current helper/runtime path
- whether the gap is semantic, adapter, or host-only
- closest existing shared runtime or adapter surface
- whether the right outcome is `ready now`, `needs runtime slice`,
  `host-only`, or `defer`
- required validation lanes

Primary target files:

- [financial.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/financial.cxx)
- [analysishelper.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysishelper.cxx)
- [analysisdefs.hxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysisdefs.hxx)
- [FinancialAddInExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/FinancialAddInExecution.hxx)
- [FinancialRuntime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/FinancialRuntime.hxx)

Completion criteria:

- there is no unclassified in-scope financial tail candidate in the touched
  surface
- each candidate has an owning later slice or an explicit retain/defer reason

### Phase 1 Inventory Snapshot

The live residual inventory for this stream is:

| Candidate | Owning caller | Current path | Gap type | Closest shared surface | Validation lanes |
| --- | --- | --- | --- | --- | --- |
| `ODDFPRICE` | [financial.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/financial.cxx) `AnalysisAddIn::getOddfprice()` | caller validates, then calls `GetOddfprice()` in [analysishelper.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysishelper.cxx), which unconditionally throws | semantic residue | odd-last-period adapter/runtime already exists in [FinancialAddInExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/FinancialAddInExecution.hxx) and [FinancialRuntime.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FinancialRuntime.cxx); no odd-first-period shared path exists yet | `CppunitTest_scaddins_analysis`, `git diff --check` |
| `ODDFYIELD` | [financial.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/financial.cxx) `AnalysisAddIn::getOddfyield()` | caller validates, then calls `GetOddfyield()` in [analysishelper.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysishelper.cxx), which unconditionally throws | semantic residue | odd-last-period adapter/runtime already exists in [FinancialAddInExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/FinancialAddInExecution.hxx) and [FinancialRuntime.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FinancialRuntime.cxx); no odd-first-period shared path exists yet | `CppunitTest_scaddins_analysis`, `git diff --check` |
| odd-first-period helper tail | [analysishelper.hxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysishelper.hxx) / [analysishelper.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysishelper.cxx) | public helper declarations plus unconditional-throw definitions | legacy wrapper residue | no shared runtime exists to point at yet | `CppunitTest_scaddins_analysis`, `git diff --check` |
| null-date and date-mode packaging around the odd-first callers | [financial.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/financial.cxx), [analysisdefs.hxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/analysisdefs.hxx) | host null-date acquisition and date-mode extraction before helper calls | candidate host seam | add-in date context and direct financial adapter helpers already exist | `CppunitTest_scaddins_analysis`, `git diff --check` |

### 2. Classify Shared-Runtime Readiness

Turn the inventory into an explicit map of what is actually left.

Required outcomes:

- distinguish true host-owned concerns from residual semantic implementation
- determine whether `ODDFPRICE` and `ODDFYIELD` can land directly on a shared
  runtime surface
- identify any minimal adapter/runtime addition required for that landing
- mark anything still not worth moving as `defer` rather than implicit residue

Completion criteria:

- each touched candidate has one explicit disposition
- the next implementation slice is selected from the highest-confidence
  convergence target

### Phase 2 Classification Result

The current classification is:

| Candidate | Disposition | Reason |
| --- | --- | --- |
| `ODDFPRICE` | `defer` | there is no shared odd-first-period runtime or adapter surface anywhere in `spreadsheet_engine/`, `scaddins/`, or `sc/`; the caller currently reaches only an unconditional-throw helper stub |
| `ODDFYIELD` | `defer` | same as `ODDFPRICE`: no engine-owned implementation exists yet, so this is not a ready-now direct-entry widening slice |
| odd-first-period helper tail | `ready now` | the helper itself is legacy residue and can be retired or replaced with an explicit named defer boundary immediately |
| null-date and date-mode packaging | `host-only` | the remaining null-date/date-mode acquisition is already part of the explicit add-in host boundary and is not the problem to solve in this stream |

That means the highest-confidence implementation slice for this stream is not a
new finance algorithm. It is:

1. replace the legacy helper path with an explicit named defer boundary at the
   add-in caller surface
2. remove the unconditional-throw helper declarations and definitions
3. add regression coverage that makes the retained defer visible and stable

This keeps the boundary honest: the odd-first-period pair is now treated as an
explicitly deferred semantic gap rather than as an ambiguous helper path that
looks converged but still just throws.

### 3. Land The Shared Runtime Or Direct Adapter Slice

Implement the actual convergence slice for the selected residual financial
tail.

Expected primary target:

- `ODDFPRICE`
- `ODDFYIELD`

Possible landing shapes:

- shared runtime support in
  [FinancialRuntime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/FinancialRuntime.hxx)
  and
  [FinancialRuntime.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FinancialRuntime.cxx)
- direct add-in adapter support in
  [FinancialAddInExecution.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/FinancialAddInExecution.hxx)
- narrower caller adoption in
  [financial.cxx](/home/ubuntu/repos/libreoffice/scaddins/source/analysis/financial.cxx)

Implementation rules:

- keep host null-date/date-mode acquisition outside the engine
- keep spreadsheet-specific pricing/yield semantics shared if feasible
- avoid broadening the public surface beyond what the selected slice needs

Completion criteria:

- the touched caller no longer depends on the legacy throwing helper tail
- the selected semantics are implemented through one clear path

### 4. Retire The Legacy Throwing Helper Path

Once the selected direct/shared path exists, remove or isolate the old helper
layer.

Allowed outcomes:

- delete the unconditional throwing helper entirely
- retain only a thin wrapper with an explicit documented reason
- keep a helper only if it still marks a genuine defer boundary

Not acceptable:

- leaving the old unconditional-throw helper active alongside the new path
- keeping ambiguous “encapsulation violation” comments as the lasting boundary

Completion criteria:

- the touched helper tail is either removed or explicitly justified
- there is one clear implementation path per adopted caller

### 5. Tighten Validation And Regression Coverage

Add focused validation that proves the selected tail is no longer special-cased
or ambiguous.

Likely focus:

- [scaddins/qa/analysis.cxx](/home/ubuntu/repos/libreoffice/scaddins/qa/analysis.cxx)
- [sc/qa/unit/ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx)
- standalone runtime/evaluator coverage if new shared runtime support lands

Completion criteria:

- the adopted financial tail has direct regression coverage
- touched Calc/add-in and standalone lanes stay green

### 6. Lock The Baseline And Close The Stream

Turn the converged add-in tail into a stable standing contract.

Required closeout work:

- rerun focused add-in and Calc coverage for every touched slice
- rerun standalone runtime/evaluator lanes if shared helpers changed
- rerun one-shot replay with `--assert-zero-fallback`
- update status and architecture docs in present tense
- record any retained or deferred residual tail explicitly

Completion criteria:

- the zero-fallback replay baseline remains intact
- docs describe the remaining Analysis add-in boundary in present tense rather
  than as an implied unfinished legacy tail

## Recommended Execution Order

Run the stream in this order:

1. freeze the residual add-in financial tail inventory
2. classify shared-runtime readiness
3. land the shared runtime or direct adapter slice
4. retire the legacy throwing helper path
5. tighten validation and regression coverage
6. rerun the full validation contract and close the stream

## Validation Contract

Every implementation slice in this stream should, at minimum, run:

- `CppunitTest_scaddins_analysis`
- `CppunitTest_sc_ucalc_formula2` if production Calc formula callers are touched
- `git diff --check`

When shared standalone runtime or evaluator helpers change, also run:

- `spreadsheetengine_fods_evaluator_tests`
- focused standalone runtime/unit lanes for the touched surface

Final closeout must also run:

- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`

## Exit Criteria

This stream is fully closed when:

1. the residual Analysis add-in financial tail has been explicitly classified
   into `shared path`, `host-only`, or `defer`
2. the selected `ODDFPRICE` / `ODDFYIELD` path no longer depends on a legacy
   unconditional-throw helper
3. the touched add-in callers use one clear shared or explicitly retained path
4. the replay baseline remains exactly:
   - `500` workbooks
   - `50,661` formula cells
   - `50,652` parsed formulas
   - `0` cached-fallback cells
   - `0` cached-fallback rate
5. status and architecture docs describe the remaining add-in boundary in
   present tense rather than as ambiguous residue
