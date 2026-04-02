# Architecture Docs

This directory is reserved for active, living architecture references.

Completed milestone plans, execution checklists, and historical closeout
documents live under [../archive/](../archive/).

Current architecture docs:

- [ENGINE_ENTRYPOINT_ADOPTION_PLAN.md](ENGINE_ENTRYPOINT_ADOPTION_PLAN.md) -
  active implementation plan for moving more bounded production Calc
  evaluation paths onto direct engine entry points through thin host adapters
- [TOKEN_BOUNDARY_REDUCTION_PLAN.md](TOKEN_BOUNDARY_REDUCTION_PLAN.md) -
  completed implementation and closeout record for reducing the remaining
  Calc-local token/container shell around shared spreadsheet semantics without
  moving token ownership out of Calc
- [HOST_BOUNDARY_CONSOLIDATION_PLAN.md](HOST_BOUNDARY_CONSOLIDATION_PLAN.md) -
  completed implementation and closeout record for shrinking legacy
  token/container coupling and making the remaining Calc-owned
  execution-adjacent surface more explicitly host-only
- [DEEPEN_ENGINE_FIRST_EXECUTION_PLAN.md](DEEPEN_ENGINE_FIRST_EXECUTION_PLAN.md) -
  completed implementation and closeout record for widening engine-owned
  execution as the default path in more production Calc call sites
- [ENGINE_FIRST_CALC_ADOPTION.md](ENGINE_FIRST_CALC_ADOPTION.md) -
  completed adoption record and standing guardrails for widening engine-first
  execution inside Calc while keeping the post-extraction host boundary clean
- [EXECUTION_BACKEND_EXTRACTION.md](EXECUTION_BACKEND_EXTRACTION.md) -
  completed boundary record for evaluator-shell execution extraction on top of
  the recalc-orchestration handoff
- [EXECUTION_SHELL_CLOSEOUT_PLAN.md](EXECUTION_SHELL_CLOSEOUT_PLAN.md) -
  completed closeout record for the final execution-shell cleanup,
  validation lanes, and exit criteria
- [RECALC_ORCHESTRATION_EXTRACTION.md](RECALC_ORCHESTRATION_EXTRACTION.md) -
  completed milestone for engine-owned recalc authority and queue/scheduling
  extraction
