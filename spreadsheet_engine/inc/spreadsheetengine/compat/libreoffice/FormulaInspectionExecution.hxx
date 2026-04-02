/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cstdint>
#include <utility>

#include <address.hxx>
#include <document.hxx>
#include <formulacell.hxx>
#include <formula/grammar.hxx>
#include <interpretercontext.hxx>
#include <scmatrix.hxx>
#include <svl/sharedstring.hxx>
#include <svl/sharedstringpool.hxx>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/runtime/FormulaInspection.hxx>

namespace spreadsheetengine::compat::libreoffice::formulainspection
{

enum class MatrixInspectionFailure : std::uint8_t
{
    None,
    IllegalArgument,
    MatrixSize
};

struct MatrixInspectionResult
{
    ScMatrixRef mpMatrix;
    MatrixInspectionFailure meFailure = MatrixInspectionFailure::None;
};

class CalcFormulaInspectionProvider final
    : public spreadsheetengine::runtime::formulainspection::Provider
{
    const ScDocument& mrDocument;
    ScInterpreterContext* mpContext;

public:
    CalcFormulaInspectionProvider(const ScDocument& rDocument,
        ScInterpreterContext* pContext = nullptr)
        : mrDocument(rDocument)
        , mpContext(pContext)
    {
    }

    [[nodiscard]] bool isFormulaCell(
        const spreadsheetengine::api::CellAddress& rAddress) const override
    {
        return mrDocument.GetFormulaCell(toLibreOfficeAddress(rAddress)) != nullptr;
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::String>
    formulaTextForCell(const spreadsheetengine::api::CellAddress& rAddress) const override
    {
        const ScFormulaCell* pCell = mrDocument.GetFormulaCell(toLibreOfficeAddress(rAddress));
        if (!pCell)
        {
            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::String>::failure(
                spreadsheetengine::api::Error::NotAvailable);
        }

        return spreadsheetengine::api::ValueResult<spreadsheetengine::api::String>::success(
            toApiString(
                pCell->GetFormula(formula::FormulaGrammar::GRAM_UNSPECIFIED, mpContext)));
    }
};

class DirectFormulaInspectionAdapter
{
    CalcFormulaInspectionProvider maProvider;

public:
    explicit DirectFormulaInspectionAdapter(const ScDocument& rDocument)
        : maProvider(rDocument)
    {
    }

    DirectFormulaInspectionAdapter(const ScDocument& rDocument, ScInterpreterContext& rContext)
        : maProvider(rDocument, &rContext)
    {
    }

    [[nodiscard]] bool isFormulaCell(const ScAddress& rAddress) const
    {
        return spreadsheetengine::runtime::formulainspection::isFormulaValue(
                   maProvider, toApiCellAddress(rAddress))
            .mfNumber
               != 0.0;
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<OUString> formulaTextForCell(
        const ScAddress& rAddress) const
    {
        const auto aResult = spreadsheetengine::runtime::formulainspection::formulaTextValue(
            maProvider, toApiCellAddress(rAddress));
        if (!aResult)
        {
            return spreadsheetengine::api::ValueResult<OUString>::failure(aResult.meError);
        }

        return spreadsheetengine::api::ValueResult<OUString>::success(
            toLibreOfficeString(aResult.maValue));
    }
};

template <typename MatrixFactory, typename CellWriter>
[[nodiscard]] inline MatrixInspectionResult buildMatrixInspection(
    const ScRange& rRange, MatrixFactory&& rFactory, CellWriter&& rWriter)
{
    MatrixInspectionResult aResult;
    if (rRange.aStart.Tab() != rRange.aEnd.Tab() || rRange.aStart.Col() > rRange.aEnd.Col()
        || rRange.aStart.Row() > rRange.aEnd.Row())
    {
        aResult.meFailure = MatrixInspectionFailure::IllegalArgument;
        return aResult;
    }

    const SCSIZE nColumns = static_cast<SCSIZE>(rRange.aEnd.Col() - rRange.aStart.Col() + 1);
    const SCSIZE nRows = static_cast<SCSIZE>(rRange.aEnd.Row() - rRange.aStart.Row() + 1);
    aResult.mpMatrix = rFactory(nColumns, nRows);
    if (!aResult.mpMatrix)
    {
        aResult.meFailure = MatrixInspectionFailure::MatrixSize;
        return aResult;
    }

    SCSIZE nMatrixColumn = 0;
    SCSIZE nMatrixRow = 0;
    ScAddress aAddress(0, 0, rRange.aStart.Tab());
    for (SCCOL nColumn = rRange.aStart.Col(); nColumn <= rRange.aEnd.Col(); ++nColumn)
    {
        aAddress.SetCol(nColumn);
        for (SCROW nRow = rRange.aStart.Row(); nRow <= rRange.aEnd.Row(); ++nRow)
        {
            aAddress.SetRow(nRow);
            rWriter(*aResult.mpMatrix, nMatrixColumn, nMatrixRow, aAddress);
            ++nMatrixRow;
        }
        ++nMatrixColumn;
        nMatrixRow = 0;
    }

    return aResult;
}

template <typename MatrixFactory>
[[nodiscard]] inline MatrixInspectionResult buildIsFormulaMatrix(
    const DirectFormulaInspectionAdapter& rAdapter, const ScRange& rRange, MatrixFactory&& rFactory)
{
    return buildMatrixInspection(
        rRange, std::forward<MatrixFactory>(rFactory),
        [&](ScMatrix& rMatrix, SCSIZE nColumn, SCSIZE nRow, const ScAddress& rAddress) {
            rMatrix.PutBoolean(rAdapter.isFormulaCell(rAddress), nColumn, nRow);
        });
}

template <typename MatrixFactory>
[[nodiscard]] inline MatrixInspectionResult buildFormulaTextMatrix(
    const DirectFormulaInspectionAdapter& rAdapter, const ScRange& rRange,
    svl::SharedStringPool& rStringPool, MatrixFactory&& rFactory)
{
    return buildMatrixInspection(
        rRange, std::forward<MatrixFactory>(rFactory),
        [&](ScMatrix& rMatrix, SCSIZE nColumn, SCSIZE nRow, const ScAddress& rAddress) {
            const auto aFormulaText = rAdapter.formulaTextForCell(rAddress);
            if (!aFormulaText)
            {
                rMatrix.PutError(toFormulaError(aFormulaText.meError), nColumn, nRow);
                return;
            }

            rMatrix.PutString(rStringPool.intern(aFormulaText.maValue), nColumn, nRow);
        });
}

} // namespace spreadsheetengine::compat::libreoffice::formulainspection

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
