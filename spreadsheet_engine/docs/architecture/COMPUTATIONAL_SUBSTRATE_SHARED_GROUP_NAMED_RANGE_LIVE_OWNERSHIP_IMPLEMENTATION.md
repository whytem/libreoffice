# Computational Substrate Shared-Group Named-Range Live Ownership Implementation

Status: completed implementation note for bounded named-range-combined live ownership

## Runtime Changes

This closeout landed two runtime changes.

First, [DependencySnapshot.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/dependency/DependencySnapshot.hxx)
now retries bounded named-range target expressions as raw cell or range
references when the general formula parser returns `parse_failure`. That
removes the previous opaque dependency surface on direct targets such as
`$Data.$A$1:$A$2`.

Second, [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)
now projects shared-group named-range dependencies with mixed listener-anchor
ownership:

- named-range-origin dependencies use the shared `FormulaGroup` anchor
- member-local direct references keep the per-cell `FormulaCell` anchor

That lets predicted computational broadcasters and graph edges match Calc’s
live shared-group wiring shape on the bounded preserve family.

## Result

The bounded same-sheet global-single-area named-range-combined
`SameTextPreserve` family now applies through both:

- [ComputationalSubstrateLifecycle.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateLifecycle.hxx)
- [ComputationalSubstrateMutationEntry.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/compat/libreoffice/ComputationalSubstrateMutationEntry.hxx)

The broader named-range-combined frontier remains deferred.
