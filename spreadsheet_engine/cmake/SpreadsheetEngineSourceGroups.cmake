# Source-group manifests for the standalone build track.
#
# Public headers, core sources, and test source lists now live in smaller
# fragments under cmake/sources/ so the standalone package is easier to evolve
# without editing one monolithic manifest.

include("${SPREADSHEETENGINE_ROOT}/cmake/sources/PublicHeaders.cmake")
include("${SPREADSHEETENGINE_ROOT}/cmake/sources/CoreSources.cmake")
include("${SPREADSHEETENGINE_ROOT}/cmake/sources/TestSources.cmake")
