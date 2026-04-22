# Stack Machine Relocation Backlog

Status: active relocation backlog

## Purpose

Track only the remaining Calc-resident stack-machine surface that is still
live on the current tree.

Completed relocation slices, already-landed host contracts, and closed
authority-transfer work are intentionally omitted from this file. Historical
closeout lives in:

- [../archive/authority_transfer/](../archive/authority_transfer/)
- [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md)

The intended end state remains one production execution story:

`ScFormulaCell::InterpretTail()` -> `tryEvaluateFormula()` ->
`FormulaEvaluator` -> `RpnEvaluator`

`ScInterpreter::Interpret()` should remain only for explicitly host-owned
terminals or intentionally retained Calc policy.

## Current Baseline

The remaining backlog is no longer about missing host contracts. The major
contract gaps that previously blocked relocation are already landed:

- `RangeResolver`
- `RangeIterator`
- formula-inspection providers
- cell/type-inspection adapters
- `RuntimeEnvironment::getSearchType()`
- `RuntimeEnvironment::sampleUniformReal()`
- `SpillRangeAllocator`

The work that remains is now one of two kinds:

- semantic migration of still-live Calc terminals
- retirement of Calc wrappers that are already engine-backed in substance

Phase 1 of the relocation backlog is now complete:

- `ScTableOp` is treated as explicit host-owned repeated-operation state
- `ScTTT` and `ScDebugVar` are treated as retained debug utilities
- canonical relocation-debt metrics now track the live `Execute*` /
  `ScMatchOp` closure set rather than a broad historical `Sc*` regex

Phase 2 of the relocation backlog is now complete:

- Calc no longer declares or dispatches `ExecuteComparisonKernel`,
  `ExecuteLogicalFoldKernel`, `ExecuteUnaryMatrixOrScalarKernel`,
  `ExecuteBinaryMathKernel`, `ExecuteConcatKernel`, or `ExecuteLetKernel`
- engine-backed classic-entry dispatch attempts are already zero on the
  current tree
- canonical relocation-debt metrics now exclude the retired operator/control
  wrapper surface

Phase 3 of the relocation backlog is now complete:

- Calc no longer declares or dispatches the reference/lookup/addressing
  wrapper surface for lookup, xlookup, match, indirect, address, index,
  range-reference, union/intersection, multi-area union, or missing-token
  handling
- `ExecuteExternalTerminal` is now treated as explicitly host-owned
  external-computation behavior rather than relocation debt
- canonical relocation-debt metrics now exclude the closed reference wave

Phase 4 of the relocation backlog is now complete:

- Calc no longer declares or dispatches `ExecuteSubTotalTerminal`,
  `ExecuteDBAreaTerminal`, `ExecuteSortByTerminal`, or
  `ExecuteColRowNameAutoTerminal`
- query iteration, named DB-area materialization, and spill-shaped SORTBY
  dispatch now flow through explicit engine/host contracts rather than
  Calc-local wrapper entrypoints
- canonical relocation-debt metrics now exclude the closed query wave

Phase 5 of the relocation backlog is now complete:

- Calc no longer declares or dispatches `ExecuteTypeTerminal`,
  `ExecuteCellTerminal`, `ExecuteCellExternalTerminal`,
  `ExecuteCurrentTerminal`, `ExecuteStyleTerminal`,
  `ExecuteInfoTerminal`, or `ExecuteNTerminal`
- inspection and metadata reads now flow through explicit inspection/runtime
  adapters or intentional host-owned side-effect dispatch rather than
  Calc-local wrapper entrypoints
- canonical relocation-debt metrics now exclude the closed inspection wave

## Working Rules

1. Do not reopen closed contract-gap work unless the code proves a new gap.
2. Prefer deleting Calc execution ownership over moving logic sideways inside
   Calc.
3. When a Calc path is intentionally retained, classify it as host-owned in
   the same commit that removes it from this backlog.
4. Adapter-only wrappers are not considered complete until normal execution no
   longer depends on them.
5. Keep this file, [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md), and
   [COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md](COMPUTATIONAL_SUBSTRATE_RPN_HOST_BOUNDARY_AUDIT.md)
   aligned in the same change whenever ownership changes.

## Outstanding Inventory

### Structural Shell And Classic-Entry Debt

- `ScInterpreter::Interpret()`
- classic stack/token machinery in `interpre.hxx` and `interpr4.cxx`,
  including `sp`, `maxsp`, `Push*`, `Pop*`, token iteration helpers, and
  dispatch bookkeeping

### Matrix / Reference Projection

- `ExecuteMatValueTerminal`
- `ExecuteMatRefTerminal`

These are the remaining Calc-side matrix-reference projection terminals that
still block full shell deletion even though the broader matrix/statistical
waves already landed upstream.

### Adapter-Only Engine-Backed Residue

These surfaces are already engine-owned in substance, but Calc still carries
entrypoint or dispatch residue for them:

- `ExecuteFrequencyTerminal`
- `ExecuteForecastEtsTerminal`
- `ExecuteFourierTerminal`
- `ExecuteSumXMY2Terminal`
- `ExecuteRandomTerminal`
- `ExecuteRandbetweenTerminal`
- `ExecuteRandArrayTerminal`

## Phased Implementation Plan

### Phase 6: Matrix Projection And Adapter-Tail Retirement

Goal: remove the remaining matrix/reference projection logic and the
already-engine-backed matrix/statistical tail entrypoints from Calc.

Scope:

- retire Calc execution ownership for `ExecuteMatValueTerminal` and
  `ExecuteMatRefTerminal`
- retire Calc entrypoints for:
  `ExecuteFrequencyTerminal`, `ExecuteForecastEtsTerminal`,
  `ExecuteFourierTerminal`, `ExecuteSumXMY2Terminal`
- unify remaining matrix/reference projection through runtime helpers rather
  than interpreter-local bridges
- classify any unavoidable host projection behavior explicitly

Exit criteria:

- normal execution of the listed matrix/statistical families no longer enters
  Calc kernels
- the only surviving Calc matrix/reference behavior is explicitly documented
  host projection glue, or the terminals are deleted entirely
- matrix-state edge cases are owned by engine runtime code rather than ad hoc
  interpreter helpers
- parity coverage exists for array-context, forecast-tail, and
  local-versus-external matrix-reference shapes

### Phase 7: Random Runtime And Shell Contraction

Goal: finish the already-landed random migration and then collapse the
remaining `Interpret()` shell around the truly host-owned residue.

Scope:

- retire Calc execution ownership for:
  `ExecuteRandomTerminal`, `ExecuteRandbetweenTerminal`,
  `ExecuteRandArrayTerminal`
- keep RNG policy exclusively behind
  `RuntimeEnvironment::sampleUniformReal()`
- delete dead dispatch arms, adapter-only wrappers, and stack helpers made
  unnecessary by the earlier phases
- reduce `ScInterpreter::Interpret()` to explicit host-owned terminals and the
  utilities that Phase 1 classified as intentionally retained

Exit criteria:

- RAND, RANDBETWEEN.NV, and RANDARRAY no longer need Calc-side execution
  kernels during normal relocated execution
- no Calc-local RNG policy remains
- `ScInterpreter::Interpret()` no longer owns execution for any backlog item
  above
- remaining Calc-resident evaluator code is limited to documented host-owned
  terminals or explicitly retained debug/policy utilities

## Success Metrics

The backlog is complete when all of the following are true:

- every symbol listed in this file has been migrated, deleted, or explicitly
  classified as host-owned
- `ScInterpreter::Interpret()` no longer owns execution for any active
  relocation item
- `legacy_interpreter_subroutine_count` reflects only explicit host-owned
  terminals or intentionally retained utilities
- no completed item remains in this file
- the host-contract inventory and the boundary audit agree with the final
  ownership state

## Explicitly Out Of Scope

These remain outside the backlog unless a new product decision changes their
status:

- `ScTableOp`
- `ScTTT`
- `ScDebugVar`
- `ExecuteExternalTerminal`
- `ScMacro`
- `ScDde`
- `ScWebservice`
- `ScFilterXML`
- `ScGetPivotData`
- `ScHyperLink`

If any of these move into scope, add a new phase instead of treating them as
incidental cleanup.
