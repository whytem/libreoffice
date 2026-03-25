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
#include <type_traits>
#include <vector>

#include <spreadsheetengine/api/Host.hxx>

namespace spreadsheetengine::api::query
{

enum class Operator : sal_uInt8
{
    Equal,
    NotEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    Contains,
    DoesNotContain,
    BeginsWith,
    EndsWith,
    DoesNotBeginWith,
    DoesNotEndWith
};

enum class OperandKind : sal_uInt8
{
    Value,
    Text,
    Date,
    Empty,
    TextColor,
    BackgroundColor
};

enum class SearchType : sal_uInt8
{
    Normal,
    Wildcard,
    Regex
};

struct CellClass
{
    bool mbHasNumeric = false;
    bool mbHasString = false;
    bool mbFormulaError = false;

    [[nodiscard]] constexpr bool operator==(const CellClass& rOther) const = default;
};

using StringIdentity = const void*;

[[nodiscard]] constexpr bool isPartialTextMatchOp(Operator eOp)
{
    switch (eOp)
    {
        case Operator::Contains:
        case Operator::DoesNotContain:
        case Operator::BeginsWith:
        case Operator::EndsWith:
        case Operator::DoesNotBeginWith:
        case Operator::DoesNotEndWith:
            return true;
        default:
            return false;
    }
}

[[nodiscard]] constexpr bool isTextMatchOp(Operator eOp)
{
    if (isPartialTextMatchOp(eOp))
        return true;

    return eOp == Operator::Equal || eOp == Operator::NotEqual;
}

[[nodiscard]] constexpr bool isEndsWithOp(Operator eOp)
{
    return eOp == Operator::EndsWith || eOp == Operator::DoesNotEndWith;
}

[[nodiscard]] constexpr bool isMatchWholeCell(bool bDocMatchWholeCell, Operator eOp)
{
    return isPartialTextMatchOp(eOp) ? false : bDocMatchWholeCell;
}

[[nodiscard]] constexpr bool isRealWildOrRegExp(SearchType eSearchType, Operator eOp)
{
    return eSearchType != SearchType::Normal && isTextMatchOp(eOp);
}

[[nodiscard]] constexpr bool isTestWildOrRegExp(
    bool bHasTestEqualCondition, SearchType eSearchType, Operator eOp)
{
    return bHasTestEqualCondition && eSearchType != SearchType::Normal
           && (eOp == Operator::LessEqual || eOp == Operator::GreaterEqual);
}

[[nodiscard]] constexpr bool isQueryByValueForCell(const CellClass& rCell)
{
    return !rCell.mbFormulaError && rCell.mbHasNumeric;
}

[[nodiscard]] constexpr bool isQueryByValue(
    Operator eOp, OperandKind eType, const CellClass& rCell)
{
    if (eType == OperandKind::Text || isPartialTextMatchOp(eOp))
        return false;

    return isQueryByValueForCell(rCell);
}

[[nodiscard]] constexpr bool isQueryByString(
    Operator eOp, OperandKind eType, const CellClass& rCell)
{
    if (isTextMatchOp(eOp))
        return true;

    if (eType != OperandKind::Text)
        return false;

    return rCell.mbHasString;
}

[[nodiscard]] constexpr bool shouldTryMultiEqualityFastPath(
    Operator eOp, std::size_t nItemCount)
{
    return eOp == Operator::Equal && nItemCount >= 10;
}

[[nodiscard]] constexpr bool shouldUseFastStringEqualityPath(
    Operator eOp, bool bRealWildOrRegExp, bool bTestWildOrRegExp, bool bMatchWholeCell)
{
    return eOp == Operator::Equal && !bRealWildOrRegExp && !bTestWildOrRegExp
           && bMatchWholeCell;
}

[[nodiscard]] constexpr bool shouldUseExactStringEqualityPath(
    bool bFastPath, bool bMatchWholeCell)
{
    return bFastPath || bMatchWholeCell;
}

[[nodiscard]] constexpr bool isWholeCellSearchMatch(
    bool bMatchWholeCell, bool bMatch, sal_Int32 nStart, sal_Int32 nEnd, sal_Int32 nTextLength)
{
    return bMatch && (!bMatchWholeCell || (nStart == 0 && nEnd == nTextLength));
}

[[nodiscard]] constexpr bool evaluatePatternSearchMatch(
    Operator eOp, bool bMatch, sal_Int32 nStart, sal_Int32 nEnd, sal_Int32 nTextLength)
{
    switch (eOp)
    {
        case Operator::Equal:
        case Operator::Contains:
            return bMatch;
        case Operator::NotEqual:
        case Operator::DoesNotContain:
            return !bMatch;
        case Operator::BeginsWith:
            return bMatch && nStart == 0;
        case Operator::DoesNotBeginWith:
            return !(bMatch && nStart == 0);
        case Operator::EndsWith:
            return bMatch && nEnd == nTextLength;
        case Operator::DoesNotEndWith:
            return !(bMatch && nEnd == nTextLength);
        default:
            return false;
    }
}

[[nodiscard]] constexpr bool evaluateEqualityMatch(Operator eOp, bool bEqual)
{
    if (eOp == Operator::NotEqual)
        return !bEqual;

    return bEqual;
}

[[nodiscard]] constexpr bool evaluateSubstringMatch(Operator eOp, sal_Int32 nMatchPos)
{
    switch (eOp)
    {
        case Operator::Equal:
        case Operator::Contains:
            return nMatchPos != -1;
        case Operator::NotEqual:
        case Operator::DoesNotContain:
            return nMatchPos == -1;
        case Operator::BeginsWith:
            return nMatchPos == 0;
        case Operator::DoesNotBeginWith:
            return nMatchPos != 0;
        case Operator::EndsWith:
            return nMatchPos >= 0;
        case Operator::DoesNotEndWith:
            return nMatchPos < 0;
        default:
            return false;
    }
}

[[nodiscard]] constexpr bool shouldRejectAssignedEmptyStringQuery(
    OperandKind eType, bool bQueryStringEmpty)
{
    return eType != OperandKind::Text && bQueryStringEmpty;
}

[[nodiscard]] constexpr sal_Int32 computeSubstringSearchStart(
    Operator eOp, sal_Int32 nTextLength, sal_Int32 nPatternLength)
{
    return isEndsWithOp(eOp) ? (nTextLength - nPatternLength) : 0;
}

struct OrderedCompareResult
{
    bool mbMatch = false;
    bool mbEqual = false;

    [[nodiscard]] constexpr bool operator==(const OrderedCompareResult& rOther) const = default;
};

[[nodiscard]] constexpr OrderedCompareResult evaluateOrderedStringCompare(
    Operator eOp, sal_Int32 nCompare)
{
    switch (eOp)
    {
        case Operator::Less:
            return { nCompare < 0, false };
        case Operator::Greater:
            return { nCompare > 0, false };
        case Operator::LessEqual:
            return { nCompare <= 0, nCompare == 0 };
        case Operator::GreaterEqual:
            return { nCompare >= 0, nCompare == 0 };
        default:
            return {};
    }
}

[[nodiscard]] constexpr bool isRangeLookupStringOperand(OperandKind eType)
{
    return eType == OperandKind::Text;
}

[[nodiscard]] constexpr bool isRangeLookupComparisonSupported(
    Operator eOp, OperandKind eType)
{
    if (isRangeLookupStringOperand(eType))
        return eOp == Operator::Less || eOp == Operator::LessEqual;

    return eOp == Operator::Greater || eOp == Operator::GreaterEqual;
}

[[nodiscard]] constexpr bool evaluateRangeLookupMatch(
    Operator eOp, OperandKind eType, const CellClass& rCell)
{
    if (!isRangeLookupComparisonSupported(eOp, eType))
        return false;

    if (isRangeLookupStringOperand(eType))
    {
        if (rCell.mbFormulaError)
            return false;

        return rCell.mbHasNumeric;
    }

    return !rCell.mbHasNumeric;
}

[[nodiscard]] constexpr bool shouldUseSortedItemCache(std::size_t nItemCount)
{
    return nItemCount >= 100;
}

[[nodiscard]] constexpr bool shouldUseStringIdentityMultiEqualityFastPath(
    bool bFastCompareByString, Operator eOp, std::size_t nItemCount)
{
    return bFastCompareByString && shouldTryMultiEqualityFastPath(eOp, nItemCount);
}

[[nodiscard]] constexpr bool shouldCompareValueOperandAsString(const CellClass& rCell)
{
    return !isQueryByValueForCell(rCell);
}

[[nodiscard]] constexpr bool shouldIncludeOperandInStringIdentityCache(
    OperandKind eType, bool bCompareValueOperandAsString)
{
    return eType == OperandKind::Text
           || (bCompareValueOperandAsString && eType == OperandKind::Value);
}

template <typename Iterator, typename Predicate, typename Projection>
[[nodiscard]] inline std::vector<double> collectSortedNumericValues(
    Iterator itBegin, Iterator itEnd, Predicate aPredicate, Projection aProjection)
{
    std::vector<double> aValues;
    for (auto it = itBegin; it != itEnd; ++it)
    {
        if (aPredicate(*it))
            aValues.push_back(static_cast<double>(aProjection(*it)));
    }

    std::sort(aValues.begin(), aValues.end());
    return aValues;
}

template <typename Iterator, typename Predicate, typename Projection>
[[nodiscard]] inline bool containsLinearNumericValue(
    Iterator itBegin, Iterator itEnd, double fValue, Predicate aPredicate, Projection aProjection)
{
    for (auto it = itBegin; it != itEnd; ++it)
    {
        if (aPredicate(*it) && static_cast<double>(aProjection(*it)) == fValue)
            return true;
    }

    return false;
}

[[nodiscard]] inline bool containsSortedNumericValue(
    const std::vector<double>& rValues, double fValue)
{
    const auto it = std::lower_bound(rValues.begin(), rValues.end(), fValue);
    return it != rValues.end() && *it == fValue;
}

template <typename Iterator, typename Predicate, typename Projection>
[[nodiscard]] inline auto collectSortedStringIdentities(
    Iterator itBegin, Iterator itEnd, Predicate aPredicate, Projection aProjection)
{
    using Identity = std::decay_t<decltype(aProjection(*itBegin))>;
    std::vector<Identity> aValues;
    for (auto it = itBegin; it != itEnd; ++it)
    {
        if (aPredicate(*it))
            aValues.push_back(aProjection(*it));
    }

    std::sort(aValues.begin(), aValues.end());
    return aValues;
}

template <typename Iterator, typename Identity, typename Predicate, typename Projection>
[[nodiscard]] inline bool containsLinearStringIdentity(
    Iterator itBegin, Iterator itEnd, Identity pIdentity, Predicate aPredicate, Projection aProjection)
{
    for (auto it = itBegin; it != itEnd; ++it)
    {
        if (aPredicate(*it) && aProjection(*it) == pIdentity)
            return true;
    }

    return false;
}

template <typename Identity>
[[nodiscard]] inline bool containsSortedStringIdentity(
    const std::vector<Identity>& rValues, Identity pIdentity)
{
    const auto it = std::lower_bound(rValues.begin(), rValues.end(), pIdentity);
    return it != rValues.end() && *it == pIdentity;
}

} // namespace spreadsheetengine::api::query

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
