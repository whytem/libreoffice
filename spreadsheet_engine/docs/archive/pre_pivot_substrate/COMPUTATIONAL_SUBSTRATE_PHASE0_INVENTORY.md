# Computational Substrate Phase 0 Inventory

Status: active Phase 0 ownership inventory artifact

## Purpose

This document is the checked-in ownership map required by workstream `0.1` of
[COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md).

Its job is to classify the live Calc computational substrate into:

- computational and candidate for migration
- host-only and retained
- mixed and requiring a later seam

The goal is not to pretend that the whole substrate is ready to move today.
The goal is to make the current ownership and coupling explicit enough that
later shadow work can proceed with a precise scope.

## Classification Summary

| Surface | Primary roots | Current owner | Classification | Notes |
| --- | --- | --- | --- | --- |
| Formula tree | [document.hxx](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx#L415), [documen7.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/documen7.cxx) | Calc | Mixed | Computational queue membership and order are migration candidates; pointer-linked storage and lifecycle are still embedded in `ScDocument` and `ScFormulaCell`. |
| Broadcast track | [document.hxx](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx#L417), [documen7.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/documen7.cxx#L453) | Calc | Mixed | Recalc-notify staging is computational, but it is expressed today as Calc-owned linked-list state on formula cells. |
| Broadcast-area machine | [document.hxx](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx#L419), [bcaslot.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/bcaslot.cxx) | Calc | Mixed | Area indexing and listener registration semantics are migration candidates; current slot allocation and ownership are still tied to Calc document and table storage. |
| Listener contexts | [listenercontext.hxx](/home/ubuntu/repos/libreoffice/sc/inc/listenercontext.hxx), [listenercontext.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/listenercontext.cxx) | Calc | Mixed | The semantic role is computational, but the implementation is bound to `ScDocument`, `ColumnBlockPositionSet`, and old `ScTokenArray` state. |
| Broadcaster storage | [mtvelements.hxx](/home/ubuntu/repos/libreoffice/sc/inc/mtvelements.hxx#L146), [column4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/column4.cxx#L2254) | Calc | Mixed | Broadcaster presence is computational state; the actual container is embedded in Calc column storage and therefore moves only with a deeper storage shift. |
| Formula-cell listener lifecycle | [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx#L1256), [formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx#L2532) | Calc | Mixed | Listener establish/teardown semantics are migration candidates; lifecycle triggers remain tightly coupled to formula-cell compile, dirtying, load, and destruction flows. |
| Delayed listener startup and delayed broadcaster deletion | [document10.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/document10.cxx#L16), [column2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/column2.cxx#L3510) | Calc | Mixed | Computationally relevant for exact shadowing, but currently mediated by Calc-owned purge queues and delayed mutation policy. |
| `ScTokenArray` construction | [compiler.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/compiler.cxx#L5834), [token.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/token.cxx#L2343) | Calc | Mixed | Compile output semantics are migration candidates; the concrete container and mutation APIs remain Calc-owned. |
| Range and union token-container updates | [compiler.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/compiler.cxx#L5006), [token.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/token.cxx#L2308) | Calc | Mixed | Reference-shape semantics are computational, but merge/update behavior is currently expressed as `ScTokenArray` container mutation. |
| Reference adjustment after structural edits | [token.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/token.cxx#L3201) | Calc | Mixed | Structural reference semantics are a likely later migration candidate, but the current logic still depends on Calc token representation and edit context. |

## Detailed Ownership Notes

### Formula Tree And Broadcast Track

The live formula queue state is rooted directly in `ScDocument` through
`pFormulaTree`, `pEOFormulaTree`, `pFormulaTrack`, and `pEOFormulaTrack` in
[document.hxx](/home/ubuntu/repos/libreoffice/sc/inc/document.hxx#L415).

This state is computational in purpose:

- queue membership determines what still needs recalculation
- queue order affects evaluation order and shadow recalc comparison
- track membership controls the broadcast/recalc transition surface

But it is not yet isolated from Calc storage:

- queue links are stored on `ScFormulaCell`
- queue operations are methods on `ScDocument`
- lifecycle events are triggered from formula-cell compile, dirtying, load, and
  destroy flows

Classification:

- candidate for migration: queue semantics, membership, ordering, grouping
- retained for now: pointer-linked list ownership inside Calc formula cells
- later seam needed: engine-owned queue shadow versus Calc-owned cell objects

### Broadcast-Area Machine

The BASM is owned by `ScDocument` via `pBASM` and implemented in
[bcaslot.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/bcaslot.cxx).

The project should treat the following as computationally meaningful:

- area broadcaster registration shape
- area listener membership
- slot-machine update behavior during edits
- delayed erasure behavior

But it should also acknowledge the present Calc coupling:

- slot allocation is document-owned
- slots are organized by table and row/column partitioning chosen inside Calc
- listeners are still notified through Calc broadcaster containers

Classification:

- candidate for migration: area indexing and listener semantics
- retained for now: current slot-machine container ownership
- later seam needed: normalized area-graph view versus concrete BASM layout

### Listener Contexts And Broadcaster Storage

The listener-context layer in
[listenercontext.hxx](/home/ubuntu/repos/libreoffice/sc/inc/listenercontext.hxx)
and the broadcaster storage embedded in
[mtvelements.hxx](/home/ubuntu/repos/libreoffice/sc/inc/mtvelements.hxx#L146)
are the clearest examples of mixed ownership.

The semantic role is computational:

- find broadcaster anchors
- start and end listening
- purge empty broadcasters

The implementation is still Calc-shaped:

- `StartListeningContext` and `EndListeningContext` are parameterized by
  `ScDocument`
- column block positions are cached in Calc-only storage structures
- broadcaster containers live in column multi-type vectors

Classification:

- candidate for migration: the observable listener/broadcaster graph state
- retained for now: column block position caches and embedded broadcaster
  stores
- later seam needed: graph-oriented engine model versus Calc column storage

### Formula-Cell Listener Lifecycle

The formula-cell layer under
[formulacell.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/data/formulacell.cxx)
still owns the trigger points that build and tear down listener state.

Important lifecycle surfaces include:

- load-time listener establishment
- dirty/track transitions
- recompilation behavior
- shared-group and move/update flows
- delayed tracking during load or structural mutations

Classification:

- candidate for migration: lifecycle semantics that affect graph state
- retained for now: formula-cell object lifetime and host side effects
- later seam needed: explicit event model for listener lifecycle transitions

### `ScTokenArray` Construction, Merge, And Adjustment

The compiler and token layer still produce and mutate Calc-owned token
containers directly:

- `AddSingleReference()` in
  [token.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/token.cxx#L2343)
- `AddDoubleReference()` in
  [token.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/token.cxx#L2353)
- `MergeRangeReference()` in
  [token.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/token.cxx#L2308)
- `AdjustReferenceOnShift()` in
  [token.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/token.cxx#L3201)
- compiler-emitted construction sites in
  [compiler.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/compiler.cxx#L5834)
  and
  [compiler.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/compiler.cxx#L5006)

Classification:

- candidate for migration: reference semantics, range-shape semantics, update
  semantics
- retained for now: the current `ScTokenArray` container as the physical owner
- later seam needed: engine-owned execution IR with lowering from Calc compile
  output during transition

## Explicitly Retained Host-Owned Surfaces

The following are out of scope for computational-substrate migration even if
later phases succeed:

- UI, rendering, UNO, persistence, and shell integration
- printer, path, number-format, and similar environment services
- external-reference cache ownership and session/document service ownership
- host-specific add-in service acquisition such as null-date retrieval and
  holiday list expansion

These surfaces may still need adapters, but they are not part of the
computational substrate that Phase 0 is trying to classify.

## Proceed Implications

This inventory supports a narrower and more honest Phase 0 proceed rule:

- the project should proceed only if later phases can compare the mixed
  surfaces above through stable normalized captures
- later phases should not assume that Calc-owned containers themselves are the
  migration target
- where a surface is marked mixed, the likely destination is an engine-owned
  normalized model plus a Calc adapter, not a direct relocation of the current
  Calc type

That distinction is especially important for:

- formula-tree and track linked lists
- BASM slot layout
- column broadcaster stores
- `ScTokenArray`
