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
#include <document.hxx>
#include <interpretercontext.hxx>
#include <scmatrix.hxx>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/Lookup.hxx>
#include <spreadsheetengine/api/Query.hxx>
#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/runtime/LookupRuntime.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::compat::libreoffice::lookupexecution
{

struct LookupVectorSource
{
    std::optional<ScRange> moRange;
    ScMatrixRef mpMatrix;
};

struct MatchExecutionRequest
{
    spreadsheetengine::api::CellValue maLookupValue
        = spreadsheetengine::api::CellValue::empty();
    LookupVectorSource maSearchSource;
    spreadsheetengine::api::lookup::MatchSearchMode maLegacyModes;
    spreadsheetengine::api::lookup::MatchMode meMatchMode
        = spreadsheetengine::api::lookup::MatchMode::ExactOrNotAvailable;
    spreadsheetengine::api::lookup::SearchMode meSearchMode
        = spreadsheetengine::api::lookup::SearchMode::Forward;
    spreadsheetengine::api::query::SearchType meSearchType
        = spreadsheetengine::api::query::SearchType::Normal;
    bool mbExtended = false;
    bool mbAllowPatternMatch = false;
};

namespace detail
{

using spreadsheetengine::core::lookup::LookupInput;
using spreadsheetengine::core::lookup::LookupMaterializer;

[[nodiscard]] inline spreadsheetengine::api::CellValue toApiCellValue(const ScMatrixValue& rValue)
{
    if (ScMatrix::IsBooleanType(rValue.nType))
        return spreadsheetengine::api::CellValue::boolean(rValue.GetBoolean());

    if (ScMatrix::IsValueType(rValue.nType))
    {
        if (const FormulaError eError = rValue.GetError(); eError != FormulaError::NONE)
            return spreadsheetengine::api::CellValue::error(toApiError(eError));
        return spreadsheetengine::api::CellValue::number(rValue.fVal);
    }

    if (ScMatrix::IsRealStringType(rValue.nType))
        return spreadsheetengine::api::CellValue::text(toApiString(rValue.GetString().getString()));

    return spreadsheetengine::api::CellValue::empty();
}

[[nodiscard]] inline spreadsheetengine::api::ValueResult<LookupInput> buildLookupInput(
    const LookupVectorSource& rSource)
{
    LookupInput aInput;
    aInput.mbScalar = false;

    if (rSource.moRange)
    {
        aInput.maReference.maRange = toApiCellRange(*rSource.moRange);
        aInput.mnColumns = rSource.moRange->aEnd.Col() - rSource.moRange->aStart.Col() + 1;
        aInput.mnRows = rSource.moRange->aEnd.Row() - rSource.moRange->aStart.Row() + 1;
        return spreadsheetengine::api::ValueResult<LookupInput>::success(aInput);
    }

    if (!rSource.mpMatrix)
        return spreadsheetengine::api::ValueResult<LookupInput>::failure(
            spreadsheetengine::api::Error::IllegalArgument);

    SCSIZE nColumns = 0;
    SCSIZE nRows = 0;
    rSource.mpMatrix->GetDimensions(nColumns, nRows);
    if (nColumns == 0 || nRows == 0)
        return spreadsheetengine::api::ValueResult<LookupInput>::failure(
            spreadsheetengine::api::Error::IllegalArgument);

    aInput.mnColumns = static_cast<spreadsheetengine::api::MatrixSize>(nColumns);
    aInput.mnRows = static_cast<spreadsheetengine::api::MatrixSize>(nRows);
    aInput.maValues.reserve(nColumns * nRows);
    for (SCSIZE nRow = 0; nRow < nRows; ++nRow)
    {
        for (SCSIZE nColumn = 0; nColumn < nColumns; ++nColumn)
            aInput.maValues.push_back(toApiCellValue(rSource.mpMatrix->Get(nColumn, nRow)));
    }
    return spreadsheetengine::api::ValueResult<LookupInput>::success(std::move(aInput));
}

class CalcLookupMaterializer final : public LookupMaterializer
{
    DocumentEvaluationHost maHost;

public:
    CalcLookupMaterializer(const ScDocument& rDoc, ScInterpreterContext& rContext)
        : maHost(rDoc, rContext)
    {
    }

    [[nodiscard]] spreadsheetengine::api::ValueResult<spreadsheetengine::api::CellValue> materialize(
        const LookupInput& rInput,
        spreadsheetengine::api::MatrixCoordinate aCoordinate) const override
    {
        if (rInput.mbScalar)
        {
            if (aCoordinate.mnColumn != 0 || aCoordinate.mnRow != 0)
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::failure(
                    spreadsheetengine::api::Error::IllegalArgument);
            }
            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::CellValue>::success(rInput.maScalar);
        }

        if (!rInput.maValues.empty())
        {
            const std::int64_t nLinearIndex
                = static_cast<std::int64_t>(aCoordinate.mnRow) * rInput.mnColumns
                  + aCoordinate.mnColumn;
            if (nLinearIndex < 0
                || static_cast<std::size_t>(nLinearIndex) >= rInput.maValues.size())
            {
                return spreadsheetengine::api::ValueResult<
                    spreadsheetengine::api::CellValue>::failure(
                    spreadsheetengine::api::Error::IllegalArgument);
            }
            return spreadsheetengine::api::ValueResult<
                spreadsheetengine::api::CellValue>::success(
                rInput.maValues[static_cast<std::size_t>(nLinearIndex)]);
        }

        return maHost.getRangeValue(
            rInput.maReference.maRange, aCoordinate.mnColumn, aCoordinate.mnRow);
    }
};

} // namespace detail

[[nodiscard]] inline spreadsheetengine::api::ValueResult<spreadsheetengine::api::MatrixSize>
resolveMatchIndex(const ScDocument& rDoc, ScInterpreterContext& rContext,
    const MatchExecutionRequest& rRequest)
{
    const auto aSearchInput = detail::buildLookupInput(rRequest.maSearchSource);
    if (!aSearchInput)
    {
        return spreadsheetengine::api::ValueResult<
            spreadsheetengine::api::MatrixSize>::failure(aSearchInput.meError);
    }

    const detail::CalcLookupMaterializer aMaterializer(rDoc, rContext);
    if (!rRequest.mbExtended)
    {
        return spreadsheetengine::core::lookup::resolveMatchIndex(
            aMaterializer, rRequest.maLookupValue, aSearchInput.maValue, rRequest.maLegacyModes,
            rRequest.meSearchType);
    }

    return spreadsheetengine::core::lookup::resolveExtendedMatchIndex(aMaterializer,
        rRequest.maLookupValue, aSearchInput.maValue, rRequest.meMatchMode,
        rRequest.meSearchMode, rRequest.meSearchType, rRequest.mbAllowPatternMatch);
}

} // namespace spreadsheetengine::compat::libreoffice::lookupexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
