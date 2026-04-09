# Computational Substrate Scalar Mutation Entry Convergence Implementation

Status: completed scalar-entry convergence implementation note

## Purpose

This note records the narrow implementation change that closed the admitted
scalar `SetScalarValue` computational mismatch identified by
[COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_OBSERVATION.md](/home/ubuntu/repos/libreoffice/spreadsheet_engine/docs/architecture/COMPUTATIONAL_SUBSTRATE_SCALAR_MUTATION_ENTRY_OBSERVATION.md).

That observation workstream showed that the scalar mutation-entry gap was not
caused by duplicate or empty live broadcasters. The engine-owned scalar
authority after-state was simply not carrying any expected broadcaster
records into the computational shadow.

## What Changed

The authority path now builds a full dependency-derived observation state in
[AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
before it materializes the computational after-state.

That observation state now includes:

- formula-tree order from the recalc queue
- cell broadcasters resolved from the dependency snapshot
- area broadcasters resolved from the dependency snapshot

The scalar authority computational after-state is therefore now built from
the same dependency-derived broadcaster surface that the graph after-state was
already proving.

## Why This Fix Was Chosen

The observation workstream narrowed the scalar mismatch to:

- expected broadcasters in computational after-state: `0`
- live broadcasters after realization: non-zero
- graph after-state: already exact

That meant the narrowest correct fix was not live realization reordering. It
was aligning the predicted computational after-state with the already-exact
authority graph construction.

This keeps the change bounded:

- no widening of the admitted mutation-entry surface
- no rollback changes
- no new host-only normalization in the comparison layer
- no weakening of exact computational closeout rules

## Shared Builder Consolidation

The same dependency-derived observation builder is now reused by:

- the authority pilot
- the lifecycle pilot
- the structural pilot

That removes an unnecessary split where authority prediction had less
broadcaster information than the later lifecycle and structural builders.

## Result

After this change, the admitted scalar mutation-entry proof lane no longer
closes with `missing_expected_broadcasters`.

It now closes with:

- exact queue verification
- exact computational verification
- exact graph verification
- exact broadcaster canonicalization

The scalar-entry path is therefore ready for the evidence and decision
workstreams to reassess whether it can leave validation-only status.
