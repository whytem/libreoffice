# Stack Machine Relocation Backlog

Status: relocation backlog complete on current tree

## Purpose

Track the status of the Calc-resident stack-machine relocation effort on the
current tree.

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

The relocation backlog is no longer about missing host contracts. The major
contract gaps that previously blocked relocation are already landed:

- `RangeResolver`
- `RangeIterator`
- formula-inspection providers
- cell/type-inspection adapters
- `RuntimeEnvironment::getSearchType()`
- `RuntimeEnvironment::sampleUniformReal()`
- `SpillRangeAllocator`

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

Phase 6 of the relocation backlog is now complete:

- Calc no longer declares or dispatches `ExecuteMatValueTerminal`,
  `ExecuteMatRefTerminal`, `ExecuteFrequencyTerminal`,
  `ExecuteForecastEtsTerminal`, `ExecuteFourierTerminal`, or
  `ExecuteSumXMY2Terminal`
- matrix projection and matrix/statistical tail behavior now route through
  compat dispatch plus runtime planners rather than Calc-local wrapper
  entrypoints
- canonical relocation-debt metrics now exclude the closed matrix/statistical
  wave

Phase 7 of the relocation backlog is now complete:

- Calc no longer declares or dispatches `ExecuteRandomTerminal`,
  `ExecuteRandbetweenTerminal`, or `ExecuteRandArrayTerminal`
- RNG policy remains exclusively behind
  `RuntimeEnvironment::sampleUniformReal()`
- canonical relocation-debt metrics now report zero active wrapper residue on
  the current tree
- no active relocation items remain in scope on the current tree

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

None. Active relocation debt is closed; the surviving Calc shell is limited
to the explicitly host-owned / retained items listed out of scope below.

## Completion State

The relocation backlog is complete on the current tree because all of the
following are now true:

- every symbol listed in this file has been migrated, deleted, or explicitly
  classified as host-owned
- `ScInterpreter::Interpret()` no longer owns execution for any active
  relocation item
- canonical relocation-debt dashboard metrics reflect only explicit
  host-owned terminals or intentionally retained utilities
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
