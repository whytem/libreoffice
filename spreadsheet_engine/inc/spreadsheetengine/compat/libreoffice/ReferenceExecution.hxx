/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <address.hxx>
#include <compiler.hxx>
#include <document.hxx>
#include <formula/grammar.hxx>
#include <scmatrix.hxx>

#include <spreadsheetengine/api/Reference.hxx>
#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/runtime/ReferenceText.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::compat::libreoffice::referenceexecution
{

using ReferenceAxis = spreadsheetengine::api::reference::ReferenceAxis;
using AxisReferencePlan = spreadsheetengine::api::reference::AxisReferencePlan;

namespace detail
{

[[nodiscard]] inline OUString extractDocPrefixAndNormalizeSheetToken(
    OUString& rSheetToken, formula::FormulaGrammar::AddressConvention eConvention)
{
    if (rSheetToken.isEmpty() || eConvention != formula::FormulaGrammar::CONV_OOO)
        return {};

    const sal_Int32 nDocTabPos = ScCompiler::GetDocTabPos(rSheetToken);
    if (nDocTabPos == -1)
        return {};

    sal_Int32 nSplitPos = nDocTabPos + 1;
    if (nSplitPos < rSheetToken.getLength() && rSheetToken[nSplitPos] == '$')
        ++nSplitPos;

    OUString aDocPrefix = rSheetToken.copy(0, nSplitPos);
    rSheetToken = rSheetToken.copy(nSplitPos);
    return aDocPrefix;
}

} // namespace detail

struct AddressFunctionRequest
{
    spreadsheetengine::api::RowIndex mnRow = 0;
    spreadsheetengine::api::ColumnIndex mnColumn = 0;
    std::int32_t mnAbsMode = 1;
    bool mbA1Style = true;
    OUString maSheetToken;
    formula::FormulaGrammar::AddressConvention meConvention
        = formula::FormulaGrammar::CONV_OOO;
};

[[nodiscard]] inline spreadsheetengine::api::ValueResult<OUString> formatAddressFunctionResult(
    const AddressFunctionRequest& rRequest)
{
    if (rRequest.mnAbsMode < 1 || rRequest.mnAbsMode > 8 || rRequest.mnRow < 0
        || rRequest.mnColumn < 0)
    {
        return spreadsheetengine::api::ValueResult<OUString>::failure(
            spreadsheetengine::api::Error::IllegalArgument);
    }

    const std::int32_t nAbsMode
        = rRequest.mnAbsMode >= 5 ? rRequest.mnAbsMode - 4 : rRequest.mnAbsMode;

    OUString aSheetToken = rRequest.maSheetToken;
    OUString aDocPrefix
        = detail::extractDocPrefixAndNormalizeSheetToken(aSheetToken, rRequest.meConvention);
    if (!aSheetToken.isEmpty() && (aSheetToken[0] != '\'' || !aSheetToken.endsWith("'")))
        ScCompiler::CheckTabQuotes(aSheetToken, rRequest.meConvention);

    const spreadsheetengine::api::String aFormatted
        = spreadsheetengine::runtime::referencetext::formatAddressFunctionResult(
            rRequest.mnRow, rRequest.mnColumn, nAbsMode, rRequest.mbA1Style, {});
    OUString aResult = toLibreOfficeString(aFormatted);
    if (!aSheetToken.isEmpty())
    {
        const std::u16string_view aSeparator
            = (rRequest.meConvention == formula::FormulaGrammar::CONV_XL_A1
                  || rRequest.meConvention == formula::FormulaGrammar::CONV_XL_R1C1)
                  ? std::u16string_view(u"!")
                  : std::u16string_view(u".");
        aResult = aDocPrefix + aSheetToken + OUString(aSeparator) + aResult;
    }
    else if (!aDocPrefix.isEmpty())
    {
        aResult = aDocPrefix + aResult;
    }
    return spreadsheetengine::api::ValueResult<OUString>::success(aResult);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<std::size_t> normalizeAreaSelection(
    sal_Int32 nArea, std::size_t nAreaCount)
{
    return spreadsheetengine::api::reference::normalizeAreaSelection(nArea, nAreaCount);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<AxisReferencePlan> planAxisReference(
    const ScRange& rRange, ReferenceAxis eAxis)
{
    spreadsheetengine::api::CellRange aRange
        = toApiCellRange(ScRange(rRange.aStart.Col(), rRange.aStart.Row(), 0, rRange.aEnd.Col(),
            rRange.aEnd.Row(), 0));
    return spreadsheetengine::api::reference::planAxisReference(aRange, eAxis);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<double> countReferenceAxisSpan(
    const ScRange& rRange, ReferenceAxis eAxis)
{
    return spreadsheetengine::api::reference::countReferenceAxisSpan(
        toApiCellRange(rRange), eAxis, true);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<double> countMatrixAxisSpan(
    const ScMatrixRef& pMatrix, ReferenceAxis eAxis)
{
    if (!pMatrix)
        return spreadsheetengine::api::ValueResult<double>::failure(
            spreadsheetengine::api::Error::IllegalArgument);

    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    pMatrix->GetDimensions(nColumns, nRows);
    return spreadsheetengine::api::reference::countMatrixAxisSpan(
        { static_cast<spreadsheetengine::api::MatrixSize>(nColumns),
            static_cast<spreadsheetengine::api::MatrixSize>(nRows) },
        eAxis);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<double> countAreas(std::size_t nAreaCount)
{
    return spreadsheetengine::api::reference::countAreas(nAreaCount);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<ScAddress> selectScalarReferenceCell(
    const ScRange& rRange, const ScAddress& rFormulaPos,
    const std::optional<spreadsheetengine::api::MatrixCoordinate>& oJumpCoordinate = std::nullopt)
{
    const auto aSelection = spreadsheetengine::api::reference::selectScalarReferenceCell(
        toApiCellRange(rRange), toApiCellAddress(rFormulaPos), oJumpCoordinate);
    if (!aSelection)
        return spreadsheetengine::api::ValueResult<ScAddress>::failure(aSelection.meError);
    return spreadsheetengine::api::ValueResult<ScAddress>::success(
        toLibreOfficeAddress(aSelection.maValue));
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<
    spreadsheetengine::api::reference::ReferenceListMaterializationPlan>
planReferenceListMaterialization(
    std::size_t nEntryCount, bool bMatrixFormula, bool bAllSingleCellReferences)
{
    return spreadsheetengine::api::reference::planReferenceListMaterialization(
        nEntryCount, bMatrixFormula, bAllSingleCellReferences);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<double> sheetOrdinal(
    const ScRange& rRange)
{
    return spreadsheetengine::api::reference::sheetOrdinalFromReference(toApiCellRange(rRange));
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<double> sheetCount(const ScRange& rRange)
{
    return spreadsheetengine::api::reference::sheetCountFromReference(toApiCellRange(rRange));
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<double> sheetOrdinal(
    const ScDocument& rDocument, const OUString& rSheetName)
{
    SCTAB nTab = 0;
    if (!rDocument.GetTable(rSheetName, nTab))
    {
        return spreadsheetengine::api::ValueResult<double>::failure(
            spreadsheetengine::api::Error::IllegalArgument);
    }

    return spreadsheetengine::api::reference::sheetOrdinalFromSheetId(nTab);
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<double> workbookSheetCount(
    const ScDocument& rDocument)
{
    return spreadsheetengine::api::reference::sheetCountFromWorkbookSize(
        rDocument.GetTableCount());
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<ScRange> planOffsetRange(
    const ScRange& rBaseRange,
    spreadsheetengine::api::RowIndex nRowOffset,
    spreadsheetengine::api::ColumnIndex nColumnOffset,
    const std::optional<spreadsheetengine::api::RowIndex>& oNewHeight,
    const std::optional<spreadsheetengine::api::ColumnIndex>& oNewWidth, SCCOL nMaxColumn,
    SCROW nMaxRow)
{
    const auto aPlanned = spreadsheetengine::api::reference::planOffsetRange(
        toApiCellRange(rBaseRange), nRowOffset, nColumnOffset, oNewHeight, oNewWidth, nMaxColumn,
        nMaxRow);
    if (!aPlanned)
        return spreadsheetengine::api::ValueResult<ScRange>::failure(aPlanned.meError);
    return spreadsheetengine::api::ValueResult<ScRange>::success(
        toLibreOfficeRange(aPlanned.maValue));
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<
    spreadsheetengine::api::reference::IndexReferenceSelection>
planIndexReferenceSelection(const ScRange& rSourceRange, spreadsheetengine::api::RowIndex nRow,
    spreadsheetengine::api::ColumnIndex nColumn, std::uint8_t nParamCount)
{
    return spreadsheetengine::api::reference::planIndexReferenceSelection(
        toApiCellRange(rSourceRange), nRow, nColumn, nParamCount);
}

} // namespace spreadsheetengine::compat::libreoffice::referenceexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
