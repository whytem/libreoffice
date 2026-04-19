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
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include <spreadsheetengine/api/Array.hxx>
#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/api/Matrix.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

// Batch 5 substrate: engine-native dynamic-array / spill decision layer.
//
// This header defines the pure planners the engine RPN evaluator needs for
// the dynamic-array family: FILTER / SORT / SORTBY / UNIQUE / TAKE / DROP
// (Phase 5A), and later HSTACK / VSTACK / CHOOSECOLS / CHOOSEROWS / EXPAND /
// TOCOL / TOROW / WRAPCOLS / WRAPROWS / TEXTSPLIT (Phase 5B).
//
// Per the RPN Evaluator Initiative policy, a sub-phased admission lands the
// pure-shape planners first (no host-range materialization) and the scope
// fence at the dispatch site only accepts svMatrix sources. Range inputs wait
// on the Host facade range-to-matrix materialization contract (Phase D).
//
// Geometry helpers live in `spreadsheetengine::api::array` (see Array.hxx).
// The planners below compose those helpers and apply Calc-specific value
// semantics: empty cells coerce to zero for numeric comparisons, text cells
// coerce to a case-folded string for equality, errors propagate.
//
// `#SPILL!` error semantics (NEW behavior):
// Legacy Calc `ScFilter` / `ScSort` / `ScSortBy` / `ScUnique` / `ScTakeOrDrop`
// return a matrix token; they never emit a dedicated `#SPILL!` error.  The
// engine introduces a canonical spill lifecycle: `allocateSpillRange` asks
// the host to reserve a rectangle of cells, and if `checkSpillCollision`
// sees any non-empty target cell, the allocator returns
// `SpillError::Collision` which the dispatch site maps to `#SPILL!`. This
// is new behavior, not a migration of existing behavior — it only applies
// when an admitted opcode reaches the allocator.  The simple-shape phase
// (5A) does not yet need the allocator; it returns the engine-computed
// `MatrixOperand` up to the Calc dispatch bridge which calls PushMatrix
// just as the legacy bodies do.

namespace spreadsheetengine::core::rpn::spill
{

// Classification of spill-allocation failures.  Collision is the only error
// surfaced as `#SPILL!` in the Calc sense; the other variants are
// engine-internal planning failures that map to IllegalArgument /
// NotAvailable at the dispatch bridge.
enum class SpillError : std::uint8_t
{
    // A non-empty cell sits inside the proposed spill rectangle.
    Collision,
    // The requested anchor or dimensions fall outside the sheet bounds.
    OutOfBounds,
    // The requested dimensions are zero, negative, or otherwise invalid.
    InvalidShape,
    // A precondition of the allocator (anchor absent, host not ready) was
    // not met.  Surfaces as IllegalArgument.
    InvalidRequest
};

// A request to allocate a spill rectangle.  Anchor is the top-left cell
// that carries the dynamic-array formula.
struct SpillRequest
{
    api::CellAddress maAnchor;
    api::MatrixDimensions maDimensions { 0, 0 };

    [[nodiscard]] constexpr bool operator==(const SpillRequest& rOther) const = default;
};

// Planners for the Phase 5A members.  Each of these is a *pure* function:
// it consumes a MatrixOperand (already resolved from whatever svMatrix /
// materialized-reference source the dispatch site had) plus the option
// arguments, and returns a new MatrixOperand.  None of these planners talks
// to the host; the allocator layer below is where host-side spill-range
// collision lives.

// SORT: reorder rows of `rSource` (or columns when `bByRow == false`) by
// the values of the key columns / rows named in `rSortIndices`
// (1-based). `rAscending` is parallel-aligned to `rSortIndices`; when its
// size is `1` the single order applies to every key.  Non-numeric keys
// fall back to a string comparison using the cell's text form.
[[nodiscard]] inline RpnCoercionResult<MatrixOperand> planSort(
    const MatrixOperand& rSource, const std::vector<api::MatrixSize>& rSortIndices,
    const std::vector<bool>& rAscending, bool bByRow)
{
    if (rSource.isEmpty())
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);
    if (rSortIndices.empty())
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);
    if (!rAscending.empty() && rAscending.size() != 1
        && rAscending.size() != rSortIndices.size())
    {
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);
    }

    const api::MatrixSize nAxisLimit
        = bByRow ? rSource.maDimensions.mnColumns : rSource.maDimensions.mnRows;
    for (const auto nIdx : rSortIndices)
    {
        if (nIdx >= nAxisLimit)
            return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);
    }

    const api::MatrixSize nOuter
        = bByRow ? rSource.maDimensions.mnRows : rSource.maDimensions.mnColumns;
    std::vector<api::MatrixSize> aOrder;
    aOrder.reserve(static_cast<std::size_t>(nOuter));
    for (api::MatrixSize i = 0; i < nOuter; ++i)
        aOrder.push_back(i);

    const auto ascendingFor = [&](std::size_t nKey) {
        if (rAscending.empty())
            return true;
        if (rAscending.size() == 1)
            return rAscending.front();
        return rAscending[nKey];
    };

    const auto cellAt = [&](api::MatrixSize nOuterIdx, api::MatrixSize nInnerIdx)
        -> const api::CellValue& {
        const std::size_t nLinear
            = bByRow
                ? static_cast<std::size_t>(nOuterIdx) * rSource.maDimensions.mnColumns
                      + nInnerIdx
                : static_cast<std::size_t>(nInnerIdx) * rSource.maDimensions.mnColumns
                      + nOuterIdx;
        return rSource.maValues[nLinear];
    };

    std::stable_sort(aOrder.begin(), aOrder.end(),
                     [&](api::MatrixSize nLeft, api::MatrixSize nRight) {
                         for (std::size_t k = 0; k < rSortIndices.size(); ++k)
                         {
                             const auto nKey = rSortIndices[k];
                             const api::CellValue& rLeft = cellAt(nLeft, nKey);
                             const api::CellValue& rRight = cellAt(nRight, nKey);
                             const bool bAsc = ascendingFor(k);
                             // Error cells order equal to one another but sort
                             // to the end in either direction so the caller
                             // can still see them in the result.
                             if (rLeft.meKind == api::CellValueKind::Error
                                 && rRight.meKind != api::CellValueKind::Error)
                                 return false;
                             if (rRight.meKind == api::CellValueKind::Error
                                 && rLeft.meKind != api::CellValueKind::Error)
                                 return true;
                             const bool bLeftNum
                                 = rLeft.meKind == api::CellValueKind::Number
                                   || rLeft.meKind == api::CellValueKind::Boolean;
                             const bool bRightNum
                                 = rRight.meKind == api::CellValueKind::Number
                                   || rRight.meKind == api::CellValueKind::Boolean;
                             if (bLeftNum && bRightNum)
                             {
                                 if (rLeft.mfNumber == rRight.mfNumber)
                                     continue;
                                 return bAsc ? rLeft.mfNumber < rRight.mfNumber
                                             : rLeft.mfNumber > rRight.mfNumber;
                             }
                             // Mixed numeric / text: numbers sort before text
                             // ascending, text before numbers descending —
                             // matches Calc's legacy ScSort policy.
                             if (bLeftNum != bRightNum)
                                 return bAsc ? bLeftNum : !bLeftNum;
                             // Both text-shaped: compare by raw text bytes.
                             const auto& rLs = rLeft.maString;
                             const auto& rRs = rRight.maString;
                             if (rLs == rRs)
                                 continue;
                             return bAsc ? rLs < rRs : rRs < rLs;
                         }
                         return false;
                     });

    MatrixOperand aResult;
    aResult.maDimensions = rSource.maDimensions;
    aResult.maValues.resize(rSource.maValues.size());
    aResult.meProvenance = MatrixProvenance::ComputedResult;
    if (bByRow)
    {
        for (api::MatrixSize r = 0; r < nOuter; ++r)
        {
            const api::MatrixSize nSrcRow = aOrder[r];
            for (api::MatrixSize c = 0; c < rSource.maDimensions.mnColumns; ++c)
            {
                aResult.maValues[static_cast<std::size_t>(r) * rSource.maDimensions.mnColumns + c]
                    = rSource.maValues[static_cast<std::size_t>(nSrcRow)
                                           * rSource.maDimensions.mnColumns
                                       + c];
            }
        }
    }
    else
    {
        for (api::MatrixSize c = 0; c < nOuter; ++c)
        {
            const api::MatrixSize nSrcCol = aOrder[c];
            for (api::MatrixSize r = 0; r < rSource.maDimensions.mnRows; ++r)
            {
                aResult.maValues[static_cast<std::size_t>(r) * rSource.maDimensions.mnColumns + c]
                    = rSource.maValues[static_cast<std::size_t>(r)
                                           * rSource.maDimensions.mnColumns
                                       + nSrcCol];
            }
        }
    }
    return RpnCoercionResult<MatrixOperand>::success(aResult);
}

// FILTER: retain rows (or columns) of `rSource` where the parallel `rMask`
// cell is truthy.  A truthy cell is a non-zero Number / Boolean; anything
// else (string, empty, error) drops the row.  If the mask retains zero
// rows the planner returns NotAvailable so the dispatch site can surface
// `#N/A` or the caller's fallback value.
[[nodiscard]] inline std::variant<MatrixOperand, SpillError> planFilter(
    const MatrixOperand& rSource, const MatrixOperand& rMask)
{
    if (rSource.isEmpty() || rMask.isEmpty())
        return SpillError::InvalidShape;

    // Mask must be a single row or single column whose length matches the
    // corresponding axis of the source.
    const bool bMaskIsColumn = rMask.maDimensions.mnColumns == 1
                               && rMask.maDimensions.mnRows == rSource.maDimensions.mnRows;
    const bool bMaskIsRow = rMask.maDimensions.mnRows == 1
                            && rMask.maDimensions.mnColumns == rSource.maDimensions.mnColumns;
    if (!bMaskIsColumn && !bMaskIsRow)
        return SpillError::InvalidShape;

    std::vector<api::MatrixSize> aKept;
    const api::MatrixSize nOuter = bMaskIsColumn ? rSource.maDimensions.mnRows
                                                 : rSource.maDimensions.mnColumns;
    aKept.reserve(static_cast<std::size_t>(nOuter));
    for (api::MatrixSize i = 0; i < nOuter; ++i)
    {
        const auto& rCell = rMask.maValues[static_cast<std::size_t>(i)];
        const bool bTruthy
            = (rCell.meKind == api::CellValueKind::Number
               || rCell.meKind == api::CellValueKind::Boolean)
              && rCell.mfNumber != 0.0;
        if (bTruthy)
            aKept.push_back(i);
    }
    if (aKept.empty())
        return SpillError::OutOfBounds; // caller translates to NotAvailable

    MatrixOperand aResult;
    aResult.meProvenance = MatrixProvenance::ComputedResult;
    if (bMaskIsColumn)
    {
        aResult.maDimensions
            = { rSource.maDimensions.mnColumns, static_cast<api::MatrixSize>(aKept.size()) };
        aResult.maValues.reserve(static_cast<std::size_t>(aResult.maDimensions.mnColumns)
                                 * aResult.maDimensions.mnRows);
        for (const auto nRow : aKept)
        {
            for (api::MatrixSize c = 0; c < rSource.maDimensions.mnColumns; ++c)
            {
                aResult.maValues.push_back(
                    rSource.maValues[static_cast<std::size_t>(nRow)
                                         * rSource.maDimensions.mnColumns
                                     + c]);
            }
        }
    }
    else
    {
        aResult.maDimensions
            = { static_cast<api::MatrixSize>(aKept.size()), rSource.maDimensions.mnRows };
        aResult.maValues.resize(static_cast<std::size_t>(aResult.maDimensions.mnColumns)
                                * aResult.maDimensions.mnRows);
        for (api::MatrixSize r = 0; r < rSource.maDimensions.mnRows; ++r)
        {
            for (std::size_t i = 0; i < aKept.size(); ++i)
            {
                aResult.maValues[static_cast<std::size_t>(r) * aResult.maDimensions.mnColumns + i]
                    = rSource.maValues[static_cast<std::size_t>(r)
                                           * rSource.maDimensions.mnColumns
                                       + aKept[i]];
            }
        }
    }
    return aResult;
}

namespace detail
{

// Build a stable canonical key for a cell used by UNIQUE equality.  Matches
// the legacy ScUnique policy: case-folding is applied upstream by the
// dispatch bridge; here we just produce a byte-sequence that identifies
// numeric vs. text distinctly.
[[nodiscard]] inline std::string canonicalKey(const api::CellValue& rCell)
{
    switch (rCell.meKind)
    {
        case api::CellValueKind::Empty:
            return std::string(1, '\x01');
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
        {
            // Preserve integer vs. fractional distinctions in the key.
            std::string aKey(1, '#');
            aKey.append(reinterpret_cast<const char*>(&rCell.mfNumber), sizeof(double));
            return aKey;
        }
        case api::CellValueKind::Text:
        {
            std::string aKey(1, '$');
            aKey.append(rCell.maString.begin(), rCell.maString.end());
            return aKey;
        }
        case api::CellValueKind::Error:
        {
            std::string aKey(1, '!');
            const auto nCode = static_cast<std::uint8_t>(rCell.meError);
            aKey.append(reinterpret_cast<const char*>(&nCode), sizeof(std::uint8_t));
            return aKey;
        }
    }
    return {};
}

} // namespace detail

// UNIQUE: retain the first occurrence of each distinct row (when
// `bByColumns` is false) or distinct column (when `bByColumns` is true)
// of `rSource`.  When `bExactlyOnce` is true the planner instead keeps
// rows that appear exactly once in the source.
[[nodiscard]] inline RpnCoercionResult<MatrixOperand> planUnique(
    const MatrixOperand& rSource, bool bByColumns, bool bExactlyOnce)
{
    if (rSource.isEmpty())
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);

    const bool bByRow = !bByColumns;
    const api::MatrixSize nOuter
        = bByRow ? rSource.maDimensions.mnRows : rSource.maDimensions.mnColumns;
    const api::MatrixSize nInner
        = bByRow ? rSource.maDimensions.mnColumns : rSource.maDimensions.mnRows;

    const auto buildKey = [&](api::MatrixSize nOuterIdx) {
        std::string aKey;
        aKey.reserve(static_cast<std::size_t>(nInner) * 9);
        for (api::MatrixSize j = 0; j < nInner; ++j)
        {
            const std::size_t nLinear
                = bByRow ? static_cast<std::size_t>(nOuterIdx) * rSource.maDimensions.mnColumns
                               + j
                         : static_cast<std::size_t>(j) * rSource.maDimensions.mnColumns
                               + nOuterIdx;
            aKey.append(detail::canonicalKey(rSource.maValues[nLinear]));
            aKey.push_back('\x00');
        }
        return aKey;
    };

    std::unordered_map<std::string, std::pair<api::MatrixSize, std::size_t>> aCounts;
    std::vector<std::pair<std::string, api::MatrixSize>> aOrder;
    aOrder.reserve(static_cast<std::size_t>(nOuter));
    for (api::MatrixSize i = 0; i < nOuter; ++i)
    {
        auto aKey = buildKey(i);
        auto [it, bInserted] = aCounts.emplace(aKey, std::make_pair(i, std::size_t { 1 }));
        if (bInserted)
            aOrder.emplace_back(std::move(aKey), i);
        else
            ++it->second.second;
    }

    std::vector<api::MatrixSize> aKept;
    aKept.reserve(aOrder.size());
    for (const auto& [rKey, nIdx] : aOrder)
    {
        if (bExactlyOnce && aCounts.at(rKey).second != 1)
            continue;
        aKept.push_back(nIdx);
    }
    if (aKept.empty())
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::NotAvailable);

    MatrixOperand aResult;
    aResult.meProvenance = MatrixProvenance::ComputedResult;
    if (bByRow)
    {
        aResult.maDimensions = { rSource.maDimensions.mnColumns,
                                 static_cast<api::MatrixSize>(aKept.size()) };
        aResult.maValues.reserve(static_cast<std::size_t>(aResult.maDimensions.mnColumns)
                                 * aResult.maDimensions.mnRows);
        for (const auto nRow : aKept)
        {
            for (api::MatrixSize c = 0; c < rSource.maDimensions.mnColumns; ++c)
            {
                aResult.maValues.push_back(
                    rSource.maValues[static_cast<std::size_t>(nRow)
                                         * rSource.maDimensions.mnColumns
                                     + c]);
            }
        }
    }
    else
    {
        aResult.maDimensions = { static_cast<api::MatrixSize>(aKept.size()),
                                 rSource.maDimensions.mnRows };
        aResult.maValues.resize(static_cast<std::size_t>(aResult.maDimensions.mnColumns)
                                * aResult.maDimensions.mnRows);
        for (api::MatrixSize r = 0; r < rSource.maDimensions.mnRows; ++r)
        {
            for (std::size_t i = 0; i < aKept.size(); ++i)
            {
                aResult.maValues[static_cast<std::size_t>(r) * aResult.maDimensions.mnColumns + i]
                    = rSource.maValues[static_cast<std::size_t>(r)
                                           * rSource.maDimensions.mnColumns
                                       + aKept[i]];
            }
        }
    }
    return RpnCoercionResult<MatrixOperand>::success(aResult);
}

// TAKE / DROP: composes `api::array::planTakeDropSlice` with a
// MatrixOperand-level copy. When `bTake` is true the planner keeps the
// leading / trailing window named by `oRows` / `oColumns`; when false it
// drops that window and keeps the complement.
[[nodiscard]] inline RpnCoercionResult<MatrixOperand> planTake(
    const MatrixOperand& rSource, const std::optional<sal_Int32>& oRows,
    const std::optional<sal_Int32>& oColumns);

[[nodiscard]] inline RpnCoercionResult<MatrixOperand> planDrop(
    const MatrixOperand& rSource, const std::optional<sal_Int32>& oRows,
    const std::optional<sal_Int32>& oColumns);

namespace detail
{

[[nodiscard]] inline RpnCoercionResult<MatrixOperand> sliceMatrixOperand(
    const MatrixOperand& rSource, bool bTake,
    const std::optional<sal_Int32>& oRows,
    const std::optional<sal_Int32>& oColumns)
{
    if (rSource.isEmpty())
        return RpnCoercionResult<MatrixOperand>::failure(api::Error::IllegalArgument);

    const auto aSlice = api::array::planTakeDropSlice(
        rSource.maDimensions, bTake, oRows, oColumns);
    if (!aSlice)
        return RpnCoercionResult<MatrixOperand>::failure(aSlice.meError);

    MatrixOperand aResult;
    aResult.meProvenance = MatrixProvenance::ComputedResult;
    aResult.maDimensions = aSlice.maValue.maDimensions;
    aResult.maValues.resize(static_cast<std::size_t>(aResult.maDimensions.mnColumns)
                            * aResult.maDimensions.mnRows);
    for (api::MatrixSize r = 0; r < aResult.maDimensions.mnRows; ++r)
    {
        for (api::MatrixSize c = 0; c < aResult.maDimensions.mnColumns; ++c)
        {
            const std::size_t nSrc
                = static_cast<std::size_t>(aSlice.maValue.maStart.mnRow + r)
                      * rSource.maDimensions.mnColumns
                  + (aSlice.maValue.maStart.mnColumn + c);
            aResult.maValues[static_cast<std::size_t>(r) * aResult.maDimensions.mnColumns + c]
                = rSource.maValues[nSrc];
        }
    }
    return RpnCoercionResult<MatrixOperand>::success(aResult);
}

} // namespace detail

inline RpnCoercionResult<MatrixOperand> planTake(
    const MatrixOperand& rSource, const std::optional<sal_Int32>& oRows,
    const std::optional<sal_Int32>& oColumns)
{
    return detail::sliceMatrixOperand(rSource, /*bTake*/ true, oRows, oColumns);
}

inline RpnCoercionResult<MatrixOperand> planDrop(
    const MatrixOperand& rSource, const std::optional<sal_Int32>& oRows,
    const std::optional<sal_Int32>& oColumns)
{
    return detail::sliceMatrixOperand(rSource, /*bTake*/ false, oRows, oColumns);
}

} // namespace spreadsheetengine::core::rpn::spill

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
