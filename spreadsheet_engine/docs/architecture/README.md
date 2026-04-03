# Architecture Docs

This directory is reserved for active, living architecture references.

Completed milestone plans, execution checklists, and historical closeout
documents live under [../archive/](../archive/).

Current architecture docs:

- [COMPUTATIONAL_SUBSTRATE_PHASE0_INVENTORY.md](COMPUTATIONAL_SUBSTRATE_PHASE0_INVENTORY.md) -
  completed ownership map for Phase 0, classifying the live Calc
  computational substrate into migration candidates, retained host surfaces,
  and mixed seams
- [COMPUTATIONAL_SUBSTRATE_PHASE0_OBSERVABLE_STATE_MODEL.md](COMPUTATIONAL_SUBSTRATE_PHASE0_OBSERVABLE_STATE_MODEL.md) -
  completed comparison schema for Phase 0, defining the stable observable
  shapes and verdict rules for formula-tree, broadcaster, and recalc-state
  captures
- [COMPUTATIONAL_SUBSTRATE_PHASE0_SCENARIO_MATRIX.md](COMPUTATIONAL_SUBSTRATE_PHASE0_SCENARIO_MATRIX.md) -
  completed representative mutation matrix for Phase 0, mapping edit classes
  to required captures, expected outputs, and owning validation lanes
- [COMPUTATIONAL_SUBSTRATE_PHASE0_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_PHASE0_DECISION_RECORD.md) -
  completed closeout decision for Phase 0, recording the proceed-on-narrowed-
  subset outcome for the computational substrate program
- [COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md](COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md) -
  completed implementation and closeout record for Phase 0 observability,
  inventory, and validation work in the computational substrate program
- [COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md](COMPUTATIONAL_SUBSTRATE_EXTRACTION_PLAN.md) -
  active staged architecture plan for the second-stage boundary shift where
  `spreadsheet_engine/` would own the live computational substrate, including
  formula tree, broadcaster/listener graph, computation-facing table and
  column storage, and an engine-owned execution-facing IR
- [COMPUTATIONAL_SUBSTRATE_PHASE1_PLAN.md](COMPUTATIONAL_SUBSTRATE_PHASE1_PLAN.md) -
  completed implementation and closeout record for Phase 1 shadow
  computational storage on the narrowed subset admitted by the completed
  Phase 0 gate
- [COMPUTATIONAL_SUBSTRATE_PHASE1_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_PHASE1_DECISION_RECORD.md) -
  completed proceed record for Phase 1, admitting the rebuilt shadow subset
  plus representative row-insert and column-delete widening as the entry
  surface for Phase 2
- [COMPUTATIONAL_SUBSTRATE_PHASE2_PLAN.md](COMPUTATIONAL_SUBSTRATE_PHASE2_PLAN.md) -
  completed implementation and closeout record for Phase 2 live
  dependency-graph shadowing on the admitted Phase 1 subset
- [COMPUTATIONAL_SUBSTRATE_PHASE2_GRAPH_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_PHASE2_GRAPH_SCHEMA.md) -
  completed Phase 2 schema note for graph nodes, listener anchors,
  broadcaster nodes, and edge identity
- [COMPUTATIONAL_SUBSTRATE_PHASE2_GRAPH_MAPPING_RULES.md](COMPUTATIONAL_SUBSTRATE_PHASE2_GRAPH_MAPPING_RULES.md) -
  completed Phase 2 mapping and equivalence reference for graph ids,
  normalized comparisons, and forbidden Calc identity shortcuts
- [COMPUTATIONAL_SUBSTRATE_PHASE2_SPECIAL_CASE_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_PHASE2_SPECIAL_CASE_EVIDENCE.md) -
  completed Phase 2 evidence note for delayed listener startup, delayed
  broadcaster deletion, and representative structural rebuild coverage
- [COMPUTATIONAL_SUBSTRATE_PHASE2_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_PHASE2_DECISION_RECORD.md) -
  completed proceed-on-narrowed-subset decision for Phase 2, recording the
  admitted graph shadow surface and the retained defer boundaries
- [COMPUTATIONAL_SUBSTRATE_PHASE3_PLAN.md](COMPUTATIONAL_SUBSTRATE_PHASE3_PLAN.md) -
  completed implementation and closeout record for defining the first
  engine-owned execution IR boundary on top of the narrowed Phase 2
  graph-shadow subset
- [COMPUTATIONAL_SUBSTRATE_PHASE3_IR_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_PHASE3_IR_CONTRACT.md) -
  completed contract note for the first execution-facing IR slice, freezing
  the admitted semantic surface, normalization rules, and defer list for the
  completed Phase 3 boundary
- [COMPUTATIONAL_SUBSTRATE_PHASE3_IR_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_PHASE3_IR_SCHEMA.md) -
  completed schema note for the first engine-owned execution IR types,
  payloads, and workbook/formula ownership rules on the admitted Phase 3
  subset
- [COMPUTATIONAL_SUBSTRATE_PHASE3_REFERENCE_UPDATE_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_PHASE3_REFERENCE_UPDATE_EVIDENCE.md) -
  completed evidence note for the admitted Phase 3 reference-shape and
  representative structural-update semantics at the IR boundary
- [COMPUTATIONAL_SUBSTRATE_PHASE3_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_PHASE3_DECISION_RECORD.md) -
  completed narrow-proceed decision for Phase 3, admitting the first
  engine-owned execution IR shadow boundary and carrying a narrowed subset
  into the Phase 4 authority pilot
- [COMPUTATIONAL_SUBSTRATE_PHASE4_PLAN.md](COMPUTATIONAL_SUBSTRATE_PHASE4_PLAN.md) -
  completed implementation and closeout record for the first
  engine-authoritative dependency and recalc pilot on the narrowed IR-backed
  subset admitted by the completed Phase 3 decision record
- [COMPUTATIONAL_SUBSTRATE_PHASE4_AUTHORITY_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_PHASE4_AUTHORITY_CONTRACT.md) -
  completed authority-boundary note for the first engine-authoritative pilot,
  freezing the admitted mutation surface, preconditions, normalization rules,
  and rollback triggers
- [COMPUTATIONAL_SUBSTRATE_PHASE4_PILOT_MUTATION_MATRIX.md](COMPUTATIONAL_SUBSTRATE_PHASE4_PILOT_MUTATION_MATRIX.md) -
  completed mutation classification for the first authority pilot, separating
  admitted, validation-only, and rejected mutation classes
- [COMPUTATIONAL_SUBSTRATE_PHASE4_AUTHORITY_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_PHASE4_AUTHORITY_SCHEMA.md) -
  completed schema note for the first authority pilot, defining the engine-owned
  transition, verification, and verdict records used by Phase 4
- [COMPUTATIONAL_SUBSTRATE_PHASE4_DIFFERENTIAL_SURFACE.md](COMPUTATIONAL_SUBSTRATE_PHASE4_DIFFERENTIAL_SURFACE.md) -
  completed differential-validation note for the authority pilot, freezing the
  explicit applied, normalized, rolled-back, and rejected verdict categories
- [COMPUTATIONAL_SUBSTRATE_PHASE4_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_PHASE4_DECISION_RECORD.md) -
  completed closeout decision for Phase 4, admitting the narrowed
  graph-and-queue-authoritative subset and carrying a still-narrower surface
  into Phase 5
- [COMPUTATIONAL_SUBSTRATE_PHASE5_PLAN.md](COMPUTATIONAL_SUBSTRATE_PHASE5_PLAN.md) -
  completed implementation and closeout record for the first scalar
  formula-lifecycle authority pilot on top of the narrowed Phase 4
  graph-and-queue-authoritative subset
- [COMPUTATIONAL_SUBSTRATE_PHASE5_DIFFERENTIAL_SURFACE.md](COMPUTATIONAL_SUBSTRATE_PHASE5_DIFFERENTIAL_SURFACE.md) -
  completed differential-validation note for the Phase 5 lifecycle pilot,
  freezing the explicit applied, normalized-equivalent, rejected,
  rolled-back, and repair-detected verdict categories
- [COMPUTATIONAL_SUBSTRATE_PHASE5_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_PHASE5_DECISION_RECORD.md) -
  completed narrow-proceed decision for Phase 5, admitting only the scalar
  lifecycle-authoritative subset into Phase 6 while keeping shared-group and
  structural widening deferred
- [COMPUTATIONAL_SUBSTRATE_PHASE6_PLAN.md](COMPUTATIONAL_SUBSTRATE_PHASE6_PLAN.md) -
  completed implementation and closeout record for the first structural-edit
  and reference-update widening slice on top of the narrowed scalar
  lifecycle-authoritative subset admitted by the completed Phase 5 decision
- [COMPUTATIONAL_SUBSTRATE_PHASE6_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_PHASE6_DECISION_RECORD.md) -
  completed narrow-proceed decision for Phase 6, admitting only the first
  structural-authoritative slice for single-sheet row insert and column
  delete on the ordinary-scalar-formula subset while keeping broader
  structural classes deferred into Phase 7
- [COMPUTATIONAL_SUBSTRATE_PHASE7_PLAN.md](COMPUTATIONAL_SUBSTRATE_PHASE7_PLAN.md) -
  completed implementation and closeout record for the host-boundary re-cut
  and rollout-decision phase, ending in a narrow-proceed recommendation
  rather than a broad boundary flip
- [COMPUTATIONAL_SUBSTRATE_PHASE7_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_PHASE7_DECISION_RECORD.md) -
  completed final decision record for the computational-substrate program,
  recommending narrow proceed on the admitted authority slice and deferring
  broader structural, storage, and token-container rollout
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_PLAN.md](COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_PLAN.md) -
  active implementation-ready plan for bounded rollout engineering after the
  Phase 7 narrow-proceed decision, including the bolder validation and
  promotion step for `DeleteRows` and `InsertColumns`
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_CONTRACT.md) -
  active contract note for the initial admitted rollout slice, freezing the
  exact enabled surface, authority gates, and immediate deactivation rules
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_IMPLEMENTATION.md) -
  active implementation note for the first opt-in rollout step, including the
  umbrella rollout gate and per-surface override behavior
- [COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md) -
  active widening contract note for the first bolder promotion candidates,
  freezing the proof threshold for `DeleteRows` and `InsertColumns`
- [COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_EVIDENCE.md) -
  active widening evidence note for the first bolder promotion candidates,
  recording the exact validation, rejection, and rollback proof for
  `DeleteRows` and `InsertColumns`
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_EVIDENCE.md) -
  active bounded rollout evidence note for the admitted rollout slice,
  recording correctness, gate behavior, and the first measured operational
  sample after opt-in rollout wiring
- [ADDIN_FINANCIAL_TAIL_CONVERGENCE_PLAN.md](ADDIN_FINANCIAL_TAIL_CONVERGENCE_PLAN.md) -
  completed implementation and closeout record for the final residual
  Analysis add-in financial tail, including explicit defer classification for
  `ODDFPRICE` and `ODDFYIELD`, removal of the legacy helper stub path, and
  focused regression coverage
- [EXTERNAL_REFERENCE_FACADE_TIGHTENING_PLAN.md](EXTERNAL_REFERENCE_FACADE_TIGHTENING_PLAN.md) -
  completed implementation and closeout record for tightening the remaining
  external-reference fetch/projection facade while keeping external cache and
  token ownership in Calc
- [COMPILER_EVALUATION_BOUNDARY_REASSESSMENT_PLAN.md](COMPILER_EVALUATION_BOUNDARY_REASSESSMENT_PLAN.md) -
  completed implementation and closeout record for the fresh compiler and
  evaluation boundary reassessment, including the selected compiler-entry
  tightening slice, direct financial add-in entry widening, and retained
  host-owned defer list
- [HOST_SERVICE_FACADE_NARROWING_PLAN.md](HOST_SERVICE_FACADE_NARROWING_PLAN.md) -
  completed implementation and closeout record for narrowing the remaining
  host-service packaging around intentionally Calc-owned production surfaces
  without moving service ownership out of Calc
- [ENGINE_ENTRY_WIDENING_PLAN.md](ENGINE_ENTRY_WIDENING_PLAN.md) -
  completed implementation and closeout record for the second-wave widening
  of direct engine-entry use inside Calc, including the selected add-in
  financial callers and bounded local `CELL(...)` inspection path
- [PRODUCTION_BOUNDARY_TIGHTENING_PLAN.md](PRODUCTION_BOUNDARY_TIGHTENING_PLAN.md) -
  completed implementation and closeout record for tightening the remaining
  production compiler/evaluation boundary around explicitly host-owned Calc
  and add-in surfaces
- [ENGINE_ENTRYPOINT_ADOPTION_PLAN.md](ENGINE_ENTRYPOINT_ADOPTION_PLAN.md) -
  completed implementation and closeout record for moving the first bounded
  production Calc evaluation paths onto direct engine entry points through
  thin host adapters
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
