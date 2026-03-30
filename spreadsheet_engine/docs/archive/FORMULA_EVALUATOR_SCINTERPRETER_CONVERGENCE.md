# FormulaEvaluator / ScInterpreter Convergence Status

## Purpose

This document now records the closeout state of the `ScInterpreter`
convergence work. The large `FormulaEvaluator` refactor program is complete
enough that it is no longer the active planning focus here except where it
supports already-landed Calc delegations.

## Current State

The shared engine now provides the major runtime surfaces needed for the
completed convergence program, including:

- `MathStatistical`
- `MathAggregate`
- `QueryRuntime`
- `LookupRuntime`
- `FinancialRuntime`
- `ConversionRuntime`
- the extracted evaluator support/runtime family modules that are already in use

Calc delegation is live for:

- the initial Fisher / beta / poisson / binomial wave
- the first distribution wave: normal, log-normal, chi-square, legacy chi,
  gamma, student-t, F, and `BINOM.INV`
- the first new shared-runtime wave: inverse distributions and confidence
  functions
- the aggregate/statistics delegation wave: hypergeometric, percentile,
  quartile, median, mode, skew, and skewp
- the remaining shared-runtime additions wave: exponential distribution,
  Weibull distribution, negative-binomial distribution, combinatorics, and
  `Erf` / `Erfc`

The shared runtime policy is now:

- when behavior or edge-case parity is at risk, the shared runtime adopts
  Calc's original implementation rather than maintaining a second independent
  algorithm
- this policy is already applied for inverse-distribution iteration and the
  gamma-family behavior-sensitive paths

## Closeout Status

The in-scope convergence work tracked by this document is complete.

Completed convergence families:

- Fisher / beta / poisson / binomial
- normal, log-normal, chi-square, legacy chi, gamma, student-t, F, and
  `BINOM.INV`
- inverse distributions and confidence functions
- hypergeometric, percentile, quartile, median, mode, skew, and skewp
- exponential distribution, Weibull distribution, negative-binomial
  distribution, combinatorics, and `Erf` / `Erfc`

The remaining work is no longer a convergence backlog. Anything left outside
these families falls into either ongoing maintenance or the out-of-scope Calc
authority areas listed below.

## Convergence Policy

For the remaining work, the shared engine should stay the single maintained
implementation for any pure-computation family that is extracted.

Rules:

- do not keep parallel Calc-only and shared-runtime implementations for the
  same behavior-sensitive algorithm
- if adopting a shared implementation would otherwise risk a changed result,
  port Calc's original algorithm into the shared runtime and make both callers
  consume it
- keep Calc-local code only where the function is inherently tied to Calc
  document/storage/runtime infrastructure

Already resolved under this policy:

- inverse distribution iteration uses Calc's original interpolation/bracketing
  strategy in shared runtime
- shared gamma behavior follows Calc's original Lanczos-based path for the
  behavior-sensitive surfaces that were previously divergent

## Functions That Remain Out Of Scope

The following should remain Calc-owned unless the surrounding architecture
changes materially:

- database/query functions tied to `ScDocument`
- external / macro / UNO functions
- matrix operations tightly coupled to `ScMatrix`
- heavily storage-layout-specific Calc aggregation internals
- reference/scheduling-heavy behaviors whose authority remains in Calc

## Validation Requirements

Every remaining convergence slice should satisfy all of the following before it
lands.

### Calc-side Validation

- add or extend targeted Cppunit coverage in `sc/qa/unit/`
- verify the delegated Calc entry points still preserve existing error and edge
  behavior
- prefer differential assertions when replacing a historically sensitive Calc
  implementation

### Shared-Engine Validation

- add focused standalone tests for any new shared runtime family
- keep representative replay targets green for the touched family
- keep the promoted standalone replay corpus green

### Required Gates

- `make -j4 CppunitTest_sc_ucalc_formula2` for the statistical convergence lane
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests`
- `git diff --check`

## Completion Criteria

This convergence plan is complete when all of the following remain true:

- no in-scope pure-computation family tracked here has separate Calc-only and
  shared-runtime implementations
- behavior-sensitive families continue to use the Calc-aligned shared-runtime
  algorithms
- Calc and standalone validation stay green on the shared-runtime paths

## Critical Files

| File | Role |
| --- | --- |
| [MathStatistical.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/MathStatistical.hxx) | Shared statistical runtime surface |
| [MathStatistical.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/MathStatistical.cxx) | Shared statistical runtime implementation |
| [MathAggregate.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/MathAggregate.hxx) | Shared aggregate/statistics runtime surface |
| [MathAggregate.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/MathAggregate.cxx) | Shared aggregate/statistics runtime implementation |
| [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx) | Calc aggregate dispatch touchpoints |
| [interpr3.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr3.cxx) | Calc statistical function home |
| [interpr7.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr7.cxx) | Calc error-function and related helpers |
| [ucalc_formula2.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/ucalc_formula2.cxx) | Focused Calc-side convergence coverage |
