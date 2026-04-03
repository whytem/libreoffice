/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <document.hxx>
#include <dociter.hxx>
#include <formulacell.hxx>
#include <rangenam.hxx>
#include <table.hxx>

#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/Grammar.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacade.hxx>

namespace spreadsheetengine::compat::libreoffice
{

namespace detail
{
namespace facade = spreadsheetengine::detail::facade;
}

[[nodiscard]] inline detail::facade::NamedRangeDescriptor
makeNamedRangeDescriptor(const ScDocument& rDoc, const ScRangeData& rData,
    std::optional<detail::facade::SheetId> oScopeSheet)
{
    detail::facade::NamedRangeDescriptor aDesc;
    aDesc.maName = toApiString(rData.GetName());

    if (oScopeSheet.has_value())
    {
        aDesc.maId = detail::facade::NamedRangeId {
            static_cast<sal_Int32>(rData.GetIndex()), oScopeSheet };
        aDesc.meScope = detail::facade::NamedRangeScope::SheetLocal;
        aDesc.moScopeSheet = oScopeSheet;
    }
    else
    {
        aDesc.maId = detail::facade::NamedRangeId {
            static_cast<sal_Int32>(rData.GetIndex()), std::nullopt };
        aDesc.meScope = detail::facade::NamedRangeScope::Global;
    }

    aDesc.maBaseAddress = toApiCellAddress(rData.GetPos());
    aDesc.maTargetExpression = toApiString(rData.GetSymbol(rDoc.GetGrammar()));
    return aDesc;
}

/// Calc-backed implementation of WorkbookFacade.
///
/// This is a sidecar-style, non-owning adapter over ScDocument. It does
/// not duplicate workbook state — it projects read-only views of the
/// calculation-facing surface through the facade contract.
class CalcWorkbookFacade final : public detail::facade::WorkbookFacade
{
    const ScDocument& mrDoc;
    sal_Int64 mnGeneration;

public:
    explicit CalcWorkbookFacade(const ScDocument& rDoc)
        : mrDoc(rDoc)
        , mnGeneration(0)
    {
    }

    CalcWorkbookFacade(const ScDocument& rDoc, sal_Int64 nGeneration)
        : mrDoc(rDoc)
        , mnGeneration(nGeneration)
    {
    }

    // --- Workbook-level queries ---

    [[nodiscard]] sal_Int32 getSheetCount() const override
    {
        return mrDoc.GetTableCount();
    }

    [[nodiscard]] std::optional<detail::facade::SheetId> findSheetId(
        api::StringView rName) const override
    {
        SCTAB nTab = 0;
        const OUString aName = toLibreOfficeString(api::String(rName));
        if (!mrDoc.GetTable(aName, nTab))
            return std::nullopt;
        return static_cast<detail::facade::SheetId>(nTab);
    }

    [[nodiscard]] std::optional<detail::facade::SheetDescriptor> getSheetDescriptor(
        detail::facade::SheetId nSheet) const override
    {
        if (!mrDoc.HasTable(nSheet))
            return std::nullopt;

        OUString aName;
        mrDoc.GetName(nSheet, aName);

        detail::facade::SheetDescriptor aDesc;
        aDesc.mnId = nSheet;
        aDesc.maName = toApiString(aName);
        aDesc.mbHidden = !mrDoc.IsVisible(nSheet);
        return aDesc;
    }

    [[nodiscard]] std::vector<detail::facade::SheetDescriptor>
    getSheetDescriptors() const override
    {
        std::vector<detail::facade::SheetDescriptor> aResult;
        const sal_Int32 nCount = mrDoc.GetTableCount();
        aResult.reserve(nCount);
        for (sal_Int32 nSheet = 0; nSheet < nCount; ++nSheet)
        {
            auto oDesc = getSheetDescriptor(nSheet);
            if (oDesc)
                aResult.push_back(std::move(*oDesc));
        }
        return aResult;
    }

    [[nodiscard]] api::Grammar getGrammar() const override
    {
        return toApiGrammar(mrDoc.GetGrammar());
    }

    [[nodiscard]] detail::facade::WorkbookSnapshotInfo getSnapshotInfo() const override
    {
        detail::facade::WorkbookSnapshotInfo aInfo;
        aInfo.mnGeneration = mnGeneration;
        aInfo.mnSheetCount = mrDoc.GetTableCount();

        sal_Int32 nFormulaCount = 0;
        for (sal_Int32 nSheet = 0; nSheet < aInfo.mnSheetCount; ++nSheet)
        {
            ScCellIterator aIter(const_cast<ScDocument&>(mrDoc),
                ScRange(0, 0, nSheet, mrDoc.MaxCol(), mrDoc.MaxRow(), nSheet));
            for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
            {
                if (aIter.getType() == CELLTYPE_FORMULA)
                    ++nFormulaCount;
            }
        }

        aInfo.mnFormulaCellCount = nFormulaCount;
        return aInfo;
    }

    // --- Cell-level queries ---

    [[nodiscard]] bool hasCell(const api::CellAddress& rAddress) const override
    {
        const ScAddress aAddr = toLibreOfficeAddress(rAddress);
        if (!mrDoc.ValidAddress(aAddr) || !mrDoc.HasTable(aAddr.Tab()))
            return false;

        ScRefCellValue aCell(const_cast<ScDocument&>(mrDoc), aAddr);
        return !aCell.isEmpty();
    }

    [[nodiscard]] detail::facade::CellDescriptor getCellDescriptor(
        const api::CellAddress& rAddress) const override
    {
        detail::facade::CellDescriptor aDesc;
        aDesc.maAddress = rAddress;

        const ScAddress aAddr = toLibreOfficeAddress(rAddress);
        if (!mrDoc.ValidAddress(aAddr) || !mrDoc.HasTable(aAddr.Tab()))
            return aDesc;

        ScRefCellValue aCell(const_cast<ScDocument&>(mrDoc), aAddr);
        if (aCell.isEmpty())
            return aDesc;

        if (aCell.getType() == CELLTYPE_FORMULA)
        {
            aDesc.meKind = detail::facade::CellKind::Formula;
            aDesc.mbHasFormula = true;
            ScFormulaCell* pFCell = aCell.getFormula();
            if (pFCell)
            {
                if (pFCell->GetErrCode() != FormulaError::NONE)
                    aDesc.maValue = api::CellValue::error(toApiError(pFCell->GetErrCode()));
                else if (pFCell->IsValue())
                    aDesc.maValue = api::CellValue::number(pFCell->GetRawValue());
                else
                    aDesc.maValue = api::CellValue::text(
                        toApiString(pFCell->GetRawString().getString()));
            }
        }
        else
        {
            aDesc.meKind = detail::facade::CellKind::Scalar;
            if (aCell.hasNumeric())
                aDesc.maValue = api::CellValue::number(aCell.getRawValue());
            else if (aCell.hasString())
                aDesc.maValue = api::CellValue::text(
                    toApiString(aCell.getRawString(mrDoc)));
        }

        return aDesc;
    }

    [[nodiscard]] std::optional<detail::facade::FormulaCellDescriptor>
    getFormulaCellDescriptor(const api::CellAddress& rAddress) const override
    {
        const ScAddress aAddr = toLibreOfficeAddress(rAddress);
        if (!mrDoc.ValidAddress(aAddr) || !mrDoc.HasTable(aAddr.Tab()))
            return std::nullopt;

        ScRefCellValue aCell(const_cast<ScDocument&>(mrDoc), aAddr);
        if (aCell.getType() != CELLTYPE_FORMULA || !aCell.getFormula())
            return std::nullopt;

        ScFormulaCell* pFCell = aCell.getFormula();

        detail::facade::FormulaCellDescriptor aDesc;
        aDesc.maId = detail::facade::FormulaCellId { rAddress };

        if (pFCell->GetErrCode() != FormulaError::NONE)
            aDesc.maCachedValue = api::CellValue::error(toApiError(pFCell->GetErrCode()));
        else if (pFCell->IsValue())
            aDesc.maCachedValue = api::CellValue::number(pFCell->GetRawValue());
        else
            aDesc.maCachedValue = api::CellValue::text(
                toApiString(pFCell->GetRawString().getString()));

        // Formula source text.
        {
            OUString aFormula = const_cast<ScFormulaCell*>(pFCell)->GetFormula();
            aDesc.maFormulaSource = toApiString(aFormula);
        }

        // Classification.
        if (pFCell->GetMatrixFlag() == ScMatrixMode::Formula)
            aDesc.meKind = detail::facade::FormulaCellKind::MatrixOrigin;
        else if (pFCell->GetMatrixFlag() != ScMatrixMode::NONE)
            aDesc.meKind = detail::facade::FormulaCellKind::MatrixMember;
        else if (pFCell->IsShared())
            aDesc.meKind = detail::facade::FormulaCellKind::SharedGroupMember;
        else
            aDesc.meKind = detail::facade::FormulaCellKind::Ordinary;

        aDesc.mbDirty = pFCell->GetDirty();
        aDesc.mbNeedsRecalc = pFCell->NeedsInterpret();
        return aDesc;
    }

    void visitCells(detail::facade::SheetId nSheet,
        const detail::facade::CellVisitor& rVisitor) const override
    {
        if (!mrDoc.HasTable(nSheet))
            return;

        ScCellIterator aIter(const_cast<ScDocument&>(mrDoc),
            ScRange(0, 0, nSheet, mrDoc.MaxCol(), mrDoc.MaxRow(), nSheet));
        for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
        {
            const ScAddress& rPos = aIter.GetPos();
            api::CellAddress aApiAddr { nSheet,
                static_cast<api::ColumnIndex>(rPos.Col()),
                static_cast<api::RowIndex>(rPos.Row()) };

            const auto aDesc = getCellDescriptor(aApiAddr);
            if (aDesc.meKind != detail::facade::CellKind::Empty && !rVisitor(aDesc))
                return;
        }
    }

    void visitAllCells(const detail::facade::CellVisitor& rVisitor) const override
    {
        bool bStopped = false;
        const sal_Int32 nCount = mrDoc.GetTableCount();
        for (sal_Int32 nSheet = 0; nSheet < nCount && !bStopped; ++nSheet)
        {
            visitCells(nSheet, [&rVisitor, &bStopped](const detail::facade::CellDescriptor& rDesc) {
                if (!rVisitor(rDesc))
                {
                    bStopped = true;
                    return false;
                }
                return true;
            });
        }
    }

    // --- Formula-cell iteration ---
    void visitFormulaCells(detail::facade::SheetId nSheet,
        const detail::facade::FormulaCellVisitor& rVisitor) const override
    {
        if (!mrDoc.HasTable(nSheet))
            return;

        ScCellIterator aIter(const_cast<ScDocument&>(mrDoc),
            ScRange(0, 0, nSheet, mrDoc.MaxCol(), mrDoc.MaxRow(), nSheet));
        for (bool bHas = aIter.first(); bHas; bHas = aIter.next())
        {
            if (aIter.getType() != CELLTYPE_FORMULA)
                continue;

            const ScAddress& rPos = aIter.GetPos();
            api::CellAddress aApiAddr { nSheet,
                static_cast<api::ColumnIndex>(rPos.Col()),
                static_cast<api::RowIndex>(rPos.Row()) };

            auto oDesc = getFormulaCellDescriptor(aApiAddr);
            if (oDesc && !rVisitor(*oDesc))
                return;
        }
    }

    void visitAllFormulaCells(
        const detail::facade::FormulaCellVisitor& rVisitor) const override
    {
        bool bStopped = false;
        const sal_Int32 nCount = mrDoc.GetTableCount();
        for (sal_Int32 nSheet = 0; nSheet < nCount && !bStopped; ++nSheet)
        {
            visitFormulaCells(nSheet,
                [&rVisitor, &bStopped](const detail::facade::FormulaCellDescriptor& rDesc) {
                    if (!rVisitor(rDesc))
                    {
                        bStopped = true;
                        return false;
                    }
                    return true;
                });
        }
    }

    // --- Named-range queries ---

    [[nodiscard]] sal_Int32 getNamedRangeCount() const override
    {
        sal_Int32 nCount = 0;
        const ScRangeName* pGlobal = mrDoc.GetRangeName();
        if (pGlobal)
            nCount += static_cast<sal_Int32>(pGlobal->size());

        const sal_Int32 nSheets = mrDoc.GetTableCount();
        for (sal_Int32 nSheet = 0; nSheet < nSheets; ++nSheet)
        {
            const ScRangeName* pLocal = mrDoc.GetRangeName(nSheet);
            if (pLocal)
                nCount += static_cast<sal_Int32>(pLocal->size());
        }
        return nCount;
    }

    [[nodiscard]] std::optional<detail::facade::NamedRangeDescriptor> findNamedRange(
        api::StringView rName,
        std::optional<detail::facade::SheetId> oScopeSheet) const override
    {
        const OUString aName = toLibreOfficeString(api::String(rName));

        if (oScopeSheet.has_value())
        {
            const ScRangeName* pLocal = mrDoc.GetRangeName(*oScopeSheet);
            if (pLocal)
            {
                const ScRangeData* pData = pLocal->findByUpperName(
                    ScGlobal::getCharClass().uppercase(aName));
                if (pData)
                    return makeNamedRangeDescriptor(pData, *oScopeSheet);
            }
        }

        const ScRangeName* pGlobal = mrDoc.GetRangeName();
        if (pGlobal)
        {
            const ScRangeData* pData = pGlobal->findByUpperName(
                ScGlobal::getCharClass().uppercase(aName));
            if (pData)
                return makeNamedRangeDescriptor(pData, std::nullopt);
        }

        return std::nullopt;
    }

    [[nodiscard]] std::vector<detail::facade::NamedRangeDescriptor>
    getNamedRangeDescriptors() const override
    {
        std::vector<detail::facade::NamedRangeDescriptor> aResult;

        const ScRangeName* pGlobal = mrDoc.GetRangeName();
        if (pGlobal)
        {
            for (const auto& rEntry : *pGlobal)
            {
                auto oDesc = makeNamedRangeDescriptor(rEntry.second.get(), std::nullopt);
                if (oDesc)
                    aResult.push_back(std::move(*oDesc));
            }
        }

        const sal_Int32 nSheets = mrDoc.GetTableCount();
        for (sal_Int32 nSheet = 0; nSheet < nSheets; ++nSheet)
        {
            const ScRangeName* pLocal = mrDoc.GetRangeName(nSheet);
            if (!pLocal)
                continue;
            for (const auto& rEntry : *pLocal)
            {
                auto oDesc = makeNamedRangeDescriptor(
                    rEntry.second.get(), static_cast<detail::facade::SheetId>(nSheet));
                if (oDesc)
                    aResult.push_back(std::move(*oDesc));
            }
        }

        return aResult;
    }

    // --- Shared-formula/group queries ---

    [[nodiscard]] std::optional<detail::facade::FormulaGroupDescriptor>
    getFormulaGroupDescriptor(const api::CellAddress& rAddress) const override
    {
        const ScAddress aAddr = toLibreOfficeAddress(rAddress);
        if (!mrDoc.ValidAddress(aAddr) || !mrDoc.HasTable(aAddr.Tab()))
            return std::nullopt;

        ScRefCellValue aCell(const_cast<ScDocument&>(mrDoc), aAddr);
        if (aCell.getType() != CELLTYPE_FORMULA || !aCell.getFormula())
            return std::nullopt;

        ScFormulaCell* pFCell = aCell.getFormula();
        if (!pFCell->IsShared())
            return std::nullopt;

        detail::facade::FormulaGroupDescriptor aDesc;
        aDesc.maAnchor = toApiCellAddress(
            ScAddress(aAddr.Col(), pFCell->GetSharedTopRow(), aAddr.Tab()));
        aDesc.mnLength = pFCell->GetSharedLength();
        aDesc.mbShareable = true;
        return aDesc;
    }

private:
    [[nodiscard]] std::optional<detail::facade::NamedRangeDescriptor>
    makeNamedRangeDescriptor(const ScRangeData* pData,
        std::optional<detail::facade::SheetId> oScopeSheet) const
    {
        if (!pData)
            return std::nullopt;

        return compat::libreoffice::makeNamedRangeDescriptor(mrDoc, *pData, oScopeSheet);
    }
};

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
