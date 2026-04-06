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
  completed closeout record for bounded rollout engineering after the Phase 7
  narrow-proceed decision, ending in a widened but still opt-in rollout
  surface
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_CONTRACT.md) -
  completed closeout contract note for the widened narrow rollout surface,
  freezing the exact enabled slice, authority gates, and deactivation rules
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_IMPLEMENTATION.md) -
  completed implementation note for the opt-in narrow rollout, including the
  umbrella rollout gate and per-surface override behavior
- [COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_CONTRACT.md) -
  completed widening contract note for the first bolder promotion candidates,
  freezing the proof threshold for `DeleteRows` and `InsertColumns`
- [COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_NARROW_WIDENING_EVIDENCE.md) -
  completed widening evidence note for the first bolder promotion candidates,
  recording the exact validation, rejection, and rollback proof for
  `DeleteRows` and `InsertColumns`
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_EVIDENCE.md) -
  completed bounded rollout evidence note for the admitted rollout slice,
  recording correctness, gate behavior, and the first measured operational
  sample after opt-in rollout wiring
- [COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_NARROW_ROLLOUT_DECISION_RECORD.md) -
  completed closeout decision for the narrow rollout plan, widening the
  structural rollout surface by one bounded step to include `DeleteRows` and
  `InsertColumns`
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_WIDENING_PLAN.md](COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_WIDENING_PLAN.md) -
  completed closeout record for the named-range-sensitive structural widening
  proof cycle, ending in a validation-only global single-area slice rather
  than live rollout admission
- [COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_NAMED_RANGE_STRUCTURAL_DECISION_RECORD.md) -
  completed closeout decision for the named-range-sensitive structural proof
  cycle, keeping the live rollout unchanged while retaining a bounded
  validation-only global named-range slice
- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_PLAN.md](COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_PLAN.md) -
  completed closeout record for the bounded promotion proof cycle on the
  validation-only global single-area named-range slice, ending without live
  rollout admission
- [COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_GLOBAL_NAMED_RANGE_ADMISSION_DECISION_RECORD.md) -
  completed closeout decision for the global named-range promotion cycle,
  keeping the live rollout unchanged while recording exact standalone
  prediction and a gated live-candidate path for the bounded global slice
- [COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_PLAN.md](COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_PLAN.md) -
  completed implementation and closeout record for the first
  storage-and-wiring proof cycle toward engine-owned cell storage and
  listener/broadcaster authority, ending in admitted mutable-sidecar and
  graph-target ownership on the bounded slice
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md](COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_PLAN.md) -
  completed implementation and closeout record for the next bounded
  migration step, admitting engine-resident cell storage on the admitted
  slice while keeping formula-cell lifetime and live container residency in
  Calc
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_RESIDENCY_CONTRACT.md) -
  frozen authority contract for the first admitted-slice cell-residency
  pilot, fixing the admitted mutation classes, engine-owned resident cell
  surface, retained Calc host surfaces, and exact verification standard
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_SCHEMA.md) -
  frozen schema note for admitted engine-resident cell records, address
  identity, formula payload ownership, and exact storage comparison rules
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_IMPLEMENTATION.md) -
  completed implementation note for the engine-owned admitted-slice cell
  store, including bootstrap, reconciliation, and resident-state validation
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_MIRRORING.md](COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_MIRRORING.md) -
  completed implementation note for Calc-side mirroring of admitted
  engine-resident cells before wiring replay and final exact verification
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_EVIDENCE.md) -
  frozen evidence note for the admitted cell-residency path, summarizing
  exact mirror recovery, retained rollback coverage, and bounded operational
  samples
- [COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_CELL_STORAGE_DECISION_RECORD.md) -
  completed closeout decision for the first cell-residency proof cycle,
  admitting engine-resident cell storage on the bounded slice while keeping
  formula-cell lifetime and live container residency in Calc
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md](COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_PLAN.md) -
  completed closeout record for the next bounded migration step, admitting
  engine-resident live wiring-container residency on the admitted slice while
  keeping formula-cell object lifetime, mutation entry, and rollback in Calc
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_RESIDENCY_CONTRACT.md) -
  frozen authority contract for the first admitted-slice wiring-container
  residency pilot, fixing the admitted mutation classes, engine-owned
  resident wiring surface, retained Calc host surfaces, and exact
  verification standard
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_SCHEMA.md) -
  frozen schema note for admitted engine-resident broadcaster nodes,
  listener edges, formula-tree and formula-track realized order, and exact
  wiring comparison rules
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_IMPLEMENTATION.md) -
  completed implementation note for the engine-owned admitted-slice wiring
  store, including bootstrap, graph-delta reconciliation, and resident-state
  validation
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_REALIZATION.md](COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_REALIZATION.md) -
  completed implementation note for Calc-side realization of admitted
  engine-resident wiring containers before exact computational and graph
  verification
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_EVIDENCE.md) -
  frozen evidence note for the admitted wiring-container residency path,
  summarizing exact resident realization, retained rollback coverage, and
  bounded operational samples
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md](COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_PLAN.md) -
  completed implementation and closeout record for the admitted-slice
  formula-cell lifetime proof cycle, admitting engine-owned lifetime
  decisions while keeping mutation entry and rollback in Calc
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_CONTRACT.md) -
  frozen contract note for the admitted formula-cell lifetime slice, fixing
  the admitted workbook classes, retained host surfaces, and exact success
  criteria
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_SCHEMA.md) -
  frozen schema note for engine-owned admitted formula-cell lifetime
  records, stable identity, replace/remove semantics, and equivalence rules
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_IMPLEMENTATION.md) -
  completed implementation note for the engine-owned admitted lifetime
  store, including bootstrap and transition-driven reconciliation
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_REALIZATION.md](COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_REALIZATION.md) -
  completed implementation note for Calc-side realization of admitted live
  `ScFormulaCell` objects from engine-owned lifetime state
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_EVIDENCE.md) -
  frozen evidence note for the admitted formula-cell lifetime path,
  summarizing exact realization, retained rollback coverage, and bounded
  operational samples
- [COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_FORMULA_CELL_LIFETIME_DECISION_RECORD.md) -
  completed closeout decision for the admitted formula-cell lifetime proof
  cycle, admitting engine-owned lifetime decisions while keeping mutation
  entry and live realization in Calc
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md](COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_PLAN.md) -
  completed closeout record for the first admitted-slice mutation-entry
  proof cycle, ending in a validation-only result rather than settled live
  admission
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_CONTRACT.md) -
  frozen contract note for the admitted mutation-entry slice, fixing the
  admitted request classes, retained Calc host surfaces, and exact success
  criteria for the completed proof cycle
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_REALIZATION.md](COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_REALIZATION.md) -
  completed implementation note for Calc-side apply and realization of
  engine-owned mutation-entry output on the admitted slice
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_EVIDENCE.md) -
  frozen evidence note for the admitted mutation-entry proof cycle,
  summarizing exact formula and structural entry results plus the remaining
  broadcaster-canonicalization caveat on scalar entry
- [COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_MUTATION_ENTRY_DECISION_RECORD.md) -
  completed historical closeout decision for the first mutation-entry proof
  cycle, which kept direct mutation entry validation-only before the scalar
  convergence reassessment was completed
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md](COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md) -
  completed closeout record for the bounded scalar broadcaster-convergence
  reassessment, ending in admitted live scalar mutation entry on the bounded
  slice
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_CONTRACT.md) -
  frozen contract note for the scalar convergence cycle, fixing the admitted
  scalar mutation surface, exact success standard, and immediate defer
  triggers
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CANONICALIZATION_MATRIX.md](COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CANONICALIZATION_MATRIX.md) -
  frozen matrix note for scalar broadcaster canonicalization, defining the
  covered listener shapes, diagnostic categories, and forbidden shortcuts
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_OBSERVATION.md](COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_OBSERVATION.md) -
  completed observation note for the scalar mismatch classification layer,
  recording the original `missing_expected_broadcasters` diagnosis
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_IMPLEMENTATION.md) -
  completed implementation note for the scalar convergence fix, aligning the
  authority after-state with the dependency-derived broadcaster surface
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_EVIDENCE.md) -
  frozen evidence note for the scalar convergence cycle, recording the
  before/after broadcaster result and the green bounded validation contract
- [COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_DECISION_RECORD.md) -
  completed closeout decision for scalar mutation-entry convergence,
  admitting direct scalar mutation entry on the bounded slice and naming
  broader object realization as the next adjacent concern
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md](COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_REASSESSMENT_PLAN.md) -
  completed closeout record for the bounded object-realization
  reassessment, proceeding with engine-authored admitted-slice live object
  realization and naming final rollback reassessment as the next adjacent
  concern
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_CONTRACT.md) -
  frozen contract note for the admitted object-realization slice, fixing the
  covered live object classes, retained Calc host surfaces, and exact
  success standard
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_MATRIX.md](COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_MATRIX.md) -
  frozen scenario matrix for admitted object realization, classifying
  candidate, validation-only, and deferred realization cases
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_OBSERVATION.md](COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_OBSERVATION.md) -
  completed observation note for object-realization drift classification,
  introducing exact, ordering-only, missing-object, host-repair, mismatch,
  and out-of-contract result families
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_IMPLEMENTATION.md) -
  completed implementation note for the engine-authored admitted
  object-realization record and the Calc realization path that consumes it
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_EVIDENCE.md) -
  frozen evidence note for the object-realization proof cycle, recording
  exact admitted realization, missing-object classification, retained
  rollback coverage, and a bounded runtime sample
- [COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_OBJECT_REALIZATION_DECISION_RECORD.md) -
  completed closeout decision for object-realization reassessment,
  admitting engine-authored object realization on the bounded slice while
  leaving raw mutation APIs and final rollback in Calc
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md](COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_REASSESSMENT_PLAN.md) -
  completed closeout record for the bounded final-rollback reassessment,
  proceeding with engine-authored admitted-slice rollback and naming broader
  raw mutation API migration as the next adjacent concern
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_CONTRACT.md) -
  frozen contract note for the admitted rollback slice, fixing covered
  rollback classes, retained Calc host surfaces, and exact success standard
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_MATRIX.md](COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_MATRIX.md) -
  frozen scenario matrix for admitted rollback, classifying candidate,
  validation-only, and deferred rollback cases
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_OBSERVATION.md](COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_OBSERVATION.md) -
  completed observation note for rollback drift classification, introducing
  exact, ordering-only, missing-restored-object, host-reconstruction,
  mismatch, and out-of-contract result families
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_IMPLEMENTATION.md) -
  completed implementation note for the engine-authored admitted rollback
  record and the Calc rollback shell that consumes it
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_EVIDENCE.md) -
  frozen evidence note for the final-rollback proof cycle, recording exact
  admitted rollback restore, runtime rollback observation, and replay
  stability
- [COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_FINAL_ROLLBACK_DECISION_RECORD.md) -
  completed closeout decision for final rollback reassessment, admitting
  engine-authored rollback on the bounded slice while leaving raw mutation
  APIs in Calc
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md](COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_MIGRATION_PLAN.md) -
  completed closeout record for the admitted raw-mutation reassessment,
  proceeding with engine-authored raw mutation records on the bounded slice
  while leaving the broader live apply shell in Calc
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_CONTRACT.md) -
  frozen contract note for the admitted raw-mutation slice, fixing the
  covered mutation classes, retained Calc host surfaces, and exact success
  standard
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_SCHEMA.md) -
  frozen schema note for the admitted raw-mutation record, stable mutation
  identity, payload rules, and forbidden host shortcuts
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_OBSERVATION.md](COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_OBSERVATION.md) -
  completed observation note for raw-mutation-shell drift classification,
  introducing exact, ordering-only, hidden-reconstruction, missing-object,
  mismatch, and out-of-contract result families
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_IMPLEMENTATION.md) -
  completed implementation note for the engine-authored admitted raw
  mutation record and the Calc host apply path that consumes it
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_EVIDENCE.md) -
  frozen evidence note for the raw-mutation proof cycle, recording exact
  admitted apply, exact dirty-baseline rollback, bounded runtime samples,
  and the narrowed host shell
- [COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_RAW_MUTATION_API_DECISION_RECORD.md) -
  completed closeout decision for raw-mutation migration, admitting
  engine-authored raw mutation records on the bounded slice and naming
  broader live apply-shell reassessment as the next adjacent concern
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md](COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_REASSESSMENT_PLAN.md) -
  completed closeout record for the admitted live apply-shell reassessment,
  proceeding with engine-authored live apply sequencing on the bounded slice
  while leaving the primitive mutation APIs and final verification host shell
  in Calc
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_CONTRACT.md) -
  frozen contract note for the admitted live apply-shell slice, fixing the
  covered stage families, retained Calc host surfaces, and exact success
  standard
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_SCHEMA.md) -
  frozen schema note for the admitted live apply plan, stable stage
  identity, stage ordering, and forbidden host orchestration shortcuts
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_OBSERVATION.md](COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_OBSERVATION.md) -
  completed observation note for live apply-shell drift classification,
  introducing exact, ordering-only, hidden-orchestration, missing-object,
  mismatch, and out-of-contract result families
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_IMPLEMENTATION.md) -
  completed implementation note for the engine-authored admitted live apply
  plan and the mutation-entry path that carries it
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_EVIDENCE.md) -
  frozen evidence note for the live apply-shell proof cycle, recording exact
  admitted apply, exact dirty-baseline rollback, bounded runtime samples,
  and the narrowed host shell
- [COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_LIVE_APPLY_SHELL_DECISION_RECORD.md) -
  completed closeout decision for live apply-shell migration, admitting
  engine-authored admitted apply plans on the bounded slice and naming raw
  document mutation API migration as the next adjacent concern
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md](COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_MIGRATION_PLAN.md) -
  completed implementation and closeout record for the bounded reassessment
  after live-apply closeout, ending in admitted engine-authored primitive
  mutation records and apply verdicts on the bounded slice
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_CONTRACT.md) -
  frozen authority contract for the admitted primitive document-mutation
  shell, fixing the bounded mutation classes, retained Calc host surfaces,
  and exact success standard
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_SCHEMA.md) -
  frozen schema note for admitted primitive mutation records, stable
  identity, normalized payload rules, and top-level verdict families
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_OBSERVATION.md](COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_OBSERVATION.md) -
  completed observation note for primitive document-mutation drift
  classification, introducing the bounded exact, ordering-only,
  hidden-orchestration, missing-object, mismatch, and out-of-contract
  families for primitive execution
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_IMPLEMENTATION.md) -
  completed implementation note for the engine-authored primitive
  document-mutation record, apply result, and mutation-entry integration
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_EVIDENCE.md) -
  frozen evidence note for the raw document mutation proof cycle, recording
  exact admitted primitive execution, exact dirty-baseline rollback
  carriage, bounded runtime samples, and the narrowed host shell
- [COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_RAW_DOCUMENT_MUTATION_API_DECISION_RECORD.md) -
  completed closeout decision for raw document mutation migration, admitting
  engine-authored admitted primitive mutation records and apply verdicts on
  the bounded slice while naming primitive realization and rollback shell
  reassessment as the next adjacent concern
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_REASSESSMENT_PLAN.md) -
  completed implementation and closeout record for the bounded reassessment
  after raw document mutation closeout, ending in admitted engine-authored
  primitive realization and rollback records and apply verdicts on the
  bounded slice
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_CONTRACT.md) -
  frozen authority contract for the admitted primitive realization and
  rollback shell, fixing the bounded mutation classes, retained Calc host
  surfaces, and exact success standard
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_SCHEMA.md) -
  frozen schema note for admitted primitive realization and rollback
  records, stable identity, normalized payload rules, and top-level verdict
  families
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_OBSERVATION.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_OBSERVATION.md) -
  completed observation note for primitive realization and rollback drift
  classification, introducing the bounded exact, ordering-only,
  hidden-orchestration, missing-object, mismatch, and out-of-contract
  families for primitive execution
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_IMPLEMENTATION.md) -
  completed implementation note for the engine-authored primitive
  realization and rollback records, apply results, and mutation-entry
  integration
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_EVIDENCE.md) -
  frozen evidence note for the primitive realization and rollback proof
  cycle, recording exact admitted execution, exact dirty-baseline rollback
  carriage, bounded runtime samples, and the narrowed host shell
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_REALIZATION_ROLLBACK_DECISION_RECORD.md) -
  completed closeout decision for primitive realization and rollback
  migration, admitting engine-authored admitted primitive realization and
  rollback records and apply verdicts on the bounded slice while naming
  final verification host-shell reassessment as the next adjacent concern
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md](COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_REASSESSMENT_PLAN.md) -
  completed closeout record for the final verification host-shell
  reassessment, proceeding with engine-authored admitted final
  verification on the bounded slice while naming primitive execution
  host-operation reassessment as the next adjacent concern
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_CONTRACT.md) -
  frozen authority contract for the admitted final verification slice,
  fixing the bounded workbook surface, retained Calc host shell, and exact
  success standard
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_SCHEMA.md) -
  frozen schema note for the admitted final verification record, stable
  verification identity, and verdict families
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_OBSERVATION.md](COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_OBSERVATION.md) -
  completed observation note for final verification drift classification,
  distinguishing exact, normalized, ordering-only, hidden-host, missing
  input, mismatch, and out-of-contract outcomes
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_IMPLEMENTATION.md) -
  completed implementation note for the engine-authored admitted final
  verification record, rollback comparison carriage, and mutation-entry
  integration
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_EVIDENCE.md) -
  frozen evidence note for the final verification proof cycle, recording
  exact admitted apply and rollback lanes, bounded runtime samples, and the
  narrower retained host shell
- [COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_FINAL_VERIFICATION_HOST_SHELL_DECISION_RECORD.md) -
  completed closeout decision for final verification migration, admitting
  engine-authored admitted final verification on the bounded slice while
  naming primitive execution host-operation reassessment as the next
  adjacent concern
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_REASSESSMENT_PLAN.md) -
  completed implementation and closeout record for the primitive execution
  host reassessment, admitting an explicit primitive execution plan and
  observation on the bounded slice while keeping the primitive host-call
  executor in Calc
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_CONTRACT.md) -
  frozen contract note for the admitted primitive execution slice, fixing
  the bounded workbook surface, retained Calc host shell, and exact success
  standard
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_SCHEMA.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_SCHEMA.md) -
  frozen schema note for the admitted primitive execution plan, stable
  primitive execution identity, and verdict families
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_OBSERVATION.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_OBSERVATION.md) -
  completed observation note for primitive execution drift classification,
  distinguishing exact, normalized, ordering-only, hidden-host,
  missing-input, mismatch, and out-of-contract outcomes
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_IMPLEMENTATION.md) -
  completed implementation note for the engine-authored admitted primitive
  execution plan, mutation-entry integration, and bounded host-shell
  reduction
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_EVIDENCE.md) -
  frozen evidence note for the primitive execution proof cycle, recording
  exact admitted apply and rollback lanes, bounded runtime samples, and the
  narrower retained host shell
- [COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_PRIMITIVE_EXECUTION_HOST_DECISION_RECORD.md) -
  completed closeout decision for primitive execution migration, admitting
  engine-authored admitted primitive execution on the bounded slice while
  naming primitive host-call executor reassessment as the next adjacent
  concern
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md](ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_PLAN.md) -
  completed implementation and closeout record for finishing substantive
  ownership on the current admitted slice, ending in an ownership-complete
  boundary on that slice while leaving Calc as only a thin primitive-call
  host
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_CONTRACT.md](ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_CONTRACT.md) -
  frozen contract note for the ownership-closeout cycle, fixing the bounded
  workbook surface, retained primitive host-call shell, and success
  standard for current-slice completion
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_SCHEMA.md](ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_SCHEMA.md) -
  frozen schema note for the admitted primitive host-call executor plan,
  stable executor identity, and verdict families
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_OBSERVATION.md](ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_OBSERVATION.md) -
  completed observation note for primitive host-call drift classification,
  distinguishing exact, normalized, ordering-only, hidden-host,
  missing-input, mismatch, and out-of-contract outcomes
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_IMPLEMENTATION.md](ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_IMPLEMENTATION.md) -
  completed implementation note for the engine-authored primitive host-call
  executor path, mutation-entry integration, and verified-result gating
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_EVIDENCE.md](ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_EVIDENCE.md) -
  frozen evidence note for the ownership-closeout proof cycle, recording
  exact admitted apply and rollback lanes, bounded runtime samples, and the
  smaller retained host shell
- [ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_DECISION_RECORD.md](ADMITTED_SLICE_OWNERSHIP_CLOSEOUT_DECISION_RECORD.md) -
  completed closeout decision for current-slice ownership completion,
  marking the admitted slice as substantively ownership-complete and
  shifting the roadmap to slice widening
- [COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_WIRING_CONTAINER_DECISION_RECORD.md) -
  completed closeout decision for the first wiring-container residency proof
  cycle, admitting engine-resident live wiring containers on the bounded
  slice while keeping formula-cell object lifetime and mutation entry in Calc
- [COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_CONTRACT.md](COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_CONTRACT.md) -
  frozen authority contract for the first storage-and-wiring pilot, fixing
  the admitted workbook slice, engine-owned sidecar state, engine-issued
  graph deltas, retained Calc host surfaces, and verification standard
- [COMPUTATIONAL_SUBSTRATE_MUTABLE_SUBSTRATE_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_MUTABLE_SUBSTRATE_IMPLEMENTATION.md) -
  completed implementation note for the engine-owned mutable sidecar
  substrate, including the value-semantic state container, bootstrap path,
  and transition-driven update model on the admitted slice
- [COMPUTATIONAL_SUBSTRATE_GRAPH_DELTA_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_GRAPH_DELTA_IMPLEMENTATION.md) -
  completed implementation note for the engine-owned graph and wiring delta
  surface, including listener-edge, broadcaster-node, formula-tree, and
  formula-track deltas derived from before/after graph shadows
- [COMPUTATIONAL_SUBSTRATE_WIRING_APPLY_IMPLEMENTATION.md](COMPUTATIONAL_SUBSTRATE_WIRING_APPLY_IMPLEMENTATION.md) -
  completed implementation note for the Calc-side apply adapters that replay
  the engine-owned admitted wiring target into retained host listener,
  formula-tree, and formula-track containers
- [COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_EVIDENCE.md](COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_EVIDENCE.md) -
  completed evidence note for the first storage-and-wiring proof cycle,
  summarizing exact host replay results, retained rollback coverage, and
  bounded operational samples
- [COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_DECISION_RECORD.md](COMPUTATIONAL_SUBSTRATE_STORAGE_AND_WIRING_DECISION_RECORD.md) -
  completed closeout decision for the first storage-and-wiring proof cycle,
  admitting engine-owned mutable sidecar state and graph/wiring targets on
  the bounded slice while keeping physical container residency in Calc
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
