# Engine RPN Evaluator Initiative

Status: active follow-on initiative after relocation backlog closure

The completed strategic pivot is tracked in
[COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md),
and the relocation closeout record lives in
[STACK_MACHINE_RELOCATION_BACKLOG.md](STACK_MACHINE_RELOCATION_BACKLOG.md).

## Purpose

The next deliberate Calc evaluator-expansion work is no longer well-described
as "the next hundred functions."

After the current honest baseline in
[../PROJECT_STATUS.md](../PROJECT_STATUS.md) — especially strong replay
parity, a closed relocation backlog, and a deliberately retained Calc shell —
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

The follow-on work should therefore be framed as one subsystem initiative:

- build `RpnEvaluator` inside `spreadsheet_engine/`
- keep the host boundary explicit and narrow
- measure progress by expanding evaluator-owned authority while keeping the
  retained Calc surface explicit, not only by shrinking `interpre.hxx`

## Initiative Scope

The `RpnEvaluator` initiative is the engine-native execution core for the
surviving host-owned / retained Calc evaluator surface we may choose to widen
deliberately over time.

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
- do not reintroduce `pushLegacy*`-style wrapper debt or anonymous compat
  shells for families the evaluator does not already own
- no new lower-seam `ScInterpreter::Interpret()` admission should land unless
  it is explicitly deletion-backed, documented in the architecture docs, and
  clearly temporary
- if a family is not yet engine-authoritative, leaving it as
  `ScInterpreter::ScXxx()` is preferable to growing `Interpret()`
- progress is measured by evaluator-owned execution plus preserved parity, not
  by reviving lower-seam attempt counters after the relocation backlog has
  already reached zero debt
- every retained Calc shell must stay classified in
  [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md) and out of the closed
  relocation backlog
- every widened family must land with explicit host/runtime contract coverage
  before the Calc-side overlap is removed

### Retirement Template

A future evaluator-authoritative retirement is only complete when all of the
following hold:

- the authoritative path runs through a documented engine-owned seam rather
  than through ad hoc Calc-local wrapper motion
- engine unit tests, Calc parity tests, and corpus/replay guardrails all stay
  green on the retirement commit
- the retirement commit deletes the redundant Calc implementation or
  reclassifies the behavior as intentionally host-owned in the same change
- [PROJECT_STATUS.md](../PROJECT_STATUS.md),
  [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md), and any affected
  initiative/backlog doc are updated in the same commit
- any temporary lower-seam bridge is either removed in the same wave or
  documented as an explicit transitional exception with a clear removal gate

## Milestone Plan

The per-batch execution sequence, dependencies, member lists, and retirement
gates are tracked in:

- [COMPUTATIONAL_SUBSTRATE_RPN_BATCH_EXECUTION_PLAN.md](COMPUTATIONAL_SUBSTRATE_RPN_BATCH_EXECUTION_PLAN.md)
- [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md)

### 1. Host-Boundary Audit

Current baseline:

- [COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md](COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md)
  and [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md) already define the
  current host-service categories and retained host-owned non-goals
- the closed relocation backlog has already consumed the missing-contract wave

Exit condition for future work:

- any newly widened family lands only after its required host surface is
  explicitly documented, tested, and classified in those two documents

### 2. Engine Stack-Value Model

Current baseline:

- `spreadsheetengine/runtime/RpnValue.hxx` already models scalar, reference,
  matrix, and error-bearing operand shapes
- typed coercion can already defer explicitly through
  `NeedsReferenceResolution` and `NeedsMatrixMaterialization`

Exit condition:

- newly widened evaluator-owned families run on engine-native value/state
  objects instead of `PushDouble()` / `PopType()` plumbing or raw
  `FormulaToken` boundary types

### 3. Engine Operator Dispatch

Current baseline:

- `spreadsheetengine/runtime/RpnOperators.hxx` already carries the scalar
  arithmetic / concat / comparison substrate
- the relocation backlog has already retired the Calc-owned operator wrapper
  wave, so this milestone is now about evaluator-native execution rather than
  wrapper cleanup

Exit condition:

- the engine-native evaluator owns arithmetic, concat, comparison, and unary
  numeric semantics for its admitted operand shapes
- admitted operator paths no longer depend on lower-seam `Interpret()`
  pilots or long-lived `FormulaToken` operand boundaries

### 4. Engine Control Flow

Current baseline:

- `spreadsheetengine/runtime/RpnControlFlow.hxx` already provides pure
  branch-planning primitives and `LET` scope helpers
- the remaining work is evaluator-state ownership, not discovering another
  control-flow contract surface

Exit condition:

- `IF`, `CHOOSE`, `LET`, and adjacent lazy-branch families execute through
  evaluator-owned state and branch planning rather than Calc token-iterator
  state

### 5. Engine Reference, Query, and Matrix Frame

Current baseline:

- `RangeResolver`, `RangeIterator`, matrix materialization helpers, search
  policy, random policy, and spill allocation contracts are already exposed
- the relocation backlog has already retired the wrapper terminals that used
  those contracts

Exit condition:

- evaluator-native execution can carry reference-shaped operands, criteria and
  database walks, matrix frame state, and spill/matrix planning without
  rebuilding `ScInterpreter` stack semantics inside Calc

### 6. Long-Tail Families

Current baseline:

- the remaining Calc shell is now explicit host-owned / retained behavior, not
  anonymous relocation debt

Exit condition:

- any additional family that moves into engine ownership ports against the
  evaluator contracts above, with same-commit parity coverage and doc updates,
  instead of opening a fresh wrapper-debt queue

## Success Criteria

The initiative is making real progress when:

- retained host-owned overlap shrinks only where evaluator-owned authority
  actually grows
- `legacy_interpreter_subroutine_count` stays at `0`
- `interp4_dispatch_legacy_lambda_count` stays at `0`
- `interpret_tail_live_unique_fallback_formula_cells` stays `0`
- `interpret_tail_live_unique_unsupported_function_formula_cells` stays `0`
- `interpret_tail_live_unique_unseen_formula_cells` does not regress
- widened families arrive with explicit host/runtime contracts plus engine
  unit tests, Calc parity tests, and corpus validation

The initiative is not making real progress if wrapper count falls while the
authoritative evaluator surface does not actually expand, or if new work grows
`ScInterpreter::Interpret()` instead of shrinking the need for it.
