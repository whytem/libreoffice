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
#include <optional>
#include <vector>

#include <spreadsheetengine/api/Grammar.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacadeTypes.hxx>

namespace spreadsheetengine::detail::substrate
{

enum class ListenerAnchorKind : std::uint8_t
{
    FormulaCell,
    FormulaGroup,
    HostUnknown
};

struct ShadowCellId
{
    api::CellAddress maAddress;

    [[nodiscard]] constexpr bool operator==(const ShadowCellId& rOther) const = default;
};

struct ShadowFormulaGroupId
{
    api::CellAddress maAnchor;
    sal_Int32 mnLength = 0;

    [[nodiscard]] constexpr bool operator==(const ShadowFormulaGroupId& rOther) const = default;
    [[nodiscard]] constexpr bool isValid() const { return mnLength > 0; }
};

struct ListenerAnchorId
{
    ListenerAnchorKind meKind = ListenerAnchorKind::HostUnknown;
    api::CellAddress maAnchor;
    sal_Int32 mnLength = 0;

    [[nodiscard]] constexpr bool operator==(const ListenerAnchorId& rOther) const = default;
};

struct ShadowCellRecord
{
    ShadowCellId maId;
    facade::CellDescriptor maCell;
    std::optional<facade::FormulaCellDescriptor> moFormula;
    std::optional<ShadowFormulaGroupId> moFormulaGroup;
    bool mbInFormulaTree = false;
    bool mbInFormulaTrack = false;

    [[nodiscard]] constexpr bool operator==(const ShadowCellRecord& rOther) const = default;
    [[nodiscard]] constexpr bool hasFormula() const { return moFormula.has_value(); }
};

struct ShadowFormulaGroupRecord
{
    ShadowFormulaGroupId maId;
    facade::FormulaGroupDescriptor maDescriptor;
    std::vector<ShadowCellId> maMembers;

    [[nodiscard]] constexpr bool operator==(const ShadowFormulaGroupRecord& rOther) const = default;
};

struct CellBroadcasterRecord
{
    api::CellAddress maBroadcaster;
    std::vector<ListenerAnchorId> maListeners;

    [[nodiscard]] constexpr bool operator==(const CellBroadcasterRecord& rOther) const = default;
};

struct AreaBroadcasterRecord
{
    api::CellRange maBroadcaster;
    std::vector<ListenerAnchorId> maListeners;

    [[nodiscard]] constexpr bool operator==(const AreaBroadcasterRecord& rOther) const = default;
};

struct ComputationalSheetShadow
{
    facade::SheetDescriptor maSheet;
    std::vector<ShadowCellRecord> maCells;

    [[nodiscard]] constexpr bool operator==(const ComputationalSheetShadow& rOther) const = default;
};

struct ComputationalWorkbookShadow
{
    facade::WorkbookSnapshotInfo maSnapshot;
    api::Grammar maGrammar;
    std::vector<ComputationalSheetShadow> maSheets;
    std::vector<facade::NamedRangeDescriptor> maNamedRanges;
    std::vector<ShadowFormulaGroupRecord> maFormulaGroups;
    std::vector<CellBroadcasterRecord> maCellBroadcasters;
    std::vector<AreaBroadcasterRecord> maAreaBroadcasters;
    std::vector<api::CellAddress> maFormulaTree;
    std::vector<api::CellAddress> maFormulaTrack;

    [[nodiscard]] constexpr bool operator==(const ComputationalWorkbookShadow& rOther) const = default;

    [[nodiscard]] sal_Int32 getCellCount() const
    {
        sal_Int32 nCount = 0;
        for (const auto& rSheet : maSheets)
            nCount += static_cast<sal_Int32>(rSheet.maCells.size());
        return nCount;
    }

    [[nodiscard]] sal_Int32 getFormulaCellCount() const
    {
        sal_Int32 nCount = 0;
        for (const auto& rSheet : maSheets)
        {
            nCount += static_cast<sal_Int32>(std::count_if(rSheet.maCells.begin(),
                rSheet.maCells.end(), [](const ShadowCellRecord& rCell) {
                    return rCell.hasFormula();
                }));
        }
        return nCount;
    }

    [[nodiscard]] const ShadowCellRecord* findCell(const api::CellAddress& rAddress) const
    {
        for (const auto& rSheet : maSheets)
        {
            for (const auto& rCell : rSheet.maCells)
            {
                if (rCell.maId.maAddress == rAddress)
                    return &rCell;
            }
        }
        return nullptr;
    }
};

} // namespace spreadsheetengine::detail::substrate

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
