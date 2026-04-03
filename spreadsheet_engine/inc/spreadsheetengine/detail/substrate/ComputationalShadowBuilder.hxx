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
#include <utility>
#include <vector>

#include <spreadsheetengine/detail/substrate/ComputationalShadow.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowMapping.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacade.hxx>

namespace spreadsheetengine::detail::substrate
{

struct ComputationalObservationState
{
    std::vector<api::CellAddress> maFormulaTree;
    std::vector<api::CellAddress> maFormulaTrack;
    std::vector<CellBroadcasterRecord> maCellBroadcasters;
    std::vector<AreaBroadcasterRecord> maAreaBroadcasters;

    [[nodiscard]] constexpr bool operator==(const ComputationalObservationState& rOther) const
        = default;
};

namespace detail
{

[[nodiscard]] inline bool containsAddress(
    const std::vector<api::CellAddress>& rAddresses, const api::CellAddress& rAddress)
{
    return std::find(rAddresses.begin(), rAddresses.end(), rAddress) != rAddresses.end();
}

[[nodiscard]] inline auto findOrCreateGroupRecord(
    std::vector<ShadowFormulaGroupRecord>& rGroups, const ShadowFormulaGroupId& rId,
    const facade::FormulaGroupDescriptor& rDescriptor)
{
    auto aIt = std::find_if(rGroups.begin(), rGroups.end(),
        [&rId](const ShadowFormulaGroupRecord& rRecord) { return rRecord.maId == rId; });
    if (aIt != rGroups.end())
        return aIt;

    ShadowFormulaGroupRecord aRecord;
    aRecord.maId = rId;
    aRecord.maDescriptor = rDescriptor;
    rGroups.push_back(std::move(aRecord));
    return std::prev(rGroups.end());
}

} // namespace detail

[[nodiscard]] inline ComputationalWorkbookShadow buildComputationalWorkbookShadow(
    const facade::WorkbookFacade& rFacade,
    const ComputationalObservationState& rObservation = {})
{
    ComputationalWorkbookShadow aShadow;
    aShadow.maSnapshot = rFacade.getSnapshotInfo();
    aShadow.maGrammar = rFacade.getGrammar();
    aShadow.maNamedRanges = rFacade.getNamedRangeDescriptors();
    aShadow.maCellBroadcasters = rObservation.maCellBroadcasters;
    aShadow.maAreaBroadcasters = rObservation.maAreaBroadcasters;
    aShadow.maFormulaTree = rObservation.maFormulaTree;
    aShadow.maFormulaTrack = rObservation.maFormulaTrack;

    const auto aSheets = rFacade.getSheetDescriptors();
    aShadow.maSheets.reserve(aSheets.size());

    for (const auto& rSheet : aSheets)
    {
        ComputationalSheetShadow aSheetShadow;
        aSheetShadow.maSheet = rSheet;
        rFacade.visitCells(rSheet.mnId, [&](const facade::CellDescriptor& rCell) {
            ShadowCellRecord aRecord;
            aRecord.maId = mapping::makeShadowCellId(rCell.maAddress);
            aRecord.maCell = rCell;
            aRecord.moFormula = rFacade.getFormulaCellDescriptor(rCell.maAddress);
            aRecord.mbInFormulaTree
                = detail::containsAddress(rObservation.maFormulaTree, rCell.maAddress);
            aRecord.mbInFormulaTrack
                = detail::containsAddress(rObservation.maFormulaTrack, rCell.maAddress);

            if (const auto oGroup = rFacade.getFormulaGroupDescriptor(rCell.maAddress); oGroup)
            {
                aRecord.moFormulaGroup = mapping::makeShadowFormulaGroupId(*oGroup);
                auto aGroupIt = detail::findOrCreateGroupRecord(aShadow.maFormulaGroups,
                    *aRecord.moFormulaGroup, *oGroup);
                aGroupIt->maMembers.push_back(aRecord.maId);
            }

            aSheetShadow.maCells.push_back(std::move(aRecord));
            return true;
        });
        aShadow.maSheets.push_back(std::move(aSheetShadow));
    }

    return aShadow;
}

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
