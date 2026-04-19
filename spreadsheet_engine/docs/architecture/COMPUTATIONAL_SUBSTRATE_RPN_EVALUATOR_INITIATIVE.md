# Engine RPN Evaluator Initiative

Status: active next-phase initiative

## Purpose

The remaining Calc migration work is no longer well-described as "the next
hundred functions."

After the current honest baseline of:

- `legacy_interpreter_subroutine_count=100`
- `interp4_dispatch_legacy_lambda_count=62`
- `interp4_dispatch_engine_attempt_count=14`
- `interp4_dispatch_engine_attempted_total=0`
- `interp4_dispatch_engine_succeeded_total=0`
- `interp4_dispatch_engine_declined_total=0`
- `interp4_dispatch_engine_attempted_total_core_forced_full_legacy=602`
- `interp4_dispatch_engine_succeeded_total_core_forced_full_legacy=602`
- `interp4_dispatch_engine_declined_total_core_forced_full_legacy=0`
- `sc_formula_executor_classic_interpret_total_live=0`
- `sc_formula_executor_classic_interpret_total_core_forced_full_legacy=602`
- `interpret_tail_live_unique_unseen_formula_cells=49`
- `interpret_tail_live_unique_fallback_formula_cells=0`
- `interpret_tail_live_unique_unsupported_function_formula_cells=0`

the hard part is the RPN evaluator subsystem itself:

- operator semantics over polymorphic stack values
- control-flow opcodes and jump execution
- reference-shaped operands
- matrix broadcast and array-formula state
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
- if a family is not yet engine-authoritative, leaving it as
  `ScInterpreter::ScXxx()` is preferable to growing `Interpret()`
- every surviving `pushLegacy*` path must remain quarantine-warned
- a static rise in `interp4_dispatch_engine_attempt_count` is not by itself
  migration progress unless the live or full-legacy runtime counters move
- the next success metric is meaningful reduction in
  `interp4_dispatch_legacy_lambda_count`, not just another drop in
  `legacy_interpreter_subroutine_count`

## Milestone Plan

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
- no opcode routes through this layer yet; it is a prerequisite slice for the
  later operator migration, not a stealth dispatch change

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
  `interp4_dispatch_engine_attempted_total=0`
- the new core-forced full-legacy replay lane now proves that classic
  `ScInterpreter::Interpret()` is reachable again (`602` executions on the
  standing corpus), and it now reports
  `interp4_dispatch_engine_attempted_total_core_forced_full_legacy=602` with
  `602` successes and `0` declines, so engine-first dispatch is carrying real
  load inside the residual classic tail
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
