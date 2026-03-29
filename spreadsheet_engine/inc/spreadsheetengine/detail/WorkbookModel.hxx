/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <map>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include <spreadsheetengine/api/Host.hxx>

namespace spreadsheetengine::core::workbook
{

enum class FormulaSearchType : sal_uInt8
{
    Normal,
    Wildcard,
    Regex
};

struct Cell
{
    api::CellValue maValue;
    api::String maFormula;
    api::String maRawValueType;
    api::String maRawValue;
    bool mbCovered = false;

    [[nodiscard]] bool hasFormula() const { return !maFormula.empty(); }
};

enum class SheetSourceMode : sal_uInt8
{
    Unknown,
    CopyResultsOnly
};

struct SheetSource
{
    api::String maHref;
    api::String maTableName;
    SheetSourceMode meMode = SheetSourceMode::Unknown;

    [[nodiscard]] constexpr bool isCopyResultsOnly() const
    {
        return meMode == SheetSourceMode::CopyResultsOnly;
    }
};

struct Sheet
{
    api::String maName;
    std::map<std::pair<api::ColumnIndex, api::RowIndex>, Cell> maCells;
    std::set<api::RowIndex> maHiddenRows;
    std::optional<SheetSource> moSource;

    void setCell(api::ColumnIndex nColumn, api::RowIndex nRow, const Cell& rCell)
    {
        maCells[{ nColumn, nRow }] = rCell;
    }

    void setRowHidden(api::RowIndex nRow, bool bHidden = true)
    {
        if (bHidden)
            maHiddenRows.insert(nRow);
        else
            maHiddenRows.erase(nRow);
    }

    [[nodiscard]] const Cell* findCell(api::ColumnIndex nColumn, api::RowIndex nRow) const
    {
        const auto aIt = maCells.find({ nColumn, nRow });
        return aIt == maCells.end() ? nullptr : &aIt->second;
    }

    [[nodiscard]] bool isRowHidden(api::RowIndex nRow) const
    {
        return maHiddenRows.contains(nRow);
    }
};

struct NamedRange
{
    api::String maName;
    api::String maScopeSheetName;
    api::String maBaseCellAddress;
    api::String maCellRangeAddress;

    [[nodiscard]] bool isGlobal() const { return maScopeSheetName.empty(); }
};

struct Workbook
{
    std::vector<Sheet> maSheets;
    std::vector<NamedRange> maNamedRanges;
    FormulaSearchType meFormulaSearchType = FormulaSearchType::Regex;
    bool mbSearchCriteriaMustApplyToWholeCell = true;

    [[nodiscard]] std::optional<api::SheetId> findSheetId(api::StringView rName) const
    {
        for (std::size_t nIndex = 0; nIndex < maSheets.size(); ++nIndex)
        {
            if (maSheets[nIndex].maName == rName)
                return static_cast<api::SheetId>(nIndex);
        }

        return std::nullopt;
    }

    [[nodiscard]] const Sheet* findSheet(api::StringView rName) const
    {
        for (const auto& rSheet : maSheets)
        {
            if (rSheet.maName == rName)
                return &rSheet;
        }

        return nullptr;
    }

    [[nodiscard]] const NamedRange* findNamedRange(
        api::StringView rName, api::StringView rScopeSheetName = {}) const
    {
        for (const auto& rRange : maNamedRanges)
        {
            if (rRange.maName == rName && rRange.maScopeSheetName == rScopeSheetName)
                return &rRange;
        }

        return nullptr;
    }
};

} // namespace spreadsheetengine::core::workbook

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
