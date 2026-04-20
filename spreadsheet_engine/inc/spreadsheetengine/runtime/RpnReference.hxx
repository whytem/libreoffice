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
#include <optional>

#include <spreadsheetengine/api/Reference.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

// Batch 2 substrate: engine-native reference-ops decision layer.
//
// This header defines *pure* planners for the reference family:
// ScColumn / ScRow / ScSheet (axis-ordinal queries),
// ScColumns / ScRows / ScSheets (axis-span count),
// ScMultiArea / ScAreas (reference-list count),
// ScAddressFunc (address text formatting),
// ScIndirect / ScOffset / ScIndex (reference resolution and projection).
//
// The planners consume RpnValue inputs and dispatch to existing
// `api::reference::*` helpers. They never read Calc state, never mutate
// PC, never touch FormulaToken. Reference-shaped RpnValue inputs that
// require host-side resolution still defer through
// NeedsReferenceResolution; matrix inputs that require materialization
// defer through NeedsMatrixMaterialization.
//
// This layer now backs the admitted scalar/reference paths for
// ocColumn / ocRow / ocSheet / ocColumns / ocRows / ocSheets / ocAreas /
// ocAddress (2-arg form) / ocIndirect / ocOffset / ocIndex (scalar projection).
// Matrix-returning and broader host-sensitive forms still defer to Calc-host
// logic, but this is no longer a substrate-only header.

namespace spreadsheetengine::core::rpn
{

enum class AxisOrdinalKind : std::uint8_t
{
    // Column ordinal of the (single-cell) reference, 1-based.
    Column,
    // Row ordinal of the (single-cell) reference, 1-based.
    Row,
    // Sheet ordinal of the (single-cell) reference, 1-based.
    Sheet
};

enum class SpanCountKind : std::uint8_t
{
    // Number of columns spanned by the reference range.
    Columns,
    // Number of rows spanned by the reference range.
    Rows,
    // Number of sheets spanned by the reference range.
    Sheets
};

// Coerce an RpnValue argument to a 1-based positive integer index, defer
// matrix/reference inputs explicitly. Used by Index / Choose / Offset.
[[nodiscard]] inline RpnCoercionResult<std::size_t> coerceToPositiveIndex(
    const RpnValue& rValue)
{
    if (rValue.meKind == RpnValueKind::Reference)
        return RpnCoercionResult<std::size_t>::deferred(
            RpnCoercionReadiness::NeedsReferenceResolution);
    if (rValue.meKind == RpnValueKind::Matrix)
        return RpnCoercionResult<std::size_t>::deferred(
            RpnCoercionReadiness::NeedsMatrixMaterialization);

    const auto aNumber = coerceToNumber(rValue);
    if (aNumber.meReadiness != RpnCoercionReadiness::Ready)
        return RpnCoercionResult<std::size_t>::deferred(aNumber.meReadiness);
    if (!aNumber)
        return RpnCoercionResult<std::size_t>::failure(aNumber.meError);
    if (aNumber.maValue < 1.0)
        return RpnCoercionResult<std::size_t>::failure(api::Error::IllegalArgument);

    return RpnCoercionResult<std::size_t>::success(
        static_cast<std::size_t>(aNumber.maValue));
}

// Coerce an RpnValue argument to an offset that may be zero or negative
// (Offset row/col). Defer references and matrices explicitly.
[[nodiscard]] inline RpnCoercionResult<std::int64_t> coerceToSignedOffset(
    const RpnValue& rValue)
{
    if (rValue.meKind == RpnValueKind::Reference)
        return RpnCoercionResult<std::int64_t>::deferred(
            RpnCoercionReadiness::NeedsReferenceResolution);
    if (rValue.meKind == RpnValueKind::Matrix)
        return RpnCoercionResult<std::int64_t>::deferred(
            RpnCoercionReadiness::NeedsMatrixMaterialization);

    const auto aNumber = coerceToNumber(rValue);
    if (aNumber.meReadiness != RpnCoercionReadiness::Ready)
        return RpnCoercionResult<std::int64_t>::deferred(aNumber.meReadiness);
    if (!aNumber)
        return RpnCoercionResult<std::int64_t>::failure(aNumber.meError);

    return RpnCoercionResult<std::int64_t>::success(
        static_cast<std::int64_t>(aNumber.maValue));
}

// COLUMN / ROW / SHEET (with explicit reference operand).
[[nodiscard]] inline RpnCoercionResult<double> planAxisOrdinal(
    const RpnValue& rReference, AxisOrdinalKind eKind)
{
    if (rReference.meKind != RpnValueKind::Reference)
        return RpnCoercionResult<double>::deferred(
            RpnCoercionReadiness::NeedsReferenceResolution);

    const api::CellRange& rRange = rReference.maReference.maRange;
    if (eKind == AxisOrdinalKind::Sheet)
    {
        const auto aResult = api::reference::sheetOrdinalFromReference(rRange);
        if (!aResult)
            return RpnCoercionResult<double>::failure(aResult.meError);
        return RpnCoercionResult<double>::success(aResult.maValue);
    }

    const api::reference::ReferenceAxis eAxis
        = (eKind == AxisOrdinalKind::Column)
              ? api::reference::ReferenceAxis::Column
              : api::reference::ReferenceAxis::Row;
    const auto aPlan = api::reference::planAxisReference(rRange, eAxis);
    if (!aPlan)
        return RpnCoercionResult<double>::failure(aPlan.meError);

    // Scalar planning only — caller owns matrix expansion when needed.
    if (!aPlan.maValue.requiresMatrixResult())
        return RpnCoercionResult<double>::success(aPlan.maValue.mfStart);

    return RpnCoercionResult<double>::deferred(
        RpnCoercionReadiness::NeedsMatrixMaterialization);
}

// COLUMNS / ROWS / SHEETS span count. The caller passes
// bMultiplyAcrossSheets to mirror the legacy COLUMNS/ROWS contract that
// multiplies by the sheet span when the reference is 3D.
[[nodiscard]] inline RpnCoercionResult<double> planSpanCount(
    const RpnValue& rReference, SpanCountKind eKind, bool bMultiplyAcrossSheets = false)
{
    if (rReference.meKind == RpnValueKind::Matrix)
    {
        const auto eAxis = (eKind == SpanCountKind::Columns)
                               ? api::reference::ReferenceAxis::Column
                               : api::reference::ReferenceAxis::Row;
        const auto aResult
            = api::reference::countMatrixAxisSpan(rReference.maMatrixDimensions, eAxis);
        if (!aResult)
            return RpnCoercionResult<double>::failure(aResult.meError);
        return RpnCoercionResult<double>::success(aResult.maValue);
    }

    if (rReference.meKind != RpnValueKind::Reference)
        return RpnCoercionResult<double>::deferred(
            RpnCoercionReadiness::NeedsReferenceResolution);

    const api::CellRange& rRange = rReference.maReference.maRange;
    switch (eKind)
    {
        case SpanCountKind::Columns:
        {
            const auto aResult = api::reference::countReferenceAxisSpan(
                rRange, api::reference::ReferenceAxis::Column, bMultiplyAcrossSheets);
            if (!aResult)
                return RpnCoercionResult<double>::failure(aResult.meError);
            return RpnCoercionResult<double>::success(aResult.maValue);
        }
        case SpanCountKind::Rows:
        {
            const auto aResult = api::reference::countReferenceAxisSpan(
                rRange, api::reference::ReferenceAxis::Row, bMultiplyAcrossSheets);
            if (!aResult)
                return RpnCoercionResult<double>::failure(aResult.meError);
            return RpnCoercionResult<double>::success(aResult.maValue);
        }
        case SpanCountKind::Sheets:
        {
            const auto aResult = api::reference::sheetCountFromReference(rRange);
            if (!aResult)
                return RpnCoercionResult<double>::failure(aResult.meError);
            return RpnCoercionResult<double>::success(aResult.maValue);
        }
    }
    return RpnCoercionResult<double>::failure(api::Error::IllegalArgument);
}

// AREAS / multi-area: count of distinct reference areas in a list. The
// caller is responsible for assembling the list count (the engine doesn't
// own ScRefList iteration); this helper just validates the count.
[[nodiscard]] inline RpnCoercionResult<double> planAreaCount(std::size_t nAreaCount)
{
    const auto aResult = api::reference::countAreas(nAreaCount);
    if (!aResult)
        return RpnCoercionResult<double>::failure(aResult.meError);
    return RpnCoercionResult<double>::success(aResult.maValue);
}

// OFFSET: build a planned target range from a base range plus signed
// row/column offsets and optional new dimensions. Mirrors the legacy
// ScOffset contract while consuming RpnValue arguments. The caller passes
// nMaxColumn / nMaxRow from the host's workbook bounds.
struct OffsetParameters
{
    api::RowIndex mnRowOffset = 0;
    api::ColumnIndex mnColumnOffset = 0;
    std::optional<api::RowIndex> moNewHeight;
    std::optional<api::ColumnIndex> moNewWidth;
    api::ColumnIndex mnMaxColumn = 0;
    api::RowIndex mnMaxRow = 0;
};

[[nodiscard]] inline RpnCoercionResult<api::CellRange> planOffset(
    const RpnValue& rBase, const OffsetParameters& rParams)
{
    if (rBase.meKind != RpnValueKind::Reference)
        return RpnCoercionResult<api::CellRange>::deferred(
            RpnCoercionReadiness::NeedsReferenceResolution);

    const auto aResult = api::reference::planOffsetRange(
        rBase.maReference.maRange, rParams.mnRowOffset, rParams.mnColumnOffset,
        rParams.moNewHeight, rParams.moNewWidth, rParams.mnMaxColumn, rParams.mnMaxRow);
    if (!aResult)
        return RpnCoercionResult<api::CellRange>::failure(aResult.meError);
    return RpnCoercionResult<api::CellRange>::success(aResult.maValue);
}

// INDEX (reference form): project a sub-range out of a source reference
// using 1-based row / column / area indices. Each index may be 0 to mean
// "the whole axis."
struct IndexProjectionParameters
{
    api::RowIndex mnRowIndex = 0;       // 0 means "whole row axis"
    api::ColumnIndex mnColumnIndex = 0; // 0 means "whole column axis"
    bool mbColumnArgumentMissing = false;
    std::uint8_t mnParamCount = 0;
    std::size_t mnAreaIndex = 1;
    std::size_t mnAreaCount = 1;
};

[[nodiscard]] inline RpnCoercionResult<api::reference::IndexReferenceSelection>
projectIndexReference(const RpnValue& rSource, const IndexProjectionParameters& rParams)
{
    if (rSource.meKind != RpnValueKind::Reference)
        return RpnCoercionResult<api::reference::IndexReferenceSelection>::deferred(
            RpnCoercionReadiness::NeedsReferenceResolution);

    const auto aNormalizedArea
        = api::reference::normalizeAreaSelection(rParams.mnAreaIndex, rParams.mnAreaCount);
    if (!aNormalizedArea)
        return RpnCoercionResult<api::reference::IndexReferenceSelection>::failure(
            aNormalizedArea.meError);

    const auto aResult = api::reference::planIndexReferenceSelection(
        rSource.maReference.maRange, rParams.mnRowIndex, rParams.mnColumnIndex,
        rParams.mnParamCount);
    if (!aResult)
        return RpnCoercionResult<api::reference::IndexReferenceSelection>::failure(
            aResult.meError);
    return RpnCoercionResult<api::reference::IndexReferenceSelection>::success(
        aResult.maValue);
}

[[nodiscard]] inline RpnCoercionResult<api::reference::IndexMatrixSelection>
projectIndexMatrix(
    const RpnValue& rSource, const IndexProjectionParameters& rParams)
{
    if (rSource.meKind != RpnValueKind::Matrix)
        return RpnCoercionResult<api::reference::IndexMatrixSelection>::deferred(
            RpnCoercionReadiness::NeedsMatrixMaterialization);

    const auto aResult = api::reference::planIndexMatrixSelection(
        rSource.maMatrixDimensions, rParams.mnRowIndex, rParams.mnColumnIndex,
        rParams.mbColumnArgumentMissing, rParams.mnParamCount);
    if (!aResult)
        return RpnCoercionResult<api::reference::IndexMatrixSelection>::failure(
            aResult.meError);
    return RpnCoercionResult<api::reference::IndexMatrixSelection>::success(
        aResult.maValue);
}

} // namespace spreadsheetengine::core::rpn

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
