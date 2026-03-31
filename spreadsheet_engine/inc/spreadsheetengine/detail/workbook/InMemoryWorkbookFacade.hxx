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
#include <map>
#include <vector>

#include <spreadsheetengine/detail/workbook/WorkbookFacade.hxx>

namespace spreadsheetengine::detail::facade
{

/// In-memory implementation of WorkbookFacade for standalone testing.
///
/// This mirrors the InMemoryHost pattern — a simple, non-Calc facade
/// implementation backed by std containers. It is used for standalone
/// unit tests and for validating the facade contract without requiring
/// a LibreOffice runtime.
class InMemoryWorkbookFacade final : public WorkbookFacade
{
public:
    struct CellEntry
    {
        api::CellValue maValue;
        api::String maFormula; ///< empty = scalar cell
        FormulaCellKind meFormulaKind = FormulaCellKind::Ordinary;
        bool mbDirty = false;
        bool mbNeedsRecalc = false;
    };

    struct SheetEntry
    {
        api::String maName;
        bool mbHidden = false;
        std::map<std::pair<api::ColumnIndex, api::RowIndex>, CellEntry> maCells;
    };

    struct NamedRangeEntry
    {
        api::String maName;
        std::optional<SheetId> moScopeSheet;
        api::CellAddress maBaseAddress;
        api::String maTargetExpression;
    };

    struct FormulaGroupEntry
    {
        api::CellAddress maAnchor;
        sal_Int32 mnLength = 0;
        bool mbShareable = true;
    };

private:
    std::vector<SheetEntry> maSheets;
    std::vector<NamedRangeEntry> maNamedRanges;
    std::map<std::pair<api::SheetId, std::pair<api::ColumnIndex, api::RowIndex>>,
        FormulaGroupEntry> maFormulaGroups;
    api::Grammar maGrammar;
    std::int64_t mnGeneration = 0;

public:
    InMemoryWorkbookFacade() = default;

    // --- Builder methods ---

    SheetId addSheet(api::StringView rName, bool bHidden = false)
    {
        SheetEntry aSheet;
        aSheet.maName = api::String(rName);
        aSheet.mbHidden = bHidden;
        maSheets.push_back(std::move(aSheet));
        return static_cast<SheetId>(maSheets.size() - 1);
    }

    void setCell(const api::CellAddress& rAddress, const api::CellValue& rValue)
    {
        if (rAddress.mnSheet < 0
            || static_cast<std::size_t>(rAddress.mnSheet) >= maSheets.size())
            return;

        CellEntry aEntry;
        aEntry.maValue = rValue;
        maSheets[rAddress.mnSheet].maCells[{ rAddress.mnColumn, rAddress.mnRow }]
            = std::move(aEntry);
    }

    void setFormulaCell(const api::CellAddress& rAddress, api::StringView rFormula,
        const api::CellValue& rCachedValue = api::CellValue::number(0.0),
        FormulaCellKind eKind = FormulaCellKind::Ordinary,
        bool bDirty = false, bool bNeedsRecalc = false)
    {
        if (rAddress.mnSheet < 0
            || static_cast<std::size_t>(rAddress.mnSheet) >= maSheets.size())
            return;

        CellEntry aEntry;
        aEntry.maValue = rCachedValue;
        aEntry.maFormula = api::String(rFormula);
        aEntry.meFormulaKind = eKind;
        aEntry.mbDirty = bDirty;
        aEntry.mbNeedsRecalc = bNeedsRecalc;
        maSheets[rAddress.mnSheet].maCells[{ rAddress.mnColumn, rAddress.mnRow }]
            = std::move(aEntry);
    }

    void addNamedRange(api::StringView rName,
        std::optional<SheetId> oScopeSheet,
        const api::CellAddress& rBaseAddress,
        api::StringView rTargetExpression)
    {
        NamedRangeEntry aEntry;
        aEntry.maName = api::String(rName);
        aEntry.moScopeSheet = oScopeSheet;
        aEntry.maBaseAddress = rBaseAddress;
        aEntry.maTargetExpression = api::String(rTargetExpression);
        maNamedRanges.push_back(std::move(aEntry));
    }

    void addFormulaGroup(const api::CellAddress& rAnchor, sal_Int32 nLength,
        bool bShareable = true)
    {
        FormulaGroupEntry aEntry;
        aEntry.maAnchor = rAnchor;
        aEntry.mnLength = nLength;
        aEntry.mbShareable = bShareable;

        for (sal_Int32 nOffset = 0; nOffset < nLength; ++nOffset)
        {
            api::CellAddress aMemberAddr = rAnchor;
            aMemberAddr.mnRow += nOffset;
            maFormulaGroups[{ aMemberAddr.mnSheet,
                { aMemberAddr.mnColumn, aMemberAddr.mnRow } }] = aEntry;
        }
    }

    void setGrammar(const api::Grammar& rGrammar) { maGrammar = rGrammar; }
    void setGeneration(std::int64_t nGeneration) { mnGeneration = nGeneration; }

    // --- WorkbookFacade implementation ---

    [[nodiscard]] sal_Int32 getSheetCount() const override
    {
        return static_cast<sal_Int32>(maSheets.size());
    }

    [[nodiscard]] std::optional<SheetId> findSheetId(
        api::StringView rName) const override
    {
        for (std::size_t nIndex = 0; nIndex < maSheets.size(); ++nIndex)
        {
            if (maSheets[nIndex].maName == rName)
                return static_cast<SheetId>(nIndex);
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<SheetDescriptor> getSheetDescriptor(
        SheetId nSheet) const override
    {
        if (nSheet < 0 || static_cast<std::size_t>(nSheet) >= maSheets.size())
            return std::nullopt;

        const auto& rSheet = maSheets[nSheet];
        return SheetDescriptor { nSheet, rSheet.maName, rSheet.mbHidden };
    }

    [[nodiscard]] std::vector<SheetDescriptor> getSheetDescriptors() const override
    {
        std::vector<SheetDescriptor> aResult;
        aResult.reserve(maSheets.size());
        for (std::size_t nIndex = 0; nIndex < maSheets.size(); ++nIndex)
        {
            aResult.push_back(SheetDescriptor {
                static_cast<SheetId>(nIndex),
                maSheets[nIndex].maName,
                maSheets[nIndex].mbHidden });
        }
        return aResult;
    }

    [[nodiscard]] api::Grammar getGrammar() const override { return maGrammar; }

    [[nodiscard]] WorkbookSnapshotInfo getSnapshotInfo() const override
    {
        WorkbookSnapshotInfo aInfo;
        aInfo.mnGeneration = mnGeneration;
        aInfo.mnSheetCount = static_cast<sal_Int32>(maSheets.size());

        sal_Int32 nFormulaCount = 0;
        for (const auto& rSheet : maSheets)
        {
            for (const auto& rCell : rSheet.maCells)
            {
                if (!rCell.second.maFormula.empty())
                    ++nFormulaCount;
            }
        }
        aInfo.mnFormulaCellCount = nFormulaCount;
        return aInfo;
    }

    [[nodiscard]] bool hasCell(const api::CellAddress& rAddress) const override
    {
        if (rAddress.mnSheet < 0
            || static_cast<std::size_t>(rAddress.mnSheet) >= maSheets.size())
            return false;

        return maSheets[rAddress.mnSheet].maCells.contains(
            { rAddress.mnColumn, rAddress.mnRow });
    }

    [[nodiscard]] CellDescriptor getCellDescriptor(
        const api::CellAddress& rAddress) const override
    {
        CellDescriptor aDesc;
        aDesc.maAddress = rAddress;

        if (rAddress.mnSheet < 0
            || static_cast<std::size_t>(rAddress.mnSheet) >= maSheets.size())
            return aDesc;

        const auto aIt = maSheets[rAddress.mnSheet].maCells.find(
            { rAddress.mnColumn, rAddress.mnRow });
        if (aIt == maSheets[rAddress.mnSheet].maCells.end())
            return aDesc;

        const auto& rEntry = aIt->second;
        if (!rEntry.maFormula.empty())
        {
            aDesc.meKind = CellKind::Formula;
            aDesc.mbHasFormula = true;
        }
        else
        {
            aDesc.meKind = CellKind::Scalar;
        }
        aDesc.maValue = rEntry.maValue;
        return aDesc;
    }

    [[nodiscard]] std::optional<FormulaCellDescriptor> getFormulaCellDescriptor(
        const api::CellAddress& rAddress) const override
    {
        if (rAddress.mnSheet < 0
            || static_cast<std::size_t>(rAddress.mnSheet) >= maSheets.size())
            return std::nullopt;

        const auto aIt = maSheets[rAddress.mnSheet].maCells.find(
            { rAddress.mnColumn, rAddress.mnRow });
        if (aIt == maSheets[rAddress.mnSheet].maCells.end())
            return std::nullopt;

        const auto& rEntry = aIt->second;
        if (rEntry.maFormula.empty())
            return std::nullopt;

        FormulaCellDescriptor aDesc;
        aDesc.maId = FormulaCellId { rAddress };
        aDesc.maCachedValue = rEntry.maValue;
        aDesc.maFormulaSource = rEntry.maFormula;
        aDesc.meKind = rEntry.meFormulaKind;
        aDesc.mbDirty = rEntry.mbDirty;
        aDesc.mbNeedsRecalc = rEntry.mbNeedsRecalc;
        return aDesc;
    }

    void visitFormulaCells(SheetId nSheet,
        const FormulaCellVisitor& rVisitor) const override
    {
        if (nSheet < 0 || static_cast<std::size_t>(nSheet) >= maSheets.size())
            return;

        for (const auto& rCell : maSheets[nSheet].maCells)
        {
            if (rCell.second.maFormula.empty())
                continue;

            api::CellAddress aAddr { nSheet, rCell.first.first, rCell.first.second };
            auto oDesc = getFormulaCellDescriptor(aAddr);
            if (oDesc && !rVisitor(*oDesc))
                return;
        }
    }

    void visitAllFormulaCells(const FormulaCellVisitor& rVisitor) const override
    {
        bool bStopped = false;
        for (std::size_t nSheet = 0; nSheet < maSheets.size() && !bStopped; ++nSheet)
        {
            visitFormulaCells(static_cast<SheetId>(nSheet),
                [&rVisitor, &bStopped](const FormulaCellDescriptor& rDesc) {
                    if (!rVisitor(rDesc))
                    {
                        bStopped = true;
                        return false;
                    }
                    return true;
                });
        }
    }

    [[nodiscard]] sal_Int32 getNamedRangeCount() const override
    {
        return static_cast<sal_Int32>(maNamedRanges.size());
    }

    [[nodiscard]] std::optional<NamedRangeDescriptor> findNamedRange(
        api::StringView rName,
        std::optional<SheetId> oScopeSheet) const override
    {
        // Search sheet-local first if scope is specified.
        if (oScopeSheet.has_value())
        {
            for (sal_Int32 nIndex = 0;
                 nIndex < static_cast<sal_Int32>(maNamedRanges.size()); ++nIndex)
            {
                const auto& rEntry = maNamedRanges[nIndex];
                if (rEntry.maName == rName && rEntry.moScopeSheet == oScopeSheet)
                    return makeNamedRangeDescriptor(nIndex);
            }
        }

        // Search global.
        for (sal_Int32 nIndex = 0;
             nIndex < static_cast<sal_Int32>(maNamedRanges.size()); ++nIndex)
        {
            const auto& rEntry = maNamedRanges[nIndex];
            if (rEntry.maName == rName && !rEntry.moScopeSheet.has_value())
                return makeNamedRangeDescriptor(nIndex);
        }

        return std::nullopt;
    }

    [[nodiscard]] std::vector<NamedRangeDescriptor>
    getNamedRangeDescriptors() const override
    {
        std::vector<NamedRangeDescriptor> aResult;
        aResult.reserve(maNamedRanges.size());
        for (sal_Int32 nIndex = 0;
             nIndex < static_cast<sal_Int32>(maNamedRanges.size()); ++nIndex)
        {
            auto oDesc = makeNamedRangeDescriptor(nIndex);
            if (oDesc)
                aResult.push_back(std::move(*oDesc));
        }
        return aResult;
    }

    [[nodiscard]] std::optional<FormulaGroupDescriptor> getFormulaGroupDescriptor(
        const api::CellAddress& rAddress) const override
    {
        const auto aIt = maFormulaGroups.find(
            { rAddress.mnSheet, { rAddress.mnColumn, rAddress.mnRow } });
        if (aIt == maFormulaGroups.end())
            return std::nullopt;

        const auto& rEntry = aIt->second;
        return FormulaGroupDescriptor { rEntry.maAnchor, rEntry.mnLength,
            rEntry.mbShareable };
    }

private:
    [[nodiscard]] std::optional<NamedRangeDescriptor> makeNamedRangeDescriptor(
        sal_Int32 nIndex) const
    {
        if (nIndex < 0
            || static_cast<std::size_t>(nIndex) >= maNamedRanges.size())
            return std::nullopt;

        const auto& rEntry = maNamedRanges[nIndex];
        NamedRangeDescriptor aDesc;
        aDesc.maId = NamedRangeId { nIndex, rEntry.moScopeSheet };
        aDesc.maName = rEntry.maName;
        aDesc.meScope = rEntry.moScopeSheet.has_value()
            ? NamedRangeScope::SheetLocal : NamedRangeScope::Global;
        aDesc.moScopeSheet = rEntry.moScopeSheet;
        aDesc.maBaseAddress = rEntry.maBaseAddress;
        aDesc.maTargetExpression = rEntry.maTargetExpression;
        return aDesc;
    }
};

} // namespace spreadsheetengine::detail::facade

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
