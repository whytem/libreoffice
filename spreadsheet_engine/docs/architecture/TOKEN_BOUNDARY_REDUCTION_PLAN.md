# Token Boundary Reduction Plan

## Purpose

The extraction, replay-promotion, recalc-orchestration, execution-shell,
engine-first adoption, and host-boundary consolidation programs are complete.

The next implementation stream is to reduce the remaining Calc-local
token/container shell that still sits between production Calc entry points and
engine-owned spreadsheet semantics.

This is a boundary-reduction program, not a storage migration, a recalc
rewrite, or a wholesale replacement of `ScInterpreter`.

## What This Workstream Is For

This stream is successful when the remaining in-scope Calc token/container
surface:

- is clearly limited to ownership, mutation, and host-shaped cursor control
- exposes narrower non-owning traversal and adaptation seams to shared engine
  logic
- relies less on ad hoc `ScTokenArray` inspection in shared-semantic paths
- is easier to audit as an intentional host boundary instead of legacy
  coupling

In concrete terms, the focus is:

- non-owning token traversal and inspection helpers
- repeated range/union and token-shape adaptation patterns
- value-shape translation adjacent to token walking
- explicit classification of what remains Calc-owned on purpose

## Out Of Scope

This plan does not:

- move formula-tree ownership or token mutation out of Calc
- move `ScDocument` storage or document mutation into the engine
- reopen the recalc authority or queue-construction boundary
- replace `ScInterpreter` wholesale
- change the zero-fallback replay contract

## Definition Of Done

This workstream is complete when all of the following are true:

1. the remaining in-scope token/container surfaces are explicitly classified as
   `compat seam`, `host-only`, or `defer`
2. repeated non-owning token traversal and token-shape translation patterns in
   the touched scope route through named compat seams
3. shared-semantic paths no longer depend on scattered direct
   `ScTokenArray` inspection where a bounded compat helper is feasible
4. the touched Calc code reads primarily as ownership/cursor management rather
   than spreadsheet semantics
5. the promoted replay corpus remains at `0` cached fallback under the strict
   summary assertion gate
6. Calc and standalone validation lanes stay green for every slice

## Boundary Rules

Every slice in this stream must continue to respect the current project
boundary:

- keep token ownership, mutation, and formula-tree lifetime in Calc
- extract only non-owning traversal, inspection, or adaptation layers
- prefer thin compat seams over widening engine internals to mirror Calc
  containers
- prefer engine-owned vocabulary for spreadsheet semantics and value shapes
- keep host service lookups explicit and localized
- do not introduce duplicate semantic implementations

## Workstreams

### 1. Freeze The Remaining Token-Boundary Inventory

Build and maintain an explicit inventory of the remaining Calc-local
token/container seams that still matter to the broader extraction objective.

For each candidate, record:

- file and entry point
- whether it is token traversal, container adaptation, cursor ownership, or
  host service usage
- whether an engine-owned helper or compat seam already exists nearby
- whether the correct outcome is `compat seam`, `host-only`, or `defer`
- required validation lanes

Primary target files:

- [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- [interpr1.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr1.cxx)
- [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx)
- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- compat headers under
  [spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice)

Completion criteria:

- there is no unclassified in-scope token/container tail in the touched areas
- each candidate has a bounded owning later slice or an explicit defer reason

### 2. Extract Non-Owning Token Traversal Helpers

Concentrate repeated token-reading and token-shape inspection patterns into
named compat helpers while leaving mutation and ownership in Calc.

Target categories:

- bounded token iteration over shared-semantic paths
- token-type and operand-shape inspection
- lightweight cursor/read-state packaging that does not transfer ownership

Implementation rules:

- keep the actual interpreter stack and token cursor ownership in Calc
- extract only the reusable read/inspection policy
- avoid broad generic token frameworks; prefer narrow seams per repeated shape

Validation:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- standalone unit coverage if a shared seam is also used there

Completion criteria:

- the touched token-reading logic exists in named helpers rather than at many
  call sites
- Calc-local callers are thinner and more obviously host-owned

### 3. Collapse Range, Union, And Value-Shape Adaptation

Converge repeated conversion patterns that adapt Calc token/container state
into value/reference/range shapes needed by shared engine semantics.

Target categories:

- range and union adaptation that feeds extracted helpers
- single-ref vs multi-ref discrimination adjacent to token walking
- token-derived value-shape packaging that currently repeats across
  interpreter call sites

Implementation rules:

- keep actual container ownership and push/pop mechanics in Calc
- move only the planning/translation layer behind named compat seams
- do not introduce a second semantic implementation of the touched behavior

Validation:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `spreadsheetengine_reference_tests` if touched
- zero-fallback replay summary

Completion criteria:

- repeated adaptation logic exists in one named seam rather than several
  interpreter-local copies
- remaining Calc code in the touched scope is about ownership and final push
  behavior, not translation policy

### 4. Normalize Token-Adjacent Vocabulary

Reduce direct exposure of Calc-local token/container vocabulary in helper
surfaces that are really shared semantic contracts.

Target categories:

- helper signatures that unnecessarily expose `ScTokenArray` or Calc-only token
  vocabulary
- value/reference wrapper types that can use engine-owned API types or clearer
  compat abstractions
- comments and helper names that still frame shared behavior in Calc-internal
  terms

Implementation rules:

- use engine-owned or compat-local vocabulary when semantics are shared
- keep Calc-native types at the boundary only where host ownership is real
- do not widen public API only to mirror Calc internals

Validation:

- focused Calc coverage for touched call sites
- standalone coverage if shared helper signatures change
- `git diff --check`

Completion criteria:

- touched helper surfaces read in engine/compat vocabulary first
- unnecessary Calc-only vocabulary is pushed outward to the adapter edge

### 5. Isolate What Stays Host-Only

For anything still retained in Calc after the first four workstreams, make the
host-only reason explicit and local.

Target categories:

- token mutation and ownership paths
- formula-tree cursor advancement that is still tightly bound to Calc storage
- external-reference cache/session-backed token services
- other token/container behaviors that are valid defer or retain cases

Implementation rules:

- use names/comments/helpers that make host-only ownership obvious
- avoid leaving ambiguous utility code behind after compat seams are proven
- update docs whenever a retain/defer boundary changes

Validation:

- focused Calc Cppunit coverage for touched host-only seams
- doc consistency pass
- `git diff --check`

Completion criteria:

- retained Calc-local logic in the touched scope reads as host-only
- no touched helper remains ambiguous about whether it is semantic or host
  infrastructure

### 6. Lock The Boundary And Close The Stream

Turn the reduced token boundary into a stable standing contract.

Required guardrails:

- one-shot `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- focused Calc lanes for every touched seam
- standalone coverage for shared seam changes
- doc/status updates that describe the reduced boundary in present tense
- clean diff hygiene

Completion criteria:

- validation is documented and repeatable
- the status docs describe the remaining token/container surface as an
  intentional host boundary
- any further widening work is clearly a new frontier rather than unfinished
  token-boundary cleanup

## Phased Implementation Approach

### Phase 1. Freeze And Publish The Token Inventory

Status: complete

Create the explicit inventory table for the remaining in-scope token/container
boundary.

Initial inventory targets:

- direct `ScTokenArray` inspection patterns that still feed shared semantic
  helpers
- repeated operand-shape checks adjacent to extracted execution helpers
- range/union adaptation code that still lives inline in interpreter callers
- token-derived value/reference packaging that is duplicated across touched
  call sites
- Calc-owned cursor/mutation paths that should be retained and named as such

Closeout standard:

- the inventory is concrete, classified, and linked from the status docs
- every later phase has a bounded landing surface

Frozen inventory:

| Candidate surface | Current file area | Boundary type | Planned outcome | Validation lanes | Owning later phase |
| --- | --- | --- | --- | --- | --- |
| Reference-like token kind checks for `INTERSECT` / `UNION` | `sc/source/core/tool/interpr2.cxx` (`ScIntersect`, `ScUnionFunc`) | non-owning token traversal | move repeated `svSingleRef` / `svDoubleRef` / `svRefList` inspection behind named compat helpers | `CppunitTest_sc_ucalc_formula2`, `CppunitTest_sc_ucalc_shared_cases` | Phase 2 |
| Ref-list coercion and append policy for `INTERSECT` / `UNION` | `sc/source/core/tool/interpr2.cxx` | range/value-shape adaptation | move token-to-ref-list and ref-list append policy behind compat seams while keeping stack ownership local | `CppunitTest_sc_ucalc_formula2`, `CppunitTest_sc_ucalc_shared_cases`, replay zero-fallback | Phase 3 |
| Ref-list area counting and legacy multi-area promotion | `sc/source/core/tool/interpr1.cxx` (`ScAreas`, `ScMultiArea`) | token-adjacent adaptation | align caller vocabulary and route through the same reference compat vocabulary used by the adopted seam | `CppunitTest_sc_ucalc_formula2`, replay zero-fallback | Phase 4 |
| `PopDoubleRef(ScRange&, short&, size_t&)` ref-list cursor path | `sc/source/core/tool/interpr4.cxx` | token/container adaptation | keep stack mutation local but clarify/refactor the ref-list cursor path only if needed by adopted seams | `CppunitTest_sc_ucalc_formula2`, `CppunitTest_sc_ucalc_shared_cases` | retain for now / revisit after Phase 3 |
| `PopExternalDoubleRef`, `GetExternalDoubleRef`, external token-array plumbing | `sc/source/core/tool/interpr4.cxx`, external-ref cache plumbing | host service plus token-container ownership | keep host-only and make boundary explicit; not in scope for shared-semantic extraction here | focused Calc coverage only | Phase 5 |
| `pArr`, `aCode`, `pStack`, `sp`, and token cursor/stack ownership | `sc/source/core/inc/interpre.hxx` and interpreter core | Calc ownership and mutation | retain in Calc explicitly; not in scope for this stream | Calc formula Cppunit plus replay baseline | retain/defer |

### Phase 2. Land The First Non-Owning Token Traversal Seam

Status: complete

Start with the smallest high-value repeated token-reading pattern.

Target shape:

- one narrow traversal or inspection seam
- no ownership transfer
- clear reduction in duplicated interpreter-local read logic

Likely file targets:

- [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- [interpr2.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr2.cxx)
- compat headers under
  [spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice)

Closeout standard:

- the touched token-reading logic now exists in one named seam
- Calc callers are thinner and more obviously about cursor ownership

Closeout result:

- reference-operand token-kind checks for `INTERSECT` and `UNION` now route
  through named helpers in
  `compat/libreoffice/ReferenceExecution.hxx`
- the interpreter callers in `interpr2.cxx` no longer hard-code the repeated
  `svSingleRef` / `svDoubleRef` / `svRefList` classification inline at each
  entry point

### Phase 3. Converge The First Range/Value Adaptation Cluster

Move the next repeated range/union or value-shape translation pattern behind a
named compat seam.

Target shape:

- one bounded adaptation cluster, not a broad token rewrite
- no change to stack push/pop ownership
- focused Calc and standalone regression coverage on the adopted seam

Likely file targets:

- [interpr4.cxx](/home/ubuntu/repos/libreoffice/sc/source/core/tool/interpr4.cxx)
- [interpre.hxx](/home/ubuntu/repos/libreoffice/sc/source/core/inc/interpre.hxx)
- [spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice)

Closeout standard:

- the touched translation cluster is no longer duplicated across interpreter
  call sites
- the remaining Calc-local code is clearly about ownership/finalization only

### Phase 4. Normalize The Touched Helper Vocabulary

Once the first seams are in place, clean up the helper signatures and naming so
the adopted surfaces read as engine/compat contracts rather than Calc-local
internals.

Target shape:

- no behavior change beyond boundary clarity
- narrower helper signatures
- clearer separation between semantic and host-only vocabulary

Closeout standard:

- touched helper signatures no longer expose unnecessary Calc-only token
  vocabulary
- code review of the touched surfaces no longer requires deep Calc-internal
  context to understand the seam

### Phase 5. Mark Retain/Defer Host Paths Explicitly

Rename, isolate, and document the token/container paths that remain
intentionally Calc-owned after the adopted seams land.

Target shape:

- explicit host-only naming
- comments or doc references for retained/deferred clusters
- no ambiguous leftover utility code in the touched scope

Closeout standard:

- retained token/container helpers in the touched areas are obviously host-only
- the code and docs agree on why they remain in Calc

### Phase 6. Re-Run The Full Baseline And Close The Stream

Re-run the standing validation contract, update the status and architecture
docs in present tense, and close the stream.

Closeout standard:

- replay baseline remains at zero fallback
- docs describe the remaining token/container boundary as stable
- the next frontier is clearly defined

## Validation Contract

Every implementation slice in this stream should validate as applicable with:

- `CppunitTest_sc_ucalc_formula2`
- `CppunitTest_sc_ucalc_shared_cases`
- `CppunitTest_sc_ucalc_dependency_shadow`
- `CppunitTest_sc_ucalc_workbook_facade`
- standalone unit lanes for touched compat/helper surfaces
- `spreadsheetengine_fods_evaluator_tests`
- `spreadsheetengine_fods_replay_tests --summary --assert-zero-fallback`
- `git diff --check`

## Exit Criteria

This stream can be closed when:

- the remaining in-scope token/container surfaces are inventoried and
  classified
- the touched non-owning token traversal and adaptation layers route through
  narrower named compat seams
- the touched Calc code is plainly about ownership, mutation, or host services
- no known duplicate token-adjacent helper remains in the touched scope
- the zero-fallback promoted replay baseline is still intact

At that point, the project can reassess the next frontier from a cleaner,
smaller token boundary instead of carrying forward implicit interpreter
coupling as technical debt.
