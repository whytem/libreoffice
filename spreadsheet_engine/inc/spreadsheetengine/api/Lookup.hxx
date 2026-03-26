/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <cmath>
#include <optional>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/api/Matrix.hxx>

namespace spreadsheetengine::api::lookup
{

enum class Operation : sal_uInt8
{
    Match,
    XMatch,
    Lookup,
    HLookup,
    VLookup,
    XLookup
};

enum class MatchMode : sal_Int8
{
    ExactOrNotAvailable = 0,
    ExactOrNextSmaller = -1,
    ExactOrNextLarger = 1,
    Wildcard = 2,
    Regex = 3
};

enum class SearchMode : sal_Int8
{
    Forward = 1,
    Reverse = -1,
    BinaryAscending = 2,
    BinaryDescending = -2
};

enum class VectorOrientation : sal_uInt8
{
    Row,
    Column
};

enum class ComparisonOp : sal_uInt8
{
    Equal,
    LessEqual,
    GreaterEqual
};

enum class PatternMode : sal_uInt8
{
    Normal,
    Detect,
    Wildcard,
    Regex
};

struct MatchSearchMode
{
    MatchMode meMatchMode = MatchMode::ExactOrNotAvailable;
    SearchMode meSearchMode = SearchMode::Forward;

    [[nodiscard]] constexpr bool operator==(const MatchSearchMode& rOther) const = default;
};

struct VectorLayout
{
    VectorOrientation meOrientation = VectorOrientation::Column;
    MatrixSize mnLength = 0;

    [[nodiscard]] constexpr bool operator==(const VectorLayout& rOther) const = default;
};

struct SearchPolicy
{
    ComparisonOp meComparison = ComparisonOp::Equal;
    PatternMode mePattern = PatternMode::Normal;
    bool mbAllowMatchEmpty = false;

    [[nodiscard]] constexpr bool operator==(const SearchPolicy& rOther) const = default;
};

struct VectorSlice
{
    MatrixCoordinate maStart;
    MatrixDimensions maDimensions { 1, 1 };

    [[nodiscard]] constexpr bool isSingleCell() const
    {
        return maDimensions.mnColumns == 1 && maDimensions.mnRows == 1;
    }

    [[nodiscard]] constexpr bool operator==(const VectorSlice& rOther) const = default;
};

[[nodiscard]] constexpr bool isBinarySearchMode(SearchMode eSearchMode)
{
    return eSearchMode == SearchMode::BinaryAscending
           || eSearchMode == SearchMode::BinaryDescending;
}

[[nodiscard]] constexpr bool isExtendedLookupOperation(Operation eOperation)
{
    return eOperation == Operation::XMatch || eOperation == Operation::XLookup;
}

[[nodiscard]] inline ValueResult<MatchSearchMode> normalizeMatchType(double fType)
{
    if (!std::isfinite(fType))
        return ValueResult<MatchSearchMode>::failure(Error::IllegalArgument);

    switch (static_cast<int>(fType))
    {
        case -1:
            return ValueResult<MatchSearchMode>::success(
                { MatchMode::ExactOrNextLarger, SearchMode::BinaryDescending });
        case 0:
            return ValueResult<MatchSearchMode>::success(
                { MatchMode::ExactOrNotAvailable, SearchMode::Forward });
        case 1:
            return ValueResult<MatchSearchMode>::success(
                { MatchMode::ExactOrNextSmaller, SearchMode::BinaryAscending });
        default:
            return ValueResult<MatchSearchMode>::failure(Error::IllegalArgument);
    }
}

[[nodiscard]] inline ValueResult<SearchMode> normalizeSearchMode(sal_Int16 nMode)
{
    if (nMode >= -2 && nMode <= 2 && nMode != 0)
        return ValueResult<SearchMode>::success(static_cast<SearchMode>(nMode));

    return ValueResult<SearchMode>::failure(Error::IllegalArgument);
}

[[nodiscard]] inline ValueResult<MatchMode> normalizeExtendedMatchMode(sal_Int16 nMode)
{
    if (nMode >= -1 && nMode <= 3)
        return ValueResult<MatchMode>::success(static_cast<MatchMode>(nMode));

    return ValueResult<MatchMode>::failure(Error::IllegalArgument);
}

[[nodiscard]] inline ValueResult<VectorLayout> detectVectorLayout(
    const MatrixDimensions& rDimensions)
{
    if (isColumnVector(rDimensions))
        return ValueResult<VectorLayout>::success(
            { VectorOrientation::Column, rDimensions.mnRows });

    if (isRowVector(rDimensions))
        return ValueResult<VectorLayout>::success(
            { VectorOrientation::Row, rDimensions.mnColumns });

    return ValueResult<VectorLayout>::failure(Error::IllegalArgument);
}

[[nodiscard]] constexpr VectorLayout majorVectorLayout(const MatrixDimensions& rDimensions)
{
    if (rDimensions.mnRows >= rDimensions.mnColumns)
        return { VectorOrientation::Column, rDimensions.mnRows };

    return { VectorOrientation::Row, rDimensions.mnColumns };
}

[[nodiscard]] inline ValueResult<VectorOrientation> validateXLookupResultShape(
    const MatrixDimensions& rSearchDimensions, const MatrixDimensions& rResultDimensions)
{
    if (rSearchDimensions.isEmpty() || rResultDimensions.isEmpty())
        return ValueResult<VectorOrientation>::failure(Error::IllegalArgument);

    const VectorLayout aSearchLayout = majorVectorLayout(rSearchDimensions);
    if (aSearchLayout.meOrientation == VectorOrientation::Column)
    {
        if (rSearchDimensions.mnRows != rResultDimensions.mnRows)
            return ValueResult<VectorOrientation>::failure(Error::IllegalArgument);
    }
    else if (rSearchDimensions.mnColumns != rResultDimensions.mnColumns)
    {
        return ValueResult<VectorOrientation>::failure(Error::IllegalArgument);
    }

    return ValueResult<VectorOrientation>::success(aSearchLayout.meOrientation);
}

[[nodiscard]] inline ValueResult<SearchPolicy> buildSearchPolicy(
    Operation eOperation, MatchMode eMatchMode, SearchMode eSearchMode, bool bStringSearch,
    bool bVBAMode, bool bMayBeWildcard, bool bMayBeRegex)
{
    SearchPolicy aPolicy;

    switch (eMatchMode)
    {
        case MatchMode::ExactOrNotAvailable:
            aPolicy.meComparison = ComparisonOp::Equal;
            break;
        case MatchMode::ExactOrNextSmaller:
            aPolicy.meComparison = ComparisonOp::LessEqual;
            aPolicy.mbAllowMatchEmpty = isExtendedLookupOperation(eOperation);
            break;
        case MatchMode::ExactOrNextLarger:
            aPolicy.meComparison = ComparisonOp::GreaterEqual;
            aPolicy.mbAllowMatchEmpty = isExtendedLookupOperation(eOperation);
            break;
        case MatchMode::Wildcard:
        case MatchMode::Regex:
            if (!isExtendedLookupOperation(eOperation))
                return ValueResult<SearchPolicy>::failure(Error::IllegalArgument);

            if (isBinarySearchMode(eSearchMode))
                return ValueResult<SearchPolicy>::failure(Error::NoValue);

            aPolicy.meComparison = ComparisonOp::Equal;
            if (!bStringSearch)
            {
                aPolicy.mePattern = PatternMode::Normal;
            }
            else if (eMatchMode == MatchMode::Wildcard && bMayBeWildcard)
            {
                aPolicy.mePattern = PatternMode::Wildcard;
            }
            else if (eMatchMode == MatchMode::Regex && bMayBeRegex)
            {
                aPolicy.mePattern = PatternMode::Regex;
            }
            else
            {
                aPolicy.mePattern = PatternMode::Normal;
            }
            return ValueResult<SearchPolicy>::success(aPolicy);
    }

    if (bStringSearch && eOperation == Operation::Match)
        aPolicy.mePattern = bVBAMode ? PatternMode::Wildcard : PatternMode::Detect;

    return ValueResult<SearchPolicy>::success(aPolicy);
}

[[nodiscard]] inline ValueResult<MatrixSize> resolveSearchResultIndex(
    Operation eOperation, MatrixSize nHitIndex, std::optional<MatrixSize> oBestFit)
{
    if (nHitIndex > 0)
    {
        return ValueResult<MatrixSize>::success(
            eOperation == Operation::XLookup ? nHitIndex - 1 : nHitIndex);
    }

    if (!oBestFit)
        return ValueResult<MatrixSize>::failure(Error::NotAvailable);

    return ValueResult<MatrixSize>::success(
        eOperation == Operation::XLookup ? *oBestFit : *oBestFit + 1);
}

[[nodiscard]] inline ValueResult<MatrixCoordinate> planVectorElement(
    VectorOrientation eOrientation, MatrixSize nIndex, const MatrixDimensions& rDimensions)
{
    if (nIndex < 0)
        return ValueResult<MatrixCoordinate>::failure(Error::IllegalArgument);

    MatrixCoordinate aCoordinate;
    if (eOrientation == VectorOrientation::Column)
    {
        if (rDimensions.mnColumns <= 0 || nIndex >= rDimensions.mnRows)
            return ValueResult<MatrixCoordinate>::failure(Error::IllegalArgument);
        aCoordinate.mnRow = nIndex;
    }
    else
    {
        if (rDimensions.mnRows <= 0 || nIndex >= rDimensions.mnColumns)
            return ValueResult<MatrixCoordinate>::failure(Error::IllegalArgument);
        aCoordinate.mnColumn = nIndex;
    }

    return ValueResult<MatrixCoordinate>::success(aCoordinate);
}

[[nodiscard]] inline ValueResult<MatrixCoordinate> planTabularLookupResult(
    VectorOrientation eSearchOrientation, MatrixSize nHitIndex, MatrixSize nResultIndex,
    const MatrixDimensions& rDimensions)
{
    MatrixCoordinate aCoordinate;
    if (eSearchOrientation == VectorOrientation::Column)
    {
        aCoordinate.mnColumn = nResultIndex;
        aCoordinate.mnRow = nHitIndex;
    }
    else
    {
        aCoordinate.mnColumn = nHitIndex;
        aCoordinate.mnRow = nResultIndex;
    }

    if (!isValidCoordinate(rDimensions, aCoordinate))
        return ValueResult<MatrixCoordinate>::failure(Error::IllegalArgument);

    return ValueResult<MatrixCoordinate>::success(aCoordinate);
}

[[nodiscard]] inline ValueResult<VectorSlice> planXLookupResultSlice(
    VectorOrientation eSearchOrientation, MatrixSize nHitIndex,
    const MatrixDimensions& rResultDimensions)
{
    VectorSlice aSlice;
    if (eSearchOrientation == VectorOrientation::Column)
    {
        if (nHitIndex < 0 || nHitIndex >= rResultDimensions.mnRows)
            return ValueResult<VectorSlice>::failure(Error::IllegalArgument);

        aSlice.maStart.mnRow = nHitIndex;
        aSlice.maDimensions = { rResultDimensions.mnColumns, 1 };
    }
    else
    {
        if (nHitIndex < 0 || nHitIndex >= rResultDimensions.mnColumns)
            return ValueResult<VectorSlice>::failure(Error::IllegalArgument);

        aSlice.maStart.mnColumn = nHitIndex;
        aSlice.maDimensions = { 1, rResultDimensions.mnRows };
    }

    return ValueResult<VectorSlice>::success(aSlice);
}

} // namespace spreadsheetengine::api::lookup

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
