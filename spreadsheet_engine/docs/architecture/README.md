# Architecture Docs

This directory holds the active architecture references for
`spreadsheet_engine/`.

## Start Here

- [../PROJECT_STATUS.md](../PROJECT_STATUS.md):
  current-state snapshot and top-level handoff
- [STACK_MACHINE_RELOCATION_BACKLOG.md](STACK_MACHINE_RELOCATION_BACKLOG.md):
  relocation closeout record
- [COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md](COMPUTATIONAL_SUBSTRATE_AUTHORITY_TRANSFER_PIVOT_PLAN.md):
  completion record for the authority-transfer pivot
- [COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md](COMPUTATIONAL_SUBSTRATE_RPN_EVALUATOR_INITIATIVE.md):
  forward-looking evaluator-expansion initiative
- [HOST_FACADE_CONTRACTS.md](HOST_FACADE_CONTRACTS.md):
  current host-contract inventory for surviving Calc-owned / retained surface
- [COMPUTATIONAL_SUBSTRATE_MASTER.md](COMPUTATIONAL_SUBSTRATE_MASTER.md):
  broader program context and narrative reference
- [CALC_TEST_KNOWN_REGRESSIONS.md](CALC_TEST_KNOWN_REGRESSIONS.md):
  active parity exceptions and guardrails

## Current Model

The active model is now simple:

1. keep the replay and regression guardrails exact
2. treat the relocation backlog as closed unless new code proves fresh debt
3. widen evaluator ownership only through explicit host/runtime contracts
4. keep retained Calc shells intentional and documented
5. archive closed slice reports instead of carrying them as active docs

## Archive

Closed or superseded material now lives in:

- [../archive/authority_transfer/](../archive/authority_transfer/)
- [../archive/interpret_tail/](../archive/interpret_tail/)
- [../archive/pre_pivot_substrate/](../archive/pre_pivot_substrate/)
- [../archive/extraction-history/](../archive/extraction-history/)

Use the archive only for implementation archaeology or original decision
context.
