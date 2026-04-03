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
