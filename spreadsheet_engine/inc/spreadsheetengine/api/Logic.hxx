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

#include <spreadsheetengine/runtime/FloatingPoint.hxx>
#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/Error.hxx>

namespace spreadsheetengine::api::logic
{

enum class IfBranchAction
{
    PropagateError,
    ThenPath,
    ElsePath,
    ReturnTrue,
    ReturnFalse
};

inline IfBranchAction selectIfBranch(
    bool bCondition, bool bConditionError, bool bHasThenPath, bool bHasElsePath)
{
    if (bConditionError)
        return IfBranchAction::PropagateError;

    if (bCondition)
        return bHasThenPath ? IfBranchAction::ThenPath : IfBranchAction::ReturnTrue;

    return bHasElsePath ? IfBranchAction::ElsePath : IfBranchAction::ReturnFalse;
}

inline bool matchesIfErrorPolicy(api::Error eError, bool bNAonly)
{
    if (eError == api::Error::None)
        return false;

    return !bNAonly || eError == api::Error::NotAvailable;
}

enum class IfErrorAction
{
    KeepPrimary,
    EvaluateAlternate
};

inline IfErrorAction selectIfErrorAction(api::Error eError, bool bNAonly)
{
    return matchesIfErrorPolicy(eError, bNAonly) ? IfErrorAction::EvaluateAlternate
                                                 : IfErrorAction::KeepPrimary;
}

inline std::optional<sal_Int16> normalizeChooseIndex(double fValue, sal_Int16 nJumpCount)
{
    if (!std::isfinite(fValue))
        return std::nullopt;

    const double fFloor = spreadsheetengine::core::fp::approxFloor(fValue);
    if (fFloor < 1 || fFloor >= nJumpCount)
        return std::nullopt;

    return static_cast<sal_Int16>(fFloor);
}

inline api::ValueResult<sal_Int16> chooseJumpIndex(sal_Int16 nIndex, sal_Int16 nJumpCount)
{
    if (nIndex >= 1 && nIndex < nJumpCount)
        return api::ValueResult<sal_Int16>::success(nIndex);

    return api::ValueResult<sal_Int16>::failure(api::Error::IllegalArgument);
}

enum class IfsAction
{
    SelectCurrentResult,
    SkipCurrentResult,
    ReturnNotAvailable,
    ReturnParameterExpected,
    ReturnNoValue
};

inline IfsAction evaluateIfsCondition(
    bool bCondition, bool bConditionError, sal_Int16 nRemainingParamsAfterCondition)
{
    if (bConditionError)
        return IfsAction::ReturnNoValue;

    if (bCondition)
    {
        return nRemainingParamsAfterCondition >= 1 ? IfsAction::SelectCurrentResult
                                                   : IfsAction::ReturnParameterExpected;
    }

    return nRemainingParamsAfterCondition >= 3 ? IfsAction::SkipCurrentResult
                                               : IfsAction::ReturnNotAvailable;
}

} // namespace spreadsheetengine::api::logic

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
