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

constexpr std::uint64_t kAverageMatrixElementBytes = 12;
constexpr std::uint64_t kArbitraryColumnCap = 128;
constexpr std::uint8_t kEmptyResultFlagValue = 1;
constexpr std::uint8_t kEmptyPathFlagValue = 2;

enum class StoredElementType : std::uint8_t
{
    Unknown,
    Empty,
    Numeric,
    Boolean,
    String
};

enum class StoredFlagType : std::uint8_t
{
    Unknown,
    Empty,
    Integer
};

enum class StoredEmptyKind : std::uint8_t
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
    StoredFlagType eFlagType, std::uint8_t nFlagValue)
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

[[nodiscard]] constexpr std::uint8_t storedFlagValue(StoredEmptyKind eKind)
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

[[nodiscard]] constexpr bool isStoredEmptyResult(StoredElementType eType, std::uint8_t nFlagValue)
{
    return eType == StoredElementType::Empty
           && classifyStoredEmptyKind(StoredFlagType::Integer, nFlagValue) == StoredEmptyKind::Result;
}

[[nodiscard]] constexpr bool isStoredEmptyPath(StoredElementType eType, std::uint8_t nFlagValue)
{
    return eType == StoredElementType::Empty
           && classifyStoredEmptyKind(StoredFlagType::Integer, nFlagValue) == StoredEmptyKind::Path;
}

[[nodiscard]] constexpr bool isStoredLogicalEmpty(StoredElementType eType, std::uint8_t nFlagValue)
{
    return eType == StoredElementType::Empty && !isStoredEmptyPath(eType, nFlagValue);
}

[[nodiscard]] constexpr api::MatrixValueType classifyStoredValueType(
    StoredElementType eType, StoredFlagType eFlagType, std::uint8_t nFlagValue)
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

[[nodiscard]] constexpr std::uint64_t defaultMemoryBudgetBytes(std::uint64_t nPointerBytes)
{
    return nPointerBytes < 8 ? 0x40000000 : 0x180000000;
}

[[nodiscard]] constexpr std::uint64_t elementsForMemoryBudget(
    std::uint64_t nMemoryBytes, std::uint64_t nBytesPerElement = kAverageMatrixElementBytes)
{
    return nBytesPerElement ? nMemoryBytes / nBytesPerElement : 0;
}

[[nodiscard]] constexpr std::uint64_t cappedElementLimitForMemory(
    std::uint64_t nMemoryBytes, std::uint64_t nMaxRowCount,
    std::uint64_t nColumnCap = kArbitraryColumnCap,
    std::uint64_t nBytesPerElement = kAverageMatrixElementBytes)
{
    const std::uint64_t nElementLimit = elementsForMemoryBudget(nMemoryBytes, nBytesPerElement);
    const std::uint64_t nArbitraryLimit = nMaxRowCount * nColumnCap;
    return nElementLimit < nArbitraryLimit ? nElementLimit : nArbitraryLimit;
}

[[nodiscard]] constexpr std::uint64_t defaultElementLimitForPlatform(
    std::uint64_t nMaxRowCount, std::uint64_t nPointerBytes)
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
    const api::MatrixDimensions& rDimensions, std::uint64_t nElementLimit)
{
    if (!hasAllocatableShape(rDimensions))
        return false;
    if (rDimensions.isEmpty())
        return true;

    const std::uint64_t nColumns = static_cast<std::uint64_t>(rDimensions.mnColumns);
    const std::uint64_t nRows = static_cast<std::uint64_t>(rDimensions.mnRows);
    return nColumns <= (nElementLimit / nRows);
}

[[nodiscard]] constexpr AllocationPlan planAllocation(
    const api::MatrixDimensions& rRequestedDimensions, std::uint64_t nElementLimit,
    AllocationFallback eFallbackOnFailure)
{
    if (fitsWithinElementLimit(rRequestedDimensions, nElementLimit))
        return { rRequestedDimensions, AllocationFallback::None };

    return { { 1, 1 }, eFallbackOnFailure };
}

[[nodiscard]] constexpr std::uint64_t budgetWithReleasedCurrentElements(
    std::uint64_t nRemainingElementBudget, std::uint64_t nCurrentElementCount)
{
    return nRemainingElementBudget + nCurrentElementCount;
}

[[nodiscard]] constexpr std::uint64_t budgetAfterAllocation(
    std::uint64_t nAvailableElementBudget, const api::MatrixDimensions& rAllocatedDimensions)
{
    return nAvailableElementBudget - rAllocatedDimensions.elementCount();
}

[[nodiscard]] constexpr std::uint64_t budgetAfterConstruction(
    std::uint64_t nRemainingElementBudget, const api::MatrixDimensions& rAllocatedDimensions)
{
    return budgetAfterAllocation(nRemainingElementBudget, rAllocatedDimensions);
}

[[nodiscard]] constexpr std::uint64_t budgetAfterDestruction(
    std::uint64_t nRemainingElementBudget, const api::MatrixDimensions& rReleasedDimensions)
{
    return budgetWithReleasedCurrentElements(nRemainingElementBudget, rReleasedDimensions.elementCount());
}

[[nodiscard]] constexpr std::uint64_t budgetAfterResize(
    std::uint64_t nRemainingElementBudget, std::uint64_t nCurrentElementCount,
    const api::MatrixDimensions& rAllocatedDimensions)
{
    return budgetAfterAllocation(
        budgetWithReleasedCurrentElements(nRemainingElementBudget, nCurrentElementCount),
        rAllocatedDimensions);
}

[[nodiscard]] constexpr AllocationPlan planResize(
    const api::MatrixDimensions& rRequestedDimensions, std::uint64_t nCurrentElementCount,
    std::uint64_t nRemainingElementBudget, AllocationFallback eFallbackOnFailure)
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
