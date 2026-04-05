# Computational Substrate Scalar Mutation Entry Canonicalization Matrix

Status: frozen scalar-entry broadcaster canonicalization matrix

## Purpose

This note defines the representative scalar-entry broadcaster scenarios and
the equivalence rules used by
[COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_CONVERGENCE_PLAN.md).

It exists to distinguish broadcaster-shape noise from real admitted-slice
divergence without relaxing the closeout requirement for exact computational
equivalence.

## Covered Scenario Classes

The scalar-entry convergence cycle must cover at least these broadcaster
shapes:

- direct scalar-to-formula listener
  Example: `A1` feeding `B1`
- transitive scalar invalidation chain
  Example: `A1` feeding `B1`, then `B1` feeding `C1`
- multi-fanout scalar broadcaster
  Example: `A1` feeding multiple ordinary scalar formulas
- scalar overwrite with broadcaster reuse
  Example: a stable scalar address keeps dependents after a value change
- scalar overwrite with broadcaster cleanup
  Example: delayed empty-broadcaster states must not survive closeout

The cycle remains out of contract for:

- shared-group listeners
- named-range-sensitive consumers
- area-broadcaster-only structural cases
- sheet-local or scope-ambiguous name consumers
- off-sheet structural side effects

## Canonicalization Rules

The closeout expectation for admitted live broadcaster state is:

- broadcaster nodes are ordered by stable cell or area address
- listeners within each broadcaster are ordered by listener kind, then
  anchor, then group length
- duplicate listeners are not allowed
- empty broadcasters are not allowed at closeout
- host-unknown listeners are not allowed on the admitted scalar slice
- listener anchors must match the admitted engine-owned listener-anchor form

The closeout comparison remains exact. These rules define the exact shape
being compared.

## Diagnostic Classification Surface

Observation and classification may use the following temporary mismatch
labels during implementation:

- `ordering_only`
- `duplicate_listener`
- `empty_broadcaster`
- `listener_anchor_canonicalization`
- `unexpected_host_listener`
- `true_dependency_divergence`

These labels are only for diagnosis. They do not create a relaxed success
path.

## Allowed Diagnostic Normalization

The implementation may temporarily normalize for diagnosis when it is trying
to determine which mismatch class is present:

- stable sort of broadcasters
- stable sort of listeners
- duplicate counting
- explicit empty-broadcaster detection

That diagnostic normalization is allowed only to explain a mismatch. It is
not allowed to convert a failing closeout into a passing one.

## Forbidden Shortcuts

The following are explicitly forbidden in this cycle:

- treating a broadcaster mismatch as acceptable because graph comparison is
  already exact
- silently dropping empty broadcasters only in the comparison layer
- silently deduplicating listeners only in the comparison layer
- accepting `HostUnknown` listeners as equivalent to admitted formula-cell
  listeners
- widening the scenario set to hide the direct scalar-entry mismatch inside a
  broader aggregate metric

If the live state still needs any of those shortcuts, the cycle has not
converged.

## Closeout Standard

The matrix is satisfied only when every covered scalar-entry scenario closes
with:

- exact queue verification
- exact graph verification
- exact computational verification
- broadcaster state matching this note without diagnostic-only normalization

If any covered scenario still requires diagnostic-only normalization to look
equivalent, the scalar-entry path remains validation-only or is deferred
again.
