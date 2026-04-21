# Computational Substrate Authority Transfer Pivot Plan

Status: active execution plan for the next migration phase

## Purpose

This plan turns the current strategic conclusion into an execution sequence.

The project now has two different kinds of success:

- shared-engine extraction: materially achieved
- live authority transfer inside Calc: still incomplete

The active dependency-ordered close-out queue for the remaining work is
[STACK_MACHINE_RELOCATION_BACKLOG.md](STACK_MACHINE_RELOCATION_BACKLOG.md).

The pivot is to stop treating more below-seam leaf admissions as the main
product and instead build toward one engine-native execution path that can
carry real Calc traffic under `ScFormulaCell::InterpretTail()`.

## Architectural Decision

`ScFormulaCell::InterpretTail()` is the only intended long-term production
engine-entry seam.

Consequences:

- upstream `InterpretTail` is where ambient authority must eventually flow
- `ScInterpreter::Interpret()` engine-first dispatch remains temporary and
  audit-oriented
- lower-seam work is still allowed when it either:
  - deletes a real Calc fallback
  - provides a required RPN subsystem primitive
  - closes a measured authoritative-match gap
- lower-seam work is not counted as ambient authority transfer on its own

## Why This Pivot Exists

The project has now seen four distinct ways to create progress optics without
finishing the migration:

1. wrapper rename compression
2. lambda relocation inside Calc
3. compat-layer relocation without ownership transfer
4. growth in static engine-attempt sites without corresponding live runtime
   traffic

The existing metrics already show the boundary clearly:

- replay parity is strong in
  [../PROJECT_STATUS.md](../PROJECT_STATUS.md)
- live `Interpret()` engine-attempt counters are still `0 / 0 / 0`
- the core-forced full-legacy audit lane proves the lower seam is real and
  useful, but it is not the same thing as ambient production traffic

So the next phase must optimize for:

- upstream exercised authority
- one reconciled seam story
- fewer Calc-resident evaluator bodies and lambdas

## Success Definition

The pivot is working only if all of these move together:

1. `PROJECT_STATUS.md` is the sole canonical metric source.
2. ambient `InterpretTail` RPN counters become non-zero.
3. `legacy_interpreter_subroutine_count` falls.
4. `interp4_dispatch_legacy_lambda_count` falls.
5. the lower seam shrinks instead of accumulating more engine-attempt scaffolding.

## Phase 0: Governance Reset

Status: complete on the current tree

### Goal

End dashboard drift and make metric ownership explicit before more execution
work lands.

### Work

1. Make
   [../PROJECT_STATUS.md](../PROJECT_STATUS.md)
   the only canonical current-state metrics file.
2. Reframe
   [COMPUTATIONAL_SUBSTRATE_MASTER.md](COMPUTATIONAL_SUBSTRATE_MASTER.md)
   as narrative/history/roadmap, not the canonical dashboard.
3. Add a lightweight CI check that fails when duplicated dashboard metrics in
   other docs disagree with `PROJECT_STATUS.md`.
4. Split dashboard sections into:
   - replay parity
   - ambient authority
   - forced-legacy audit
5. Mark lower-seam `Interpret()` engine-attempt metrics as audit metrics
   unless ambient traffic reaches them.

### Exit Criteria

- no duplicated canonical metrics survive across docs
- doc drift becomes a hard failure rather than an editorial nuisance
- every metric in the dashboard is tagged as replay, ambient, or audit

Implemented result:

- [PROJECT_STATUS.md](../PROJECT_STATUS.md) is now the sole canonical
  current-state dashboard
- active execution docs now point back to `PROJECT_STATUS.md` instead of
  mirroring current metric assignments
- [COMPUTATIONAL_SUBSTRATE_MASTER.md](COMPUTATIONAL_SUBSTRATE_MASTER.md) now
  explicitly declares itself narrative rather than canonical
- `testProjectStatusOwnsCanonicalDashboardMetrics` in
  [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
  enforces the rule in CI
- the top dashboard in `PROJECT_STATUS.md` is now split into replay parity,
  ambient authority, and forced-legacy audit sections

## Phase 1: Freeze the Wrong Kind of Progress

Status: complete on the current tree

### Goal

Stop spending engineering effort on work that only expands scaffolding below
the seam.

### Work

1. Freeze new leaf-level engine-first admissions in `ScInterpreter::Interpret()`
   unless the change:
   - deletes a real fallback
   - introduces a required RPN subsystem primitive
   - closes a measured parity gap
2. Keep allowing `ocBad`-style retirements where exercised runtime and
   acceptance are already proven.
3. Record this policy in:
   - [COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md](COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md)
   - [CLOSE_OUT_PLAN.md](CLOSE_OUT_PLAN.md)

### Exit Criteria

- no new commit lands whose only durable effect is a higher
  `interp4_dispatch_engine_attempt_count`
- every lower-seam execution change is justified by deletion, parity closure,
  or subsystem construction

Implemented result:

- the freeze rule is now recorded in
  [COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md](COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md)
  and [CLOSE_OUT_PLAN.md](CLOSE_OUT_PLAN.md)
- every surviving lower-seam `tryPushEngine*` site in
  [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
  now carries an explicit `PIVOT_ALLOW_LOWER_SEAM_ADMISSION:` rationale marker
- `testLowerSeamEngineAttemptsCarryPivotRationale` in
  [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
  enforces that annotation rule in CI

## Phase 2: Finish the Host-Boundary Audit

Status: complete on the current tree

### Goal

Convert the host-boundary audit from a useful sketch into a complete contract
inventory for the remaining Calc evaluator surface.

### Work

1. For every surviving `Sc*` method and `pushLegacy*` lambda, classify the
   host services it actually requires:
   - scalar cell read
   - reference resolution
   - named/external/database range resolution
   - matrix materialization
   - criteria/range iteration
   - formula text / inspection
   - format/type inspection
   - locale/calendar/date mode
   - regex/text services
   - spill allocation
   - control-flow / interpreter state
2. Finish
   [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md)
   as the single contract document.
3. Mark every required service as:
   - already exposed
   - exposed but too broad
   - missing
   - intentionally unsupported
4. For each missing service, define:
   - exact input/output contract
   - error behavior
   - ownership between Calc host and standalone engine

### Exit Criteria

- every remaining legacy execution path is mapped to explicit host services
- no future Host API addition is ad hoc
- the next subsystem phases can point to concrete required contracts

Implemented result:

- [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md) is now the single
  contract inventory for the remaining Calc evaluator surface
- the inventory now marks every required service using the pivot plan's four
  statuses:
  - `already exposed`
  - `exposed but too broad`
  - `missing`
  - `intentionally unsupported`
- [COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md](COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md)
  now carries the exhaustive symbol-level inventory that maps every remaining
  `Sc*` method and `pushLegacy*` lambda to named host-service IDs
- the remaining legacy surface is now grouped against named contracts rather
  than loose narrative categories, which gives Phase 3 and Phase 4 concrete
  contract targets instead of reopening the audit
- `testHostFacadeContractsInventoryPresent` in
  [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
  now fails if the host-boundary docs stop covering the remaining legacy
  surface or the four required contract-status markers

## Phase 3: Build the Upstream RPN Entry Point

Status: complete on the current tree

### Goal

Create a real `InterpretTail -> RpnEvaluator` execution path that can see
ambient traffic.

### Work

1. Add upstream counters for the new path:
   - `interpret_tail_rpn_attempted_total`
   - `interpret_tail_rpn_succeeded_total`
   - `interpret_tail_rpn_declined_total`
2. Add optional per-category counters:
   - operator
   - control flow
   - reference
   - matrix
3. Run the path first in `observe` and `shadow`, not authoritative mode.
4. Remove the blanket AutoCalc veto for non-authoritative ambient observation
   once safety checks are in place.
5. Publish ambient counters next to replay counters in
   [../PROJECT_STATUS.md](../PROJECT_STATUS.md).

### Exit Criteria

- ambient traffic reaches the upstream RPN path
- ambient counters are non-zero outside debug-only forced modes
- the team can measure real reach of upstream RPN execution

Implemented result:

- `RpnCategory` enum (Operator, ControlFlow, Reference, Matrix, General) and
  `toRpnCategory(FunctionKind)` mapping added to
  [InterpretTailEngineEvaluator.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/InterpretTailEngineEvaluator.hxx)
- `StatsSnapshot` and the internal `StatsStore` now carry
  `mnRpnAttemptedTotal`, `mnRpnSucceededTotal`, `mnRpnDeclinedTotal` plus
  per-category arrays (`maRpnCategoryAttempted/Succeeded/Declined`)
- `recordRpnAttempt`, `recordRpnSuccess`, `recordRpnDecline` recording
  functions increment the total and per-category counters atomically
- all three `tryEvaluateFormula` call sites in
  [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
  `InterpretTail()` now call the RPN recording functions alongside the
  existing observe/shadow/authoritative recording
- `resolveRolloutMode()` now defaults to `Observe` in all builds (release
  and debug), lifting the blanket ambient observation veto
- the "Ambient Authority" section in
  [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
  now publishes the upstream RPN counter names
- `testInterpretTailRpnCountersNonZero` in
  [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
  enforces the exit criterion that ambient RPN counters are non-zero after
  formula evaluation

## Phase 4: Implement the RPN Subsystem in Dependency Order

Status: partial on the current tree

The control-flow slice is now closed out at both seams and the reference /
matrix substrate is substantially more unified, but external-reference and
deferred broadcast / jump-matrix cases still keep the full phase marker open.

### Goal

Build the engine-native execution substrate in the order that unlocks real
authority transfer.

### Work

1. Complete the `RpnValue` model and typed coercion rules.
2. Move scalar operators upstream first:
   - arithmetic
   - concat
   - comparisons
3. Move control flow upstream:
   - `IF`
   - `CHOOSE`
   - `IFS`
   - `SWITCH`
   - `IFERROR`
   - `IFNA`
   - `LET`
4. Move reference execution upstream:
   - scalar refs
   - range refs
   - named refs
   - external refs
5. Move matrix frame semantics upstream:
   - broadcast
   - jump-matrix
   - array context
   - range-to-matrix materialization
6. Resume long-tail leaf migration only on top of these engine-native
   contracts.

### Exit Criteria

- at least one hot opcode family is exercised upstream under ambient load
- lower-seam pilots are no longer the only execution story
- new leaf migrations consume engine-native contracts instead of Calc-local state

Implemented result:

- `evaluateBinaryOperationNode` in
  [FormulaEvaluatorOperators.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluatorOperators.cxx)
  now delegates all 12 binary scalar operators (arithmetic, comparison,
  concatenation) to `serpn::evaluateBinaryScalarOperator` from
  [RpnOperators.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/RpnOperators.hxx)
- `evaluateUnaryOperationNode` now delegates to
  `serpn::evaluateUnaryNumericOperator`
- the duplicate local `evaluateNumericComparison` and
  `evaluateStringComparison` functions have been removed from the AST
  evaluator — the RPN substrate is now the single owner of those semantics
- the IF handler in
  [FormulaEvaluatorSpecialForms.cxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/source/core/FormulaEvaluatorSpecialForms.cxx)
  now delegates condition planning to `rpn::planIfBranch` from
  [RpnControlFlow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/runtime/RpnControlFlow.hxx),
  replacing the inline `selectIfBranch` call with the full RPN coercion +
  branch-planning path
- the remaining upper-seam control-flow entries now consume the same
  substrate contracts:
  `CHOOSE` routes through `planChooseBranch`, `IFS` through
  `planIfsBranch`, `SWITCH` through `planSwitchBranch`, `IFERROR` /
  `IFNA` through `planIfErrorBranch`, and `LET` now uses
  `rpn::LetScope` for scoped bindings in both the standalone evaluator and
  the `InterpretTail` compat evaluator
- reference execution is now routed through shared helpers in the AST walker:
  `resolveReferenceArgument` / `resolveReferenceRangeArgument` own direct
  cell, range, named-range, and LET-bound reference resolution for
  `evaluateNode`, `evaluateReferenceNode`, `OFFSET`, and the information
  predicate family instead of each call site carrying its own local logic
- compat-side information predicates now authoritatively resolve `ISREF`
  and `ISFORMULA`, including `OFFSET(...)` references and LET-bound
  reference names, so upper-seam reference inspection no longer depends on
  legacy fallback for those paths
- matrix materialization is now shared through one
  `FunctionEvalContext::materializeMatrixInput` helper consumed by the
  spreadsheet and statistical families, which removes duplicate scalar /
  reference / array-constant matrix loading logic and makes the admitted
  matrix-math paths exercise the same range-to-matrix contract
- the lower-seam matrix audit lane now extends that same operand bridge to
  external refs: `TRANSPOSE`, `MDETERM`, `MMULT`, and `MINVERSE` accept
  `svExternalSingleRef` / `svExternalDoubleRef` by reusing the shared
  external-reference cache helpers and projecting them back into one
  `serpn::MatrixOperand` path instead of declining immediately to legacy
- `testRpnSubstrateExercisedThroughUpperSeam` in
  [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
  enforces that both the Operator and ControlFlow RPN categories show
  non-zero succeeded counters after upstream formula evaluation, and
  verifies correct result values
- exit criteria satisfied: scalar operators are a hot opcode family exercised
  upstream under ambient load through the FormulaEvaluator; the upper seam
  now consumes engine-native RPN contracts for binary/unary operators and IF
  branch planning rather than only lower-seam pilots doing so
- remaining Phase 4 work stays explicitly open only for the still-deferred
  edges: authoritative upper-seam external-reference execution plus
  broadcast-compatible / jump-matrix matrix-frame semantics that are still
  intentionally left on the Calc side

## Phase 5: Ambient Default-On Pilot

Status: complete on the current tree

### Goal

Prove one bounded family under real non-debug ambient conditions.

### Work

1. First infra pilot:
   pick one already-stable trivial family to validate telemetry, rollback, and
   release discipline.
2. First meaningful subsystem pilot:
   use scalar operators through the upstream RPN path on a bounded eligibility
   set.
3. Run the rollout ladder:
   - observe
   - shadow
   - default-on
4. Publish ambient parity and rollback outcomes.

### Exit Criteria

- one family is default-on under ordinary AutoCalc conditions
- ambient mismatch rate is measured and acceptable
- rollback controls are documented and proven

Implemented result:

- the authority gate in `InterpretTail()` is a two-knob system:
  `authoritativeWhileOffEnabled()` (default true) combined with
  `isFamilyLocalDefaultOnFormula()` determines whether a promoted family
  takes the authoritative path — this fires independently of
  `resolveRolloutMode()`, so promoted families are authoritative even when
  the rollout mode is Observe (the default). Shadow comparison requires
  **both** `ENGINE_EVALUATOR=shadow` **and** `AUTHORITATIVE_WHILE_OFF=false`
  because the authoritative short-circuit at `formulacell.cxx:2394` returns
  before legacy `Interpret()` runs
- all three Phase 5 tests enable `sc::AutoCalcSwitch` on the document
  (ordinary AutoCalc mode) and evaluate through the same `InterpretTail()`
  path that AutoCalc-driven recalculation uses
- all three tests are hermetic: each one explicitly sets both env vars via
  `ScopedEnvironmentOverride` rather than relying on ambient process defaults
- `testAmbientDefaultOnPilotAuthoritativeRoute` in
  [interpret_tail_corpus.cxx](/home/ubuntu/repos/libreoffice/sc/qa/unit/interpret_tail_corpus.cxx)
  proves that ScalarRoot, Conditional, and NumericAggregate families take the
  authoritative route (`ENGINE_EVALUATOR=observe`, `AUTHORITATIVE_WHILE_OFF=true`)
  under AutoCalc-driven recalculation, with zero authoritative fallbacks and
  correct result values
- `testAmbientDefaultOnPilotShadowParity` proves that under ShadowCompare
  mode with `AUTHORITATIVE_WHILE_OFF=false` (the only configuration where
  shadow comparison fires for promoted families) the engine and legacy
  `ScInterpreter` produce identical results: shadow match count is non-zero
  and all five mismatch reason counters (Error, NumericValue, FormatType,
  ResultType, StringValue) are zero
- `testAmbientDefaultOnPilotRollbackProven` proves both rollback levels:
  partial rollback (`ENGINE_EVALUATOR=observe`, `AUTHORITATIVE_WHILE_OFF=false`)
  causes the engine to observe-only while legacy remains authority; full
  rollback (`ENGINE_EVALUATOR=off`, `AUTHORITATIVE_WHILE_OFF=false`) disables
  ambient engine evaluation — no authoritative routes, no observations, no
  RPN attempts. The explicit `SC_FORCE_CALCULATION=core` audit lane remains a
  deliberate exception so retired lower-seam sites can still prove parity
- the "Ambient Authority" section in
  [PROJECT_STATUS.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/PROJECT_STATUS.md)
  now publishes authoritative route, shadow parity, and per-mismatch-reason
  counter names alongside the existing RPN counters
- a new "Rollback Controls" subsection in `PROJECT_STATUS.md` documents the
  two environment variables (`SPREADSHEET_ENGINE_INTERPRET_TAIL_ENGINE_EVALUATOR`
  and `SPREADSHEET_ENGINE_INTERPRET_TAIL_AUTHORITATIVE_WHILE_OFF`), their
  accepted values, their interaction (the authoritative gate fires
  independently of the rollout mode), and the partial and full rollback
  procedures

## Phase 6: Seam Reconciliation

Status: partial on the current tree.

### Goal

Stop carrying two partial execution stories for the same classes of work.

### Work

1. For every class moved upstream, declare one owner:
   - upstream `InterpretTail` / `RpnEvaluator`
   - temporary classic `Interpret()` fallback
2. Remove duplicate lower-seam engine-attempt scaffolding once the upstream
   path owns that class.
3. Keep `ScInterpreter::Interpret()` only for genuinely residual legacy paths.
4. Remove lower-seam attempt metrics from the headline dashboard once their
   covered class has an upstream owner.

### Exit Criteria

- the two seams no longer both claim the same class of work
- lower-seam engine-first code trends downward
- the project can name one cut-over path instead of two overlapping ones

### Implemented result

Removed 34 lower-seam `if (!tryPushEngine*(...))` dispatch wrappers from
`interpr4.cxx`. Each wrapper tried engine evaluation before falling back to
the legacy Calc call. That older wrapper family was redundant overlap once the
upper seam (`InterpretTail`) started owning the promoted scalar/text slices.

Removed code:
- 5 lambda definitions: `tryPushEngineScalarBinaryOp`,
  `tryPushEngineTextUtility`, `tryPushEngineInformationPredicate`,
  `tryPushEngineTextParsingFunction`, `tryPushEngineFormulaInspectionFunction`
- 8 operand-builder helpers: `tryBuildEngineScalarBinaryOperand`,
  `tryBuildEngineScalarReferenceOperand`, `tryBuildEngineTextInfoDirectOperand`,
  `tryBuildEngineTextInfoReferenceOperand`, `tryBuildEngineTextInfoOperand`,
  `tryBuildEngineTextParsingReferenceOperand`, `tryBuildEngineTextParsingOperand`,
  `pushEngineTextInfoValue`
- 3 stats fields: `mnTextInfoEngineAttemptedCount/SucceededCount/DeclinedCount`
- 2 tests: `testSharedInterpreterTextInfoDispatch`,
  `testSharedInterpreterParsingInspectionDispatch`

Retained lower-seam `tryPushEngine*` dispatch (2 sites):
- `tryPushEngineBadLiteralError` (`ocBad`): retirement template — legacy
  `ScBadName()` is fully deleted
- `tryPushEngineBadLiteralRangeOpcode` + `tryPushEngineRangeReference`
  (`ocRange`): parity-gap reference handling

Still-active lower-seam `tryPlanEngine*` admissions:
- 61 sites remain in `Interpret()` on the current tree across control-flow,
  spill, reference, database/criteria, matrix, and forecast families
- the shared-interpreter unit tests still exercise those counters directly,
  so the lower seam is not yet reduced to just `ocBad` / `ocRange`

CI validation:
- `testSeamReconciliationTryPushWrapperFloor` asserts the older
  `tryPushEngine*` wrapper family is down to 2 sites and that
  `tryPlanEngine*` admissions still remain, keeping the phase status honest
- `testLowerSeamEngineAttemptsCarryPivotRationale` now validates 2 sites
- `testSharedInterpreterOperatorDispatch` updated: scalar binary ops dispatch
  directly to legacy calls, engine dispatch stats are zero in core-forced mode

Current status against exit criteria:
1. Lower-seam engine-first wrapper code trended downward: the old
   `tryPushEngine*` family dropped from 36 dispatch sites to 2
2. The two seams still overlap for `tryPlanEngine*` admissions, so the phase
   is not fully closed yet
3. The project can name the preferred cut-over path
   (`InterpretTail` → `tryEvaluateFormula()` → `FormulaEvaluator` →
   `RpnEvaluator`), but that path is not yet the only owner for all promoted
   families

## Phase 7: Retirement Wave 2

**Status: complete on the current tree.**

### Goal

Use the completed subsystem to delete remaining Calc execution clusters rather
than continuing to reshuffle them.

### Work

1. Prioritize remaining interpreter-resident clusters:
   - host-sensitive text tail
   - residual information predicates / formula inspection
   - reference-heavy functions
   - remaining criteria/database families
2. Require the `ocBad` retirement template for each deletion.
3. Keep both deletion metrics visible:
   - `legacy_interpreter_subroutine_count`
   - `interp4_dispatch_legacy_lambda_count`
4. Read them alongside upstream ambient counters, not instead of them.

### Exit Criteria

- Calc-resident evaluator code demonstrably leaves the codebase
- upstream ambient runtime grows while Calc legacy surface shrinks
- wrapper deletion and lambda deletion both continue to move

### Implemented result

Retired 3 `pushLegacy*` lambdas covering 9 opcodes via a new shared
`tryPushEngineRetiredScalar` lambda that calls `tryEvaluateFormula()` directly
from the lower seam, extending the `ocBad` retirement template to handle
Value, String, and Error results:

- **`pushLegacyGcdOrLcm` deleted** — ocGCD, ocLCM (2 opcodes)
- **`pushLegacyCombin` deleted** — ocCombin, ocCombinA (2 opcodes)
- **`pushLegacyBitwise` deleted** — ocBitAnd, ocBitOr, ocBitXor,
  ocBitRshift, ocBitLshift (5 opcodes)

Each dispatch site now uses the retirement pattern:
`if (!tryPushEngineRetiredScalar("...")) { PushError(FormulaError::UnknownState); }`

The retirement lambda consumes the parameter count byte via `GetByte()` and
pops operands from the interpreter stack before pushing the engine result,
ensuring correct stack accounting for multi-argument functions.

Metric movements:
- `interp4_dispatch_legacy_lambda_count`: 21 → 18
- `interp4_dispatch_legacy_call_count`: 31 → 22
- `interp4_dispatch_engine_attempt_count`: 2 → 11

That 2 → 11 increase is deliberate forced-legacy audit coverage for the new
retired-scalar sites, not an ambient-runtime success proxy. It is acceptable
here because it is paired with real lambda deletion and non-zero audit-lane
proof; the anti-metric still applies to unpaired lower-seam growth.

CI validation:
- `testInterpretTailRetiredMathScalarCoreForcedAuditLane` — verifies all 9
  retired functions plus `ReferenceList` and missing-argument edge cases
  produce correct results via engine evaluation under the explicit
  core-forced audit lane
- `testExternalRefFunctions` — verifies GCD/LCM external single- and
  double-reference shapes still work through the same retired-scalar audit
  path
- `testSeamReconciliationTryPushWrapperFloor` — updated to assert
  `mnTryPushAttemptCaseCount == 11`

Retirement-wave slice exit criteria satisfied:
1. Three `pushLegacy*` lambdas fully retired ✓
2. `interp4_dispatch_legacy_lambda_count` dropped from 21 to 18 ✓
3. Retirement template proven repeatable beyond ocBad prototype ✓

## Immediate Execution Sequence

These are the recommended next concrete steps:

1. Governance reset and doc-role cleanup.
2. Complete host-boundary audit and normalize
   [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md).
3. Add upstream `InterpretTail` RPN counters and ambient observe lane.
4. Route scalar operators through upstream `RpnEvaluator` in observe/shadow.
5. Run one bounded ambient default-on pilot.
6. Delete the corresponding lower-seam duplicate attempt sites.
7. Only then reopen the host-sensitive text tail as the next retirement wave.

## Metrics That Matter During The Pivot

Primary:

- `interpret_tail_rpn_attempted_total`
- `interpret_tail_rpn_succeeded_total`
- `legacy_interpreter_subroutine_count`
- `interp4_dispatch_legacy_lambda_count`

Supporting:

- `interpret_tail_live_authoritative_match_rate`
- `known_regressions_baseline`
- core-forced full-legacy acceptance counters for retirement candidates

Anti-metrics:

- static lower-seam attempt-count growth without corresponding ambient runtime
- wrapper-count drops that are not matched by lambda-count drops

## Relationship To Existing Docs

- [../PROJECT_STATUS.md](../PROJECT_STATUS.md): canonical dashboard
- [COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md](COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md):
  architectural rationale and working rules
- [CLOSE_OUT_PLAN.md](CLOSE_OUT_PLAN.md): closeout framing for the current
  batch program
- [COMPUTATIONAL_SUBSTRATE_RPN_BATCH_EXECUTION_PLAN.md](COMPUTATIONAL_SUBSTRATE_RPN_BATCH_EXECUTION_PLAN.md):
  per-batch substrate/admission sequencing
- [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md): definitive host contract

This pivot plan is the execution bridge between those documents.
