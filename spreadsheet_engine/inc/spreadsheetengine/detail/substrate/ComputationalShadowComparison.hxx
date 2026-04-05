/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <vector>

#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>

namespace spreadsheetengine::detail::substrate
{

struct ComputationalShadowComparison
{
    bool mbCellPopulationMatch = false;
    bool mbFormulaTreeMatch = false;
    bool mbFormulaTrackMatch = false;
    bool mbBroadcasterMatch = false;
    bool mbGroupMatch = false;
    bool mbNamedRangeMatch = false;
    bool mbFullMatch = false;

    [[nodiscard]] constexpr bool operator==(const ComputationalShadowComparison& rOther) const
        = default;
};

enum class BroadcasterCanonicalizationKind : std::uint8_t
{
    Exact,
    OrderingOnly,
    DuplicateMaterializationOnly,
    EmptyBroadcastersOnly,
    DuplicateAndEmptyBroadcasters,
    MissingExpectedBroadcasters,
    ListenerAnchorCanonicalizationOnly,
    UnexpectedHostListeners,
    Mixed,
    Unknown
};

struct BroadcasterCanonicalizationComparison
{
    BroadcasterCanonicalizationKind meKind = BroadcasterCanonicalizationKind::Unknown;
    sal_Int32 mnExpectedCellBroadcasters = 0;
    sal_Int32 mnExpectedAreaBroadcasters = 0;
    sal_Int32 mnLiveCellBroadcasters = 0;
    sal_Int32 mnLiveAreaBroadcasters = 0;
    sal_Int32 mnLiveDuplicateBroadcasterCount = 0;
    sal_Int32 mnLiveDuplicateListenerCount = 0;
    sal_Int32 mnLiveEmptyBroadcasterCount = 0;
    sal_Int32 mnLiveHostUnknownListenerCount = 0;
    bool mbOrderingEquivalent = false;
    bool mbDeduplicatedEquivalent = false;
    bool mbDropEmptyEquivalent = false;
    bool mbIgnoreListenerKindEquivalent = false;
    bool mbDiagnosticCanonicalEquivalent = false;
    bool mbExactMatch = false;

    [[nodiscard]] constexpr bool operator==(const BroadcasterCanonicalizationComparison& rOther) const
        = default;
};

namespace detail
{

struct AddressLess
{
    [[nodiscard]] bool operator()(const api::CellAddress& rLeft, const api::CellAddress& rRight) const
    {
        if (rLeft.mnSheet != rRight.mnSheet)
            return rLeft.mnSheet < rRight.mnSheet;
        if (rLeft.mnColumn != rRight.mnColumn)
            return rLeft.mnColumn < rRight.mnColumn;
        return rLeft.mnRow < rRight.mnRow;
    }
};

struct RangeLess
{
    [[nodiscard]] bool operator()(const api::CellRange& rLeft, const api::CellRange& rRight) const
    {
        if (!(rLeft.maStart == rRight.maStart))
            return AddressLess {}(rLeft.maStart, rRight.maStart);
        return AddressLess {}(rLeft.maEnd, rRight.maEnd);
    }
};

struct ListenerAnchorLess
{
    [[nodiscard]] bool operator()(const ListenerAnchorId& rLeft, const ListenerAnchorId& rRight) const
    {
        if (rLeft.meKind != rRight.meKind)
            return rLeft.meKind < rRight.meKind;
        if (!(rLeft.maAnchor == rRight.maAnchor))
            return AddressLess {}(rLeft.maAnchor, rRight.maAnchor);
        return rLeft.mnLength < rRight.mnLength;
    }
};

struct ListenerAnchorIgnoringKindLess
{
    [[nodiscard]] bool operator()(const ListenerAnchorId& rLeft, const ListenerAnchorId& rRight) const
    {
        if (!(rLeft.maAnchor == rRight.maAnchor))
            return AddressLess {}(rLeft.maAnchor, rRight.maAnchor);
        return rLeft.mnLength < rRight.mnLength;
    }
};

[[nodiscard]] inline std::vector<api::CellAddress> collectFacadeCellAddresses(
    const facade::WorkbookFacade& rFacade)
{
    std::vector<api::CellAddress> aAddresses;
    rFacade.visitAllCells([&aAddresses](const facade::CellDescriptor& rCell) {
        aAddresses.push_back(rCell.maAddress);
        return true;
    });
    std::sort(aAddresses.begin(), aAddresses.end(), AddressLess {});
    return aAddresses;
}

[[nodiscard]] inline std::vector<api::CellAddress> collectShadowCellAddresses(
    const ComputationalWorkbookShadow& rShadow)
{
    std::vector<api::CellAddress> aAddresses;
    for (const auto& rSheet : rShadow.maSheets)
    {
        for (const auto& rCell : rSheet.maCells)
            aAddresses.push_back(rCell.maId.maAddress);
    }
    std::sort(aAddresses.begin(), aAddresses.end(), AddressLess {});
    return aAddresses;
}

[[nodiscard]] inline std::vector<facade::FormulaGroupDescriptor> collectFacadeFormulaGroups(
    const facade::WorkbookFacade& rFacade)
{
    std::vector<facade::FormulaGroupDescriptor> aGroups;
    rFacade.visitAllFormulaCells([&](const facade::FormulaCellDescriptor& rFormula) {
        const auto oGroup = rFacade.getFormulaGroupDescriptor(rFormula.maId.maAddress);
        if (oGroup && std::find(aGroups.begin(), aGroups.end(), *oGroup) == aGroups.end())
            aGroups.push_back(*oGroup);
        return true;
    });
    std::sort(aGroups.begin(), aGroups.end(), [](const auto& rLeft, const auto& rRight) {
        if (rLeft.maAnchor != rRight.maAnchor)
            return AddressLess {}(rLeft.maAnchor, rRight.maAnchor);
        return rLeft.mnLength < rRight.mnLength;
    });
    return aGroups;
}

[[nodiscard]] inline std::vector<facade::FormulaGroupDescriptor> collectShadowFormulaGroups(
    const ComputationalWorkbookShadow& rShadow)
{
    std::vector<facade::FormulaGroupDescriptor> aGroups;
    aGroups.reserve(rShadow.maFormulaGroups.size());
    for (const auto& rGroup : rShadow.maFormulaGroups)
        aGroups.push_back(rGroup.maDescriptor);
    std::sort(aGroups.begin(), aGroups.end(), [](const auto& rLeft, const auto& rRight) {
        if (rLeft.maAnchor != rRight.maAnchor)
            return AddressLess {}(rLeft.maAnchor, rRight.maAnchor);
        return rLeft.mnLength < rRight.mnLength;
    });
    return aGroups;
}

[[nodiscard]] inline std::vector<facade::NamedRangeDescriptor> sortNamedRanges(
    std::vector<facade::NamedRangeDescriptor> aRanges)
{
    std::sort(aRanges.begin(), aRanges.end(), [](const auto& rLeft, const auto& rRight) {
        if (rLeft.maId.mnIndex != rRight.maId.mnIndex)
            return rLeft.maId.mnIndex < rRight.maId.mnIndex;
        return rLeft.maName < rRight.maName;
    });
    return aRanges;
}

template <typename BroadcasterRecord, typename BroadcasterLess>
void sortAndCanonicalizeBroadcasters(std::vector<BroadcasterRecord>& rBroadcasters,
    BroadcasterLess aBroadcasterLess, bool bDeduplicate, bool bDropEmpty)
{
    std::sort(rBroadcasters.begin(), rBroadcasters.end(),
        [&aBroadcasterLess](const BroadcasterRecord& rLeft, const BroadcasterRecord& rRight) {
            return aBroadcasterLess(rLeft.maBroadcaster, rRight.maBroadcaster);
        });

    std::vector<BroadcasterRecord> aMerged;
    aMerged.reserve(rBroadcasters.size());
    for (auto& rEntry : rBroadcasters)
    {
        std::sort(rEntry.maListeners.begin(), rEntry.maListeners.end(), ListenerAnchorLess {});
        if (bDeduplicate)
        {
            rEntry.maListeners.erase(
                std::unique(rEntry.maListeners.begin(), rEntry.maListeners.end()),
                rEntry.maListeners.end());
        }

        if (bDropEmpty && rEntry.maListeners.empty())
            continue;

        if (!aMerged.empty()
            && !aBroadcasterLess(aMerged.back().maBroadcaster, rEntry.maBroadcaster)
            && !aBroadcasterLess(rEntry.maBroadcaster, aMerged.back().maBroadcaster))
        {
            aMerged.back().maListeners.insert(aMerged.back().maListeners.end(), rEntry.maListeners.begin(),
                rEntry.maListeners.end());
            std::sort(aMerged.back().maListeners.begin(), aMerged.back().maListeners.end(),
                ListenerAnchorLess {});
            if (bDeduplicate)
            {
                aMerged.back().maListeners.erase(
                    std::unique(aMerged.back().maListeners.begin(), aMerged.back().maListeners.end()),
                    aMerged.back().maListeners.end());
            }
            continue;
        }

        aMerged.push_back(std::move(rEntry));
    }

    rBroadcasters = std::move(aMerged);
}

[[nodiscard]] inline sal_Int32 countDuplicateListeners(
    const std::vector<ListenerAnchorId>& rListeners)
{
    if (rListeners.empty())
        return 0;

    std::vector<ListenerAnchorId> aSorted = rListeners;
    std::sort(aSorted.begin(), aSorted.end(), ListenerAnchorLess {});
    const auto aUniqueEnd = std::unique(aSorted.begin(), aSorted.end());
    return static_cast<sal_Int32>(std::distance(aUniqueEnd, aSorted.end()));
}

template <typename BroadcasterRecord, typename BroadcasterLess>
[[nodiscard]] inline sal_Int32 countDuplicateBroadcasters(
    const std::vector<BroadcasterRecord>& rBroadcasters, BroadcasterLess aBroadcasterLess)
{
    sal_Int32 nDuplicates = 0;
    if (rBroadcasters.empty())
        return nDuplicates;

    std::vector<BroadcasterRecord> aSorted = rBroadcasters;
    std::sort(aSorted.begin(), aSorted.end(),
        [&aBroadcasterLess](const BroadcasterRecord& rLeft, const BroadcasterRecord& rRight) {
            return aBroadcasterLess(rLeft.maBroadcaster, rRight.maBroadcaster);
        });

    for (std::size_t nIndex = 1; nIndex < aSorted.size(); ++nIndex)
    {
        const auto& rPrevious = aSorted[nIndex - 1];
        const auto& rCurrent = aSorted[nIndex];
        if (!aBroadcasterLess(rPrevious.maBroadcaster, rCurrent.maBroadcaster)
            && !aBroadcasterLess(rCurrent.maBroadcaster, rPrevious.maBroadcaster))
        {
            ++nDuplicates;
        }
    }

    return nDuplicates;
}

template <typename BroadcasterRecord>
[[nodiscard]] inline sal_Int32 countEmptyBroadcasters(
    const std::vector<BroadcasterRecord>& rBroadcasters)
{
    return static_cast<sal_Int32>(std::count_if(rBroadcasters.begin(), rBroadcasters.end(),
        [](const BroadcasterRecord& rEntry) { return rEntry.maListeners.empty(); }));
}

template <typename BroadcasterRecord>
[[nodiscard]] inline sal_Int32 countHostUnknownListeners(
    const std::vector<BroadcasterRecord>& rBroadcasters)
{
    sal_Int32 nCount = 0;
    for (const auto& rEntry : rBroadcasters)
    {
        nCount += static_cast<sal_Int32>(std::count_if(rEntry.maListeners.begin(), rEntry.maListeners.end(),
            [](const ListenerAnchorId& rListener) {
                return rListener.meKind == ListenerAnchorKind::HostUnknown;
            }));
    }
    return nCount;
}

template <typename BroadcasterRecord>
[[nodiscard]] inline std::vector<BroadcasterRecord> sortBroadcastersOnly(
    std::vector<BroadcasterRecord> aBroadcasters)
{
    for (auto& rEntry : aBroadcasters)
        std::sort(rEntry.maListeners.begin(), rEntry.maListeners.end(), ListenerAnchorLess {});
    return aBroadcasters;
}

template <typename BroadcasterRecord>
[[nodiscard]] inline std::vector<BroadcasterRecord> canonicalizeBroadcastersForDiagnostics(
    std::vector<BroadcasterRecord> aBroadcasters, bool bDeduplicate, bool bDropEmpty)
{
    if constexpr (std::is_same_v<BroadcasterRecord, CellBroadcasterRecord>)
        sortAndCanonicalizeBroadcasters(aBroadcasters, AddressLess {}, bDeduplicate, bDropEmpty);
    else
        sortAndCanonicalizeBroadcasters(aBroadcasters, RangeLess {}, bDeduplicate, bDropEmpty);
    return aBroadcasters;
}

template <typename BroadcasterRecord>
[[nodiscard]] inline std::vector<BroadcasterRecord> normalizeListenersIgnoringKind(
    std::vector<BroadcasterRecord> aBroadcasters)
{
    if constexpr (std::is_same_v<BroadcasterRecord, CellBroadcasterRecord>)
        std::sort(aBroadcasters.begin(), aBroadcasters.end(),
            [](const BroadcasterRecord& rLeft, const BroadcasterRecord& rRight) {
                return AddressLess {}(rLeft.maBroadcaster, rRight.maBroadcaster);
            });
    else
        std::sort(aBroadcasters.begin(), aBroadcasters.end(),
            [](const BroadcasterRecord& rLeft, const BroadcasterRecord& rRight) {
                return RangeLess {}(rLeft.maBroadcaster, rRight.maBroadcaster);
            });

    for (auto& rEntry : aBroadcasters)
    {
        std::sort(rEntry.maListeners.begin(), rEntry.maListeners.end(), ListenerAnchorIgnoringKindLess {});
    }
    return aBroadcasters;
}

[[nodiscard]] inline BroadcasterCanonicalizationComparison compareBroadcasterCanonicalization(
    const ComputationalWorkbookShadow& rExpected, const ComputationalObservationState& rActual)
{
    BroadcasterCanonicalizationComparison aComparison;
    aComparison.mnExpectedCellBroadcasters = static_cast<sal_Int32>(rExpected.maCellBroadcasters.size());
    aComparison.mnExpectedAreaBroadcasters = static_cast<sal_Int32>(rExpected.maAreaBroadcasters.size());
    aComparison.mnLiveCellBroadcasters = static_cast<sal_Int32>(rActual.maCellBroadcasters.size());
    aComparison.mnLiveAreaBroadcasters = static_cast<sal_Int32>(rActual.maAreaBroadcasters.size());
    aComparison.mnLiveDuplicateBroadcasterCount
        = countDuplicateBroadcasters(rActual.maCellBroadcasters, AddressLess {})
          + countDuplicateBroadcasters(rActual.maAreaBroadcasters, RangeLess {});
    for (const auto& rBroadcaster : rActual.maCellBroadcasters)
        aComparison.mnLiveDuplicateListenerCount += countDuplicateListeners(rBroadcaster.maListeners);
    for (const auto& rBroadcaster : rActual.maAreaBroadcasters)
        aComparison.mnLiveDuplicateListenerCount += countDuplicateListeners(rBroadcaster.maListeners);
    aComparison.mnLiveEmptyBroadcasterCount = countEmptyBroadcasters(rActual.maCellBroadcasters)
        + countEmptyBroadcasters(rActual.maAreaBroadcasters);
    aComparison.mnLiveHostUnknownListenerCount = countHostUnknownListeners(rActual.maCellBroadcasters)
        + countHostUnknownListeners(rActual.maAreaBroadcasters);
    aComparison.mbExactMatch = rExpected.maCellBroadcasters == rActual.maCellBroadcasters
        && rExpected.maAreaBroadcasters == rActual.maAreaBroadcasters;

    auto aExpectedOrdered = sortBroadcastersOnly(rExpected.maCellBroadcasters);
    auto aActualOrdered = sortBroadcastersOnly(rActual.maCellBroadcasters);
    auto aExpectedOrderedAreas = sortBroadcastersOnly(rExpected.maAreaBroadcasters);
    auto aActualOrderedAreas = sortBroadcastersOnly(rActual.maAreaBroadcasters);
    aComparison.mbOrderingEquivalent = aExpectedOrdered == aActualOrdered
        && aExpectedOrderedAreas == aActualOrderedAreas;

    auto aExpectedDeduplicated
        = canonicalizeBroadcastersForDiagnostics(rExpected.maCellBroadcasters, true, false);
    auto aActualDeduplicated
        = canonicalizeBroadcastersForDiagnostics(rActual.maCellBroadcasters, true, false);
    auto aExpectedDeduplicatedAreas
        = canonicalizeBroadcastersForDiagnostics(rExpected.maAreaBroadcasters, true, false);
    auto aActualDeduplicatedAreas
        = canonicalizeBroadcastersForDiagnostics(rActual.maAreaBroadcasters, true, false);
    aComparison.mbDeduplicatedEquivalent = aExpectedDeduplicated == aActualDeduplicated
        && aExpectedDeduplicatedAreas == aActualDeduplicatedAreas;

    auto aExpectedDropEmpty
        = canonicalizeBroadcastersForDiagnostics(rExpected.maCellBroadcasters, false, true);
    auto aActualDropEmpty
        = canonicalizeBroadcastersForDiagnostics(rActual.maCellBroadcasters, false, true);
    auto aExpectedDropEmptyAreas
        = canonicalizeBroadcastersForDiagnostics(rExpected.maAreaBroadcasters, false, true);
    auto aActualDropEmptyAreas
        = canonicalizeBroadcastersForDiagnostics(rActual.maAreaBroadcasters, false, true);
    aComparison.mbDropEmptyEquivalent = aExpectedDropEmpty == aActualDropEmpty
        && aExpectedDropEmptyAreas == aActualDropEmptyAreas;

    auto aExpectedIgnoringKind = normalizeListenersIgnoringKind(rExpected.maCellBroadcasters);
    auto aActualIgnoringKind = normalizeListenersIgnoringKind(rActual.maCellBroadcasters);
    auto aExpectedIgnoringKindAreas = normalizeListenersIgnoringKind(rExpected.maAreaBroadcasters);
    auto aActualIgnoringKindAreas = normalizeListenersIgnoringKind(rActual.maAreaBroadcasters);
    aComparison.mbIgnoreListenerKindEquivalent = aExpectedIgnoringKind == aActualIgnoringKind
        && aExpectedIgnoringKindAreas == aActualIgnoringKindAreas;

    auto aExpectedDiagnostic
        = canonicalizeBroadcastersForDiagnostics(rExpected.maCellBroadcasters, true, true);
    auto aActualDiagnostic
        = canonicalizeBroadcastersForDiagnostics(rActual.maCellBroadcasters, true, true);
    auto aExpectedDiagnosticAreas
        = canonicalizeBroadcastersForDiagnostics(rExpected.maAreaBroadcasters, true, true);
    auto aActualDiagnosticAreas
        = canonicalizeBroadcastersForDiagnostics(rActual.maAreaBroadcasters, true, true);
    aComparison.mbDiagnosticCanonicalEquivalent = aExpectedDiagnostic == aActualDiagnostic
        && aExpectedDiagnosticAreas == aActualDiagnosticAreas;

    if (aComparison.mbExactMatch)
        aComparison.meKind = BroadcasterCanonicalizationKind::Exact;
    else if (aComparison.mbOrderingEquivalent)
        aComparison.meKind = BroadcasterCanonicalizationKind::OrderingOnly;
    else if (aComparison.mnLiveHostUnknownListenerCount > 0
             && aComparison.mbIgnoreListenerKindEquivalent)
        aComparison.meKind = BroadcasterCanonicalizationKind::UnexpectedHostListeners;
    else if (aComparison.mbIgnoreListenerKindEquivalent)
        aComparison.meKind = BroadcasterCanonicalizationKind::ListenerAnchorCanonicalizationOnly;
    else if (aComparison.mbDeduplicatedEquivalent && aComparison.mbDropEmptyEquivalent
             && (aComparison.mnLiveDuplicateBroadcasterCount > 0
                 || aComparison.mnLiveDuplicateListenerCount > 0)
             && aComparison.mnLiveEmptyBroadcasterCount > 0)
        aComparison.meKind = BroadcasterCanonicalizationKind::DuplicateAndEmptyBroadcasters;
    else if (aComparison.mbDeduplicatedEquivalent
             && (aComparison.mnLiveDuplicateBroadcasterCount > 0
                 || aComparison.mnLiveDuplicateListenerCount > 0))
        aComparison.meKind = BroadcasterCanonicalizationKind::DuplicateMaterializationOnly;
    else if (aComparison.mbDropEmptyEquivalent && aComparison.mnLiveEmptyBroadcasterCount > 0)
        aComparison.meKind = BroadcasterCanonicalizationKind::EmptyBroadcastersOnly;
    else if ((aComparison.mnExpectedCellBroadcasters + aComparison.mnExpectedAreaBroadcasters) == 0
             && (aComparison.mnLiveCellBroadcasters + aComparison.mnLiveAreaBroadcasters) > 0)
        aComparison.meKind = BroadcasterCanonicalizationKind::MissingExpectedBroadcasters;
    else if (aComparison.mbDiagnosticCanonicalEquivalent)
        aComparison.meKind = BroadcasterCanonicalizationKind::Mixed;
    else
        aComparison.meKind = BroadcasterCanonicalizationKind::Unknown;

    return aComparison;
}

[[nodiscard]] inline const char* toString(BroadcasterCanonicalizationKind eKind)
{
    switch (eKind)
    {
        case BroadcasterCanonicalizationKind::Exact:
            return "exact";
        case BroadcasterCanonicalizationKind::OrderingOnly:
            return "ordering_only";
        case BroadcasterCanonicalizationKind::DuplicateMaterializationOnly:
            return "duplicate_materialization_only";
        case BroadcasterCanonicalizationKind::EmptyBroadcastersOnly:
            return "empty_broadcasters_only";
        case BroadcasterCanonicalizationKind::DuplicateAndEmptyBroadcasters:
            return "duplicate_and_empty_broadcasters";
        case BroadcasterCanonicalizationKind::MissingExpectedBroadcasters:
            return "missing_expected_broadcasters";
        case BroadcasterCanonicalizationKind::ListenerAnchorCanonicalizationOnly:
            return "listener_anchor_canonicalization_only";
        case BroadcasterCanonicalizationKind::UnexpectedHostListeners:
            return "unexpected_host_listeners";
        case BroadcasterCanonicalizationKind::Mixed:
            return "mixed";
        case BroadcasterCanonicalizationKind::Unknown:
            return "unknown";
    }

    return "unknown";
}

} // namespace detail

[[nodiscard]] inline ComputationalShadowComparison compareComputationalShadow(
    const ComputationalWorkbookShadow& rShadow,
    const facade::WorkbookFacade& rFacade,
    const ComputationalObservationState& rObservation)
{
    ComputationalShadowComparison aComparison;
    aComparison.mbCellPopulationMatch
        = detail::collectShadowCellAddresses(rShadow) == detail::collectFacadeCellAddresses(rFacade);
    aComparison.mbFormulaTreeMatch = rShadow.maFormulaTree == rObservation.maFormulaTree;
    aComparison.mbFormulaTrackMatch = rShadow.maFormulaTrack == rObservation.maFormulaTrack;
    aComparison.mbBroadcasterMatch = rShadow.maCellBroadcasters == rObservation.maCellBroadcasters
        && rShadow.maAreaBroadcasters == rObservation.maAreaBroadcasters;
    aComparison.mbGroupMatch = detail::collectShadowFormulaGroups(rShadow)
        == detail::collectFacadeFormulaGroups(rFacade);
    aComparison.mbNamedRangeMatch = detail::sortNamedRanges(rShadow.maNamedRanges)
        == detail::sortNamedRanges(rFacade.getNamedRangeDescriptors());
    aComparison.mbFullMatch = aComparison.mbCellPopulationMatch && aComparison.mbFormulaTreeMatch
        && aComparison.mbFormulaTrackMatch && aComparison.mbBroadcasterMatch
        && aComparison.mbGroupMatch && aComparison.mbNamedRangeMatch;
    return aComparison;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
