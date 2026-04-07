# Computational Substrate Shared-Group Non-Structural Multi-Group Collapse Implementation

Status: complete implementation note for the exact multi-group collapse cycle

## What Landed

The cycle landed bounded groundwork, not a new admitted family.

The implementation now includes:

- facade-side explicit `MultiGroupCollapse` classification for synthetic
  three-participant full-span collapse
- engine-authored participant discovery for one above group, one touched
  middle group, and one below group
- engine-authored full-span rebuild window for that bounded synthetic class
- exact synthetic after-group matching against the full three-group span

The core runtime work is in:

- [FacadeConsumers.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/workbook/FacadeConsumers.hxx)
- [AuthorityPilotBuilder.hxx](/home/ubuntu/repos/libreoffice/spreadsheet_engine/inc/spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx)

## What Did Not Land

This cycle did not widen the admitted slice.

The reason is that live Calc did not produce the exact full-span one-group
after-topology required by the synthetic model. The bounded live attempt
still kept the far group separate.

## Resulting Boundary

After this cycle:

- exact standalone three-participant collapse is modeled
- live admitted shared-group families remain unchanged
- multi-group collapse remains explicitly deferred
