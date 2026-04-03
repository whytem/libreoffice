# Computational Substrate Phase 0 Observable State Model

Status: active Phase 0 comparison schema artifact

## Purpose

This document is the checked-in observable-state model required by workstream
`0.2` of
[COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_PHASE0_PLAN.md).

It defines the comparison shapes and verdict rules that later Phase 0 capture
helpers and tests must use when they compare Calc's live computational
substrate to engine-side planning state or to future shadow state.

The core rule is simple:

- capture the smallest stable shape that still reflects real computational
  ownership
- prefer exact comparisons where the live state is ordered and deterministic
- allow normalized-equivalent matches only where Calc's concrete storage shape
  is intentionally more detailed than the comparison target

## Comparison Verdicts

Every Phase 0 comparison must classify its result as one of these:

### Exact Match

The captured state matches exactly after applying the comparison surface's
declared ordering and serialization rules.

This is the default required outcome for:

- formula-tree membership and order
- formula-track membership and order
- cell broadcaster membership
- area broadcaster membership
- listener registration shape

### Normalized-Equivalent Match

The captured state differs in concrete representation but is semantically
equivalent after applying the surface's declared normalization rules.

This is allowed only when the normalization rule is explicit in this document.
Examples include:

- deduplicating repeated address captures from fallback scans
- comparing area-listener ranges by canonical address order rather than raw
  slot iteration order
- comparing engine recalc plans to live Calc state after filtering predicted
  queue members to actual live queue membership, matching the existing
  recalc-shadow policy

### Unacceptable Divergence

The captured state differs in a way that changes computational meaning or
blocks trustworthy later shadowing.

Examples:

- missing live formula-tree members
- missing listeners or broadcasters
- under-invalidation or under-scheduling
- order mismatch for ordered queue surfaces
- mismatched group anchor or group length where group semantics are part of the
  surface

## Comparison Surfaces

### Formula Tree Snapshot

#### Shape

The capture shape for the formula tree is:

- ordered list of formula cell addresses

#### Exact Match Rule

The two ordered address lists must match exactly.

#### Normalization Rule

A fallback scan may first:

- filter to formula cells currently reported as in the formula tree
- sort and deduplicate addresses only if a linked-list head cannot be
  recovered

That fallback normalization is acceptable only for observational capture. Once
an ordered head is known, comparisons must use the linked-list order.

#### Unacceptable Divergence

- any missing address
- any extra address when the surface claims full queue ownership
- first-order mismatch at any index

### Formula Track Snapshot

#### Shape

The capture shape for the formula track is:

- ordered list of formula cell addresses

#### Exact Match Rule

The two ordered address lists must match exactly.

#### Normalization Rule

The same fallback normalization rule as the formula tree may be used only when
the linked-list head is not directly available to the capture helper.

#### Unacceptable Divergence

- any missing address
- any extra address when the surface claims full track ownership
- first-order mismatch at any index

### Broadcaster State Snapshot

#### Shape

The capture shape for broadcaster state is:

- sorted map from cell address to sorted listener entries
- sorted map from range to sorted listener entries

Listener entries are normalized to a stable public shape:

- formula cell listener by anchor address
- formula group listener by top-cell anchor and length
- unknown host listener by type label only

#### Exact Match Rule

The normalized stores must match exactly by key and by sorted listener entry
list.

#### Normalization Rule

The following normalizations are allowed:

- sort broadcaster keys by canonical address/range order
- sort listeners within each broadcaster by normalized listener identity
- collapse raw pointer identity into semantic listener labels

#### Unacceptable Divergence

- missing broadcaster entries
- missing formula-cell listeners
- area versus cell broadcaster misclassification
- mismatched formula-group anchor or length

### Listener Registration Shape

#### Shape

The capture shape for listener registration is derived from broadcaster state:

- set of `(listener, broadcaster)` relations for single-cell broadcasters
- set of `(listener, broadcaster-range)` relations for area broadcasters

#### Exact Match Rule

The two relation sets must match exactly after stable sorting.

#### Normalization Rule

Unknown host listeners may be compared by stable label rather than raw pointer,
but formula-cell listeners must compare by exact address.

#### Unacceptable Divergence

- any missing relation for a formula-cell listener
- any extra relation that changes the live dependency graph

### Recalc Queue Snapshot

#### Shape

The capture shape for queue comparison is:

- ordered list of queue addresses
- ordered list of shared-group anchors and lengths where group semantics apply

#### Exact Match Rule

The queue order and group list must match exactly.

#### Normalization Rule

The existing recalc-shadow policy is the acceptable normalized-equivalent rule:

- predicted queue may be filtered to members that actually exist in the live
  queue before order comparison
- predicted group list may be filtered to anchors that actually exist in the
  live queue before group comparison
- extra predicted members are tolerated only as a conservative superset, not
  as an exact match

#### Unacceptable Divergence

- under-scheduling
- order mismatch
- group mismatch

### Engine Dependency Snapshot Correspondence

#### Shape

The capture shape for this surface is:

- predicted dirty formula set from an engine invalidation plan
- actual dirty/recalc-needed formula set from the Calc-backed workbook facade

#### Exact Match Rule

The normalized sets must match exactly.

#### Normalization Rule

The existing dependency-shadow policy is allowed:

- sort and deduplicate addresses
- treat extra predicted invalidation as conservative widening

#### Unacceptable Divergence

- any missing predicted dirty formula relative to actual live dirty state
- any comparison result currently classified as `UnderInvalidation`

## Serialization Rules

To make captures stable across tests and future shadow phases:

- addresses serialize in sheet, column, row order
- ranges serialize by start address then end address
- listener entries serialize by kind, then anchor identity
- unknown host listeners must never serialize raw pointer values into
  golden-comparison output
- group listeners serialize as `(anchor, length)`

## Practical Mapping To Existing Shadow Lanes

This model intentionally aligns with the comparison semantics already present
in:

- [DependencyShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/DependencyShadow.hxx)
- [RecalcShadow.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/RecalcShadow.hxx)

That means:

- `Exact` remains the preferred comparison kind
- `ConservativeSuperset` remains acceptable only for specific predicted-plan
  comparisons
- `UnderInvalidation`, `UnderScheduling`, `OrderMismatch`, and `GroupMismatch`
  all map to unacceptable divergence for Phase 0 purposes

## Exit Implication

Phase 0 work should treat this document as the comparison contract.

If a later capture helper or test needs a new normalization rule, that rule
must be added here first. It should not be introduced silently inside a test or
helper implementation.
