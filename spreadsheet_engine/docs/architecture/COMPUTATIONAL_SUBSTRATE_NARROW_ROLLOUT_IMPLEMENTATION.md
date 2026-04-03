# Computational Substrate Narrow Rollout Implementation

Status: active implementation note for the narrow rollout plan

## Purpose

This note records the implementation shape of the first opt-in rollout step
for the admitted computational substrate authority slice.

## Runtime Gates

The rollout uses the following environment gates:

- `SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT`
  - umbrella gate for the admitted narrow rollout slice
- `SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY`
  - per-surface override for the earlier authority pilot surface
- `SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE`
  - per-surface override for the admitted scalar lifecycle surface
- `SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL`
  - per-surface override for the admitted structural slice

## Gate Resolution Rule

The gate resolution rule is:

- if a per-surface variable is explicitly set, it controls that surface
- otherwise the umbrella narrow-rollout gate controls the admitted surface

This keeps the rollout coherent by default while still allowing targeted
narrowing and debugging.

## Enabled Surface

When `SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT=1` and no per-surface
override disables a component, the rollout enables:

- admitted authority capture
- admitted scalar lifecycle capture
- admitted structural capture

The admitted verification and rollback rules do not change:

- queue comparison remains exact
- computational comparison remains exact
- graph comparison remains exact
- structural repair-detected divergence still rolls back

## Validation Shape

The first rollout implementation is validated by:

- disabled-by-default capture checks
- umbrella-enabled lifecycle and structural application checks
- per-surface override checks proving the rollout can narrow again without
  disabling the whole feature set
