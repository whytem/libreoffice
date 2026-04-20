# Engine RPN Evaluator Initiative

Status: active next-phase initiative

Detailed execution of the current strategic pivot is tracked in
[COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md).

## Purpose

The remaining Calc migration work is no longer well-described as "the next
hundred functions."

After the current honest baseline in
[../PROJECT_STATUS.md](../PROJECT_STATUS.md) — especially the combination of
strong replay parity, zero live lower-seam runtime attempts, and non-zero
forced-legacy audit traffic — the hard part is the RPN evaluator subsystem
itself:

- operator semantics over polymorphic stack values
- control-flow opcodes and jump execution
- reference-shaped operands
- matrix broadcast and array-formula state

The latest classic-tail follow-through proved an important planning point:
before widening more opcode pilots, we needed to make the unseen unknown-root
tail legible. The new per-root samples showed the old `operator:+` bucket was
really imported `TODAY() + n` date-offset formulas, not a broad arithmetic
contract gap. That band is now closed by materializing `TODAY()` as a scalar
child inside promoted scalar-root expressions, which cuts unseen live cells
from `49` to `21` without pretending root `TODAY()` has already migrated as its
own delegated family.
- criteria/database iteration
- stack and format/error propagation state

Those pieces do not peel apart cleanly into another long queue of isolated
leaf-function migrations.

## Why The Framing Changes Now

The project has already seen three failure modes that move secondary metrics
without truly advancing engine authority:

1. rename-only cleanup: wrapper count moves, but Calc still computes the same
   logic
2. lambda relocation into `Interpret()`: code moves from `interpr*.cxx` into
   `interpr4.cxx`, but still runs on the legacy stack machine
3. compat-layer relocation without authority: code moves closer to the engine,
   but still depends on Calc evaluator state rather than an engine-native
   execution model

The remaining work should therefore be framed as one subsystem initiative:

- build `RpnEvaluator` inside `spreadsheet_engine/`
- keep the host boundary explicit and narrow
- measure progress by shrinking the relocated legacy dispatch surface, not
  only by shrinking `interpre.hxx`

## Initiative Scope

The `RpnEvaluator` initiative is the engine-native execution core for the
remaining Calc evaluator surface.

It is composed of six milestones:

1. host-boundary audit
2. engine stack-value model and typed coercion
3. engine operator dispatch
4. engine control flow
5. engine reference resolution and matrix frame
6. long-tail leaf functions on top of the new engine contracts

## Immediate Policy Changes

For this initiative, the project should use the following working rules:

- wrapper deletion is not counted as engine migration unless the authoritative
  path is engine-owned and the relocated legacy surface also falls
- no new `pushLegacy*` lambdas should be added for families the engine does
  not already own at the root
- no new lower-seam `tryPushEngine*` admission should land in
  `ScInterpreter::Interpret()` unless it is explicitly deletion-backed,
  required as an RPN subsystem primitive, or closes a measured parity gap
- if a family is not yet engine-authoritative, leaving it as
  `ScInterpreter::ScXxx()` is preferable to growing `Interpret()`
- every surviving `pushLegacy*` path must remain quarantine-warned
- a static rise in `interp4_dispatch_engine_attempt_count` is not by itself
  migration progress unless the live or full-legacy runtime counters move
- every surviving lower-seam `tryPushEngine*` admission site must carry an
  explicit pivot rationale marker in source so review and CI can distinguish
  deletion-backed work from new scaffolding
- the next success metric is meaningful reduction in
  `interp4_dispatch_legacy_lambda_count`, not just another drop in
  `legacy_interpreter_subroutine_count`
- the next retirement wave should preferentially target the densest remaining
  interpreter clusters that already lean on engine/shared helpers; the pure
  text/info and parsing/inspection subset tracked in
  [COMPUTATIONAL_SUBSTRATE_TEXT_INFO_RETIREMENT_PLAN.md](COMPUTATIONAL_SUBSTRATE_TEXT_INFO_RETIREMENT_PLAN.md)
  is now complete on the current tree, so the remaining follow-on target is
  the host-sensitive text tail rather than another broad pure-text sweep

### Retirement Template

A batch member can retire its legacy `ScInterpreter::Sc*` body or
`pushLegacy*` lambda fallback only when all of the following hold:

- `interp4_dispatch_engine_attempted_total_core_forced_full_legacy` is
  non-zero for the opcode across the standing corpus, i.e. the engine-first
  path has been exercised under real workload (not only unit tests)
- acceptance rate is `100%`, or every decline path has a documented legacy
  counterpart that produces the same observable result as the deleted
  `Sc*`/`pushLegacy*` implementation would have
- a deliberate-decline test exists for at least one operand shape the engine
  intentionally does not own, so the decline instrumentation is known to fire
- the retirement commit deletes the method declaration from
  [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx),
  the body from `interpr*.cxx`, and any corresponding `pushLegacy*` lambda
  from [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- the fallback path is replaced with `OSL_FAIL(...)` + an engine-consistent
  error push (`FormulaError::UnknownState` or batch-appropriate), never with
  a silent `PushError(FormulaError::NONE)` or an unchecked fall-through

The reference implementation of this template is
`532b10392 computational: retire ocBad legacy fallback`. Every future
engine-authoritative retirement should match its shape.

## Milestone Plan

The per-batch execution sequence, dependencies, member lists, and retirement
gates are tracked in:

- [COMPUTATIONAL_SUBSTRATE_RPN_BATCH_EXECUTION_PLAN.md](COMPUTATIONAL_SUBSTRATE_RPN_BATCH_EXECUTION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md)

### 1. Host-Boundary Audit

Produce the minimal host contract required by a full engine-side RPN loop.

This audit is tracked in:

- [COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md](COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md)

Output:

- fixed categories of host service
- representative surviving Calc surfaces per category
- explicit non-goals that should stay host-owned

### 2. Engine Stack-Value Model

Introduce an engine-native tagged value model for:

- scalar numbers
- strings
- booleans
- errors
- references
- matrices

This replaces implicit `PushDouble()` / `PopType()`-style control with typed
operations and explicit coercion rules.

Checkpoint:

- the initial substrate now exists in
  `spreadsheetengine/runtime/RpnValue.hxx`
- the first contract is intentionally narrow:
  - scalar kinds are fully modeled
  - reference and matrix operands are explicit value kinds
  - typed coercion returns `NeedsReferenceResolution` or
    `NeedsMatrixMaterialization` instead of silently pretending those cases are
    scalar-ready
- this substrate is now actively consumed by the operator, control-flow,
  reference, criteria, database, and matrix planners; the remaining work is to
  widen admitted shapes and retire Calc-host fallbacks rather than to
  introduce the value model itself

### 3. Engine Operator Dispatch

Admit the hot operator opcodes into the engine:

- arithmetic
- concatenation
- comparisons
- unary numeric operators

This is expected to be the first large, honest drop in
`interp4_dispatch_legacy_lambda_count`.

Checkpoint:

- the initial scalar operator contract now exists in
  `spreadsheetengine/runtime/RpnOperators.hxx`
- Calc now has a first engine-opcode pilot for scalar binary operators in
  `ScInterpreter::Interpret()`, but the broad replay corpus still reports
  zero live lower-seam runtime attempts in
  [../PROJECT_STATUS.md](../PROJECT_STATUS.md)
- the new core-forced full-legacy replay lane now proves that classic
  `ScInterpreter::Interpret()` is reachable again and reports non-zero
  forced-legacy engine-first traffic in
  [../PROJECT_STATUS.md](../PROJECT_STATUS.md), so engine-first dispatch is
  carrying real load inside the residual classic tail but still has a narrow
  decline pocket to close before broad retirement claims are justified
- that movement now comes from both the `ocBad` root-error-literal slice and
  the follow-up `ocRange` audit/fix; the classic opcode census still shows
  `Bad=506` and `Range=96` because it records opcode entry before the switch
  decides whether engine or legacy computes the result
- the `ocRange` audit proved that the standing corpus’s `96` apparent `Range`
  rows were really bracketed ODF error-literal syntax like `=[.OF:.ERR]:502`,
  not true reference-range work, so they now reroute through the bad-literal
  engine path
- the focused `OFFSET(...):OFFSET(...)` proof still shows valid dynamic range
  construction already succeeds through the dedicated range path
- the next real engine-admission value is therefore the broader real reference
  substrate and the control/matrix work beyond it, not more error-literal work
- it covers:
  - unary numeric `Plus` / `Minus`
  - binary scalar `Add`, `Subtract`, `Multiply`, `Divide`, `Power`
  - concatenation
  - scalar comparisons
- reference and matrix operands still defer explicitly through
  `NeedsReferenceResolution` / `NeedsMatrixMaterialization`
- the current bridge is intentionally transitional:
  `FormulaToken` is still the operand boundary type for the pilot, and that
  debt should shrink as the engine-native RPN loop takes shape
- the next operator slice should be measured by moving the runtime attempt /
  success / decline totals inside the full-legacy audit lane, not just by
  increasing the static attempt-case count

### 4. Engine Control Flow

Move jump and lazy-evaluation semantics into engine code:

- `IF`
- `CHOOSE`
- `LET`
- matrix-aware jump variants

Checkpoint:

- the initial control-flow substrate now exists in
  `spreadsheetengine/runtime/RpnControlFlow.hxx`
- it covers:
  - `planIfBranch(RpnValue, then-slot?, else-slot?)` → `BranchPlan`
  - `planChooseBranch(RpnValue, branch-count)` → `BranchPlan`
  - `planIfsBranch(RpnValue, condition-index, remaining-params)` → `BranchPlan`
  - `planSwitchBranch(RpnValue, case-labels, default-slot?)` → `BranchPlan`
  - `planIfErrorBranch(error, bNAOnly, alternate-slot)` → `BranchPlan`
  - `LetScope` with `bind()` / `lookup()` for scalar name binding
- planners are *pure*: they never mutate PC state, never read Calc state,
  never touch `FormulaToken`
- matrix and reference conditions defer explicitly through
  `NeedsMatrixMaterialization` / `NeedsReferenceResolution`
- limited Calc opcode routing now exists through this layer:
  scalar `ocIf`, `ocChoose`, `ocIfError` / `ocIfNA`, `ocIfs_MS`, and
  `ocSwitch_MS` each use the corresponding planner when their operands stay
  within the admitted scalar contract
- matrix-condition and nested-interpreter `ocLet` paths still defer to legacy

### 4b. Criteria and Database (Batch 3)

The criteria / database family shares a single predicate machinery across
nine criteria functions (COUNTIF / SUMIF / AVERAGEIF / COUNTIFS / SUMIFS
/ AVERAGEIFS / MINIFS_MS / MAXIFS_MS / COUNTEMPTY) and twelve database
functions (DSUM / DCOUNT / DCOUNT2 / DAVERAGE / DGET / DMAX / DMIN /
DPRODUCT / DSTDEV(P) / DVAR(P)).

Checkpoint:

- the criteria and database substrate now exists in
  `spreadsheetengine/runtime/RpnCriteria.hxx` and
  `spreadsheetengine/runtime/RpnDatabase.hxx`
- it covers:
  - `buildCriteriaPredicate(RpnValue, parsers)` returning a
    `core::query::CriteriaPredicate` with explicit reference / matrix
    deferral
  - `SingleCriterionAggregateRequest` + `planSingleCriterionAggregate`
    for the IF family (one criteria range, one predicate, optional
    aggregation range)
  - `MultiCriterionAggregateRequest` + `planMultiCriterionAggregate`
    for the IFS family (N parallel (range, predicate) pairs)
  - `countEmptyCells` for COUNTBLANK-style scalar iteration
  - `DatabaseQueryDescriptor` with a 1-based-column-index or
    header-name field selector and the three canonical references
    (data range with header, criteria range with header, optional
    field)
  - `applyFieldSelector` to bridge RpnValue arguments into the
    descriptor
  - `bridgeAggregation` mapping `DatabaseAggregation` to
    `core::query::CriteriaAggregateKind` for the simple reductions
    plus flags for variance / stddev / product / get which need their
    own variant-specific iteration
- reference and matrix operands defer explicitly through
  `NeedsReferenceResolution` / `NeedsMatrixMaterialization`
- Calc now routes COUNTIF / SUMIF / AVERAGEIF, the IFS aggregate family,
  COUNTEMPTY, and the first DB aggregate/variance/get members through this
  layer when their arguments stay within the admitted single-sheet scalarized
  contract

### 5. Engine Reference & Matrix Frame

Complete the hard substrate for:

- reference-shaped operands
- array broadcast
- matrix frame state
- spill/matrix materialization semantics needed by promoted opcodes

This is likely the longest phase and should be treated as such.

### 6. Long-Tail Functions

After the engine stack, operators, control flow, and reference/matrix
contracts exist, the remaining leaf functions can port against those engine
signatures rather than against Calc's stack machine.

## Success Criteria

The initiative is making real progress when:

- `interp4_dispatch_legacy_lambda_count` falls materially
- `interp4_dispatch_engine_attempted_total` or
  `interp4_dispatch_engine_attempted_total_core_forced_full_legacy` move for real
  workload lanes rather than staying flat while static attempt sites grow
- `legacy_interpreter_subroutine_count` also falls, but no longer leads the
  story by itself
- `interpret_tail_live_unique_fallback_formula_cells` stays `0`
- `interpret_tail_live_unique_unsupported_function_formula_cells` stays `0`
- `interpret_tail_live_unique_unseen_formula_cells` does not regress

The initiative is not making real progress if wrapper count falls while the
relocated legacy dispatch surface stays flat.
