# FormulaEvaluator / ScInterpreter Convergence Plan

## Purpose

This document is the implementation reference for the next body of work after
the initial `FormulaEvaluator` runtime extractions. It consolidates:

- the current extracted-runtime baseline
- the remaining refactor slices needed to shrink
  [FormulaEvaluator.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluator.cxx)
- the specific convergence opportunities between the standalone engine and
  Calc's `ScInterpreter`
- the validation and sequencing rules we should follow as we land the work

This is not just a historical report. It is the working plan for finishing the
runtime extraction and creating a practical shared-implementation path between
the standalone evaluator and Calc.

## Executive Summary

The `FormulaEvaluator` refactor is well underway, and slices 1 through 9 are
now substantially complete in the codebase.

What is already true:

- the evaluator has been renamed from the old FODS-specific naming
- duplicated runtime logic for `CEILING.MATH`, `UPPER`, `LOWER`, `CHAR`, and
  `CODE` is no longer inline
- the first pure statistical cluster has been extracted to
  `MathStatistical`
- query and criteria matching now live in `QueryRuntime`
- criteria aggregate execution also now lives in `QueryRuntime`
- lookup traversal now lives in `LookupRuntime`
- shared evaluator text support now lives in `TextRuntimeSupport`
- the major extracted text-family runtime now lives in `TextFunctionRuntime`
- date/time parsing and coercion now live in `DateTimeParse`
- financial orchestration now lives in `FinancialRuntime`
- the remaining statistical foundation and distribution layer now live in
  `MathStatistical`
- aggregate/statistics helpers now live in `MathAggregate`
- conversion and numeral/add-in wrappers now live in `ConversionRuntime`
- the remaining evaluator math wrappers now largely live in
  `MathFunctionRuntime`

What is not yet true:

- aggregate traversal families, compiled-token inflation, and special-form
  orchestration still keep meaningful logic inline in `FormulaEvaluator`
- Calc's `ScInterpreter` still does not delegate to the extracted engine
  runtime modules

The next phase of work should finish the `FormulaEvaluator` split into
family-level runtime modules and then begin systematic `ScInterpreter`
convergence where the shared runtime is already mature enough.

## Goals

By the end of this program:

- `FormulaEvaluator.cxx` should stop being the monolithic home for unrelated
  function families and helper subsystems
- pure computation should live in shared runtime modules under
  `spreadsheet_engine/runtime/`
- evaluator-owned code should be limited to:
  - workbook/reference materialization
  - recursion/cycle/caching
  - local-binding and special-form orchestration
  - thin dispatch into runtime modules
- `ScInterpreter` should begin consuming the same runtime implementations for
  shared pure-computation function families

## Non-Goals

This plan does not aim to:

- rewrite `ScInterpreter` wholesale
- extract database, UNO, macro, or UI-tied functions
- move storage ownership out of Calc
- force every function into runtime form if it is inherently reference- or
  scheduling-heavy
- settle all execution-authority questions between Calc and standalone

## Current Runtime Baseline

As of this plan, there are 25 runtime modules under
`spreadsheet_engine/inc/spreadsheetengine/runtime/`:

| Module | Primary Domain |
| --- | --- |
| `MathStatistical` | Statistical distributions and related math |
| `MathFinancial` | Financial kernels |
| `MathRounding` | Rounding behavior |
| `MathTranscendental` | Trigonometric and exponential math |
| `MathBitwise` | Bitwise operators |
| `MathScalar` | Scalar math |
| `QueryRuntime` | Criteria matching and criteria aggregates |
| `TextCase` | Case conversion |
| `TextScalar` | Scalar text helpers |
| `TextWidth` | Width conversion (`ASC` / `JIS`) |
| `DateTimeParts` | Date/time parts and serial helpers |
| `DateTimeParse` | Date/time lexical parsing and coercion helpers |
| `DateTimeWeek` | Week-number behavior |
| `DateTimeWorkday` | Workday/network-day behavior |
| `ConversionRuntime` | Spreadsheet-facing conversion and add-in wrappers |
| `FinancialRuntime` | Spreadsheet-facing financial orchestration |
| `MathAggregate` | Aggregate/statistics helpers over collected numeric inputs |
| `MathFunctionRuntime` | Spreadsheet-facing math wrapper behavior |
| `NumeralConversion` | Base/roman/numeral conversion |
| `TextServices` | Case-mapping and encoding interfaces |
| `LookupRuntime` | Lookup traversal and search planning |
| `TextFunctionRuntime` | Higher-level text search/slice/replace helpers |
| `TextRuntimeSupport` | Shared ICU-backed text plumbing and services |
| `InMemoryHost` | Standalone host support |
| `LibraryProbe` | Shared library probe logic |

## Completed Extractions

### Runtime extraction already completed

The following evaluator concerns are already split out:

| Area | Shared Runtime Surface | Notes |
| --- | --- | --- |
| Case conversion | `TextCase` | `UPPER` / `LOWER` use the runtime layer |
| Text scalar encoding | `TextScalar` | `CHAR` / `CODE` use shared runtime services |
| Precise ceiling/floor math | `MathRounding` | `CEILING.MATH` and `FLOOR.MATH` no longer duplicate the logic inline |
| First statistical cluster | `MathStatistical` | Fisher, beta, poisson, binomial, and related helpers extracted |
| Query matching | `QueryRuntime` | folded text compare, wildcard/regex semantics, criteria predicate construction/matching |
| Criteria aggregates | `QueryRuntime` | `COUNTIF(S)`, `SUMIF(S)`, `AVERAGEIF(S)`, `MAXIFS`, `MINIFS` aggregate loop extracted |
| Lookup traversal | `LookupRuntime` | `VLOOKUP`, `HLOOKUP`, `LOOKUP`, `MATCH`, `XMATCH`, `XLOOKUP` traversal extracted |
| Shared text plumbing | `TextRuntimeSupport` | evaluator-local Unicode, encoding, width, and code-point helpers extracted |
| Text family runtime | `TextFunctionRuntime` | `SEARCH` / `FIND`, `TEXTAFTER` / `TEXTBEFORE`, `MID`, `REPLACE`, `SUBSTITUTE`, `LEFT` / `RIGHT`, byte-text helpers extracted |
| Date/time parsing | `DateTimeParse` | standalone number/date/time coercion and serial-building helpers extracted |
| Financial orchestration | `FinancialRuntime` | `FV`, `PV`, `PMT`, `NPER`, `RATE`, `ISPMT`, `IPMT`, `PPMT`, `CUMIPMT`, `CUMPRINC`, `DDB`, `VDB` orchestration extracted |

### Naming and evaluator scope clarification already completed

The evaluator is now correctly named and positioned as a general engine
formula evaluator rather than a FODS-specific implementation:

- `FodsEvaluator.hxx` -> `FormulaEvaluator.hxx`
- `FodsEvaluator.cxx` -> `FormulaEvaluator.cxx`
- namespace moved to `spreadsheetengine::core::eval`

## Current Shape Of FormulaEvaluator

[FormulaEvaluator.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluator.cxx)
is still approximately 11k lines. It still contains four broad classes of code:

1. evaluator-owned orchestration
2. still-inline family implementations
3. representation bridges and compiled-token inflation
4. a giant function dispatch ladder

That means the remaining work should not be treated as one blob. It should be
completed as deliberate slices that each reduce one cohesive concern.

## Target End State

The target shape is:

- `FormulaEvaluator` owns:
  - workbook access and materialization
  - AST / compiled-token entrypoints
  - cache and cycle tracking
  - local-binding and special-form orchestration
  - minimal adapters for runtime modules
- runtime modules own:
  - pure computation
  - family-level traversal rules that only need abstract materialization seams
  - text/query/date parsing helpers
  - statistical and aggregate kernels
- token inflation and compiler-token bridging move to detail/compiler support
  code rather than living in the evaluator

## Recommended Remaining Slices

The remaining work should be executed in the following order.

### Slice 1: LookupRuntime extraction

Status: substantially complete

Extract the large lookup traversal cluster from
[FormulaEvaluator.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluator.cxx),
centered around:

- `VLOOKUP`
- `HLOOKUP`
- `LOOKUP`
- `MATCH`
- `XMATCH`
- `XLOOKUP`

This slice should move:

- `LookupInput`
- exact text-vs-number comparison behavior
- approximate search traversal
- forward/reverse/binary-style search modes
- result-index planning and row/column orientation logic

Recommended target:

- new `runtime/LookupRuntime.hxx`
- new `source/core/LookupRuntime.cxx`
- evaluator-owned `LookupMaterializer` seam similar to the new
  `QueryRuntime` materializer

Why this is next:

- it is the biggest remaining reusable traversal subsystem
- it already partially depends on query-style helpers now in `QueryRuntime`
- it will substantially shrink the giant dispatch ladder

### Slice 2: Shared evaluator text support extraction

Status: substantially complete

Extract the top-of-file Unicode/text support utilities that still live in the
evaluator:

- Unicode conversion helpers
- code-point slicing and replacement
- case/encoding service implementations
- search-oriented text utilities still owned by the evaluator

Recommended target:

- extend `TextServices`, `TextScalar`, or add a small `TextSearchRuntime`
  support module if the responsibilities are clearer that way

Goal:

- stop using evaluator-local ICU helpers as ad hoc infrastructure for many
  unrelated text functions

### Slice 3: Text function family extraction

Status: substantially complete

Move the major text family into runtime modules:

- `SEARCH` / `FIND`
- `TEXTAFTER` / `TEXTBEFORE`
- `MID`
- `REPLACE`
- `LEFT` / `RIGHT`
- `SUBSTITUTE`
- `PROPER`
- `ASC` / `JIS`
- `LENB`, `FINDB`, `SEARCHB`, `REPLACEB`
- `TEXTJOIN`, `CONCAT`, and remaining string assembly helpers where practical

Recommended target:

- extend `TextScalar` and `TextWidth`
- add a new `TextSearch` or `TextTransform` runtime module if the extracted
  surface becomes too broad for existing modules

Goal:

- keep string-processing logic together and reusable

### Slice 4: Date/time parsing and coercion extraction

Status: substantially complete

Move date/time lexical parsing and serial-building orchestration out of the
evaluator:

- date text parsing
- time text parsing
- ODF duration parsing
- `VALUE`
- `DATEVALUE`
- `TIMEVALUE`
- `DATE`
- `TIME`
- `DATEDIF`
- `EDATE`
- `EOMONTH`
- `DAYSINMONTH`
- `DAYSINYEAR`
- `ISOWEEKNUM`
- `WEEKS`

Recommended target:

- extend `DateTimeParts`, `DateTimeWeek`, and `DateTimeWorkday`
- add a new `DateTimeParse.hxx` / `.cxx` for lexical parsing if needed

Goal:

- separate pure date/time rules from evaluator-specific argument handling

### Slice 5: Financial orchestration extraction

Status: substantially complete

The numerical kernels already live in `MathFinancial`, but much of the
function-family orchestration still lives inline:

- `FV`
- `PV`
- `PMT`
- `NPER`
- `RATE`
- `ISPMT`
- `IPMT`
- `PPMT`
- `CUMIPMT`
- `CUMPRINC`
- `DDB`
- `VDB`

Recommended target:

- extend `MathFinancial` with argument-normalization-friendly entrypoints
- possibly add a small `FinancialRuntime` wrapper if we want to keep raw
  numerical kernels separate from spreadsheet-facing orchestration

Goal:

- make the evaluator dispatch thin and reusable for both standalone and Calc

### Slice 6: MathStatistical phase 2

Status: substantially complete

Extend `MathStatistical` to absorb the remaining inline statistical foundation
and distribution functions.

#### Tier A: Foundation math

Extract the pure helper layer:

- `phiValue`
- `taylorPolynomial`
- `gaussValue`
- `gammaContinuedFraction`
- `gammaSeries`
- `lowRegularizedIncompleteGamma`
- `upRegularizedIncompleteGamma`
- `invertMonotonicPositiveDistribution`

#### Tier B: Distribution evaluators

Extract the distribution layer that depends on the above:

- `evaluateLegacyChiDist`
- `evaluateBinomialInverse`
- `evaluateNormalDistribution`
- `evaluateLogNormalDistribution`
- `evaluateChiSquareDistribution`
- `evaluateGammaDistribution`
- `evaluateGammaValue`
- `evaluateStudentDistribution`
- `evaluateTInverse`
- `evaluateFRightTailDistribution`
- `evaluateFInverseRightTail`

Goal:

- make `MathStatistical` the single home for the engine's shared statistical
  runtime

Current state:

- the remaining statistical foundation and distribution helpers are now routed
  through `MathStatistical`
- the former inline helper stubs have been removed from `FormulaEvaluator`
- a small residual statistical tail may still be worth polishing later, but
  the slice goal is met

### Slice 7: Aggregate/statistics runtime extraction

Status: substantially complete

Move the remaining aggregate/statistical collection-based helpers out of the
evaluator:

- `evaluateVarianceNumbers`
- `evaluateTrimmean`
- `evaluateModeSingle`
- `evaluateHypergeometricDistribution`
- `evaluatePercentrank`
- `evaluateAggregateNumbers`
- `evaluateAggregateRankedNumbers`

Recommended target:

- either extend `MathStatistical`
- or introduce a new `MathAggregate.hxx` / `.cxx`

The decision should be guided by cohesion:

- if the functions are mostly distribution/statistics helpers, keep them in
  `MathStatistical`
- if the file becomes too broad, use `MathAggregate`

Current state:

- `MathAggregate` now owns the extracted aggregate/statistics helper family
- evaluator extrema dispatch is reduced to thinner collection plus runtime
  delegation
- `SUBTOTAL` / `AGGREGATE` traversal is still evaluator-owned and is the main
  remaining follow-on for this area

### Slice 8: Conversion and numeral/add-in wrapper extraction

Status: substantially complete

Move the conversion cluster that still lives inline:

- `EUROCONVERT`
- `CONVERT`
- `DECIMAL`
- `DEC2HEX`
- `BASE`
- `ROMAN`

Recommended target:

- extend `NumeralConversion`
- add a narrow conversion-runtime wrapper if the function-family argument
  behavior should be kept separate from raw conversion kernels

Current state:

- the conversion cluster now lives in `ConversionRuntime`
- evaluator call sites for `EUROCONVERT`, `CONVERT`, `DECIMAL`, `DEC2HEX`,
  `BASE`, and `ROMAN` delegate through the runtime layer

### Slice 9: Remaining math-family wrappers

Status: substantially complete

Finish moving evaluator-local math wrappers into the existing runtime families:

- `ROUND`
- `ROUNDUP`
- `ROUNDDOWN`
- `CEILING` / `FLOOR` variants
- `ROUNDSIG`
- `MROUND`
- `COMBIN`
- `COMBINA`
- `MULTINOMIAL`
- `CSC`
- `CSCH`
- `TRUNC`
- `LOG`
- `MOD`
- remaining scalar/transcendental helpers that still dispatch inline

Recommended target:

- `MathRounding`
- `MathScalar`
- `MathTranscendental`
- `MathBitwise`
- `MathStatistical`

Current state:

- the bulk of the remaining evaluator-local math wrappers now dispatch through
  `MathFunctionRuntime`
- `MOD` and the extrema helpers joined that runtime-oriented cleanup in the
  same refactor window
- any remaining inline math calls are now tail work rather than a major
  evaluator-owned subsystem

### Slice 10: Aggregate-family extraction

Move the aggregate traversal families out of the evaluator:

- `MAX`
- `MIN`
- `MAXA`
- `MINA`
- `SUBTOTAL`
- `AGGREGATE`
- `LARGE`
- `SMALL`
- `PERCENTILE`
- `QUARTILE`
- `SKEW`
- `SKEWP`

This will likely depend on Slice 7 first.

### Slice 11: Special-form isolation

Not every evaluator-owned feature should become a pure runtime module, but it
still should stop living inside one giant file. Isolate the evaluator-specific
special-form cluster into its own implementation unit:

- `IF`
- `LET`
- `IFERROR`
- `IFNA`
- `FORMULA`
- `INDIRECT`
- `OFFSET`
- `HYPERLINK`
- reference-heavy evaluator-only behaviors

Recommended target:

- new evaluator-local implementation file such as
  `FormulaEvaluatorSpecialForms.cxx`

### Slice 12: Compiled-token inflation extraction

Extract compiled-token inflation and AST reconstruction helpers from the
evaluator into detail/compiler support.

Primary target:

- `inflateCompiledFormulaNode`

Recommended target:

- `detail/compiler/CompiledFormulaInflation.hxx`
- `source/core/CompiledFormulaInflation.cxx`

This is lower priority than family-level runtime extraction because it is more
representation-specific than function-family-specific.

### Slice 13: Dispatch-table cleanup

After the family extractions land, replace the long
`if (aFunctionName == ...)` ladder in `Evaluator::evaluateFunction()` with a
clearer family dispatch structure.

Options:

- family-level dispatch functions
- a function table keyed by normalized function name
- a mixed approach where high-level family dispatch remains hand-written and
  runtime-family entrypoints are table-driven

Goal:

- make further growth and maintenance tractable

## Recommended Execution Order

The preferred order is:

1. `LookupRuntime`
2. shared text support
3. text family
4. date/time parsing
5. financial orchestration
6. `MathStatistical` phase 2
7. aggregate/statistics runtime
8. conversion/numeral wrappers
9. remaining math wrappers
10. aggregate-family traversal
11. special-form isolation
12. compiled-token inflation
13. dispatch cleanup

Reasoning:

- the first slices remove the biggest cross-cutting reusable subsystems
- the middle slices align with existing runtime module boundaries
- the later slices depend on the earlier extractions for cleaner seams

## ScInterpreter Convergence Plan

Once the engine runtime surface is broad enough, begin `ScInterpreter`
convergence in three phases.

### Phase A: Immediate delegations

Functions that can delegate immediately once runtime APIs are stable and LO
adapter plumbing is added:

| ScInterpreter Function | Current Runtime Equivalent |
| --- | --- |
| `ScFisher` | `fisherTransform()` |
| `ScFisherInv` | `inverseFisherTransform()` |
| `ScPoissonDist` | `evaluatePoissonDistribution()` |
| `ScBinomDist` | `evaluateBinomialDistribution()` |
| `ScB` | `evaluateBinomialRangeDistribution()` |
| `ScBetaDist` | `evaluateBetaDistribution()` |
| `ScBetaDist_MS` | `evaluateBetaDistribution()` |
| `GetBeta` | `betaValue()` |
| `GetBetaDist` | `betaCdf()` |

### Phase B: Delegations unlocked by remaining extraction

These Calc functions should converge after `MathStatistical` phase 2 and the
aggregate/statistics extraction:

- chi-square family
- gamma family
- normal/log-normal family
- student-t family
- F-distribution inverse family
- `BINOM.INV`
- hypergeometric
- percentile/quartile/median/mode/skew

### Phase C: New runtime implementations

These exist in Calc but do not yet have evaluator/runtime counterparts and
should be treated as new shared-runtime work items if we decide they belong in
the shared engine:

- inverse distributions such as `ScNormInv`, `ScGammaInv`, `ScBetaInv`,
  `ScChiSqInv`
- confidence-interval functions
- exponential / Weibull / negative-binomial families
- additional descriptive-statistics helpers
- combinatorics not yet shared
- `Erf` / `Erfc`
- `gaussinv`

## Functions That Should Not Be Extracted Into Shared Runtime

The following classes should remain Calc-owned unless the architecture changes
substantially:

- database functions that depend on `ScDocument` query infrastructure
- external/macro/UNO functions
- matrix operations tightly coupled to `ScMatrix`
- heavily optimized Calc-local aggregation tied to storage layout
- reference/scheduling-heavy behaviors whose authority remains in Calc

These may still be isolated inside Calc, but they are not current
shared-runtime targets.

## Key Algorithmic Decisions To Resolve

### Inverse distribution iteration strategy

Current mismatch:

- Calc uses `lcl_IterateInverse` with inverse quadratic interpolation and
  bracketing
- the evaluator currently uses bracket-doubling plus pure bisection

Decision needed:

- choose whether the shared runtime standard should be:
  - Calc's current Brent-like behavior
  - the current evaluator behavior
  - a new shared implementation validated against both

Recommended default:

- converge on one shared runtime implementation and make both callers consume it
- prefer the algorithm that gives the best combination of stability, parity,
  and maintainability after empirical comparison

### Gamma implementation strategy

Current mismatch:

- Calc uses an explicit Lanczos-based implementation
- the evaluator uses `std::tgamma` / `std::lgamma` in some remaining paths

Decision needed:

- whether to standardize on:
  - explicit shared Lanczos-based implementation
  - standard-library gamma functions
  - a split policy with documented rationale

Recommended default:

- choose a single shared implementation if parity and edge-case behavior allow
- document and test any deliberate split if one remains necessary

## Validation Strategy

Every extraction slice should satisfy all of the following before it lands.

### Standalone validation

- focused unit tests for the extracted family
- targeted replay on the family's representative FODS workbooks
- full promoted replay gate
- `git diff --check`

### Calc-side validation for convergence slices

For any `ScInterpreter` delegation:

- add targeted Cppunit coverage in `sc/qa/unit/`
- preserve old behavior behind differential assertions during rollout when
  practical
- validate both standalone and Calc call paths against the same shared runtime
  expectations

### Required replay targets for lookup/query slices

At minimum, keep these green while lookup/query extraction is active:

- `countif.fods`
- `lookup.fods`
- `xlookup.fods`
- full promoted replay corpus

### Required replay targets for statistical slices

At minimum, keep representative statistical replay green for:

- `poisson.fods`
- `binomdist.fods`
- `betadist.fods`
- chi-square / gamma / t / f inverse workbooks as they are promoted into the
  shared runtime

## Per-Slice Acceptance Criteria

Each slice should meet these conditions before it is considered complete.

### Code shape

- a cohesive subsystem leaves `FormulaEvaluator.cxx`
- the extracted logic lives in a clearly named runtime or support module
- the evaluator depends on a narrow adapter seam, not on new global helpers

### Behavior

- no regression in the promoted standalone replay corpus
- focused family tests added or updated
- the new runtime API is reusable by future `ScInterpreter` convergence

### Documentation

- this plan is updated if scope or sequencing changes materially
- any algorithmic divergence decision is recorded in this document
- major new shared-runtime surfaces are reflected in `PROJECT_STATUS.md` as they
  land

## Work Tracking Checklist

### Runtime extraction

- [x] Rename `FodsEvaluator` to `FormulaEvaluator`
- [x] Route duplicated runtime logic to `TextCase`, `TextScalar`, and
  `MathRounding`
- [x] Create `MathStatistical` phase 1
- [x] Create `QueryRuntime` for criteria matching
- [x] Move criteria aggregate execution into `QueryRuntime`
- [x] Create `LookupRuntime`
- [x] Extract shared text support helpers
- [x] Extract text-family runtime
- [x] Extract date/time parse runtime
- [x] Extract financial orchestration
- [ ] Extend `MathStatistical` with phase 2 foundations and distributions
- [ ] Extract aggregate/statistics collection runtime
- [ ] Extract conversion and numeral wrapper runtime
- [ ] Extract remaining math-family wrappers
- [ ] Extract aggregate-family traversal
- [ ] Isolate special forms into evaluator-owned files
- [ ] Extract compiled-token inflation
- [ ] Replace the giant dispatch ladder with family dispatch/table structure

### ScInterpreter convergence

- [ ] Add first Calc delegation to already-extracted statistical runtime
- [ ] Converge the beta/poisson/binomial/fisher family
- [ ] Converge the remaining distribution family after `MathStatistical` phase 2
- [ ] Converge aggregate/statistics helpers where shared runtime becomes
  available
- [ ] Decide the shared inverse-iteration algorithm
- [ ] Decide the shared gamma strategy

### Validation hardening

- [ ] Maintain focused evaluator/replay coverage for each extracted family
- [ ] Add Calc-side targeted tests for each adopted delegation
- [ ] Keep full promoted replay green across slices

## Recommended First Active Slice From Here

The next slice after the first-five-slice batch is `MathStatistical` phase 2.

Why:

- it is now the highest-value remaining pure-computation cluster still inline in
  `FormulaEvaluator`
- it unlocks the next meaningful `ScInterpreter` convergence opportunities
- it continues the pattern of moving reusable math kernels out of evaluator
  dispatch code before tackling more evaluator-owned special forms

## Critical Files

| File | Role |
| --- | --- |
| [FormulaEvaluator.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluator.cxx) | Remaining monolith to keep shrinking |
| [FormulaEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/FormulaEvaluator.hxx) | Evaluator interface |
| [MathStatistical.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/MathStatistical.hxx) | Statistical runtime surface to extend |
| [MathStatistical.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/MathStatistical.cxx) | Statistical runtime implementation |
| [QueryRuntime.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/QueryRuntime.hxx) | Query/criteria runtime surface |
| [QueryRuntime.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/QueryRuntime.cxx) | Query/criteria runtime implementation |
| [CoreSources.cmake](/home/ubuntu/repos/libreoffice/spreadsheet_engine/cmake/sources/CoreSources.cmake) | Standalone build wiring |
| [Library_spreadsheetengine.mk](/home/ubuntu/repos/libreoffice/spreadsheet_engine/integration/libreoffice/Library_spreadsheetengine.mk) | LibreOffice build wiring |
| `sc/source/core/tool/interpr3.cxx` | Calc statistical function home |
| `sc/source/core/tool/interpr6.cxx` | Calc incomplete-gamma and related helpers |
| `sc/source/core/tool/interpr7.cxx` | Calc error-function and related helpers |
