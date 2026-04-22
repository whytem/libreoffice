# Spreadsheet Engine: Project Status

This file is the current-state entry point for `spreadsheet_engine/`.
It is intentionally a snapshot, not a running progress journal.

Historical slice writeups, phase plans, and retired ledgers now live under
[archive/](archive/).

## Start Here

- [architecture/STACK_MACHINE_RELOCATION_BACKLOG.md](architecture/STACK_MACHINE_RELOCATION_BACKLOG.md):
  active implementation queue
- [architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](architecture/COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md):
  completion record for the authority-transfer pivot
- [architecture/HOST_FACADE_CONTRACTS.md](architecture/HOST_FACADE_CONTRACTS.md):
  current host-contract inventory for remaining Calc-owned execution surface
- [architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md](architecture/COMPUTATIONAL_SUBSTRATE_MASTER.md):
  broader program context and narrative reference
- [architecture/CALC_TEST_KNOWN_REGRESSIONS.md](architecture/CALC_TEST_KNOWN_REGRESSIONS.md):
  active parity exceptions and guardrails

## Current State

- shared-engine extraction is materially achieved and remains in active use
- the authority-transfer pivot is closed on the current tree
- the stack-machine relocation backlog is closed on the current tree; new
  evaluator widening should start from explicit host/runtime contracts rather
  than reopen wrapper debt implicitly
- the intended long-term production path is
  `ScFormulaCell::InterpretTail() -> tryEvaluateFormula() -> FormulaEvaluator -> RpnEvaluator`
- `ScInterpreter::Interpret()` now remains only as the explicit host-owned /
  retained fallback shell around external terminals, repeated-operation
  utilities, and debug policy
- Phase 1 ownership classification for the relocation backlog is complete:
  `ScTableOp`, `ScTTT`, and `ScDebugVar` are now treated as explicit
  host/debug utilities rather than active relocation debt
- Phase 2 operator/control closure is complete: Calc no longer owns
  `ExecuteComparisonKernel`, `ExecuteLogicalFoldKernel`,
  `ExecuteUnaryMatrixOrScalarKernel`, `ExecuteBinaryMathKernel`,
  `ExecuteConcatKernel`, or `ExecuteLetKernel`; that execution now routes
  through compat dispatch on the engine side, and engine-backed classic-entry
  attempts are already at zero on the current tree
- Phase 3 reference/lookup/addressing closure is complete: Calc no longer
  declares or dispatches the `Execute*` / `ScMatchOp` wrapper surface for
  lookup, xlookup, indirect, address, index, intersection, union, range
  references, multi-area union, or missing-token handling; `ExecuteExternalTerminal`
  is now treated as an explicitly host-owned external-computation terminal
- Phase 4 query/criteria/transform closure is complete: Calc no longer
  declares or dispatches `ExecuteSubTotalTerminal`,
  `ExecuteDBAreaTerminal`, `ExecuteSortByTerminal`, or
  `ExecuteColRowNameAutoTerminal`; query iteration now stays behind explicit
  host/runtime contracts rather than interpreter-local wrappers
- Phase 5 inspection/metadata closure is complete: Calc no longer declares or
  dispatches `ExecuteTypeTerminal`, `ExecuteCellTerminal`,
  `ExecuteCellExternalTerminal`, `ExecuteCurrentTerminal`,
  `ExecuteStyleTerminal`, `ExecuteInfoTerminal`, or `ExecuteNTerminal`;
  inspection and metadata reads now route through explicit inspection/runtime
  adapters or host-owned side-effect dispatch
- Phase 6 matrix projection and adapter-tail closure is complete: Calc no
  longer declares or dispatches `ExecuteMatValueTerminal`,
  `ExecuteMatRefTerminal`, `ExecuteFrequencyTerminal`,
  `ExecuteForecastEtsTerminal`, `ExecuteFourierTerminal`, or
  `ExecuteSumXMY2Terminal`; the residual matrix work now lives in engine
  runtime planners plus explicit host-backed matrix consumers rather than in
  dedicated Calc wrapper terminals
- Phase 7 random/runtime closure is complete: Calc no longer declares or
  dispatches `ExecuteRandomTerminal`, `ExecuteRandbetweenTerminal`, or
  `ExecuteRandArrayTerminal`; RNG policy stays behind
  `RuntimeEnvironment::sampleUniformReal()`, and the active relocation
  backlog now has no remaining `Execute*` debt on the current tree

## Canonical Dashboard

Canonical dashboard metrics live in this file only. Static relocation-debt
metrics on the current tree are:

- `legacy_interpreter_subroutine_count=0`
- `interp4_dispatch_legacy_lambda_count=0`
- `interp4_dispatch_engine_backed_plan_engine_attempt_count=0`

Refresh the live runtime snapshot from
`SPREADSHEET_ENGINE_INTERPRET_TAIL_CORPUS_STATS=1 make CppunitTest_sc_interpret_tail_corpus CPPUNIT_TEST_NAME=testAuthorityStats`
when the runtime envelope changes:

- `interp4_dispatch_engine_attempted_total=<refresh via testAuthorityStats>`
- `interpret_tail_live_authoritative_match_total=<refresh via testAuthorityStats>`

## Current Priorities

1. Preserve replay parity and the known-regression baseline now that the
   relocation backlog is closed.
2. Keep host-facing contracts explicit before widening new evaluator surface.
3. Keep `ScInterpreter::Interpret()` limited to documented host-owned /
   retained behavior.
4. Keep active docs small and current; move closed slice reports and obsolete
   ledgers to `docs/archive/`.

## Working Rules

- `InterpretTail` is the only intended long-term production engine entry seam.
- Lower-seam engine attempts count as audit coverage, not ambient authority
  transfer.
- Replay parity and targeted Calc parity tests remain the deletion gate for
  relocation work.
- New historical narrative belongs in archived docs or commit history, not in
  this file.

## Archive

- [archive/authority_transfer/](archive/authority_transfer/):
  closed pivot-era plans, ledgers, and slice reports
- [archive/interpret_tail/](archive/interpret_tail/):
  older pass-by-pass `InterpretTail` migration material
- [archive/pre_pivot_substrate/](archive/pre_pivot_substrate/):
  pre-pivot substrate widening history
- [archive/extraction-history/](archive/extraction-history/):
  extraction-era records
