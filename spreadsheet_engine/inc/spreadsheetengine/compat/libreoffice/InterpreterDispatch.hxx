/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/types.h>

namespace spreadsheetengine::compat::libreoffice::interpreterdispatch
{

enum class ComparisonMode : sal_uInt8
{
    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual
};

enum class LogicalFoldMode : sal_uInt8
{
    And,
    Or,
    Xor
};

enum class UnaryMatrixScalarMode : sal_uInt8
{
    Negate,
    LogicalNot
};

[[nodiscard]] constexpr bool matchesComparisonResult(short nCompareResult, ComparisonMode eMode)
{
    switch (eMode)
    {
        case ComparisonMode::Equal:
            return nCompareResult == 0;
        case ComparisonMode::NotEqual:
            return nCompareResult != 0;
        case ComparisonMode::Less:
            return nCompareResult < 0;
        case ComparisonMode::Greater:
            return nCompareResult > 0;
        case ComparisonMode::LessEqual:
            return nCompareResult <= 0;
        case ComparisonMode::GreaterEqual:
            return nCompareResult >= 0;
    }

    return false;
}

[[nodiscard]] constexpr bool initialLogicalFoldValue(LogicalFoldMode eMode)
{
    return eMode == LogicalFoldMode::And;
}

[[nodiscard]] constexpr bool foldLogicalValue(
    LogicalFoldMode eMode, bool bAccumulated, bool bValue)
{
    switch (eMode)
    {
        case LogicalFoldMode::And:
            return bAccumulated && bValue;
        case LogicalFoldMode::Or:
            return bAccumulated || bValue;
        case LogicalFoldMode::Xor:
            return bAccumulated != bValue;
    }

    return false;
}

} // namespace spreadsheetengine::compat::libreoffice::interpreterdispatch

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
