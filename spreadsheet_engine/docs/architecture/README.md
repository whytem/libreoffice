# Architecture Docs

This directory holds the active architecture references for
`spreadsheet_engine/`.

## Start Here

- [../PROJECT_STATUS.md](../PROJECT_STATUS.md):
  current-state snapshot and top-level handoff
- [STACK_MACHINE_RELOCATION_BACKLOG.md](STACK_MACHINE_RELOCATION_BACKLOG.md):
  active implementation queue
- [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md):
  completion record for the authority-transfer pivot
- [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md):
  current host-contract inventory for remaining Calc-owned surface
- [COMPUTATIONAL_SUBSTRATE_MASTER.md](COMPUTATIONAL_SUBSTRATE_MASTER.md):
  broader program context and narrative reference
- [CALC_TEST_KNOWN_REGRESSIONS.md](CALC_TEST_KNOWN_REGRESSIONS.md):
  active parity exceptions and guardrails

## Current Model

The active roadmap is now simple:

1. keep the replay and regression guardrails exact
2. continue relocation through the upper seam
3. delete Calc-owned overlap only after the engine can own the same work
4. archive closed slice reports instead of carrying them as active docs

## Archive

Closed or superseded material now lives in:

- [../archive/authority_transfer/](../archive/authority_transfer/)
- [../archive/interpret_tail/](../archive/interpret_tail/)
- [../archive/pre_pivot_substrate/](../archive/pre_pivot_substrate/)
- [../archive/extraction-history/](../archive/extraction-history/)

Use the archive only for implementation archaeology or original decision
context.
