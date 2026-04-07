# Computational Substrate Shared-Group Non-Structural Frontier Admission Plan

Status: active implementation plan

## Purpose

This document defines the next explicit proof cycle after the completed
[COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_DECISION_RECORD.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SHARED_GROUP_NON_STRUCTURAL_DECISION_RECORD.md).

The previous cycle closed one deliberately bounded result:

- exact same-sheet shareable shared-group member-exit `SetScalarValue`,
  `SetFormula`, and `ClearCell` now admit through the live authority,
  lifecycle, and mutation-entry slice
- broader non-structural classes remained deferred because they still depend
  on retained host regrouping, normalization, named-range, or off-sheet
  behavior

That bounded question is now answered. The next question is broader, but it
still should not be treated as one undifferentiated rollout jump.

The remaining frontier is:

- same-text preserve replacements
- regroup and merge behavior
- named-range-combined shared-group behavior
- repair-sensitive host-only normalization
- off-sheet shared-group behavior

This plan therefore defines the staged path for deciding whether those
broader non-structural shared-group classes can move into the admitted slice
without weakening the exact-verification standard already established on the
current rollout.

## Plan Goal

Determine whether the remaining non-structural shared-group frontier can be
split into exact, engine-authored promotion families that satisfy the same
queue, computational, graph, IR, realization, rollback, and final
verification standard as the current admitted slice.

The closeout must answer one explicit question:

- which broader non-structural shared-group families can now be admitted
- which must stay validation-only
- and which must remain explicitly deferred

This plan succeeds even if the final result is partial. It does not require
all five frontier classes to admit together.

## Entry Boundary

This plan begins from the current settled state:

- the first-stage extraction boundary is complete and stable
- the computational-substrate program already has an ownership-complete
  admitted slice
- the live rollout already admits:
  - scalar mutation entry
  - scalar lifecycle authority
  - same-sheet structural `InsertRows`, `DeleteRows`, `InsertColumns`, and
    `DeleteColumns`
  - exact same-sheet shareable structural shared-group `Preserve`, `Split`,
    and `Rebuild`
  - exact same-sheet shareable non-structural shared-group member-exit
    `SetScalarValue`, `SetFormula`, and `ClearCell`
- the current shared-group runtime already has:
  - explicit shared-group descriptor collection and transition
    classification in
    [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)
  - a structural shared-group gate
  - a non-structural shared-group member-exit gate
  - engine-authored same-sheet repartitioning for member-exit topology
- the remaining deferred shared-group frontier still includes:
  - same-text preserve cases that intentionally keep the group
  - regroup and merge classes across prior group boundaries
  - named-range-combined shared-group behavior
  - repair-sensitive host-only regrouping or normalization
  - off-sheet shared-group consumers or topology changes

This plan must therefore extend a working admitted slice, not reopen the
already-closed member-exit promotion result.

## Frontier Families

The broader frontier should be treated as five distinct promotion families,
not as one all-or-nothing effort.

### 1. Same-Text Preserve

Representative shapes:

- shared-group anchor `SetFormula` with identical formula text
- shared-group interior member `SetFormula` with identical formula text
- cases where Calc preserves group membership, shareability, and formula
  payload while still mutating dirty state, tree/track state, or host-owned
  group-local metadata

Key question:

- can the engine prove exact preserve semantics without depending on hidden
  host normalization

### 2. Regroup And Merge

Representative shapes:

- replacement formula that causes the touched cell to stay grouped
- replacement formula that causes two prior runs to merge into one group
- adjacent ordinary formulas that re-enter a group after edit
- multiple prior groups on the same column that collapse into one after edit

Key question:

- can the engine author post-edit group creation and merge identity rather
  than merely validate host-observed topology

### 3. Named-Range-Combined Non-Structural

Representative shapes:

- same-sheet shared-group edits whose formulas reference admitted global or
  sheet-local named ranges
- edits whose regroup or preserve outcome depends on exact named-range
  retargeting
- off-sheet or same-sheet references routed through named-range identity

Key question:

- can shared-group topology and named-range dependency updates close exactly
  in one engine-authored after-state

### 4. Repair-Sensitive Host-Only Normalization

Representative shapes:

- cases where Calc silently repairs or normalizes group structure after the
  raw document mutation
- cases where host-local cleanup changes shareability, anchor placement, or
  run partitioning beyond the engine’s current authored rules
- cases that are currently visible only as repair-detected or deferred

Key question:

- can the retained host normalization be modeled explicitly enough to become
  an admitted normalization class, or must these cases stay outside the
  admitted slice

### 5. Off-Sheet Shared-Group

Representative shapes:

- same group affected by dependencies that cross sheets
- off-sheet consumers whose queue, broadcaster, graph, or IR state depends
  on the shared-group mutation
- shared-group behavior whose effective regroup or preserve outcome depends
  on workbook-wide rather than single-sheet state

Key question:

- can the engine close exact authority over the broader workbook surface
  without turning this into a document-wide rollout jump

## Guiding Constraints

This plan should preserve the current program discipline:

- widen by bounded proof families, not by category labels
- keep clean-baseline and exact-verification requirements unless a checked-in
  contract explicitly widens them
- separate exact admitted classes from validation-only and deferred classes
- avoid using host-observed after-topology as the source of truth for any
  newly admitted family
- prefer staged family-specific gates over a single broad shared-group
  frontier switch

## Non-Goals

This plan should not attempt to:

- collapse every deferred shared-group class into one implementation step
- reopen already-settled structural or member-exit admission decisions
- widen into sheet insert, delete, rename, or move
- widen into copy, move, clipboard, import, load-time, or undo-like flows
- weaken exact verification into broad normalized-equivalent admission
- infer broad document-wide authority from success on one off-sheet family
- reopen storage residency, token-container ownership, or broad listener
  container transfer decisions

## Required Deliverables

This plan is complete only when all of the following exist:

1. a checked-in contract note freezing the broader non-structural frontier
   into explicitly staged promotion families
2. a checked-in scenario matrix covering same-text preserve, regroup/merge,
   named-range-combined, repair-sensitive, and off-sheet cases
3. a checked-in mapping note defining group identity, merge identity,
   normalization classes, and forbidden host shortcuts for the broader
   frontier
4. one or more dedicated gated runtime paths that let each candidate family
   run through exact authority, lifecycle, or mutation-entry apply
5. a checked-in evidence note recording exact, normalized, rejected,
   repair-detected, rollback, validation-only, and deferred outcomes by
   family
6. a checked-in decision record that explicitly marks which families are:
   - admitted
   - validation-only
   - or deferred

## Staged Workstreams

### 1. Freeze The Frontier Contract

Define the broader frontier before implementation begins.

This contract should freeze:

- the five family buckets in scope:
  - same-text preserve
  - regroup/merge
  - named-range-combined
  - repair-sensitive normalization
  - off-sheet
- which mutations are candidate in each family:
  - `SetFormula`
  - `SetScalarValue`
  - `ClearCell`
- which workbook classes remain excluded even in this cycle
- which family may produce:
  - exact admit
  - normalized-equivalent but still admitted
  - validation-only
  - repair-detected
  - deferred

Required artifact:

- one checked-in shared-group non-structural frontier contract note

### 2. Freeze The Frontier Scenario Matrix

Define the exact proof surface that the cycle must cover.

The matrix should explicitly include:

- same-text preserve on anchor, interior member, and tail member
- regroup after replacement formula on anchor, interior member, and tail
- merge of adjacent runs or adjacent prior groups
- named-range-combined preserve, regroup, and merge
- repair-sensitive host normalization where live after-state diverges from
  the engine’s first authored result
- off-sheet consumers and off-sheet dependency edges

The matrix should mark each scenario as one of:

- candidate exact admit
- candidate admitted normalization class
- validation-only
- reject
- repair-detected
- deferred

Required artifact:

- one checked-in shared-group non-structural frontier scenario matrix

### 3. Freeze The Frontier Mapping Rules

Define how the broader frontier predicts after-state identity.

This mapping note should state:

- how a same-text replacement preserves or mutates group identity
- how newly formed or merged runs are identified in the engine-owned
  after-state
- how multiple before-groups map to one after-group when merge is allowed
- when named-range updates are part of group-identity closure versus a hard
  reject
- what a repair-sensitive normalization class means and how it differs from
  a silent host shortcut
- which off-sheet dependency changes are allowed to participate in an exact
  admitted after-state

Required artifact:

- one checked-in shared-group non-structural frontier mapping rules note

### 4. Extend Facade-Side Classification

Broader admission needs a richer observable language before runtime
promotion.

This workstream should:

- extend shared-group transition classification beyond preserve/rebuild/split
  labels where necessary
- surface enough metadata to distinguish:
  - same-text preserve
  - member-exit rebuild
  - regroup
  - merge
  - normalization-only host repair
- ensure named-range-combined and off-sheet cases can be identified without
  relying on hidden Calc-local interpretation

Required outcome:

- facade-side classification is rich enough to drive tests and runtime
  gating for each frontier family

### 5. Admit Same-Text Preserve

Treat same-text preserve as the first adjacent family.

This workstream should:

- determine what host-observed state changes on same-text replacement are
  semantically meaningful
- teach lifecycle and authority builders to recognize exact preserve cases
  without routing them through member-exit logic
- prove exact closure for queue, computational, graph, IR, realization, and
  mutation entry

Expected result:

- either same-text preserve becomes the next exact admitted family
- or it is downgraded to an explicit normalization-only or deferred class

### 6. Admit Regroup And Merge

Treat regroup/merge as a separate family rather than an extension of
preserve.

This workstream should:

- define engine-authored after-topology creation rules for grouped formulas
  that remain or become shareable after mutation
- define stable identity when multiple before-runs map to one after-run
- prove exact live closure for one bounded regroup family first
- widen to bounded merge only after regroup identity is stable

Recommended sequence:

- exact same-sheet shareable regroup with one touched group
- then exact same-sheet shareable merge of two adjacent runs or groups

### 7. Integrate Named-Range-Combined Cases

Named-range-combined shared-group behavior should not be attempted before
the plain regroup surface is stable.

This workstream should:

- intersect the shared-group frontier with the already-known named-range
  classes
- begin with the narrowest named-range family that already has the strongest
  standalone proof surface
- require exact named-range target, dependency snapshot, broadcaster, graph,
  and IR closure alongside shared-group topology

Recommended entry slice:

- same-sheet shareable shared-group behavior combined with bounded global
  single-area named-range semantics

### 8. Model Repair-Sensitive Normalization Explicitly

Repair-sensitive host-only behavior is the hardest boundary and should be
treated as its own proof lane.

This workstream should:

- catalogue the currently observed host normalization and repair behaviors
- separate deterministic, modelable normalization from opaque host repair
- decide whether any normalization class can be admitted as:
  - exact
  - or normalized-equivalent with an explicit checked-in contract
- keep opaque repair that cannot be modeled on the rollback or deferred lane

The default bias should remain conservative:

- if the normalization cannot be modeled explicitly, it should not move into
  the admitted slice

### 9. Widen To Off-Sheet Cases

Off-sheet families should come last because they expand the authority
surface beyond the current single-sheet bias.

This workstream should:

- identify the smallest off-sheet consumer classes worth promoting
- prove exact queue, broadcaster, graph, and IR closure across sheet
  boundaries
- define whether off-sheet group topology itself changes or only off-sheet
  dependency consumers widen
- avoid jumping from one bounded off-sheet family to a broad workbook-wide
  guarantee

Recommended entry slice:

- same-sheet shared-group mutation with off-sheet formula consumers but no
  off-sheet shared-group topology change

### 10. Gate Strategy And Mutation-Entry Carry-Through

The broader frontier should not hide behind a single generic gate.

This workstream should:

- define whether each family gets its own gate or whether a staged family
  matrix is encoded under one frontier umbrella gate
- keep authority, lifecycle, structural, and mutation-entry routing explicit
- ensure admitted families carry through mutable substrate, realization,
  rollback, and final verification without special-case host shortcuts

Recommended bias:

- family-specific sub-gates until multiple broader families have proved
  stable together

### 11. Freeze Evidence And Final Decision

Once the staged work above is complete, record the exact outcome by family.

The evidence note should summarize:

- which preserve classes closed exactly
- which regroup or merge classes closed exactly
- which named-range-combined classes admitted, normalized, or deferred
- which repair-sensitive classes remained rollback-only or deferred
- which off-sheet families admitted
- which gates were added, retained, or removed

The final decision record should mark each family independently as:

- admitted
- validation-only
- deferred

## Recommended Execution Order

The safest order for this cycle is:

1. freeze the contract, scenario matrix, and mapping rules
2. extend facade-side classification
3. land same-text preserve
4. land bounded regroup
5. land bounded merge
6. intersect that result with bounded named-range-combined classes
7. model repair-sensitive normalization
8. widen to bounded off-sheet consumers
9. close mutation-entry and gate strategy for the families that succeed
10. write evidence and the final decision record

This ordering preserves the current discipline:

- easier identity-preserving classes first
- identity-creating classes next
- named-range and off-sheet intersections later
- host-only repair last

## Success Criteria

This plan should be considered successful only if the final closeout can say
all of the following for each newly admitted family:

- the after-state is engine-authored, not copied from observed topology
- queue verification closes at the required standard
- computational verification closes at the required standard
- graph verification closes at the required standard
- IR verification closes at the required standard
- object realization, rollback, and final verification stay explicit and
  bounded
- the family boundary is documented clearly enough that the remaining
  deferred classes stay obvious

If any family cannot satisfy those criteria, the correct outcome is to keep
that family validation-only or deferred rather than broadening the admitted
slice implicitly.
