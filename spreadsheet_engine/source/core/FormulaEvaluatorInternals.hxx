/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/detail/FormulaEvaluator.hxx>

#include <spreadsheetengine/api/Array.hxx>
#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/api/Lookup.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Query.hxx>
#include <spreadsheetengine/api/Text.hxx>
#include <spreadsheetengine/api/Workday.hxx>
#include <spreadsheetengine/runtime/ConversionRuntime.hxx>
#include <spreadsheetengine/runtime/DateTimeParse.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>
#include <spreadsheetengine/runtime/FinancialRuntime.hxx>
#include <spreadsheetengine/runtime/FloatingPoint.hxx>
#include <spreadsheetengine/runtime/LookupRuntime.hxx>
#include <spreadsheetengine/runtime/MathAggregate.hxx>
#include <spreadsheetengine/runtime/MathFunctionRuntime.hxx>
#include <spreadsheetengine/runtime/MathRounding.hxx>
#include <spreadsheetengine/runtime/MathStatistical.hxx>
#include <spreadsheetengine/runtime/QueryRuntime.hxx>
#include <spreadsheetengine/runtime/KahanSum.hxx>
#include <spreadsheetengine/runtime/TextCase.hxx>
#include <spreadsheetengine/runtime/TextFunctionRuntime.hxx>
#include <spreadsheetengine/runtime/TextRuntimeSupport.hxx>
#include <spreadsheetengine/runtime/TextScalar.hxx>

#include "DateAlgorithms.hxx"
#include "FormulaEvaluatorUtils.hxx"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <numeric>
#include <optional>
#include <vector>

namespace spreadsheetengine::core::eval
{

namespace seconvert = spreadsheetengine::core::convert;
namespace sedate = spreadsheetengine::core::detail::date;
namespace sedatetime = spreadsheetengine::core::datetime;
namespace sefinance = spreadsheetengine::core::finance;
namespace semath = spreadsheetengine::core::math;
namespace selookup = spreadsheetengine::core::lookup;
namespace sequery = spreadsheetengine::core::query;
namespace setext = spreadsheetengine::core::text;
namespace seutil = spreadsheetengine::core::util;

using detail::coerceToBoolean;
using detail::coerceToNumber;
using detail::coerceToString;
using detail::ensureScalarValue;
using detail::formatNumber;
using detail::formatQuotedString;
using detail::hasFunctionPrefix;
using detail::makeFailure;
using detail::makeReferenceResult;
using detail::makeScalarResult;
using detail::normalizeDisplayFunctionName;
using detail::normalizeFunctionName;
using detail::parseAsciiDouble;
using detail::toWholeNumber;
using detail::uppercaseAscii;

using AggregateOptions = semath::AggregateOptions;
using AggregateScan = semath::AggregateScan;
using CriteriaAggregateInput = sequery::CriteriaAggregateInput;
using CriteriaAggregateKind = sequery::CriteriaAggregateKind;
using CriteriaPredicate = sequery::CriteriaPredicate;
using LookupInput = selookup::LookupInput;

[[nodiscard]] inline bool needsQuotedSheetName(api::StringView rSheetName)
{
    if (rSheetName.empty())
        return false;

    for (const char16_t cChar : rSheetName)
    {
        const bool bAlphaNum = (cChar >= u'0' && cChar <= u'9')
                               || (cChar >= u'A' && cChar <= u'Z')
                               || (cChar >= u'a' && cChar <= u'z') || cChar == u'_';
        if (!bAlphaNum)
            return true;
    }

    return false;
}

[[nodiscard]] inline api::String quoteSheetNameForFormula(api::StringView rSheetName)
{
    if (!needsQuotedSheetName(rSheetName))
        return api::String(rSheetName);

    api::String aQuoted;
    aQuoted.reserve(rSheetName.size() + 2);
    aQuoted.push_back(u'\'');
    for (const char16_t cChar : rSheetName)
    {
        if (cChar == u'\'')
            aQuoted.push_back(u'\'');
        aQuoted.push_back(cChar);
    }
    aQuoted.push_back(u'\'');
    return aQuoted;
}

[[nodiscard]] inline api::String columnNameFromIndex(api::ColumnIndex nColumn)
{
    api::String aName;
    api::ColumnIndex nCurrent = nColumn;
    do
    {
        const api::ColumnIndex nRemainder = nCurrent % 26;
        aName.insert(aName.begin(), static_cast<char16_t>(u'A' + nRemainder));
        nCurrent = (nCurrent / 26) - 1;
    } while (nCurrent >= 0);
    return aName;
}

[[nodiscard]] inline api::String formatAddressFunctionResult(api::RowIndex nRow,
    api::ColumnIndex nColumn, std::int32_t nAbsMode, bool bA1Style, api::StringView rSheetName)
{
    api::String aResult;
    if (!rSheetName.empty())
    {
        aResult = quoteSheetNameForFormula(rSheetName);
        aResult.push_back(bA1Style ? u'.' : u'!');
    }

    const bool bRowAbsolute = nAbsMode == 1 || nAbsMode == 2;
    const bool bColumnAbsolute = nAbsMode == 1 || nAbsMode == 3;
    if (bA1Style)
    {
        if (bColumnAbsolute)
            aResult.push_back(u'$');
        aResult += columnNameFromIndex(nColumn);
        if (bRowAbsolute)
            aResult.push_back(u'$');
        aResult += formatNumber(static_cast<double>(nRow + 1));
        return aResult;
    }

    aResult.push_back(u'R');
    if (bRowAbsolute)
    {
        aResult += formatNumber(static_cast<double>(nRow + 1));
    }
    else
    {
        aResult.push_back(u'[');
        aResult += formatNumber(static_cast<double>(nRow + 1));
        aResult.push_back(u']');
    }

    aResult.push_back(u'C');
    if (bColumnAbsolute)
    {
        aResult += formatNumber(static_cast<double>(nColumn + 1));
    }
    else
    {
        aResult.push_back(u'[');
        aResult += formatNumber(static_cast<double>(nColumn + 1));
        aResult.push_back(u']');
    }

    return aResult;
}

[[nodiscard]] inline api::query::SearchType toQuerySearchType(
    workbook::FormulaSearchType eSearchType)
{
    switch (eSearchType)
    {
        case workbook::FormulaSearchType::Normal:
            return api::query::SearchType::Normal;
        case workbook::FormulaSearchType::Wildcard:
            return api::query::SearchType::Wildcard;
        case workbook::FormulaSearchType::Regex:
            return api::query::SearchType::Regex;
    }

    return api::query::SearchType::Normal;
}

[[nodiscard]] inline bool usesMicrosoftCompatibilityName(api::StringView rName)
{
    return hasFunctionPrefix(rName, u"COM.MICROSOFT.");
}

[[nodiscard]] inline std::optional<std::int16_t> classifyOdfErrorTypeLiteral(api::StringView rText)
{
    const api::String aUpper = uppercaseAscii(rText);
    if (aUpper == u"#NULL!")
        return 1;
    if (aUpper == u"#DIV/0!")
        return 2;
    if (aUpper == u"#VALUE!")
        return 3;
    if (aUpper == u"#REF!")
        return 4;
    if (aUpper == u"#NAME?")
        return 5;
    if (aUpper == u"#NUM!")
        return 6;
    if (aUpper == u"#N/A")
        return 7;
    if (aUpper == u"#GETTING_DATA")
        return std::nullopt;
    if (aUpper.starts_with(u"#ERR") && aUpper.size() > 5 && aUpper.back() == u'!')
        return std::nullopt;
    if (!rText.empty() && rText.front() == u'#')
        return 5;
    return std::nullopt;
}

[[nodiscard]] inline std::optional<std::int16_t> classifyOdfErrorType(api::Error eError)
{
    switch (eError)
    {
        case api::Error::DivisionByZero:
            return 2;
        case api::Error::NoValue:
            return 3;
        case api::Error::NoConvergence:
        case api::Error::Domain:
            return 6;
        case api::Error::NotAvailable:
            return 7;
        default:
            return std::nullopt;
    }
}

[[nodiscard]] inline std::optional<std::int32_t> classifyLegacyErrorTypeLiteral(
    api::StringView rText)
{
    const api::String aUpper = uppercaseAscii(rText);
    if (aUpper == u"#NULL!")
        return 521;
    if (aUpper == u"#DIV/0!")
        return 532;
    if (aUpper == u"#VALUE!")
        return 519;
    if (aUpper == u"#REF!")
        return 524;
    if (aUpper == u"#NAME?")
        return 525;
    if (aUpper == u"#NUM!")
        return 503;
    if (aUpper == u"#N/A")
        return 32767;
    if (aUpper == u"#GETTING_DATA")
        return 508;

    if (aUpper.starts_with(u"#ERR") && aUpper.size() > 5 && aUpper.back() == u'!')
    {
        const api::StringView aDigits = aUpper.substr(4, aUpper.size() - 5);
        std::int32_t nCode = 0;
        for (const char16_t cChar : aDigits)
        {
            if (cChar < u'0' || cChar > u'9')
                return std::nullopt;
            nCode = nCode * 10 + static_cast<std::int32_t>(cChar - u'0');
        }
        if (nCode > 0)
            return nCode;
        return std::nullopt;
    }

    const std::size_t nColon = aUpper.rfind(u':');
    if (nColon != api::StringView::npos && nColon + 1 < aUpper.size())
    {
        std::int32_t nCode = 0;
        for (const char16_t cChar : aUpper.substr(nColon + 1))
        {
            if (cChar < u'0' || cChar > u'9')
                return std::nullopt;
            nCode = nCode * 10 + static_cast<std::int32_t>(cChar - u'0');
        }
        if (nCode >= 500 || nCode == 32767)
            return nCode;
        return 525;
    }

    if (!rText.empty() && rText.front() == u'#')
        return 525;
    return std::nullopt;
}

[[nodiscard]] inline std::optional<std::int32_t> classifyLegacyErrorType(api::Error eError)
{
    switch (eError)
    {
        case api::Error::IllegalArgument:
            return 502;
        case api::Error::DivisionByZero:
            return 532;
        case api::Error::StringOverflow:
            return 513;
        case api::Error::NoValue:
            return 519;
        case api::Error::NoConvergence:
        case api::Error::Domain:
            return 503;
        case api::Error::NotAvailable:
            return 32767;
        default:
            return std::nullopt;
    }
}

[[nodiscard]] inline std::optional<int> weekdayIndexForFodsDate(api::DateSerial nDate)
{
    const auto aWeekday = api::calendar::dayOfWeek(sedatetime::defaultNullDate(), nDate, 2);
    if (!aWeekday || aWeekday.maValue < 1 || aWeekday.maValue > 7)
        return std::nullopt;
    return aWeekday.maValue - 1;
}

[[nodiscard]] inline bool isWeekendFodsDate(
    api::DateSerial nDate, const api::WeekendMask& rWeekendMask)
{
    const auto oWeekdayIndex = weekdayIndexForFodsDate(nDate);
    return oWeekdayIndex && rWeekendMask[static_cast<std::size_t>(*oWeekdayIndex)];
}

[[nodiscard]] inline bool isLiteralArrayWeekendNode(const formula::Node& rNode)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::NumberLiteral:
        case formula::NodeKind::StringLiteral:
        case formula::NodeKind::BooleanLiteral:
        case formula::NodeKind::EmptyArgument:
            return true;
        case formula::NodeKind::UnaryOperation:
            return rNode.maChildren.size() == 1 && isLiteralArrayWeekendNode(*rNode.maChildren[0]);
        default:
            return false;
    }
}

[[nodiscard]] inline bool isHolidayFodsDate(
    api::DateSerial nDate, const std::vector<api::DateSerial>& rSortedHolidays)
{
    return std::binary_search(rSortedHolidays.begin(), rSortedHolidays.end(), nDate);
}

[[nodiscard]] inline api::DateSerial countWorkdaysFods(api::DateSerial nDate1, api::DateSerial nDate2,
    const std::vector<api::DateSerial>& rSortedHolidays, const api::WeekendMask& rWeekendMask)
{
    std::int32_t nCount = 0;
    const bool bReverse = nDate1 > nDate2;
    if (bReverse)
        std::swap(nDate1, nDate2);

    while (nDate1 <= nDate2)
    {
        if (!isWeekendFodsDate(nDate1, rWeekendMask)
            && !isHolidayFodsDate(nDate1, rSortedHolidays))
        {
            ++nCount;
        }
        ++nDate1;
    }

    return bReverse ? -nCount : nCount;
}

[[nodiscard]] inline api::DateSerial advanceWorkdayFods(api::DateSerial nDate, api::DateSerial nDays,
    const std::vector<api::DateSerial>& rSortedHolidays, const api::WeekendMask& rWeekendMask)
{
    if (!nDays)
        return nDate;

    if (nDays > 0)
    {
        while (nDays)
        {
            do
            {
                ++nDate;
            } while (isWeekendFodsDate(nDate, rWeekendMask));

            if (!isHolidayFodsDate(nDate, rSortedHolidays))
                --nDays;
        }
    }
    else
    {
        while (nDays)
        {
            do
            {
                --nDate;
            } while (isWeekendFodsDate(nDate, rWeekendMask));

            if (!isHolidayFodsDate(nDate, rSortedHolidays))
                ++nDays;
        }
    }

    return nDate;
}

[[nodiscard]] inline api::String formatBasisDateTime(double fSerialValue)
{
    const api::DateParts aNullDate = sedatetime::defaultNullDate();
    const api::DateSerial nDateSerial = static_cast<api::DateSerial>(fp::approxFloor(fSerialValue));
    const double fTimeValue = sedatetime::normalizeTimeFraction(fSerialValue);

    const std::int16_t nYear = static_cast<std::int16_t>(sedatetime::extractYear(aNullDate, nDateSerial));
    const std::int16_t nMonth = static_cast<std::int16_t>(sedatetime::extractMonth(aNullDate, nDateSerial));
    const auto oDay = sedatetime::extractDay(aNullDate, nDateSerial);
    const std::int16_t nDay = static_cast<std::int16_t>(oDay.value_or(0.0));
    const std::int16_t nHour = static_cast<std::int16_t>(sedatetime::extractHour(fTimeValue));
    const std::int16_t nMinute = static_cast<std::int16_t>(sedatetime::extractMinute(fTimeValue));
    const std::int16_t nSecond = static_cast<std::int16_t>(sedatetime::extractSecond(fTimeValue));

    char aBuffer[32];
    const int nLength = std::snprintf(aBuffer, sizeof(aBuffer), "%04d-%02d-%02d %02d:%02d:%02d",
        static_cast<int>(nYear), static_cast<int>(nMonth), static_cast<int>(nDay),
        static_cast<int>(nHour), static_cast<int>(nMinute), static_cast<int>(nSecond));

    api::String aResult;
    aResult.reserve(static_cast<std::size_t>(std::max(nLength, 0)));
    for (int i = 0; i < nLength; ++i)
        aResult.push_back(static_cast<char16_t>(aBuffer[i]));
    return aResult;
}

[[nodiscard]] inline std::optional<LookupInput> makeLookupInput(const EvaluationResult& rResult)
{
    if (!rResult)
        return std::nullopt;

    LookupInput aInput;
    if (rResult.maValue.isScalar())
    {
        aInput.maScalar = rResult.maValue.maValue;
        return aInput;
    }

    aInput.mbScalar = false;
    aInput.maReference = rResult.maValue.maReference;
    const auto aDimensions = rResult.maValue.maReference.matrixDimensions();
    aInput.mnColumns = aDimensions.mnColumns;
    aInput.mnRows = aDimensions.mnRows;
    return aInput;
}

[[nodiscard]] inline LookupInput makeLookupScalarError(api::Error eError)
{
    LookupInput aInput;
    aInput.maScalar = api::CellValue::error(eError);
    return aInput;
}

[[nodiscard]] inline std::optional<CriteriaAggregateInput> makeCriteriaAggregateInput(
    const EvaluationResult& rResult)
{
    if (!rResult)
        return std::nullopt;

    CriteriaAggregateInput aInput;
    if (rResult.maValue.isScalar())
    {
        aInput.maScalar = rResult.maValue.maValue;
        return aInput;
    }

    aInput.mbScalar = false;
    aInput.maReference = rResult.maValue.maReference;
    const auto aDimensions = rResult.maValue.maReference.matrixDimensions();
    aInput.mnColumns = aDimensions.mnColumns;
    aInput.mnRows = aDimensions.mnRows;
    return aInput;
}

struct Evaluator::FunctionEvalContext
{
    Evaluator& mrEvaluator;
    const formula::Node& mrNode;
    const api::CellAddress& mrCurrentAddress;

    template <typename Visitor>
    [[nodiscard]] api::ValueResult<bool> visitFlattenedValues(
        const formula::Node& rArgument, const Visitor& rVisitor) const
    {
        if (rArgument.meKind == formula::NodeKind::ArrayConstant
            || rArgument.meKind == formula::NodeKind::ReferenceList)
        {
            for (const auto& pChild : rArgument.maChildren)
            {
                const auto aChild = visitFlattenedValues(*pChild, rVisitor);
                if (!aChild)
                    return aChild;
            }
            return api::ValueResult<bool>::success(true);
        }

        const bool bReferenceLike = rArgument.meKind == formula::NodeKind::CellReference
                                    || rArgument.meKind == formula::NodeKind::RangeReference
                                    || rArgument.meKind == formula::NodeKind::NamedReference;

        EvaluationResult aValue = bReferenceLike
                                      ? mrEvaluator.evaluateReferenceNode(rArgument, mrCurrentAddress)
                                      : mrEvaluator.evaluateNode(rArgument, mrCurrentAddress);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);

        if (aValue.maValue.isMatrixReference())
        {
            const auto& rReference = aValue.maValue.maReference;
            for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
            {
                for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                {
                    EvaluationResult aCell
                        = mrEvaluator.materializeReferenceValue(rReference, nCol, nRow);
                    if (!aCell)
                        return api::ValueResult<bool>::failure(aCell.meError);

                    const auto aVisited = rVisitor(aCell.maValue.maValue, true);
                    if (!aVisited)
                        return aVisited;
                }
            }

            return api::ValueResult<bool>::success(true);
        }

        return rVisitor(aValue.maValue.maValue, bReferenceLike);
    }

    [[nodiscard]] api::ValueResult<std::vector<double>> collectNumericArguments(
        bool bIgnoreTextAndEmptyFromReferences, bool bTreatScalarEmptyAsZero = false) const
    {
        std::vector<double> aNumbers;
        for (const auto& pChild : mrNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                *pChild,
                [&](const api::CellValue& rValue,
                    bool bFromReference) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);

                    if (bFromReference && bIgnoreTextAndEmptyFromReferences
                        && (rValue.isEmpty() || rValue.isText()))
                    {
                        return api::ValueResult<bool>::success(true);
                    }

                    if (rValue.isEmpty())
                    {
                        if (bTreatScalarEmptyAsZero)
                            aNumbers.push_back(0.0);
                        return api::ValueResult<bool>::success(true);
                    }

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber)
                    {
                        if (bFromReference && bIgnoreTextAndEmptyFromReferences
                            && rValue.isText())
                        {
                            return api::ValueResult<bool>::success(true);
                        }

                        return api::ValueResult<bool>::failure(aNumber.meError);
                    }

                    aNumbers.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        }

        return api::ValueResult<std::vector<double>>::success(std::move(aNumbers));
    }

    [[nodiscard]] api::ValueResult<std::vector<double>> collectVarianceArguments(
        bool bTextAsZero) const
    {
        std::vector<double> aNumbers;
        for (const auto& pChild : mrNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                *pChild,
                [&](const api::CellValue& rValue,
                    bool bFromReference) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);

                    if (rValue.isEmpty())
                        return api::ValueResult<bool>::success(true);

                    if (rValue.isText())
                    {
                        if (bTextAsZero)
                        {
                            aNumbers.push_back(0.0);
                            return api::ValueResult<bool>::success(true);
                        }

                        if (bFromReference)
                            return api::ValueResult<bool>::success(true);
                        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
                    }

                    if (rValue.isBoolean() && bFromReference && !bTextAsZero)
                        return api::ValueResult<bool>::success(true);

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber)
                        return api::ValueResult<bool>::failure(aNumber.meError);
                    aNumbers.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        }

        return api::ValueResult<std::vector<double>>::success(std::move(aNumbers));
    }

    [[nodiscard]] api::ValueResult<api::CellValue> evaluateScalarArgumentValue(
        const formula::Node& rArgument) const
    {
        EvaluationResult aValue
            = ensureScalarValue(mrEvaluator, mrEvaluator.evaluateNode(rArgument, mrCurrentAddress));
        if (!aValue)
            return api::ValueResult<api::CellValue>::failure(aValue.meError);
        return api::ValueResult<api::CellValue>::success(aValue.maValue.maValue);
    }

    [[nodiscard]] api::ValueResult<api::CellValue> evaluateAnchoredScalarArgumentValue(
        const formula::Node& rArgument) const
    {
        EvaluationResult aValue = mrEvaluator.evaluateNode(rArgument, mrCurrentAddress);
        if (!aValue)
            return api::ValueResult<api::CellValue>::failure(aValue.meError);
        if (aValue.maValue.isMatrixReference())
            aValue = mrEvaluator.materializeReferenceValue(aValue.maValue.maReference, 0, 0);
        if (!aValue)
            return api::ValueResult<api::CellValue>::failure(aValue.meError);
        if (!aValue.maValue.isScalar())
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::CellValue>::success(aValue.maValue.maValue);
    }

    [[nodiscard]] api::ValueResult<double> evaluateNumericArgument(
        const formula::Node& rArgument, std::optional<double> oDefaultForEmpty) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument && oDefaultForEmpty)
            return api::ValueResult<double>::success(*oDefaultForEmpty);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<double>::failure(aNumber.meError);
        return aNumber;
    }

    [[nodiscard]] api::ValueResult<double> evaluateAnchoredNumericArgument(
        const formula::Node& rArgument, std::optional<double> oDefaultForEmpty) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument && oDefaultForEmpty)
            return api::ValueResult<double>::success(*oDefaultForEmpty);

        const auto aValue = evaluateAnchoredScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);
        if (aValue.maValue.isEmpty() && oDefaultForEmpty)
            return api::ValueResult<double>::success(*oDefaultForEmpty);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<double>::failure(aNumber.meError);
        return aNumber;
    }

    [[nodiscard]] api::ValueResult<double> evaluateRequiredNumberArgument(
        const formula::Node& rArgument) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        return coerceToNumber(aValue.maValue);
    }

    [[nodiscard]] api::ValueResult<double> evaluateRequiredAnchoredNumberArgument(
        const formula::Node& rArgument) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateAnchoredScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        return coerceToNumber(aValue.maValue);
    }

    [[nodiscard]] api::ValueResult<api::DateSerial> evaluateRequiredDateArgument(
        const formula::Node& rArgument) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<api::DateSerial>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<api::DateSerial>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<api::DateSerial>::failure(api::Error::IllegalArgument);

        const auto oDateSerial = sedatetime::coerceToDateSerial(aValue.maValue);
        if (!oDateSerial)
            return api::ValueResult<api::DateSerial>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::DateSerial>::success(*oDateSerial);
    }

    [[nodiscard]] api::ValueResult<std::int32_t> evaluateOptionalWholeNumberArgument(
        const formula::Node& rArgument, std::int32_t nDefaultValue) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<std::int32_t>::success(nDefaultValue);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<std::int32_t>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<std::int32_t>::success(nDefaultValue);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<std::int32_t>::failure(aNumber.meError);

        const auto oWhole = toWholeNumber(aNumber.maValue);
        if (!oWhole)
            return api::ValueResult<std::int32_t>::failure(api::Error::IllegalArgument);
        return api::ValueResult<std::int32_t>::success(*oWhole);
    }

    [[nodiscard]] api::ValueResult<std::int32_t> evaluateRequiredWholeNumberArgument(
        const formula::Node& rArgument) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<std::int32_t>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<std::int32_t>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<std::int32_t>::failure(api::Error::IllegalArgument);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<std::int32_t>::failure(aNumber.meError);

        const auto oWhole = toWholeNumber(aNumber.maValue);
        if (!oWhole)
            return api::ValueResult<std::int32_t>::failure(api::Error::IllegalArgument);
        return api::ValueResult<std::int32_t>::success(*oWhole);
    }

    [[nodiscard]] EvaluationResult makeNumericOrErrorResult(api::ValueResult<double> aValue) const
    {
        if (!aValue)
            return makeScalarResult(api::CellValue::error(aValue.meError));
        return makeScalarResult(api::CellValue::number(aValue.maValue));
    }

    [[nodiscard]] api::ValueResult<AggregateScan> collectAggregateScanFromArgument(
        const formula::Node& rArgument) const
    {
        AggregateScan aScan;
        const auto aVisited = visitFlattenedValues(
            rArgument, [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isNumber())
                    aScan.maNumbers.push_back(rValue.mfNumber);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<AggregateScan>::failure(aVisited.meError);
        return api::ValueResult<AggregateScan>::success(std::move(aScan));
    }

    [[nodiscard]] api::ValueResult<std::vector<double>> collectExtremaArguments() const
    {
        std::vector<double> aNumbers;
        for (const auto& pChild : mrNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                *pChild,
                [&](const api::CellValue& rValue,
                    bool bFromReference) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);

                    if (bFromReference)
                    {
                        if (rValue.isNumber())
                            aNumbers.push_back(rValue.mfNumber);
                        return api::ValueResult<bool>::success(true);
                    }

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber)
                        return api::ValueResult<bool>::failure(aNumber.meError);
                    aNumbers.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<std::vector<double>>::failure(aVisited.meError);
        }

        return api::ValueResult<std::vector<double>>::success(std::move(aNumbers));
    }

    [[nodiscard]] api::ValueResult<LookupInput> evaluateLookupInputNode(
        const formula::Node& rLookupNode) const
    {
        auto evaluateMatrixOperand = [&](const formula::Node& rOperand)
            -> api::ValueResult<LookupInput> {
            EvaluationResult aValue = mrEvaluator.evaluateNode(rOperand, mrCurrentAddress);
            if (!aValue)
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(aValue.meError));

            if (aValue.maValue.isScalar())
            {
                const auto aNumber = coerceToNumber(aValue.maValue.maValue);
                if (!aNumber)
                    return api::ValueResult<LookupInput>::success(
                        makeLookupScalarError(aNumber.meError));

                LookupInput aInput;
                aInput.maScalar = api::CellValue::number(aNumber.maValue);
                return api::ValueResult<LookupInput>::success(aInput);
            }

            LookupInput aInput;
            aInput.mbScalar = false;
            const auto aDimensions = aValue.maValue.maReference.matrixDimensions();
            aInput.mnColumns = aDimensions.mnColumns;
            aInput.mnRows = aDimensions.mnRows;
            aInput.maValues.reserve(
                static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnColumns, 0))
                * static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnRows, 0)));
            for (api::MatrixSize nRow = 0; nRow < aInput.mnRows; ++nRow)
            {
                for (api::MatrixSize nCol = 0; nCol < aInput.mnColumns; ++nCol)
                {
                    EvaluationResult aElement
                        = mrEvaluator.materializeReferenceValue(aValue.maValue.maReference, nCol,
                            nRow);
                    if (!aElement)
                    {
                        return api::ValueResult<LookupInput>::success(
                            makeLookupScalarError(aElement.meError));
                    }
                    if (!aElement.maValue.isScalar())
                    {
                        return api::ValueResult<LookupInput>::success(
                            makeLookupScalarError(api::Error::IllegalArgument));
                    }

                    const auto aNumber = coerceToNumber(aElement.maValue.maValue);
                    if (!aNumber)
                    {
                        return api::ValueResult<LookupInput>::success(
                            makeLookupScalarError(aNumber.meError));
                    }
                    aInput.maValues.push_back(api::CellValue::number(aNumber.maValue));
                }
            }
            return api::ValueResult<LookupInput>::success(aInput);
        };

        if (rLookupNode.meKind == formula::NodeKind::FunctionCall
            && normalizeFunctionName(rLookupNode.maPrimaryText) == u"ISNUMBER")
        {
            if (rLookupNode.maChildren.size() != 1)
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));

            EvaluationResult aValue
                = mrEvaluator.evaluateNode(*rLookupNode.maChildren[0], mrCurrentAddress);
            if (!aValue)
            {
                LookupInput aScalar;
                aScalar.maScalar = api::CellValue::boolean(false);
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            if (aValue.maValue.isScalar())
            {
                LookupInput aScalar;
                aScalar.maScalar = api::CellValue::boolean(aValue.maValue.maValue.isNumber());
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            LookupInput aInput;
            aInput.mbScalar = false;
            const auto aDimensions = aValue.maValue.maReference.matrixDimensions();
            aInput.mnColumns = aDimensions.mnColumns;
            aInput.mnRows = aDimensions.mnRows;
            aInput.maValues.reserve(
                static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnColumns, 0))
                * static_cast<std::size_t>(std::max<api::MatrixSize>(aInput.mnRows, 0)));
            for (api::MatrixSize nRow = 0; nRow < aInput.mnRows; ++nRow)
            {
                for (api::MatrixSize nCol = 0; nCol < aInput.mnColumns; ++nCol)
                {
                    EvaluationResult aElement = mrEvaluator.materializeReferenceValue(
                        aValue.maValue.maReference, nCol, nRow);
                    const bool bIsNumber = aElement && aElement.maValue.isScalar()
                                           && aElement.maValue.maValue.isNumber();
                    aInput.maValues.push_back(api::CellValue::boolean(bIsNumber));
                }
            }

            if (aInput.mnColumns == 1 && aInput.mnRows == 1 && !aInput.maValues.empty())
            {
                LookupInput aScalar;
                aScalar.maScalar = aInput.maValues.front();
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            return api::ValueResult<LookupInput>::success(aInput);
        }

        if (rLookupNode.meKind == formula::NodeKind::FunctionCall
            && rLookupNode.maPrimaryText == u"MMULT")
        {
            if (rLookupNode.maChildren.size() != 2)
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));

            const auto aLeft = evaluateMatrixOperand(*rLookupNode.maChildren[0]);
            const auto aRight = evaluateMatrixOperand(*rLookupNode.maChildren[1]);
            if (!aLeft)
                return aLeft;
            if (!aRight)
                return aRight;
            if (aLeft.maValue.mbScalar && aLeft.maValue.maScalar.isError())
                return api::ValueResult<LookupInput>::success(aLeft.maValue);
            if (aRight.maValue.mbScalar && aRight.maValue.maScalar.isError())
                return api::ValueResult<LookupInput>::success(aRight.maValue);

            const api::MatrixSize nLeftColumns
                = aLeft.maValue.mbScalar ? 1 : aLeft.maValue.mnColumns;
            const api::MatrixSize nLeftRows = aLeft.maValue.mbScalar ? 1 : aLeft.maValue.mnRows;
            const api::MatrixSize nRightColumns
                = aRight.maValue.mbScalar ? 1 : aRight.maValue.mnColumns;
            const api::MatrixSize nRightRows = aRight.maValue.mbScalar ? 1 : aRight.maValue.mnRows;
            if (nLeftColumns != nRightRows)
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));

            auto getValueAt = [](const LookupInput& rInput, api::MatrixSize nColumn,
                                 api::MatrixSize nRow) -> double {
                if (rInput.mbScalar)
                    return rInput.maScalar.mfNumber;
                const std::size_t nIndex
                    = static_cast<std::size_t>(nRow * rInput.mnColumns + nColumn);
                return rInput.maValues[nIndex].mfNumber;
            };

            LookupInput aResult;
            aResult.mbScalar = false;
            aResult.mnColumns = nRightColumns;
            aResult.mnRows = nLeftRows;
            aResult.maValues.reserve(static_cast<std::size_t>(aResult.mnColumns)
                                     * static_cast<std::size_t>(aResult.mnRows));
            for (api::MatrixSize nRow = 0; nRow < aResult.mnRows; ++nRow)
            {
                for (api::MatrixSize nCol = 0; nCol < aResult.mnColumns; ++nCol)
                {
                    double fSum = 0.0;
                    for (api::MatrixSize nIndex = 0; nIndex < nLeftColumns; ++nIndex)
                    {
                        fSum += getValueAt(aLeft.maValue, nIndex, nRow)
                                * getValueAt(aRight.maValue, nCol, nIndex);
                    }
                    aResult.maValues.push_back(api::CellValue::number(fSum));
                }
            }

            if (aResult.mnColumns == 1 && aResult.mnRows == 1)
            {
                LookupInput aScalar;
                aScalar.maScalar = aResult.maValues.front();
                return api::ValueResult<LookupInput>::success(aScalar);
            }

            return api::ValueResult<LookupInput>::success(aResult);
        }

        EvaluationResult aValue = mrEvaluator.evaluateNode(rLookupNode, mrCurrentAddress);
        if (!aValue)
            return api::ValueResult<LookupInput>::failure(aValue.meError);

        const auto oInput = makeLookupInput(aValue);
        if (!oInput)
            return api::ValueResult<LookupInput>::failure(api::Error::IllegalArgument);
        return api::ValueResult<LookupInput>::success(*oInput);
    }

    [[nodiscard]] api::ValueResult<bool> evaluatePayTypeArgument(
        const formula::Node& rArgument, bool bDefaultValue) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<bool>::success(bDefaultValue);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<bool>::success(bDefaultValue);

        return coerceToBoolean(aValue.maValue);
    }

    [[nodiscard]] api::ValueResult<bool> evaluateStrictPaymentTypeArgument(
        const formula::Node& rArgument) const
    {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<bool>::failure(aNumber.meError);

        const auto oWhole = toWholeNumber(aNumber.maValue);
        if (!oWhole || (*oWhole != 0 && *oWhole != 1))
            return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
        return api::ValueResult<bool>::success(*oWhole != 0);
    }

    [[nodiscard]] EvaluationResult makeFiniteNumberResult(double fValue) const
    {
        if (!std::isfinite(fValue))
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::number(fValue));
    }

    [[nodiscard]] api::ValueResult<api::WeekendMask> evaluateWeekendMaskArgument(
        const formula::Node* pArgument, bool bWorkdayFunction, bool bAllowSequence = false) const
    {
        if (!pArgument || pArgument->meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<api::WeekendMask>::success(api::workday::defaultWeekendMask());

        if (bAllowSequence)
        {
            std::vector<double> aWeekendSequence;
            bool bSequenceCompatible = true;
            const auto aVisited = visitFlattenedValues(
                *pArgument, [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);
                    if (rValue.isEmpty())
                        return api::ValueResult<bool>::success(true);

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber)
                    {
                        bSequenceCompatible = false;
                        return api::ValueResult<bool>::success(true);
                    }

                    aWeekendSequence.push_back(aNumber.maValue);
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return api::ValueResult<api::WeekendMask>::failure(aVisited.meError);

            if (bSequenceCompatible && aWeekendSequence.size() == 7)
                return api::workday::weekendMaskFromSequence(aWeekendSequence);

            if (bSequenceCompatible && aWeekendSequence.size() > 1)
                return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
        }

        EvaluationResult aWeekendValue;
        if (pArgument->meKind == formula::NodeKind::ArrayConstant)
        {
            if (pArgument->maChildren.empty())
                return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
            for (const auto& pChild : pArgument->maChildren)
            {
                if (!isLiteralArrayWeekendNode(*pChild))
                    return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
            }

            aWeekendValue = ensureScalarValue(
                mrEvaluator, mrEvaluator.evaluateNode(*pArgument->maChildren.front(), mrCurrentAddress));
        }
        else
        {
            aWeekendValue = mrEvaluator.evaluateNode(*pArgument, mrCurrentAddress);
            if (aWeekendValue && aWeekendValue.maValue.isMatrixReference())
            {
                if (!aWeekendValue.maValue.maReference.isSingleCell())
                    return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
                aWeekendValue
                    = mrEvaluator.materializeReferenceValue(aWeekendValue.maValue.maReference, 0, 0);
            }
        }

        if (!aWeekendValue)
            return api::ValueResult<api::WeekendMask>::failure(aWeekendValue.meError);
        if (!aWeekendValue.maValue.isScalar())
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);

        const api::CellValue& rValue = aWeekendValue.maValue.maValue;
        if (rValue.isError())
            return api::ValueResult<api::WeekendMask>::failure(rValue.meError);
        if (rValue.isEmpty())
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);

        if (rValue.isText())
        {
            if (rValue.maString.size() != 7)
                return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
            const auto aMask
                = api::workday::weekendMaskFromMsSpec(rValue.maString, bWorkdayFunction);
            if (!aMask)
                return api::ValueResult<api::WeekendMask>::failure(aMask.meError);
            return aMask;
        }

        const auto aNumber = coerceToNumber(rValue);
        if (!aNumber)
            return api::ValueResult<api::WeekendMask>::failure(aNumber.meError);
        const auto oWholeNumber = toWholeNumber(aNumber.maValue);
        if (!oWholeNumber)
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
        if ((*oWholeNumber < 1 || *oWholeNumber > 7)
            && (*oWholeNumber < 11 || *oWholeNumber > 17))
        {
            return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
        }

        const auto aMask = api::workday::weekendMaskFromMsSpec(
            formatNumber(static_cast<double>(*oWholeNumber)), bWorkdayFunction);
        if (!aMask)
            return api::ValueResult<api::WeekendMask>::failure(aMask.meError);
        return aMask;
    }

    [[nodiscard]] api::ValueResult<std::vector<api::DateSerial>> collectHolidaySerials(
        const formula::Node* pArgument) const
    {
        std::vector<api::DateSerial> aHolidays;
        if (!pArgument || pArgument->meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<std::vector<api::DateSerial>>::success(aHolidays);

        const auto aVisited = visitFlattenedValues(
            *pArgument,
            [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isEmpty())
                    return api::ValueResult<bool>::success(true);

                const auto oDateSerial = sedatetime::coerceToDateSerial(rValue);
                if (!oDateSerial)
                    return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

                aHolidays.push_back(*oDateSerial);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<std::vector<api::DateSerial>>::failure(aVisited.meError);

        std::sort(aHolidays.begin(), aHolidays.end());
        aHolidays.erase(std::unique(aHolidays.begin(), aHolidays.end()), aHolidays.end());
        return api::ValueResult<std::vector<api::DateSerial>>::success(std::move(aHolidays));
    }
};

class EvaluatorCriteriaAggregateMaterializer final
    : public sequery::CriteriaAggregateMaterializer
{
    Evaluator& mrEvaluator;

public:
    explicit EvaluatorCriteriaAggregateMaterializer(Evaluator& rEvaluator)
        : mrEvaluator(rEvaluator)
    {
    }

    [[nodiscard]] api::ValueResult<api::CellValue> materialize(
        const CriteriaAggregateInput& rInput, api::MatrixCoordinate aCoordinate) const override
    {
        if (rInput.mbScalar)
        {
            if (aCoordinate.mnColumn != 0 || aCoordinate.mnRow != 0)
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            return api::ValueResult<api::CellValue>::success(rInput.maScalar);
        }

        EvaluationResult aResult = mrEvaluator.materializeReferenceValue(
            rInput.maReference, aCoordinate.mnColumn, aCoordinate.mnRow);
        if (!aResult)
            return api::ValueResult<api::CellValue>::failure(aResult.meError);
        if (!aResult.maValue.isScalar())
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::CellValue>::success(aResult.maValue.maValue);
    }
};

class EvaluatorLookupMaterializer final : public selookup::LookupMaterializer
{
    Evaluator& mrEvaluator;

public:
    explicit EvaluatorLookupMaterializer(Evaluator& rEvaluator)
        : mrEvaluator(rEvaluator)
    {
    }

    [[nodiscard]] api::ValueResult<api::CellValue> materialize(
        const LookupInput& rInput, api::MatrixCoordinate aCoordinate) const override
    {
        if (rInput.mbScalar)
        {
            if (aCoordinate.mnColumn != 0 || aCoordinate.mnRow != 0)
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            return api::ValueResult<api::CellValue>::success(rInput.maScalar);
        }

        if (!rInput.maValues.empty())
        {
            const std::int64_t nLinearIndex
                = static_cast<std::int64_t>(aCoordinate.mnRow) * rInput.mnColumns
                  + aCoordinate.mnColumn;
            if (nLinearIndex < 0
                || static_cast<std::size_t>(nLinearIndex) >= rInput.maValues.size())
            {
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            }
            return api::ValueResult<api::CellValue>::success(
                rInput.maValues[static_cast<std::size_t>(nLinearIndex)]);
        }

        EvaluationResult aResult = mrEvaluator.materializeReferenceValue(
            rInput.maReference, aCoordinate.mnColumn, aCoordinate.mnRow);
        if (!aResult)
            return api::ValueResult<api::CellValue>::failure(aResult.meError);
        if (!aResult.maValue.isScalar())
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::CellValue>::success(aResult.maValue.maValue);
    }
};

} // namespace spreadsheetengine::core::eval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
