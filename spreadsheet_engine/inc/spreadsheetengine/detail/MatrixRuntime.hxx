/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Matrix.hxx>

#include <spreadsheetengine/api/Types.hxx>

namespace spreadsheetengine::core::matrix
{

constexpr sal_uInt64 kAverageMatrixElementBytes = 12;
constexpr sal_uInt64 kArbitraryColumnCap = 128;
constexpr sal_uInt8 kEmptyResultFlagValue = 1;
constexpr sal_uInt8 kEmptyPathFlagValue = 2;

enum class StoredElementType : sal_uInt8
{
    Unknown,
    Empty,
    Numeric,
    Boolean,
    String
};

enum class StoredFlagType : sal_uInt8
{
    Unknown,
    Empty,
    Integer
};

enum class StoredEmptyKind : sal_uInt8
{
    Unknown,
    Cell,
    Result,
    Path
};

enum class AllocationFallback
{
    None,
    MatrixSize,
    StackOverflow
};

struct AllocationPlan
{
    api::MatrixDimensions maStorageDimensions {};
    AllocationFallback meFallback = AllocationFallback::None;

    [[nodiscard]] constexpr bool usesFallback() const
    {
        return meFallback != AllocationFallback::None;
    }
};

[[nodiscard]] constexpr bool isStoredStringOrEmpty(StoredElementType eType)
{
    return eType == StoredElementType::Empty || eType == StoredElementType::String;
}

[[nodiscard]] constexpr bool isStoredValue(StoredElementType eType)
{
    return eType == StoredElementType::Boolean || eType == StoredElementType::Numeric;
}

[[nodiscard]] constexpr bool isStoredValueOrEmpty(StoredElementType eType)
{
    return isStoredValue(eType) || eType == StoredElementType::Empty;
}

[[nodiscard]] constexpr bool isStoredBoolean(StoredElementType eType)
{
    return eType == StoredElementType::Boolean;
}

[[nodiscard]] constexpr bool isStoredEmptyCell(StoredElementType eType, StoredFlagType eFlagType)
{
    return eType == StoredElementType::Empty && eFlagType == StoredFlagType::Empty;
}

[[nodiscard]] constexpr StoredEmptyKind classifyStoredEmptyKind(
    StoredFlagType eFlagType, sal_uInt8 nFlagValue)
{
    if (eFlagType == StoredFlagType::Empty)
        return StoredEmptyKind::Cell;
    if (eFlagType != StoredFlagType::Integer)
        return StoredEmptyKind::Unknown;

    switch (nFlagValue)
    {
        case kEmptyResultFlagValue:
            return StoredEmptyKind::Result;
        case kEmptyPathFlagValue:
            return StoredEmptyKind::Path;
        default:
            break;
    }

    return StoredEmptyKind::Unknown;
}

[[nodiscard]] constexpr sal_uInt8 storedFlagValue(StoredEmptyKind eKind)
{
    switch (eKind)
    {
        case StoredEmptyKind::Result:
            return kEmptyResultFlagValue;
        case StoredEmptyKind::Path:
            return kEmptyPathFlagValue;
        case StoredEmptyKind::Cell:
        case StoredEmptyKind::Unknown:
            break;
    }

    return 0;
}

[[nodiscard]] constexpr bool isStoredEmptyResult(StoredElementType eType, sal_uInt8 nFlagValue)
{
    return eType == StoredElementType::Empty
           && classifyStoredEmptyKind(StoredFlagType::Integer, nFlagValue) == StoredEmptyKind::Result;
}

[[nodiscard]] constexpr bool isStoredEmptyPath(StoredElementType eType, sal_uInt8 nFlagValue)
{
    return eType == StoredElementType::Empty
           && classifyStoredEmptyKind(StoredFlagType::Integer, nFlagValue) == StoredEmptyKind::Path;
}

[[nodiscard]] constexpr bool isStoredLogicalEmpty(StoredElementType eType, sal_uInt8 nFlagValue)
{
    return eType == StoredElementType::Empty && !isStoredEmptyPath(eType, nFlagValue);
}

[[nodiscard]] constexpr api::MatrixValueType classifyStoredValueType(
    StoredElementType eType, StoredFlagType eFlagType, sal_uInt8 nFlagValue)
{
    switch (eType)
    {
        case StoredElementType::Boolean:
            return api::MatrixValueType::Boolean;
        case StoredElementType::Numeric:
            return api::MatrixValueType::Value;
        case StoredElementType::String:
            return api::MatrixValueType::Text;
        case StoredElementType::Empty:
            if (eFlagType == StoredFlagType::Empty)
                return api::MatrixValueType::Empty;
            return classifyStoredEmptyKind(eFlagType, nFlagValue) == StoredEmptyKind::Path
                       ? api::MatrixValueType::EmptyPath
                       : api::MatrixValueType::Empty;
        case StoredElementType::Unknown:
            break;
    }

    return api::MatrixValueType::Empty;
}

[[nodiscard]] constexpr sal_uInt64 defaultMemoryBudgetBytes(sal_uInt64 nPointerBytes)
{
    return nPointerBytes < 8 ? 0x40000000 : 0x180000000;
}

[[nodiscard]] constexpr sal_uInt64 elementsForMemoryBudget(
    sal_uInt64 nMemoryBytes, sal_uInt64 nBytesPerElement = kAverageMatrixElementBytes)
{
    return nBytesPerElement ? nMemoryBytes / nBytesPerElement : 0;
}

[[nodiscard]] constexpr sal_uInt64 cappedElementLimitForMemory(
    sal_uInt64 nMemoryBytes, sal_uInt64 nMaxRowCount,
    sal_uInt64 nColumnCap = kArbitraryColumnCap,
    sal_uInt64 nBytesPerElement = kAverageMatrixElementBytes)
{
    const sal_uInt64 nElementLimit = elementsForMemoryBudget(nMemoryBytes, nBytesPerElement);
    const sal_uInt64 nArbitraryLimit = nMaxRowCount * nColumnCap;
    return nElementLimit < nArbitraryLimit ? nElementLimit : nArbitraryLimit;
}

[[nodiscard]] constexpr sal_uInt64 defaultElementLimitForPlatform(
    sal_uInt64 nMaxRowCount, sal_uInt64 nPointerBytes)
{
    return cappedElementLimitForMemory(defaultMemoryBudgetBytes(nPointerBytes), nMaxRowCount);
}

[[nodiscard]] constexpr bool hasAllocatableShape(const api::MatrixDimensions& rDimensions)
{
    if (!rDimensions.isAllocated())
        return false;

    const bool bZeroColumns = rDimensions.mnColumns == 0;
    const bool bZeroRows = rDimensions.mnRows == 0;
    return bZeroColumns == bZeroRows;
}

[[nodiscard]] constexpr bool fitsWithinElementLimit(
    const api::MatrixDimensions& rDimensions, sal_uInt64 nElementLimit)
{
    if (!hasAllocatableShape(rDimensions))
        return false;
    if (rDimensions.isEmpty())
        return true;

    const sal_uInt64 nColumns = static_cast<sal_uInt64>(rDimensions.mnColumns);
    const sal_uInt64 nRows = static_cast<sal_uInt64>(rDimensions.mnRows);
    return nColumns <= (nElementLimit / nRows);
}

[[nodiscard]] constexpr AllocationPlan planAllocation(
    const api::MatrixDimensions& rRequestedDimensions, sal_uInt64 nElementLimit,
    AllocationFallback eFallbackOnFailure)
{
    if (fitsWithinElementLimit(rRequestedDimensions, nElementLimit))
        return { rRequestedDimensions, AllocationFallback::None };

    return { { 1, 1 }, eFallbackOnFailure };
}

[[nodiscard]] constexpr sal_uInt64 budgetWithReleasedCurrentElements(
    sal_uInt64 nRemainingElementBudget, sal_uInt64 nCurrentElementCount)
{
    return nRemainingElementBudget + nCurrentElementCount;
}

[[nodiscard]] constexpr sal_uInt64 budgetAfterAllocation(
    sal_uInt64 nAvailableElementBudget, const api::MatrixDimensions& rAllocatedDimensions)
{
    return nAvailableElementBudget - rAllocatedDimensions.elementCount();
}

[[nodiscard]] constexpr sal_uInt64 budgetAfterConstruction(
    sal_uInt64 nRemainingElementBudget, const api::MatrixDimensions& rAllocatedDimensions)
{
    return budgetAfterAllocation(nRemainingElementBudget, rAllocatedDimensions);
}

[[nodiscard]] constexpr sal_uInt64 budgetAfterDestruction(
    sal_uInt64 nRemainingElementBudget, const api::MatrixDimensions& rReleasedDimensions)
{
    return budgetWithReleasedCurrentElements(nRemainingElementBudget, rReleasedDimensions.elementCount());
}

[[nodiscard]] constexpr sal_uInt64 budgetAfterResize(
    sal_uInt64 nRemainingElementBudget, sal_uInt64 nCurrentElementCount,
    const api::MatrixDimensions& rAllocatedDimensions)
{
    return budgetAfterAllocation(
        budgetWithReleasedCurrentElements(nRemainingElementBudget, nCurrentElementCount),
        rAllocatedDimensions);
}

[[nodiscard]] constexpr AllocationPlan planResize(
    const api::MatrixDimensions& rRequestedDimensions, sal_uInt64 nCurrentElementCount,
    sal_uInt64 nRemainingElementBudget, AllocationFallback eFallbackOnFailure)
{
    return planAllocation(
        rRequestedDimensions,
        budgetWithReleasedCurrentElements(nRemainingElementBudget, nCurrentElementCount),
        eFallbackOnFailure);
}

[[nodiscard]] constexpr api::MatrixDimensions cloneDimensions(
    const api::MatrixDimensions& rSourceDimensions)
{
    return rSourceDimensions;
}

[[nodiscard]] constexpr api::MatrixDimensions extendedCloneDimensions(
    const api::MatrixDimensions& rSourceDimensions,
    const api::MatrixDimensions& rRequestedMinimumDimensions)
{
    return {
        rSourceDimensions.mnColumns >= rRequestedMinimumDimensions.mnColumns
            ? rSourceDimensions.mnColumns
            : rRequestedMinimumDimensions.mnColumns,
        rSourceDimensions.mnRows >= rRequestedMinimumDimensions.mnRows
            ? rSourceDimensions.mnRows
            : rRequestedMinimumDimensions.mnRows
    };
}

[[nodiscard]] constexpr bool canCopyIntoDestination(
    const api::MatrixDimensions& rSourceDimensions,
    const api::MatrixDimensions& rDestinationDimensions)
{
    return rSourceDimensions.mnColumns <= rDestinationDimensions.mnColumns
           && rSourceDimensions.mnRows <= rDestinationDimensions.mnRows;
}

} // namespace spreadsheetengine::core::matrix

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
