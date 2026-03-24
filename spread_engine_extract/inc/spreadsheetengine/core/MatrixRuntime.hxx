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

#include <sal/types.h>

namespace spreadsheetengine::core::matrix
{

constexpr sal_uInt64 kAverageMatrixElementBytes = 12;
constexpr sal_uInt64 kArbitraryColumnCap = 128;

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
