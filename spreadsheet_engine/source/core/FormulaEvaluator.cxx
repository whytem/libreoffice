/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/detail/FormulaEvaluator.hxx>

#include <rtl/math.hxx>

#include <spreadsheetengine/detail/WorkbookCompileHost.hxx>
#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>
#include <spreadsheetengine/detail/compiler/CompiledFormulaInflation.hxx>

#include <spreadsheetengine/api/Array.hxx>
#include <spreadsheetengine/api/Calendar.hxx>
#include <spreadsheetengine/api/Logic.hxx>
#include <spreadsheetengine/api/Lookup.hxx>
#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Numeral.hxx>
#include <spreadsheetengine/api/Query.hxx>
#include <spreadsheetengine/api/Text.hxx>
#include <spreadsheetengine/api/Workday.hxx>
#include <spreadsheetengine/runtime/ConversionRuntime.hxx>
#include <spreadsheetengine/runtime/DateTimeParse.hxx>
#include <spreadsheetengine/runtime/DateTimeParts.hxx>
#include <spreadsheetengine/runtime/FinancialRuntime.hxx>
#include <spreadsheetengine/runtime/LookupRuntime.hxx>
#include <spreadsheetengine/runtime/MathAggregate.hxx>
#include <spreadsheetengine/runtime/MathFunctionRuntime.hxx>
#include <spreadsheetengine/runtime/MathRounding.hxx>
#include <spreadsheetengine/runtime/MathStatistical.hxx>
#include <spreadsheetengine/runtime/QueryRuntime.hxx>
#include <spreadsheetengine/runtime/TextCase.hxx>
#include <spreadsheetengine/runtime/TextFunctionRuntime.hxx>
#include <spreadsheetengine/runtime/TextRuntimeSupport.hxx>
#include <spreadsheetengine/runtime/TextScalar.hxx>

#include "CoreRuntimeUtils.hxx"
#include "DateAlgorithms.hxx"
#include "FormulaEvaluatorUtils.hxx"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <string>

#include <kahan.hxx>

namespace spreadsheetengine::core::eval
{
namespace
{

namespace secompiler = spreadsheetengine::detail::compiler;
namespace seconvert = spreadsheetengine::core::convert;
namespace sedate = spreadsheetengine::core::detail::date;
namespace sedatetime = spreadsheetengine::core::datetime;
namespace sefinance = spreadsheetengine::core::finance;
namespace semath = spreadsheetengine::core::math;
namespace selookup = spreadsheetengine::core::lookup;
namespace sequery = spreadsheetengine::core::query;
namespace setext = spreadsheetengine::core::text;
namespace seutil = spreadsheetengine::core::util;

using detail::makeScalarResult;
using detail::makeReferenceResult;
using detail::makeFailure;
using detail::ensureScalarValue;
using detail::uppercaseAscii;
using detail::hasFunctionPrefix;
using detail::normalizeDisplayFunctionName;
using detail::normalizeFunctionName;
using detail::parseAsciiDouble;
using detail::coerceToNumber;
using detail::coerceToBoolean;
using detail::coerceToString;
using detail::toWholeNumber;
using detail::formatNumber;
using detail::formatQuotedString;

[[nodiscard]] std::tuple<api::SheetId, api::ColumnIndex, api::RowIndex> makeAddressKey(
    const api::CellAddress& rAddress)
{
    return { rAddress.mnSheet, rAddress.mnColumn, rAddress.mnRow };
}

[[nodiscard]] bool hasCachedFallbackValue(const workbook::Cell& rCell)
{
    return rCell.maValue.isNumber() || rCell.maValue.isBoolean() || rCell.maValue.isText()
           || rCell.maValue.isError();
}

[[nodiscard]] api::query::SearchType toQuerySearchType(workbook::FormulaSearchType eSearchType)
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

[[nodiscard]] bool usesMicrosoftCompatibilityName(api::StringView rName)
{
    return hasFunctionPrefix(rName, u"COM.MICROSOFT.");
}

[[nodiscard]] api::Error mapErrorLiteral(api::StringView rText)
{
    if (rText == u"#N/A")
        return api::Error::NotAvailable;
    if (rText == u"#DIV/0!")
        return api::Error::DivisionByZero;
    if (rText == u"#VALUE!")
        return api::Error::NoValue;
    if (rText == u"#NUM!")
        return api::Error::NoConvergence;
    if (rText == u"#NAME?" || rText == u"#REF!" || rText == u"#NULL!")
        return api::Error::IllegalArgument;
    const std::size_t nColon = rText.rfind(u':');
    if (nColon != api::StringView::npos && nColon + 1 < rText.size())
    {
        const api::StringView aCode = rText.substr(nColon + 1);
        bool bDigitsOnly = true;
        for (const char16_t cChar : aCode)
        {
            if (cChar < u'0' || cChar > u'9')
            {
                bDigitsOnly = false;
                break;
            }
        }

        if (bDigitsOnly)
        {
            if (aCode == u"503" || aCode == u"523")
                return api::Error::NoConvergence;
            if (aCode == u"513")
                return api::Error::StringOverflow;
            if (aCode == u"519")
                return api::Error::NoValue;
            if (aCode == u"532")
                return api::Error::DivisionByZero;
            return api::Error::IllegalArgument;
        }
    }

    return api::Error::NoValue;
}

[[nodiscard]] std::optional<sal_Int16> classifyOdfErrorTypeLiteral(api::StringView rText)
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

[[nodiscard]] std::optional<sal_Int16> classifyOdfErrorType(api::Error eError)
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

[[nodiscard]] std::optional<sal_Int32> classifyLegacyErrorTypeLiteral(api::StringView rText)
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
        sal_Int32 nCode = 0;
        for (const char16_t cChar : aDigits)
        {
            if (cChar < u'0' || cChar > u'9')
                return std::nullopt;
            nCode = nCode * 10 + static_cast<sal_Int32>(cChar - u'0');
        }
        if (nCode > 0)
            return nCode;
        return std::nullopt;
    }

    const std::size_t nColon = aUpper.rfind(u':');
    if (nColon != api::StringView::npos && nColon + 1 < aUpper.size())
    {
        sal_Int32 nCode = 0;
        for (const char16_t cChar : aUpper.substr(nColon + 1))
        {
            if (cChar < u'0' || cChar > u'9')
                return std::nullopt;
            nCode = nCode * 10 + static_cast<sal_Int32>(cChar - u'0');
        }
        if (nCode >= 500 || nCode == 32767)
            return nCode;
        return 525;
    }

    if (!rText.empty() && rText.front() == u'#')
        return 525;
    return std::nullopt;
}

[[nodiscard]] std::optional<sal_Int32> classifyLegacyErrorType(api::Error eError)
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

[[nodiscard]] bool isAsciiWhitespace(char16_t cChar)
{
    return cChar == u' ' || cChar == u'\t' || cChar == u'\r' || cChar == u'\n';
}

[[nodiscard]] api::StringView trimAsciiWhitespace(api::StringView rValue)
{
    while (!rValue.empty() && isAsciiWhitespace(rValue.front()))
        rValue.remove_prefix(1);
    while (!rValue.empty() && isAsciiWhitespace(rValue.back()))
        rValue.remove_suffix(1);
    return rValue;
}

[[nodiscard]] std::optional<sal_Int16> parseAsciiInt16(api::StringView rValue)
{
    if (rValue.empty())
        return std::nullopt;

    sal_Int32 nValue = 0;
    for (const char16_t cChar : rValue)
    {
        if (cChar < u'0' || cChar > u'9')
            return std::nullopt;
        nValue = (nValue * 10) + (cChar - u'0');
    }
    return static_cast<sal_Int16>(nValue);
}

[[nodiscard]] std::optional<api::CellValue> parseTypedStoredCellValue(
    const workbook::Cell& rCell)
{
    if (!rCell.maValue.isText())
        return std::nullopt;

    const api::StringView aLexical = !rCell.maRawValue.empty() ? api::StringView(rCell.maRawValue)
                                                               : api::StringView(rCell.maValue.maString);
    if (rCell.maRawValueType == u"date")
    {
        if (const auto oStoredDate = sedatetime::parseStoredDateValue(aLexical))
            return api::CellValue::number(*oStoredDate);

        if (const auto oParsed = sedatetime::parseStandaloneNumberText(aLexical))
        {
            if (oParsed->meKind == api::NumberParseResult::Kind::Date
                || oParsed->meKind == api::NumberParseResult::Kind::DateTime)
            {
                return api::CellValue::number(oParsed->mfValue);
            }
        }
    }

    if (rCell.maRawValueType == u"time")
    {
        if (const auto oDuration = sedatetime::parseOdfTimeDuration(aLexical))
        {
            return api::CellValue::number(sedatetime::normalizeTimeFraction(*oDuration));
        }

        if (const auto oParsed = sedatetime::parseStandaloneNumberText(aLexical))
        {
            if (oParsed->meKind == api::NumberParseResult::Kind::Time
                || oParsed->meKind == api::NumberParseResult::Kind::DateTime)
            {
                return api::CellValue::number(
                    sedatetime::normalizeTimeFraction(oParsed->mfValue));
            }
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<int> weekdayIndexForFodsDate(api::DateSerial nDate)
{
    const auto aWeekday = api::calendar::dayOfWeek(sedatetime::defaultNullDate(), nDate, 2);
    if (!aWeekday || aWeekday.maValue < 1 || aWeekday.maValue > 7)
        return std::nullopt;
    return aWeekday.maValue - 1;
}

[[nodiscard]] bool isWeekendFodsDate(api::DateSerial nDate, const api::WeekendMask& rWeekendMask)
{
    const auto oWeekdayIndex = weekdayIndexForFodsDate(nDate);
    return oWeekdayIndex && rWeekendMask[static_cast<std::size_t>(*oWeekdayIndex)];
}

[[nodiscard]] bool isLiteralArrayWeekendNode(const formula::Node& rNode)
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

[[nodiscard]] bool isHolidayFodsDate(
    api::DateSerial nDate, const std::vector<api::DateSerial>& rSortedHolidays)
{
    return std::binary_search(rSortedHolidays.begin(), rSortedHolidays.end(), nDate);
}

[[nodiscard]] api::DateSerial countWorkdaysFods(api::DateSerial nDate1, api::DateSerial nDate2,
    const std::vector<api::DateSerial>& rSortedHolidays, const api::WeekendMask& rWeekendMask)
{
    sal_Int32 nCount = 0;
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

[[nodiscard]] api::DateSerial advanceWorkdayFods(api::DateSerial nDate, api::DateSerial nDays,
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

[[nodiscard]] api::String formatBasisDateTime(double fSerialValue)
{
    const api::DateParts aNullDate = sedatetime::defaultNullDate();
    const api::DateSerial nDateSerial = static_cast<api::DateSerial>(rtl::math::approxFloor(fSerialValue));
    const double fTimeValue = sedatetime::normalizeTimeFraction(fSerialValue);

    const sal_Int16 nYear = static_cast<sal_Int16>(sedatetime::extractYear(aNullDate, nDateSerial));
    const sal_Int16 nMonth = static_cast<sal_Int16>(sedatetime::extractMonth(aNullDate, nDateSerial));
    const auto oDay = sedatetime::extractDay(aNullDate, nDateSerial);
    const sal_Int16 nDay = static_cast<sal_Int16>(oDay.value_or(0.0));
    const sal_Int16 nHour = static_cast<sal_Int16>(sedatetime::extractHour(fTimeValue));
    const sal_Int16 nMinute = static_cast<sal_Int16>(sedatetime::extractMinute(fTimeValue));
    const sal_Int16 nSecond = static_cast<sal_Int16>(sedatetime::extractSecond(fTimeValue));

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

using AggregateOptions = semath::AggregateOptions;
using AggregateScan = semath::AggregateScan;

using CriteriaAggregateInput = sequery::CriteriaAggregateInput;
using CriteriaPredicate = sequery::CriteriaPredicate;
using CriteriaAggregateKind = sequery::CriteriaAggregateKind;
using LookupInput = selookup::LookupInput;

[[nodiscard]] std::optional<LookupInput> makeLookupInput(const EvaluationResult& rResult)
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

[[nodiscard]] LookupInput makeLookupScalarError(api::Error eError)
{
    LookupInput aInput;
    aInput.maScalar = api::CellValue::error(eError);
    return aInput;
}

[[nodiscard]] std::optional<CriteriaAggregateInput> makeCriteriaAggregateInput(
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
            const sal_Int64 nLinearIndex
                = static_cast<sal_Int64>(aCoordinate.mnRow) * rInput.mnColumns
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

[[nodiscard]] bool formulaContainsAggregateLike(const formula::Node& rNode)
{
    if (rNode.meKind == formula::NodeKind::FunctionCall)
    {
        const api::String aName = normalizeFunctionName(rNode.maPrimaryText);
        if (aName == u"AGGREGATE" || aName == u"SUBTOTAL")
            return true;
    }

    for (const auto& pChild : rNode.maChildren)
    {
        if (formulaContainsAggregateLike(*pChild))
            return true;
    }

    return false;
}

[[nodiscard]] bool cellContainsAggregateLike(const workbook::Cell& rCell)
{
    if (!rCell.hasFormula())
        return false;

    const formula::ParseResult aParsed = formula::parseFormula(rCell.maFormula);
    return aParsed && formulaContainsAggregateLike(*aParsed.mpRoot);
}

[[nodiscard]] double sumNumbers(const std::vector<double>& rNumbers)
{
    double fSum = 0.0;
    for (const double fValue : rNumbers)
        fSum = ::rtl::math::approxAdd(fSum, fValue);
    return fSum;
}

[[nodiscard]] std::optional<api::ColumnIndex> parseColumnName(api::StringView rColumnName)
{
    if (rColumnName.empty())
        return std::nullopt;

    sal_Int64 nColumn = 0;
    for (const char16_t cChar : rColumnName)
    {
        char16_t cUpper = cChar;
        if (cUpper >= u'a' && cUpper <= u'z')
            cUpper = static_cast<char16_t>(cUpper - u'a' + u'A');
        if (cUpper < u'A' || cUpper > u'Z')
            return std::nullopt;
        nColumn = nColumn * 26 + (cUpper - u'A' + 1);
    }

    return static_cast<api::ColumnIndex>(nColumn - 1);
}

[[nodiscard]] api::String unquoteSheetName(api::StringView rSheetName)
{
    if (rSheetName.size() < 2 || rSheetName.front() != u'\'' || rSheetName.back() != u'\'')
        return api::String(rSheetName);

    api::String aResult;
    aResult.reserve(rSheetName.size() - 2);
    for (std::size_t nIndex = 1; nIndex + 1 < rSheetName.size(); ++nIndex)
    {
        if (rSheetName[nIndex] == u'\'' && nIndex + 1 < rSheetName.size() - 1
            && rSheetName[nIndex + 1] == u'\'')
        {
            aResult.push_back(u'\'');
            ++nIndex;
            continue;
        }

        aResult.push_back(rSheetName[nIndex]);
    }
    return aResult;
}

[[nodiscard]] std::optional<api::CellAddress> parseCellAddressToken(
    api::StringView rToken, const workbook::Workbook& rWorkbook, api::SheetId nImplicitSheet)
{
    const std::size_t nDotPos = rToken.rfind(u'.');
    if (nDotPos == api::StringView::npos || nDotPos + 1 >= rToken.size())
        return std::nullopt;

    api::SheetId nSheet = nImplicitSheet;
    api::StringView aSheetToken = rToken.substr(0, nDotPos);
    if (!aSheetToken.empty())
    {
        while (!aSheetToken.empty() && aSheetToken.front() == u'$')
            aSheetToken.remove_prefix(1);

        if (!aSheetToken.empty())
        {
            const api::String aSheetName = unquoteSheetName(aSheetToken);
            const auto oSheetId = rWorkbook.findSheetId(aSheetName);
            if (!oSheetId)
                return std::nullopt;
            nSheet = *oSheetId;
        }
    }

    api::StringView aAddressToken = rToken.substr(nDotPos + 1);
    if (!aAddressToken.empty() && aAddressToken.front() == u'$')
        aAddressToken.remove_prefix(1);

    std::size_t nColumnEnd = 0;
    while (nColumnEnd < aAddressToken.size())
    {
        const char16_t cChar = aAddressToken[nColumnEnd];
        const bool bAlpha = (cChar >= u'A' && cChar <= u'Z') || (cChar >= u'a' && cChar <= u'z');
        if (!bAlpha)
            break;
        ++nColumnEnd;
    }

    if (nColumnEnd == 0)
        return std::nullopt;

    const auto oColumn = parseColumnName(aAddressToken.substr(0, nColumnEnd));
    if (!oColumn)
        return std::nullopt;

    aAddressToken.remove_prefix(nColumnEnd);
    if (!aAddressToken.empty() && aAddressToken.front() == u'$')
        aAddressToken.remove_prefix(1);
    if (aAddressToken.empty())
        return std::nullopt;

    sal_Int64 nRow = 0;
    for (const char16_t cChar : aAddressToken)
    {
        if (cChar < u'0' || cChar > u'9')
            return std::nullopt;
        nRow = nRow * 10 + (cChar - u'0');
    }

    if (nRow <= 0)
        return std::nullopt;

    return api::CellAddress { nSheet, *oColumn, static_cast<api::RowIndex>(nRow - 1) };
}

[[nodiscard]] bool needsQuotedSheetName(api::StringView rSheetName)
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

[[nodiscard]] api::String quoteSheetNameForFormula(api::StringView rSheetName)
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

[[nodiscard]] api::String columnNameFromIndex(api::ColumnIndex nColumn)
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

[[nodiscard]] api::String formatAddressFunctionResult(api::RowIndex nRow, api::ColumnIndex nColumn,
    sal_Int32 nAbsMode, bool bA1Style, api::StringView rSheetName)
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

[[nodiscard]] bool evaluateNumericComparison(
    double fLeft, double fRight, formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Equal:
            return ::rtl::math::approxEqual(fLeft, fRight);
        case formula::BinaryOperator::NotEqual:
            return !::rtl::math::approxEqual(fLeft, fRight);
        case formula::BinaryOperator::Less:
            return fLeft < fRight;
        case formula::BinaryOperator::LessEqual:
            return fLeft < fRight || ::rtl::math::approxEqual(fLeft, fRight);
        case formula::BinaryOperator::Greater:
            return fLeft > fRight;
        case formula::BinaryOperator::GreaterEqual:
            return fLeft > fRight || ::rtl::math::approxEqual(fLeft, fRight);
        default:
            return false;
    }
}

[[nodiscard]] bool evaluateStringComparison(
    api::StringView rLeft, api::StringView rRight, formula::BinaryOperator eOperator)
{
    switch (eOperator)
    {
        case formula::BinaryOperator::Equal:
            return rLeft == rRight;
        case formula::BinaryOperator::NotEqual:
            return rLeft != rRight;
        case formula::BinaryOperator::Less:
            return rLeft < rRight;
        case formula::BinaryOperator::LessEqual:
            return rLeft <= rRight;
        case formula::BinaryOperator::Greater:
            return rLeft > rRight;
        case formula::BinaryOperator::GreaterEqual:
            return rLeft >= rRight;
        default:
            return false;
    }
}

} // namespace

const workbook::Sheet* Evaluator::getSheet(api::SheetId nSheet) const
{
    if (nSheet < 0 || static_cast<std::size_t>(nSheet) >= mrWorkbook.maSheets.size())
        return nullptr;
    return &mrWorkbook.maSheets[static_cast<std::size_t>(nSheet)];
}

const workbook::Cell* Evaluator::getCell(const api::CellAddress& rAddress) const
{
    const workbook::Sheet* pSheet = getSheet(rAddress.mnSheet);
    return pSheet ? pSheet->findCell(rAddress.mnColumn, rAddress.mnRow) : nullptr;
}

const EvaluationResult* Evaluator::lookupLocalBinding(api::StringView rName) const
{
    const api::String aNormalizedName = uppercaseAscii(rName);
    for (auto aScopeIt = maLocalBindings.rbegin(); aScopeIt != maLocalBindings.rend(); ++aScopeIt)
    {
        const auto aBindingIt = aScopeIt->find(aNormalizedName);
        if (aBindingIt != aScopeIt->end())
            return &aBindingIt->second;
    }
    return nullptr;
}

std::map<Evaluator::AddressKey, Evaluator::CacheEntry>& Evaluator::cacheForMode(ExecutionMode eMode)
{
    return eMode == ExecutionMode::CompiledToken ? maCompiledCellCache : maAstCellCache;
}

EvaluationResult Evaluator::materializeReferenceValue(
    const api::ResolvedReference& rReference, api::ColumnIndex nColumnOffset,
    api::RowIndex nRowOffset)
{
    if (!rReference.isNormalized() || !rReference.containsOffset(nColumnOffset, nRowOffset))
        return makeFailure(api::Error::IllegalArgument);

    return evaluateCellInternal(rReference.addressAt(nColumnOffset, nRowOffset), meActiveExecutionMode);
}

api::ValueResult<api::ResolvedReference> Evaluator::resolveReferenceText(
    api::StringView rReference, api::SheetId nCurrentSheet) const
{
    const std::size_t nColonPos = rReference.find(u':');
    if (nColonPos == api::StringView::npos)
    {
        const auto oAddress = parseCellAddressToken(rReference, mrWorkbook, nCurrentSheet);
        if (!oAddress)
            return api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

        api::ResolvedReference aReference { { *oAddress, *oAddress } };
        return api::ValueResult<api::ResolvedReference>::success(aReference);
    }

    const auto oStart
        = parseCellAddressToken(rReference.substr(0, nColonPos), mrWorkbook, nCurrentSheet);
    if (!oStart)
        return api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

    const auto oEnd = parseCellAddressToken(
        rReference.substr(nColonPos + 1), mrWorkbook, oStart->mnSheet);
    if (!oEnd || oStart->mnSheet != oEnd->mnSheet)
        return api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

    api::CellRange aRange { *oStart, *oEnd };
    if (aRange.maStart.mnColumn > aRange.maEnd.mnColumn)
        std::swap(aRange.maStart.mnColumn, aRange.maEnd.mnColumn);
    if (aRange.maStart.mnRow > aRange.maEnd.mnRow)
        std::swap(aRange.maStart.mnRow, aRange.maEnd.mnRow);

    api::ResolvedReference aReference { aRange };
    return api::ValueResult<api::ResolvedReference>::success(aReference);
}

api::ValueResult<api::ResolvedReference> Evaluator::resolveNamedRange(
    api::StringView rName, api::SheetId nScopeSheet) const
{
    if (nScopeSheet >= 0 && static_cast<std::size_t>(nScopeSheet) < mrWorkbook.maSheets.size())
    {
        const auto& rSheet = mrWorkbook.maSheets[static_cast<std::size_t>(nScopeSheet)];
        if (const auto* pLocal = mrWorkbook.findNamedRange(rName, rSheet.maName))
            return resolveReferenceText(pLocal->maCellRangeAddress, nScopeSheet);
    }

    if (const auto* pGlobal = mrWorkbook.findNamedRange(rName))
        return resolveReferenceText(pGlobal->maCellRangeAddress, nScopeSheet);

    return api::ValueResult<api::ResolvedReference>::failure(api::Error::NotAvailable);
}

EvaluationResult Evaluator::evaluateReferenceNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::CellReference:
        {
            const auto aReference = resolveReferenceText(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aReference)
                return makeFailure(aReference.meError);
            return makeReferenceResult(aReference.maValue);
        }
        case formula::NodeKind::RangeReference:
        {
            api::String aReference = rNode.maPrimaryText;
            aReference.push_back(u':');
            aReference += rNode.maSecondaryText;
            const auto aRange = resolveReferenceText(aReference, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::NamedReference:
        {
            const auto aRange = resolveNamedRange(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::RangeConstructor:
            return makeFailure(api::Error::IllegalArgument);
        case formula::NodeKind::ReferenceList:
            return makeFailure(api::Error::IllegalArgument);
        default:
        {
            EvaluationResult aValue = evaluateNode(rNode, rCurrentAddress);
            if (!aValue)
                return aValue;
            if (!aValue.maValue.isMatrixReference())
                return makeFailure(api::Error::IllegalArgument);
            return aValue;
        }
    }
}

EvaluationResult Evaluator::evaluateFunction(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    const api::String aFunctionName = normalizeFunctionName(rNode.maPrimaryText);

    constexpr std::array aStructuredDispatchers{
        &Evaluator::tryEvaluateSpecialForm,
        &Evaluator::tryEvaluateAggregateFamily,
    };
    for (const auto pDispatch : aStructuredDispatchers)
    {
        if (const auto oDispatched = (this->*pDispatch)(aFunctionName, rNode, rCurrentAddress))
            return *oDispatched;
    }

    auto visitFlattenedValues
        = [&](const auto& self, const formula::Node& rArgument,
              const auto& rVisitor) -> api::ValueResult<bool> {
        if (rArgument.meKind == formula::NodeKind::ArrayConstant
            || rArgument.meKind == formula::NodeKind::ReferenceList)
        {
            for (const auto& pChild : rArgument.maChildren)
            {
                const auto aChild = self(self, *pChild, rVisitor);
                if (!aChild)
                    return aChild;
            }
            return api::ValueResult<bool>::success(true);
        }

        const bool bReferenceLike = rArgument.meKind == formula::NodeKind::CellReference
                                    || rArgument.meKind == formula::NodeKind::RangeReference
                                    || rArgument.meKind == formula::NodeKind::NamedReference;

        EvaluationResult aValue = bReferenceLike ? evaluateReferenceNode(rArgument, rCurrentAddress)
                                                 : evaluateNode(rArgument, rCurrentAddress);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);

        auto visitScalar = [&](const api::CellValue& rValue, bool bFromReference)
            -> api::ValueResult<bool> { return rVisitor(rValue, bFromReference); };

        if (aValue.maValue.isMatrixReference())
        {
            const auto& rReference = aValue.maValue.maReference;
            for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
            {
                for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                {
                    EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                    if (!aCell)
                        return api::ValueResult<bool>::failure(aCell.meError);

                    const auto aVisited = visitScalar(aCell.maValue.maValue, true);
                    if (!aVisited)
                        return aVisited;
                }
            }

            return api::ValueResult<bool>::success(true);
        }

        return visitScalar(aValue.maValue.maValue, bReferenceLike);
    };

    auto collectNumericArguments = [&](bool bIgnoreTextAndEmptyFromReferences,
                                      bool bTreatScalarEmptyAsZero = false)
        -> api::ValueResult<std::vector<double>> {
        std::vector<double> aNumbers;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *pChild,
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

        return api::ValueResult<std::vector<double>>::success(aNumbers);
    };

    auto collectVarianceArguments = [&](bool bTextAsZero)
        -> api::ValueResult<std::vector<double>> {
        std::vector<double> aNumbers;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *pChild,
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

        return api::ValueResult<std::vector<double>>::success(aNumbers);
    };

    auto evaluateScalarArgumentValue = [&](const formula::Node& rArgument)
        -> api::ValueResult<api::CellValue> {
        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(rArgument, rCurrentAddress));
        if (!aValue)
            return api::ValueResult<api::CellValue>::failure(aValue.meError);
        return api::ValueResult<api::CellValue>::success(aValue.maValue.maValue);
    };

    auto evaluateNumericArgument = [&](const formula::Node& rArgument,
                                      std::optional<double> oDefaultForEmpty)
        -> api::ValueResult<double> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument && oDefaultForEmpty)
            return api::ValueResult<double>::success(*oDefaultForEmpty);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<double>::failure(aNumber.meError);
        return aNumber;
    };

    auto evaluateRequiredNumberArgument = [&](const formula::Node& rArgument)
        -> api::ValueResult<double> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<double>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        return coerceToNumber(aValue.maValue);
    };

    auto evaluateRequiredDateArgument = [&](const formula::Node& rArgument)
        -> api::ValueResult<api::DateSerial> {
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
    };

    auto evaluateOptionalWholeNumberArgument
        = [&](const formula::Node& rArgument, sal_Int32 nDefaultValue)
        -> api::ValueResult<sal_Int32> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<sal_Int32>::success(nDefaultValue);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<sal_Int32>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<sal_Int32>::success(nDefaultValue);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<sal_Int32>::failure(aNumber.meError);

        const auto oWhole = toWholeNumber(aNumber.maValue);
        if (!oWhole)
            return api::ValueResult<sal_Int32>::failure(api::Error::IllegalArgument);
        return api::ValueResult<sal_Int32>::success(*oWhole);
    };

    auto evaluateRequiredWholeNumberArgument = [&](const formula::Node& rArgument)
        -> api::ValueResult<sal_Int32> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<sal_Int32>::failure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<sal_Int32>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<sal_Int32>::failure(api::Error::IllegalArgument);

        const auto aNumber = coerceToNumber(aValue.maValue);
        if (!aNumber)
            return api::ValueResult<sal_Int32>::failure(aNumber.meError);

        const auto oWhole = toWholeNumber(aNumber.maValue);
        if (!oWhole)
            return api::ValueResult<sal_Int32>::failure(api::Error::IllegalArgument);
        return api::ValueResult<sal_Int32>::success(*oWhole);
    };

    auto collectAggregateScanFromArgument = [&](const formula::Node& rArgument)
        -> api::ValueResult<AggregateScan> {
        AggregateScan aScan;
        const auto aVisited = visitFlattenedValues(
            visitFlattenedValues, rArgument,
            [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                if (rValue.isNumber())
                    aScan.maNumbers.push_back(rValue.mfNumber);
                return api::ValueResult<bool>::success(true);
            });
        if (!aVisited)
            return api::ValueResult<AggregateScan>::failure(aVisited.meError);
        return api::ValueResult<AggregateScan>::success(std::move(aScan));
    };

    auto collectExtremaArguments = [&]() -> api::ValueResult<std::vector<double>> {
        std::vector<double> aNumbers;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *pChild,
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
    };

    auto evaluateLookupInputNode = [&](const formula::Node& rLookupNode)
        -> api::ValueResult<LookupInput> {
        auto evaluateMatrixOperand = [&](const formula::Node& rOperand)
            -> api::ValueResult<LookupInput> {
            EvaluationResult aValue = evaluateNode(rOperand, rCurrentAddress);
            if (!aValue)
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(aValue.meError));

            if (aValue.maValue.isScalar())
            {
                const auto aNumber = coerceToNumber(aValue.maValue.maValue);
                if (!aNumber)
                {
                    return api::ValueResult<LookupInput>::success(
                        makeLookupScalarError(aNumber.meError));
                }

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
                        = materializeReferenceValue(aValue.maValue.maReference, nCol, nRow);
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
            {
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));
            }

            EvaluationResult aValue = evaluateNode(*rLookupNode.maChildren[0], rCurrentAddress);
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
                    EvaluationResult aElement
                        = materializeReferenceValue(aValue.maValue.maReference, nCol, nRow);
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
            {
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));
            }

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
            const api::MatrixSize nRightRows
                = aRight.maValue.mbScalar ? 1 : aRight.maValue.mnRows;
            if (nLeftColumns != nRightRows)
            {
                return api::ValueResult<LookupInput>::success(
                    makeLookupScalarError(api::Error::IllegalArgument));
            }

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

        EvaluationResult aValue = evaluateNode(rLookupNode, rCurrentAddress);
        if (!aValue)
            return api::ValueResult<LookupInput>::failure(aValue.meError);

        const auto oInput = makeLookupInput(aValue);
        if (!oInput)
            return api::ValueResult<LookupInput>::failure(api::Error::IllegalArgument);
        return api::ValueResult<LookupInput>::success(*oInput);
    };

    auto evaluatePayTypeArgument = [&](const formula::Node& rArgument, bool bDefaultValue)
        -> api::ValueResult<bool> {
        if (rArgument.meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<bool>::success(bDefaultValue);

        const auto aValue = evaluateScalarArgumentValue(rArgument);
        if (!aValue)
            return api::ValueResult<bool>::failure(aValue.meError);
        if (aValue.maValue.isEmpty())
            return api::ValueResult<bool>::success(bDefaultValue);

        return coerceToBoolean(aValue.maValue);
    };

    auto evaluateStrictPaymentTypeArgument = [&](const formula::Node& rArgument)
        -> api::ValueResult<bool> {
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
    };

    auto makeFiniteNumberResult = [&](double fValue) -> EvaluationResult {
        if (!std::isfinite(fValue))
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::number(fValue));
    };

    auto evaluateWeekendMaskArgument = [&](const formula::Node* pArgument,
                                          bool bWorkdayFunction)
        -> api::ValueResult<api::WeekendMask> {
        if (!pArgument || pArgument->meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<api::WeekendMask>::success(api::workday::defaultWeekendMask());

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

            aWeekendValue = ensureScalarValue(*this, evaluateNode(*pArgument->maChildren.front(), rCurrentAddress));
        }
        else
        {
            aWeekendValue = evaluateNode(*pArgument, rCurrentAddress);
            if (aWeekendValue && aWeekendValue.maValue.isMatrixReference())
            {
                if (!aWeekendValue.maValue.maReference.isSingleCell())
                    return api::ValueResult<api::WeekendMask>::failure(api::Error::IllegalArgument);
                aWeekendValue
                    = materializeReferenceValue(aWeekendValue.maValue.maReference, 0, 0);
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
            const auto aMask = api::workday::weekendMaskFromMsSpec(
                rValue.maString, bWorkdayFunction);
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
    };

    auto collectHolidaySerials = [&](const formula::Node* pArgument)
        -> api::ValueResult<std::vector<api::DateSerial>> {
        std::vector<api::DateSerial> aHolidays;
        if (!pArgument || pArgument->meKind == formula::NodeKind::EmptyArgument)
            return api::ValueResult<std::vector<api::DateSerial>>::success(aHolidays);

        const auto aVisited = visitFlattenedValues(
            visitFlattenedValues, *pArgument,
            [&](const api::CellValue& rValue, bool /*bFromReference*/) -> api::ValueResult<bool> {
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
        return api::ValueResult<std::vector<api::DateSerial>>::success(aHolidays);
    };

    if (aFunctionName == u"TRUE")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::boolean(true));
    }

    if (aFunctionName == u"FALSE")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::boolean(false));
    }

    if (aFunctionName == u"NA")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);
        return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
    }

    if (aFunctionName == u"ABS")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);
        return makeScalarResult(api::CellValue::number(api::math::abs(aNumber.maValue)));
    }

    if (aFunctionName == u"PI")
    {
        if (!rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(api::math::pi()));
    }

    if (aFunctionName == u"DEGREES")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);
        return makeScalarResult(api::CellValue::number(api::math::degrees(aNumber.maValue)));
    }

    if (aFunctionName == u"CONCATENATE")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        api::String aResult;
        for (const auto& pChild : rNode.maChildren)
        {
            EvaluationResult aArgument
                = ensureScalarValue(*this, evaluateNode(*pChild, rCurrentAddress));
            if (!aArgument)
                return aArgument;

            const auto aText = coerceToString(aArgument.maValue.maValue);
            if (!aText)
                return makeFailure(aText.meError);
            aResult += aText.maValue;
        }

        return makeScalarResult(api::CellValue::text(aResult));
    }

    if (aFunctionName == u"ISERROR")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(true));
        return makeScalarResult(api::CellValue::boolean(aArgument.maValue.maValue.isError()));
    }

    if (aFunctionName == u"ISNUMBER")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (aArgument && !aArgument.maValue.isScalar())
            aArgument = materializeReferenceValue(aArgument.maValue.maReference, 0, 0);
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(false));
        return makeScalarResult(api::CellValue::boolean(
            aArgument.maValue.maValue.isNumber() || aArgument.maValue.maValue.isBoolean()));
    }

    if (aFunctionName == u"ISNA")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return makeScalarResult(api::CellValue::boolean(
                aArgument.meError == api::Error::NotAvailable));
        return makeScalarResult(api::CellValue::boolean(
            aArgument.maValue.maValue.isError()
            && aArgument.maValue.maValue.meError == api::Error::NotAvailable));
    }

    if (aFunctionName == u"ERROR.TYPE" || aFunctionName == u"ERRORTYPE")
    {
        const bool bLegacyErrorType = aFunctionName == u"ERRORTYPE";
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const formula::Node& rArgument = *rNode.maChildren[0];
        const auto classifyReferenceErrorType = [&](const formula::Node& rReferenceNode)
            -> std::optional<sal_Int32> {
            api::ValueResult<api::ResolvedReference> aReference
                = api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

            if (rReferenceNode.meKind == formula::NodeKind::CellReference)
            {
                aReference = resolveReferenceText(rReferenceNode.maPrimaryText, rCurrentAddress.mnSheet);
                if (!aReference && aReference.meError == api::Error::IllegalArgument)
                    return bLegacyErrorType ? 524 : 4;
            }
            else if (rReferenceNode.meKind == formula::NodeKind::RangeReference)
            {
                api::String aAddress = rReferenceNode.maPrimaryText;
                aAddress.push_back(u':');
                aAddress += rReferenceNode.maSecondaryText;
                aReference = resolveReferenceText(aAddress, rCurrentAddress.mnSheet);
                if (!aReference && aReference.meError == api::Error::IllegalArgument)
                    return bLegacyErrorType ? 524 : 4;
            }
            else if (rReferenceNode.meKind == formula::NodeKind::NamedReference)
            {
                aReference = resolveNamedRange(rReferenceNode.maPrimaryText, rCurrentAddress.mnSheet);
                if (!aReference && aReference.meError == api::Error::NotAvailable)
                    return bLegacyErrorType ? 525 : 5;
            }
            else
            {
                return std::nullopt;
            }

            if (!aReference || !aReference.maValue.isSingleCell())
                return bLegacyErrorType ? std::optional<sal_Int32>(519) : std::nullopt;

            const workbook::Cell* pCell = getCell(aReference.maValue.maRange.maStart);
            if (!pCell)
                return std::nullopt;

            if (pCell->maRawValueType == u"error")
            {
                if (bLegacyErrorType)
                    return classifyLegacyErrorTypeLiteral(pCell->maRawValue);
                return classifyOdfErrorTypeLiteral(pCell->maRawValue);
            }

            if (pCell->maValue.isError())
            {
                if (bLegacyErrorType)
                    return classifyLegacyErrorType(pCell->maValue.meError);
                return classifyOdfErrorType(pCell->maValue.meError);
            }

            return std::nullopt;
        };

        if (rArgument.meKind == formula::NodeKind::ErrorLiteral)
        {
            const api::String aUpperLiteral = uppercaseAscii(rArgument.maPrimaryText);
            if (!bLegacyErrorType
                && (aUpperLiteral == u"#GETTING_DATA"
                    || (aUpperLiteral.starts_with(u"#ERR") && aUpperLiteral.size() > 5
                        && aUpperLiteral.back() == u'!')))
            {
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
            }
            const std::optional<sal_Int32> oErrorType = bLegacyErrorType
                                                            ? classifyLegacyErrorTypeLiteral(
                                                                  rArgument.maPrimaryText)
                                                            : std::optional<sal_Int32>(
                                                                  classifyOdfErrorTypeLiteral(
                                                                      rArgument.maPrimaryText));
            if (oErrorType)
            {
                return makeScalarResult(api::CellValue::number(static_cast<double>(*oErrorType)));
            }
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        if (const auto oReferenceErrorType = classifyReferenceErrorType(rArgument))
        {
            return makeScalarResult(api::CellValue::number(static_cast<double>(*oReferenceErrorType)));
        }

        EvaluationResult aArgument = evaluateNode(rArgument, rCurrentAddress);
        if (!aArgument)
        {
            if (rArgument.meKind == formula::NodeKind::NamedReference
                && aArgument.meError == api::Error::NotAvailable)
            {
                return makeScalarResult(
                    api::CellValue::number(bLegacyErrorType ? 525.0 : 5.0));
            }
            if ((rArgument.meKind == formula::NodeKind::CellReference
                    || rArgument.meKind == formula::NodeKind::RangeReference)
                && aArgument.meError == api::Error::IllegalArgument)
            {
                return makeScalarResult(
                    api::CellValue::number(bLegacyErrorType ? 524.0 : 4.0));
            }
            const std::optional<sal_Int32> oErrorType = bLegacyErrorType
                                                            ? classifyLegacyErrorType(
                                                                  aArgument.meError)
                                                            : std::optional<sal_Int32>(
                                                                  classifyOdfErrorType(
                                                                      aArgument.meError));
            if (oErrorType)
                return makeScalarResult(api::CellValue::number(static_cast<double>(*oErrorType)));
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        if (!aArgument.maValue.isScalar() || !aArgument.maValue.maValue.isError())
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        const std::optional<sal_Int32> oErrorType = bLegacyErrorType
                                                        ? classifyLegacyErrorType(
                                                              aArgument.maValue.maValue.meError)
                                                        : std::optional<sal_Int32>(
                                                              classifyOdfErrorType(
                                                                  aArgument.maValue.maValue
                                                                      .meError));
        if (oErrorType)
            return makeScalarResult(api::CellValue::number(static_cast<double>(*oErrorType)));
        return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
    }

    if (aFunctionName == u"IFERROR" || aFunctionName == u"IFNA")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);
        if (rNode.maChildren[0]->meKind == formula::NodeKind::EmptyArgument)
            return makeFailure(api::Error::IllegalArgument);

        const bool bNAOnly = aFunctionName == u"IFNA";
        EvaluationResult aPrimary = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aPrimary)
        {
            const auto eAction
                = api::logic::selectIfErrorAction(aPrimary.meError, bNAOnly);
            if (eAction == api::logic::IfErrorAction::KeepPrimary)
                return aPrimary;
            return evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        }

        if (aPrimary.maValue.isScalar() && aPrimary.maValue.maValue.isError())
        {
            const auto eAction = api::logic::selectIfErrorAction(
                aPrimary.maValue.maValue.meError, bNAOnly);
            if (eAction == api::logic::IfErrorAction::EvaluateAlternate)
                return evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        }

        return aPrimary;
    }

    if (aFunctionName == u"COUNTIF" || aFunctionName == u"COUNTIFS"
        || aFunctionName == u"SUMIF" || aFunctionName == u"SUMIFS"
        || aFunctionName == u"AVERAGEIF" || aFunctionName == u"AVERAGEIFS"
        || aFunctionName == u"MAXIFS" || aFunctionName == u"MINIFS")
    {
        const auto eQuerySearchType = toQuerySearchType(mrWorkbook.meFormulaSearchType);
        const EvaluatorCriteriaAggregateMaterializer aMaterializer(*this);

        auto evaluateAggregateInput = [&](const formula::Node& rArgument)
            -> api::ValueResult<CriteriaAggregateInput> {
            EvaluationResult aValue = evaluateNode(rArgument, rCurrentAddress);
            if (!aValue)
                return api::ValueResult<CriteriaAggregateInput>::failure(aValue.meError);
            const auto oInput = makeCriteriaAggregateInput(aValue);
            if (!oInput)
                return api::ValueResult<CriteriaAggregateInput>::failure(api::Error::IllegalArgument);
            return api::ValueResult<CriteriaAggregateInput>::success(*oInput);
        };

        auto evaluateCriteria = [&](const formula::Node& rArgument)
            -> api::ValueResult<CriteriaPredicate> {
            EvaluationResult aValue
                = ensureScalarValue(*this, evaluateNode(rArgument, rCurrentAddress));
            if (!aValue)
                return api::ValueResult<CriteriaPredicate>::failure(aValue.meError);

            api::CellValue aCriteriaValue = aValue.maValue.maValue;
            const bool bReferenceLikeArgument = rArgument.meKind == formula::NodeKind::CellReference
                                                || rArgument.meKind == formula::NodeKind::RangeReference
                                                || rArgument.meKind == formula::NodeKind::NamedReference;
            if (bReferenceLikeArgument && aCriteriaValue.isEmpty())
                aCriteriaValue = api::CellValue::number(0.0);

            const auto oCriteria = sequery::makeCriteriaPredicate(
                aCriteriaValue, sedatetime::parseStandaloneNumberText, parseAsciiDouble);
            if (!oCriteria)
                return api::ValueResult<CriteriaPredicate>::failure(api::Error::IllegalArgument);
            return api::ValueResult<CriteriaPredicate>::success(*oCriteria);
        };

        if (aFunctionName == u"COUNTIF")
        {
            if (rNode.maChildren.size() != 2)
                return makeFailure(api::Error::IllegalArgument);

            const auto aRange = evaluateAggregateInput(*rNode.maChildren[0]);
            if (!aRange)
                return makeFailure(aRange.meError);
            const auto aCriteria = evaluateCriteria(*rNode.maChildren[1]);
            if (!aCriteria)
                return makeFailure(aCriteria.meError);

            const auto aResult = sequery::evaluateCriteriaAggregate(aMaterializer,
                { aRange.maValue }, { aCriteria.maValue }, nullptr,
                CriteriaAggregateKind::Count, eQuerySearchType,
                mrWorkbook.mbSearchCriteriaMustApplyToWholeCell);
            return aResult ? makeScalarResult(aResult.maValue) : makeFailure(aResult.meError);
        }

        if (aFunctionName == u"COUNTIFS")
        {
            if (rNode.maChildren.size() < 2 || (rNode.maChildren.size() % 2) != 0)
                return makeFailure(api::Error::IllegalArgument);

            std::vector<CriteriaAggregateInput> aRanges;
            std::vector<CriteriaPredicate> aCriteria;
            aRanges.reserve(rNode.maChildren.size() / 2);
            aCriteria.reserve(rNode.maChildren.size() / 2);
            for (std::size_t nIndex = 0; nIndex < rNode.maChildren.size(); nIndex += 2)
            {
                const auto aRange = evaluateAggregateInput(*rNode.maChildren[nIndex]);
                if (!aRange)
                    return makeFailure(aRange.meError);
                const auto aCriterion = evaluateCriteria(*rNode.maChildren[nIndex + 1]);
                if (!aCriterion)
                    return makeFailure(aCriterion.meError);
                aRanges.push_back(aRange.maValue);
                aCriteria.push_back(aCriterion.maValue);
            }

            const auto aResult = sequery::evaluateCriteriaAggregate(aMaterializer, aRanges,
                aCriteria, nullptr, CriteriaAggregateKind::Count, eQuerySearchType,
                mrWorkbook.mbSearchCriteriaMustApplyToWholeCell);
            return aResult ? makeScalarResult(aResult.maValue) : makeFailure(aResult.meError);
        }

        if (aFunctionName == u"SUMIF" || aFunctionName == u"AVERAGEIF")
        {
            if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
                return makeFailure(api::Error::IllegalArgument);

            const auto aCriteriaRange = evaluateAggregateInput(*rNode.maChildren[0]);
            if (!aCriteriaRange)
                return makeFailure(aCriteriaRange.meError);
            const auto aCriteria = evaluateCriteria(*rNode.maChildren[1]);
            if (!aCriteria)
                return makeFailure(aCriteria.meError);

            std::optional<CriteriaAggregateInput> oTargetRange;
            if (rNode.maChildren.size() == 3)
            {
                const auto aTarget = evaluateAggregateInput(*rNode.maChildren[2]);
                if (!aTarget)
                    return makeFailure(aTarget.meError);
                oTargetRange = aTarget.maValue;
            }

            const auto aResult = sequery::evaluateCriteriaAggregate(aMaterializer,
                { aCriteriaRange.maValue }, { aCriteria.maValue },
                oTargetRange ? &*oTargetRange : nullptr,
                aFunctionName == u"SUMIF" ? CriteriaAggregateKind::Sum
                                          : CriteriaAggregateKind::Average,
                eQuerySearchType,
                mrWorkbook.mbSearchCriteriaMustApplyToWholeCell);
            return aResult ? makeScalarResult(aResult.maValue) : makeFailure(aResult.meError);
        }

        if (rNode.maChildren.size() < 3 || (rNode.maChildren.size() % 2) == 0)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTargetRange = evaluateAggregateInput(*rNode.maChildren[0]);
        if (!aTargetRange)
            return makeFailure(aTargetRange.meError);

        std::vector<CriteriaAggregateInput> aRanges;
        std::vector<CriteriaPredicate> aCriteria;
        aRanges.reserve((rNode.maChildren.size() - 1) / 2);
        aCriteria.reserve((rNode.maChildren.size() - 1) / 2);
        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); nIndex += 2)
        {
            const auto aRange = evaluateAggregateInput(*rNode.maChildren[nIndex]);
            if (!aRange)
                return makeFailure(aRange.meError);
            const auto aCriterion = evaluateCriteria(*rNode.maChildren[nIndex + 1]);
            if (!aCriterion)
                return makeFailure(aCriterion.meError);
            aRanges.push_back(aRange.maValue);
            aCriteria.push_back(aCriterion.maValue);
        }

        CriteriaAggregateKind eAggregateKind = CriteriaAggregateKind::Sum;
        if (aFunctionName == u"AVERAGEIFS")
            eAggregateKind = CriteriaAggregateKind::Average;
        else if (aFunctionName == u"MAXIFS")
            eAggregateKind = CriteriaAggregateKind::Max;
        else if (aFunctionName == u"MINIFS")
            eAggregateKind = CriteriaAggregateKind::Min;

        const auto aResult = sequery::evaluateCriteriaAggregate(aMaterializer, aRanges, aCriteria,
            &aTargetRange.maValue, eAggregateKind, eQuerySearchType,
            mrWorkbook.mbSearchCriteriaMustApplyToWholeCell);
        return aResult ? makeScalarResult(aResult.maValue) : makeFailure(aResult.meError);
    }

    if (aFunctionName == u"T.TEST" || aFunctionName == u"TTEST")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aTailsResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aTailsResult)
            return aTailsResult;
        const auto aTailsNumber = coerceToNumber(aTailsResult.maValue.maValue);
        if (!aTailsNumber)
            return makeFailure(aTailsNumber.meError);

        EvaluationResult aTypeResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
        if (!aTypeResult)
            return aTypeResult;
        const auto aTypeNumber = coerceToNumber(aTypeResult.maValue.maValue);
        if (!aTypeNumber)
            return makeFailure(aTypeNumber.meError);

        const auto oTails = toWholeNumber(aTailsNumber.maValue);
        const auto oType = toWholeNumber(aTypeNumber.maValue);
        if (!oTails || !oType || (*oTails != 1 && *oTails != 2) || (*oType < 1 || *oType > 3)
            || *oType == 1)
        {
            return makeScalarResult(api::CellValue::error(api::Error::NoValue));
        }

        return makeFailure(api::Error::IllegalArgument);
    }

    if (aFunctionName == u"FISHER")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;
        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);
        const auto aFisher = semath::fisherTransform(aNumber.maValue);
        if (!aFisher)
            return makeFailure(aFisher.meError);
        return makeScalarResult(api::CellValue::number(aFisher.maValue));
    }

    if (aFunctionName == u"FISHERINV")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;
        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);
        return makeScalarResult(
            api::CellValue::number(semath::inverseFisherTransform(aNumber.maValue)));
    }

    if (aFunctionName == u"ATANH")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;
        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);

        const auto aResult = api::math::inverseHyperbolicTangent(aNumber.maValue);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::number(aResult.maValue));
    }

    if (aFunctionName == u"GAUSS")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;
        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);

        return makeScalarResult(api::CellValue::number(semath::gaussValue(aNumber.maValue)));
    }

    if (aFunctionName == u"GAMMALN" || aFunctionName == u"GAMMALN.PRECISE")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;
        const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
        if (!aNumber)
            return makeFailure(aNumber.meError);
        if (!(aNumber.maValue > 0.0))
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(std::lgamma(aNumber.maValue)));
    }

    if (aFunctionName == u"GCD" || aFunctionName == u"LCM")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumbers = collectNumericArguments(true, true);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);
        if (aNumbers.maValue.empty())
            return makeFailure(api::Error::IllegalArgument);

        sal_Int64 nResult = 0;
        bool bSawValue = false;
        for (const double fValue : aNumbers.maValue)
        {
            if (!std::isfinite(fValue) || fValue < 0.0)
                return makeFailure(api::Error::IllegalArgument);

            const auto fTruncated = std::trunc(fValue);
            if (fTruncated < static_cast<double>(std::numeric_limits<sal_Int64>::min())
                || fTruncated > static_cast<double>(std::numeric_limits<sal_Int64>::max()))
            {
                return makeFailure(api::Error::IllegalArgument);
            }

            const sal_Int64 nValue = static_cast<sal_Int64>(fTruncated);
            if (!bSawValue)
            {
                nResult = std::abs(nValue);
                bSawValue = true;
                continue;
            }

            if (aFunctionName == u"GCD")
                nResult = std::gcd(nResult, std::abs(nValue));
            else
                nResult = std::lcm(nResult, std::abs(nValue));
        }

        return makeScalarResult(api::CellValue::number(static_cast<double>(nResult)));
    }

    if (aFunctionName == u"GEOMEAN" || aFunctionName == u"HARMEAN")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumbers = collectNumericArguments(true);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);
        if (aNumbers.maValue.empty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aResult = (aFunctionName == u"GEOMEAN")
                                 ? semath::evaluateGeometricMeanNumbers(aNumbers.maValue)
                                 : semath::evaluateHarmonicMeanNumbers(aNumbers.maValue);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::number(aResult.maValue));
    }

    if (aFunctionName == u"FV" || aFunctionName == u"PV" || aFunctionName == u"PMT")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aNper = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aNper)
            return makeFailure(aNper.meError);
        const auto aPayment = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPayment)
            return makeFailure(aPayment.meError);

        double fOptionalEndpointValue = 0.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aEndpointValue = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aEndpointValue)
                return makeFailure(aEndpointValue.meError);
            fOptionalEndpointValue = aEndpointValue.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() == 5)
        {
            const auto aPayType = evaluatePayTypeArgument(*rNode.maChildren[4], false);
            if (!aPayType)
                return makeFailure(aPayType.meError);
            bPayInAdvance = aPayType.maValue;
        }

        if (aFunctionName == u"FV")
        {
            const auto aFutureValue = sefinance::evaluateFutureValue(
                aRate.maValue, aNper.maValue, aPayment.maValue, fOptionalEndpointValue,
                bPayInAdvance);
            if (!aFutureValue)
                return makeFailure(aFutureValue.meError);
            return makeScalarResult(api::CellValue::number(aFutureValue.maValue));
        }

        if (aFunctionName == u"PV")
        {
            const auto aPresentValue = sefinance::evaluatePresentValue(
                aRate.maValue, aNper.maValue, aPayment.maValue, fOptionalEndpointValue,
                bPayInAdvance);
            if (!aPresentValue)
                return makeFailure(aPresentValue.meError);
            return makeScalarResult(api::CellValue::number(aPresentValue.maValue));
        }

        if (aFunctionName == u"PMT")
        {
            const auto aPaymentValue = sefinance::evaluatePayment(
                aRate.maValue, aNper.maValue, aPayment.maValue, fOptionalEndpointValue,
                bPayInAdvance);
            if (!aPaymentValue)
                return makeFailure(aPaymentValue.meError);
            return makeScalarResult(api::CellValue::number(aPaymentValue.maValue));
        }

    }

    if (aFunctionName == u"NPER")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aPayment = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPayment)
            return makeFailure(aPayment.meError);
        const auto aPresentValue = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aFutureValue = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aFutureValue)
                return makeFailure(aFutureValue.meError);
            fFutureValue = aFutureValue.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() == 5)
        {
            const auto aPayType = evaluatePayTypeArgument(*rNode.maChildren[4], false);
            if (!aPayType)
                return makeFailure(aPayType.meError);
            bPayInAdvance = aPayType.maValue;
        }

        const auto aPeriods = sefinance::evaluatePeriodsForFutureValue(
            aRate.maValue, aPayment.maValue, aPresentValue.maValue, fFutureValue,
            bPayInAdvance);
        if (!aPeriods)
            return makeFailure(aPeriods.meError);
        return makeScalarResult(api::CellValue::number(aPeriods.maValue));
    }

    if (aFunctionName == u"RATE")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aNper = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aNper)
            return makeFailure(aNper.meError);
        const auto aPayment = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPayment)
            return makeFailure(aPayment.meError);
        const auto aPresentValue = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 4)
        {
            const auto aFutureValue = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
            if (!aFutureValue)
                return makeFailure(aFutureValue.meError);
            fFutureValue = aFutureValue.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() >= 5)
        {
            if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
                bPayInAdvance = false;
            else
            {
                const auto aPayTypeValue = evaluateScalarArgumentValue(*rNode.maChildren[4]);
                if (!aPayTypeValue)
                    return makeFailure(aPayTypeValue.meError);
                if (aPayTypeValue.maValue.isEmpty())
                    return makeFailure(api::Error::IllegalArgument);

                const auto aPayType = coerceToBoolean(aPayTypeValue.maValue);
                if (!aPayType)
                    return makeFailure(aPayType.meError);
                bPayInAdvance = aPayType.maValue;
            }
        }

        double fGuess = 0.1;
        if (rNode.maChildren.size() == 6)
        {
            if (rNode.maChildren[5]->meKind == formula::NodeKind::EmptyArgument)
                fGuess = 0.1;
            else
            {
                const auto aGuessValue = evaluateScalarArgumentValue(*rNode.maChildren[5]);
                if (!aGuessValue)
                    return makeFailure(aGuessValue.meError);
                if (aGuessValue.maValue.isEmpty())
                    return makeFailure(api::Error::IllegalArgument);

                const auto aGuess = coerceToNumber(aGuessValue.maValue);
                if (!aGuess)
                    return makeFailure(aGuess.meError);
                fGuess = aGuess.maValue;
            }
        }

        const auto aRateResult = sefinance::evaluateRate(
            aNper.maValue, aPayment.maValue, aPresentValue.maValue, fFutureValue,
            bPayInAdvance, fGuess);
        if (!aRateResult)
            return makeFailure(aRateResult.meError);
        return makeScalarResult(api::CellValue::number(aRateResult.maValue));
    }

    if (aFunctionName == u"ISPMT")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aPeriod = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPeriod)
            return makeFailure(aPeriod.meError);
        const auto aTotalPeriods = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aTotalPeriods)
            return makeFailure(aTotalPeriods.meError);
        const auto aInvestment = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aInvestment)
            return makeFailure(aInvestment.meError);

        const auto aInterest = sefinance::evaluateInterestSchedulePayment(
            aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue, aInvestment.maValue);
        if (!aInterest)
            return makeFailure(aInterest.meError);
        return makeScalarResult(api::CellValue::number(aInterest.maValue));
    }

    if (aFunctionName == u"IPMT" || aFunctionName == u"PPMT")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aPeriod = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aPeriod)
            return makeFailure(aPeriod.meError);
        const auto aTotalPeriods = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aTotalPeriods)
            return makeFailure(aTotalPeriods.meError);
        const auto aPresentValue = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);

        double fFutureValue = 0.0;
        if (rNode.maChildren.size() >= 5)
        {
            const auto aFutureValue = evaluateNumericArgument(*rNode.maChildren[4], 0.0);
            if (!aFutureValue)
                return makeFailure(aFutureValue.meError);
            fFutureValue = aFutureValue.maValue;
        }

        bool bPayInAdvance = false;
        if (rNode.maChildren.size() == 6)
        {
            const auto aPayType = evaluatePayTypeArgument(*rNode.maChildren[5], false);
            if (!aPayType)
                return makeFailure(aPayType.meError);
            bPayInAdvance = aPayType.maValue;
        }

        if (aFunctionName == u"IPMT")
        {
            const auto aInterest = sefinance::evaluateInterestPayment(
                aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue,
                aPresentValue.maValue, fFutureValue, bPayInAdvance);
            if (!aInterest)
                return makeFailure(aInterest.meError);
            return makeScalarResult(api::CellValue::number(aInterest.maValue));
        }

        const auto aPrincipal = sefinance::evaluatePrincipalPayment(
            aRate.maValue, aPeriod.maValue, aTotalPeriods.maValue, aPresentValue.maValue,
            fFutureValue, bPayInAdvance);
        if (!aPrincipal)
            return makeFailure(aPrincipal.meError);
        return makeScalarResult(api::CellValue::number(aPrincipal.maValue));
    }

    if (aFunctionName == u"CUMIPMT" || aFunctionName == u"CUMPRINC")
    {
        if (rNode.maChildren.size() != 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRate = evaluateNumericArgument(*rNode.maChildren[0], 0.0);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aTotalPeriods = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aTotalPeriods)
            return makeFailure(aTotalPeriods.meError);
        const auto aPresentValue = evaluateNumericArgument(*rNode.maChildren[2], 0.0);
        if (!aPresentValue)
            return makeFailure(aPresentValue.meError);
        const auto aStart = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aStart)
            return makeFailure(aStart.meError);
        const auto aEnd = evaluateNumericArgument(*rNode.maChildren[4], 0.0);
        if (!aEnd)
            return makeFailure(aEnd.meError);
        const auto aPayType = evaluateStrictPaymentTypeArgument(*rNode.maChildren[5]);
        if (!aPayType)
            return makeFailure(aPayType.meError);

        if (aFunctionName == u"CUMIPMT")
        {
            const auto aInterest = sefinance::evaluateCumulativeInterest(
                aRate.maValue, aTotalPeriods.maValue, aPresentValue.maValue,
                aStart.maValue, aEnd.maValue, aPayType.maValue);
            if (!aInterest)
                return makeFailure(aInterest.meError);
            return makeScalarResult(api::CellValue::number(aInterest.maValue));
        }

        const auto aPrincipal = sefinance::evaluateCumulativePrincipal(
            aRate.maValue, aTotalPeriods.maValue, aPresentValue.maValue,
            aStart.maValue, aEnd.maValue, aPayType.maValue);
        if (!aPrincipal)
            return makeFailure(aPrincipal.meError);
        return makeScalarResult(api::CellValue::number(aPrincipal.maValue));
    }

    if (aFunctionName == u"DDB")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCost = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aCost)
            return makeFailure(aCost.meError);
        const auto aSalvage = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aSalvage)
            return makeFailure(aSalvage.meError);
        const auto aLife = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aLife)
            return makeFailure(aLife.meError);
        const auto aPeriod = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
        if (!aPeriod)
            return makeFailure(aPeriod.meError);

        double fFactor = 2.0;
        if (rNode.maChildren.size() == 5)
        {
            if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
                return makeFailure(api::Error::IllegalArgument);
            const auto aFactor = evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
            if (!aFactor)
                return makeFailure(aFactor.meError);
            fFactor = aFactor.maValue;
        }

        const auto aDepreciation = sefinance::evaluateDoubleDecliningBalance(
            aCost.maValue, aSalvage.maValue, aLife.maValue, aPeriod.maValue, fFactor);
        if (!aDepreciation)
            return makeFailure(aDepreciation.meError);
        return makeScalarResult(api::CellValue::number(aDepreciation.maValue));
    }

    if (aFunctionName == u"VDB")
    {
        if (rNode.maChildren.size() < 5 || rNode.maChildren.size() > 7)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCost = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aCost)
            return makeFailure(aCost.meError);
        const auto aSalvage = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aSalvage)
            return makeFailure(aSalvage.meError);
        const auto aLife = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aLife)
            return makeFailure(aLife.meError);
        const auto aStart = evaluateNumericArgument(*rNode.maChildren[3], 0.0);
        if (!aStart)
            return makeFailure(aStart.meError);
        if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
            return makeFailure(api::Error::IllegalArgument);
        const auto aEnd = evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
        if (!aEnd)
            return makeFailure(aEnd.meError);

        double fFactor = 2.0;
        if (rNode.maChildren.size() >= 6)
        {
            if (rNode.maChildren[5]->meKind == formula::NodeKind::EmptyArgument)
                return makeFailure(api::Error::IllegalArgument);
            const auto aFactor = evaluateNumericArgument(*rNode.maChildren[5], std::nullopt);
            if (!aFactor)
                return makeFailure(aFactor.meError);
            fFactor = aFactor.maValue;
        }

        bool bNoSwitch = false;
        if (rNode.maChildren.size() == 7)
        {
            const auto aNoSwitch = evaluatePayTypeArgument(*rNode.maChildren[6], false);
            if (!aNoSwitch)
                return makeFailure(aNoSwitch.meError);
            bNoSwitch = aNoSwitch.maValue;
        }

        const auto aDepreciation = sefinance::evaluateVariableDecliningBalance(
            aCost.maValue, aSalvage.maValue, aLife.maValue, aStart.maValue,
            aEnd.maValue, fFactor, bNoSwitch);
        if (!aDepreciation)
            return makeFailure(aDepreciation.meError);
        return makeScalarResult(api::CellValue::number(aDepreciation.maValue));
    }

    if (aFunctionName == u"YEARFRAC" || aFunctionName == u"GETYEARFRAC")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aStartDate = evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aStartDate)
            return makeFailure(aStartDate.meError);
        const auto aEndDate = evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aEndDate)
            return makeFailure(aEndDate.meError);

        sal_Int32 nBasis = 0;
        if (rNode.maChildren.size() == 3)
        {
            const auto aBasis = evaluateOptionalWholeNumberArgument(*rNode.maChildren[2], 0);
            if (!aBasis)
                return makeFailure(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aYearFraction = sefinance::evaluateYearFraction(
            sedatetime::defaultNullDate(), aStartDate.maValue, aEndDate.maValue, nBasis);
        if (!aYearFraction)
            return makeFailure(aYearFraction.meError);
        return makeScalarResult(api::CellValue::number(aYearFraction.maValue));
    }

    if (aFunctionName == u"PRICE")
    {
        if (rNode.maChildren.size() < 6 || rNode.maChildren.size() > 7)
            return makeFailure(api::Error::IllegalArgument);

        const auto aSettlement = evaluateRequiredDateArgument(*rNode.maChildren[0]);
        if (!aSettlement)
            return makeFailure(aSettlement.meError);
        const auto aMaturity = evaluateRequiredDateArgument(*rNode.maChildren[1]);
        if (!aMaturity)
            return makeFailure(aMaturity.meError);
        const auto aRate = evaluateRequiredNumberArgument(*rNode.maChildren[2]);
        if (!aRate)
            return makeFailure(aRate.meError);
        const auto aYield = evaluateRequiredNumberArgument(*rNode.maChildren[3]);
        if (!aYield)
            return makeFailure(aYield.meError);
        const auto aRedemption = evaluateRequiredNumberArgument(*rNode.maChildren[4]);
        if (!aRedemption)
            return makeFailure(aRedemption.meError);
        const auto aFrequency = evaluateRequiredWholeNumberArgument(*rNode.maChildren[5]);
        if (!aFrequency)
            return makeFailure(aFrequency.meError);

        sal_Int32 nBasis = 0;
        if (rNode.maChildren.size() == 7)
        {
            const auto aBasis = evaluateOptionalWholeNumberArgument(*rNode.maChildren[6], 0);
            if (!aBasis)
                return makeFailure(aBasis.meError);
            nBasis = aBasis.maValue;
        }

        const auto aPrice = sefinance::evaluatePrice(
            sedatetime::defaultNullDate(), aSettlement.maValue, aMaturity.maValue,
            aRate.maValue, aYield.maValue, aRedemption.maValue, aFrequency.maValue, nBasis);
        if (!aPrice)
            return makeFailure(aPrice.meError);
        return makeScalarResult(api::CellValue::number(aPrice.maValue));
    }

    if (aFunctionName == u"POISSON" || aFunctionName == u"POISSON.DIST")
    {
        const bool bLegacyPoisson = aFunctionName == u"POISSON";
        if ((bLegacyPoisson && (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3))
            || (!bLegacyPoisson && rNode.maChildren.size() != 3))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);

        EvaluationResult aLambdaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aLambdaResult)
            return aLambdaResult;
        const auto aLambdaNumber = coerceToNumber(aLambdaResult.maValue.maValue);
        if (!aLambdaNumber)
            return makeFailure(aLambdaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 3)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aPoisson = semath::evaluatePoissonDistribution(
            aXNumber.maValue, aLambdaNumber.maValue, bCumulative);
        if (!aPoisson)
            return makeFailure(aPoisson.meError);
        return makeScalarResult(api::CellValue::number(aPoisson.maValue));
    }

    if (aFunctionName == u"LEGACY.CHIDIST")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aChiResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aChiResult)
            return aChiResult;
        EvaluationResult aDfResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aDfResult)
            return aDfResult;

        const auto aChiNumber = coerceToNumber(aChiResult.maValue.maValue);
        if (!aChiNumber)
            return makeFailure(aChiNumber.meError);
        const auto aDfNumber = coerceToNumber(aDfResult.maValue.maValue);
        if (!aDfNumber)
            return makeFailure(aDfNumber.meError);

        const auto aChiDist = semath::evaluateLegacyChiDist(
            aChiNumber.maValue, ::rtl::math::approxFloor(aDfNumber.maValue));
        if (!aChiDist)
            return makeFailure(aChiDist.meError);
        return makeScalarResult(api::CellValue::number(aChiDist.maValue));
    }

    if (aFunctionName == u"CHISQDIST" || aFunctionName == u"CHISQ.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName == u"CHISQ.DIST";
        if ((bMicrosoftSyntax && rNode.maChildren.size() != 3)
            || (!bMicrosoftSyntax
                && (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aDfResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        if (!aDfResult)
            return aDfResult;

        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        const auto aDfNumber = coerceToNumber(aDfResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aDfNumber)
            return makeFailure(aDfNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 3)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = semath::evaluateChiSquareDistribution(
            aXNumber.maValue, ::rtl::math::approxFloor(aDfNumber.maValue), bCumulative,
            bMicrosoftSyntax);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"NORMDIST" || aFunctionName == u"NORM.DIST")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aMeanResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aSigmaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        if (!aMeanResult)
            return aMeanResult;
        if (!aSigmaResult)
            return aSigmaResult;

        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        const auto aMeanNumber = coerceToNumber(aMeanResult.maValue.maValue);
        const auto aSigmaNumber = coerceToNumber(aSigmaResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aMeanNumber)
            return makeFailure(aMeanNumber.meError);
        if (!aSigmaNumber)
            return makeFailure(aSigmaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = semath::evaluateNormalDistribution(
            aXNumber.maValue, aMeanNumber.maValue, aSigmaNumber.maValue, bCumulative);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"LOGNORMDIST" || aFunctionName == u"LOGNORM.DIST"
        || aFunctionName == u"COM.MICROSOFT.LOGNORM.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName != u"LOGNORMDIST";
        if ((bMicrosoftSyntax && rNode.maChildren.size() != 4)
            || (!bMicrosoftSyntax
                && (rNode.maChildren.empty() || rNode.maChildren.size() > 4)))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        const auto aXNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);

        const auto aMeanNumber = rNode.maChildren.size() >= 2
                                     ? evaluateNumericArgument(*rNode.maChildren[1], 0.0)
                                     : api::ValueResult<double>::success(0.0);
        if (!aMeanNumber)
            return makeFailure(aMeanNumber.meError);

        const auto aSigmaNumber = rNode.maChildren.size() >= 3
                                      ? evaluateNumericArgument(*rNode.maChildren[2], 1.0)
                                      : api::ValueResult<double>::success(1.0);
        if (!aSigmaNumber)
            return makeFailure(aSigmaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = semath::evaluateLogNormalDistribution(
            aXNumber.maValue, aMeanNumber.maValue, aSigmaNumber.maValue, bCumulative);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"GAMMADIST" || aFunctionName == u"GAMMA.DIST")
    {
        const bool bMicrosoftSyntax = aFunctionName == u"GAMMA.DIST";
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aAlphaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aBetaResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        if (!aAlphaResult)
            return aAlphaResult;
        if (!aBetaResult)
            return aBetaResult;

        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        const auto aAlphaNumber = coerceToNumber(aAlphaResult.maValue.maValue);
        const auto aBetaNumber = coerceToNumber(aBetaResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aAlphaNumber)
            return makeFailure(aAlphaNumber.meError);
        if (!aBetaNumber)
            return makeFailure(aBetaNumber.meError);

        bool bCumulative = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = semath::evaluateGammaDistribution(
            aXNumber.maValue, aAlphaNumber.maValue, aBetaNumber.maValue, bCumulative,
            bMicrosoftSyntax);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"GAMMA" || aFunctionName == u"COM.MICROSOFT.GAMMA")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aNumber)
            return makeFailure(aNumber.meError);

        const auto aGammaValue = semath::evaluateGammaValue(aNumber.maValue);
        if (!aGammaValue)
            return makeFailure(aGammaValue.meError);
        return makeScalarResult(api::CellValue::number(aGammaValue.maValue));
    }

    if (aFunctionName == u"TINV" || aFunctionName == u"T.INV.2T"
        || aFunctionName == u"COM.MICROSOFT.T.INV.2T")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aProbability = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDegreesFreedom = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aProbability)
            return makeFailure(aProbability.meError);
        if (!aDegreesFreedom)
            return makeFailure(aDegreesFreedom.meError);

        const auto aInverse = semath::evaluateTInverse(
            aProbability.maValue, ::rtl::math::approxFloor(aDegreesFreedom.maValue), 2);
        if (!aInverse)
            return makeFailure(aInverse.meError);
        return makeScalarResult(api::CellValue::number(aInverse.maValue));
    }

    if (aFunctionName == u"T.DIST.2T" || aFunctionName == u"COM.MICROSOFT.T.DIST.2T")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aX = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDegreesFreedom = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aX)
            return makeFailure(aX.meError);
        if (!aDegreesFreedom)
            return makeFailure(aDegreesFreedom.meError);
        if (aX.maValue < 0.0)
            return makeFailure(api::Error::IllegalArgument);

        const auto aDistribution = semath::evaluateStudentDistribution(
            aX.maValue, ::rtl::math::approxFloor(aDegreesFreedom.maValue), 2);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"FINV" || aFunctionName == u"LEGACY.FINV"
        || aFunctionName == u"F.INV.RT" || aFunctionName == u"COM.MICROSOFT.F.INV.RT"
        || aFunctionName == u"F.INV" || aFunctionName == u"COM.MICROSOFT.F.INV")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aProbability = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDegreesFreedom1 = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        const auto aDegreesFreedom2 = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aProbability)
            return makeFailure(aProbability.meError);
        if (!aDegreesFreedom1)
            return makeFailure(aDegreesFreedom1.meError);
        if (!aDegreesFreedom2)
            return makeFailure(aDegreesFreedom2.meError);

        const double fDegreesFreedom1 = ::rtl::math::approxFloor(aDegreesFreedom1.maValue);
        const double fDegreesFreedom2 = ::rtl::math::approxFloor(aDegreesFreedom2.maValue);
        const bool bLeftTail = aFunctionName == u"FINV" || aFunctionName == u"F.INV"
                               || aFunctionName == u"COM.MICROSOFT.F.INV";
        if (bLeftTail && (aProbability.maValue <= 0.0 || aProbability.maValue >= 1.0))
            return makeFailure(api::Error::IllegalArgument);
        const double fRightTailProbability
            = bLeftTail ? 1.0 - aProbability.maValue : aProbability.maValue;
        const auto aInverse = semath::evaluateFInverseRightTail(
            fRightTailProbability, fDegreesFreedom1, fDegreesFreedom2);
        if (!aInverse)
            return makeFailure(aInverse.meError);
        return makeScalarResult(api::CellValue::number(aInverse.maValue));
    }

    if (aFunctionName == u"VAR" || aFunctionName == u"VAR.S" || aFunctionName == u"VARP"
        || aFunctionName == u"VAR.P" || aFunctionName == u"VARA" || aFunctionName == u"VARPA"
        || aFunctionName == u"STDEV" || aFunctionName == u"STDEV.S"
        || aFunctionName == u"STDEVP" || aFunctionName == u"STDEV.P"
        || aFunctionName == u"STDEVA" || aFunctionName == u"STDEVPA")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        const bool bTextAsZero = aFunctionName == u"VARA" || aFunctionName == u"VARPA"
                                 || aFunctionName == u"STDEVA" || aFunctionName == u"STDEVPA";
        const auto aNumbers = collectVarianceArguments(bTextAsZero);
        if (!aNumbers)
            return makeFailure(aNumbers.meError);

        const bool bSample = aFunctionName == u"VAR" || aFunctionName == u"VAR.S"
                             || aFunctionName == u"VARA" || aFunctionName == u"STDEV"
                             || aFunctionName == u"STDEV.S" || aFunctionName == u"STDEVA";
        const bool bReturnStdDev = aFunctionName == u"STDEV" || aFunctionName == u"STDEV.S"
                                   || aFunctionName == u"STDEVP"
                                   || aFunctionName == u"STDEV.P"
                                   || aFunctionName == u"STDEVA"
                                   || aFunctionName == u"STDEVPA";
        const auto aVariance = semath::evaluateVarianceNumbers(
            aNumbers.maValue, bSample, bReturnStdDev);
        if (!aVariance)
            return makeFailure(aVariance.meError);
        return makeScalarResult(api::CellValue::number(aVariance.maValue));
    }

    if (aFunctionName == u"BINOMDIST" || aFunctionName == u"BINOM.DIST")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aXResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aNResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aPResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        EvaluationResult aCumulativeResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
        if (!aXResult)
            return aXResult;
        if (!aNResult)
            return aNResult;
        if (!aPResult)
            return aPResult;
        if (!aCumulativeResult)
            return aCumulativeResult;

        const auto aXNumber = coerceToNumber(aXResult.maValue.maValue);
        const auto aNNumber = coerceToNumber(aNResult.maValue.maValue);
        const auto aPNumber = coerceToNumber(aPResult.maValue.maValue);
        const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aNNumber)
            return makeFailure(aNNumber.meError);
        if (!aPNumber)
            return makeFailure(aPNumber.meError);
        if (!aCumulativeBool)
            return makeFailure(aCumulativeBool.meError);

        const auto aBinomial = semath::evaluateBinomialDistribution(
            aXNumber.maValue, aNNumber.maValue, aPNumber.maValue, aCumulativeBool.maValue);
        if (!aBinomial)
            return makeFailure(aBinomial.meError);
        return makeScalarResult(api::CellValue::number(aBinomial.maValue));
    }

    if (aFunctionName == u"BINOM.INV")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTrials = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aProbability = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        const auto aAlpha = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        if (!aTrials)
            return makeFailure(aTrials.meError);
        if (!aProbability)
            return makeFailure(aProbability.meError);
        if (!aAlpha)
            return makeFailure(aAlpha.meError);

        const auto aInverse = semath::evaluateBinomialInverse(
            aTrials.maValue, aProbability.maValue, aAlpha.maValue);
        if (!aInverse)
            return makeFailure(aInverse.meError);
        return makeScalarResult(api::CellValue::number(aInverse.maValue));
    }

    if (aFunctionName == u"BINOM.DIST.RANGE" || aFunctionName == u"B")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aNResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        EvaluationResult aPResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        EvaluationResult aStartResult
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aNResult)
            return aNResult;
        if (!aPResult)
            return aPResult;
        if (!aStartResult)
            return aStartResult;

        const auto aNNumber = coerceToNumber(aNResult.maValue.maValue);
        const auto aPNumber = coerceToNumber(aPResult.maValue.maValue);
        const auto aStartNumber = coerceToNumber(aStartResult.maValue.maValue);
        if (!aNNumber)
            return makeFailure(aNNumber.meError);
        if (!aPNumber)
            return makeFailure(aPNumber.meError);
        if (!aStartNumber)
            return makeFailure(aStartNumber.meError);

        double fEnd = aStartNumber.maValue;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aEndResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aEndResult)
                return aEndResult;
            const auto aEndNumber = coerceToNumber(aEndResult.maValue.maValue);
            if (!aEndNumber)
                return makeFailure(aEndNumber.meError);
            fEnd = aEndNumber.maValue;
        }

        const auto aRange = semath::evaluateBinomialRangeDistribution(
            aNNumber.maValue, aPNumber.maValue, aStartNumber.maValue, fEnd);
        if (!aRange)
            return makeFailure(aRange.meError);
        return makeScalarResult(api::CellValue::number(aRange.maValue));
    }

    if (aFunctionName == u"HYPGEOMDIST" || aFunctionName == u"HYPGEOM.DIST")
    {
        if (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aX = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aTrials = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        const auto aSuccesses = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
        const auto aPopulation = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
        if (!aX)
            return makeFailure(aX.meError);
        if (!aTrials)
            return makeFailure(aTrials.meError);
        if (!aSuccesses)
            return makeFailure(aSuccesses.meError);
        if (!aPopulation)
            return makeFailure(aPopulation.meError);

        bool bCumulative = false;
        if (rNode.maChildren.size() == 5)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[4], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
        }

        const auto aDistribution = semath::evaluateHypergeometricDistribution(
            aX.maValue, aTrials.maValue, aSuccesses.maValue, aPopulation.maValue, bCumulative);
        if (!aDistribution)
            return makeFailure(aDistribution.meError);
        return makeScalarResult(api::CellValue::number(aDistribution.maValue));
    }

    if (aFunctionName == u"PERCENTRANK" || aFunctionName == u"PERCENTRANK.INC"
        || aFunctionName == u"PERCENTRANK.EXC")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aScan = collectAggregateScanFromArgument(*rNode.maChildren[0]);
        if (!aScan)
            return makeFailure(aScan.meError);

        const auto aValue = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        sal_Int32 nSignificance = 3;
        if (rNode.maChildren.size() == 3)
        {
            const auto aSignificance = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aSignificance)
                return makeFailure(aSignificance.meError);
            nSignificance = static_cast<sal_Int32>(::rtl::math::approxFloor(aSignificance.maValue));
        }

        const bool bInclusive = aFunctionName != u"PERCENTRANK.EXC";
        const auto aRank = semath::evaluatePercentrank(
            aScan.maValue.maNumbers, aValue.maValue, bInclusive, nSignificance);
        if (!aRank)
            return makeFailure(aRank.meError);
        return makeScalarResult(api::CellValue::number(aRank.maValue));
    }

    if (aFunctionName == u"MODE.SNGL")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        AggregateScan aScan;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aCollected = collectAggregateScanFromArgument(*pChild);
            if (!aCollected)
                return makeFailure(aCollected.meError);
            aScan.maNumbers.insert(aScan.maNumbers.end(), aCollected.maValue.maNumbers.begin(),
                aCollected.maValue.maNumbers.end());
        }

        const auto aMode = semath::evaluateModeSingle(aScan.maNumbers);
        if (!aMode)
            return makeFailure(aMode.meError);
        return makeScalarResult(api::CellValue::number(aMode.maValue));
    }

    if (aFunctionName == u"TRIMMEAN")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aScan = collectAggregateScanFromArgument(*rNode.maChildren[0]);
        if (!aScan)
            return makeFailure(aScan.meError);

        const auto aPercent = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aPercent)
            return makeFailure(aPercent.meError);

        const auto aTrimmean = semath::evaluateTrimmean(aScan.maValue.maNumbers, aPercent.maValue);
        if (!aTrimmean)
            return makeFailure(aTrimmean.meError);
        return makeScalarResult(api::CellValue::number(aTrimmean.maValue));
    }

    if (aFunctionName == u"CHISQ.TEST" || aFunctionName == u"LEGACY.CHITEST")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aObservedInput = evaluateLookupInputNode(*rNode.maChildren[0]);
        const auto aExpectedInput = evaluateLookupInputNode(*rNode.maChildren[1]);
        if (!aObservedInput)
            return makeFailure(aObservedInput.meError);
        if (!aExpectedInput)
            return makeFailure(aExpectedInput.meError);

        const api::MatrixSize nObservedColumns = aObservedInput.maValue.mbScalar
                                                     ? 1
                                                     : aObservedInput.maValue.mnColumns;
        const api::MatrixSize nObservedRows = aObservedInput.maValue.mbScalar ? 1
                                                                              : aObservedInput.maValue.mnRows;
        const api::MatrixSize nExpectedColumns = aExpectedInput.maValue.mbScalar
                                                     ? 1
                                                     : aExpectedInput.maValue.mnColumns;
        const api::MatrixSize nExpectedRows = aExpectedInput.maValue.mbScalar ? 1
                                                                              : aExpectedInput.maValue.mnRows;
        if (nObservedColumns != nExpectedColumns || nObservedRows != nExpectedRows)
            return makeFailure(api::Error::IllegalArgument);

        auto materializeInputCell = [&](const LookupInput& rInput, api::MatrixSize nColumn,
                                        api::MatrixSize nRow)
            -> api::ValueResult<api::CellValue> {
            if (rInput.mbScalar)
            {
                if (nColumn != 0 || nRow != 0)
                    return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
                return api::ValueResult<api::CellValue>::success(rInput.maScalar);
            }

            const std::size_t nIndex = static_cast<std::size_t>(nRow * rInput.mnColumns + nColumn);
            if (!rInput.maValues.empty())
            {
                if (nIndex >= rInput.maValues.size())
                    return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
                return api::ValueResult<api::CellValue>::success(rInput.maValues[nIndex]);
            }

            EvaluationResult aCell = materializeReferenceValue(rInput.maReference, nColumn, nRow);
            if (!aCell)
                return api::ValueResult<api::CellValue>::failure(aCell.meError);
            if (!aCell.maValue.isScalar())
                return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
            return api::ValueResult<api::CellValue>::success(aCell.maValue.maValue);
        };

        KahanSum fChi = 0.0;
        bool bSawNonEmptyPair = false;
        for (api::MatrixSize nColumn = 0; nColumn < nObservedColumns; ++nColumn)
        {
            for (api::MatrixSize nRow = 0; nRow < nObservedRows; ++nRow)
            {
                const auto aObservedCell = materializeInputCell(aObservedInput.maValue, nColumn, nRow);
                const auto aExpectedCell = materializeInputCell(aExpectedInput.maValue, nColumn, nRow);
                if (!aObservedCell)
                    return makeFailure(aObservedCell.meError);
                if (!aExpectedCell)
                    return makeFailure(aExpectedCell.meError);

                if (aObservedCell.maValue.isEmpty() || aExpectedCell.maValue.isEmpty())
                    continue;

                bSawNonEmptyPair = true;
                if (aObservedCell.maValue.isText() || aExpectedCell.maValue.isText())
                    return makeFailure(api::Error::IllegalArgument);
                if (aObservedCell.maValue.isError())
                    return makeFailure(aObservedCell.maValue.meError);
                if (aExpectedCell.maValue.isError())
                    return makeFailure(aExpectedCell.maValue.meError);

                const auto aObservedNumber = coerceToNumber(aObservedCell.maValue);
                const auto aExpectedNumber = coerceToNumber(aExpectedCell.maValue);
                if (!aObservedNumber)
                    return makeFailure(aObservedNumber.meError);
                if (!aExpectedNumber)
                    return makeFailure(aExpectedNumber.meError);
                if (::rtl::math::approxEqual(aExpectedNumber.maValue, 0.0))
                    return makeFailure(api::Error::DivisionByZero);

                const double fDifference = aObservedNumber.maValue - aExpectedNumber.maValue;
                const double fTerm = (fDifference * fDifference) / aExpectedNumber.maValue;
                if (std::isinf(fTerm))
                    return makeFailure(api::Error::NoConvergence);
                fChi += fTerm;
            }
        }

        if (!bSawNonEmptyPair)
            return makeFailure(api::Error::IllegalArgument);

        double fDegreesFreedom = 0.0;
        if (nObservedColumns == 1 || nObservedRows == 1)
        {
            fDegreesFreedom = static_cast<double>(nObservedColumns * nObservedRows - 1);
            if (::rtl::math::approxEqual(fDegreesFreedom, 0.0))
                return makeFailure(api::Error::NotAvailable);
        }
        else
        {
            fDegreesFreedom
                = static_cast<double>(nObservedColumns - 1) * static_cast<double>(nObservedRows - 1);
        }

        const auto aChiDist = semath::evaluateLegacyChiDist(fChi.get(), fDegreesFreedom);
        if (!aChiDist)
            return makeFailure(aChiDist.meError);
        return makeScalarResult(api::CellValue::number(aChiDist.maValue));
    }

    if (aFunctionName == u"BETADIST" || aFunctionName == u"BETA.DIST")
    {
        const bool bMicrosoftOrder = aFunctionName == u"BETA.DIST";
        if ((bMicrosoftOrder && (rNode.maChildren.size() < 4 || rNode.maChildren.size() > 6))
            || (!bMicrosoftOrder && (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        auto evaluateScalarNumber = [&](std::size_t nIndex) -> api::ValueResult<double> {
            EvaluationResult aValue
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[nIndex], rCurrentAddress));
            if (!aValue)
                return api::ValueResult<double>::failure(aValue.meError);
            return coerceToNumber(aValue.maValue.maValue);
        };

        const auto aXNumber = evaluateScalarNumber(0);
        const auto aAlphaNumber = evaluateScalarNumber(1);
        const auto aBetaNumber = evaluateScalarNumber(2);
        if (!aXNumber)
            return makeFailure(aXNumber.meError);
        if (!aAlphaNumber)
            return makeFailure(aAlphaNumber.meError);
        if (!aBetaNumber)
            return makeFailure(aBetaNumber.meError);

        bool bCumulative = true;
        double fLowerBound = 0.0;
        double fUpperBound = 1.0;
        if (bMicrosoftOrder)
        {
            EvaluationResult aCumulativeResult
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aCumulativeResult)
                return aCumulativeResult;
            const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
            if (!aCumulativeBool)
                return makeFailure(aCumulativeBool.meError);
            bCumulative = aCumulativeBool.maValue;
            if (rNode.maChildren.size() >= 5)
            {
                const auto aLower = evaluateScalarNumber(4);
                if (!aLower)
                    return makeFailure(aLower.meError);
                fLowerBound = aLower.maValue;
            }
            if (rNode.maChildren.size() >= 6)
            {
                const auto aUpper = evaluateScalarNumber(5);
                if (!aUpper)
                    return makeFailure(aUpper.meError);
                fUpperBound = aUpper.maValue;
            }
        }
        else
        {
            if (rNode.maChildren.size() >= 4)
            {
                const auto aLower = evaluateScalarNumber(3);
                if (!aLower)
                    return makeFailure(aLower.meError);
                fLowerBound = aLower.maValue;
            }
            if (rNode.maChildren.size() >= 5)
            {
                const auto aUpper = evaluateScalarNumber(4);
                if (!aUpper)
                    return makeFailure(aUpper.meError);
                fUpperBound = aUpper.maValue;
            }
            if (rNode.maChildren.size() == 6)
            {
                EvaluationResult aCumulativeResult = ensureScalarValue(
                    *this, evaluateNode(*rNode.maChildren[5], rCurrentAddress));
                if (!aCumulativeResult)
                    return aCumulativeResult;
                const auto aCumulativeBool = coerceToBoolean(aCumulativeResult.maValue.maValue);
                if (!aCumulativeBool)
                    return makeFailure(aCumulativeBool.meError);
                bCumulative = aCumulativeBool.maValue;
            }
        }

        const auto aBetaDistribution = semath::evaluateBetaDistribution(aXNumber.maValue,
            aAlphaNumber.maValue, aBetaNumber.maValue, fLowerBound, fUpperBound, bCumulative,
            bMicrosoftOrder);
        if (!aBetaDistribution)
            return makeFailure(aBetaDistribution.meError);
        return makeScalarResult(api::CellValue::number(aBetaDistribution.maValue));
    }

    if (aFunctionName == u"CLEAN")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        return makeScalarResult(api::CellValue::text(
            api::text::cleanPrintable(aText.maValue)));
    }

    if (aFunctionName == u"BASISODATETIME")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        double fSerialValue = 0.0;
        if (aArgument.maValue.maValue.isText())
        {
            const auto oParsed = sedatetime::parseStandaloneNumberText(aArgument.maValue.maValue.maString);
            if (!oParsed)
                return makeFailure(api::Error::IllegalArgument);
            fSerialValue = oParsed->mfValue;
        }
        else
        {
            const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
            if (!aNumber)
                return makeFailure(aNumber.meError);
            fSerialValue = aNumber.maValue;
        }

        return makeScalarResult(api::CellValue::text(formatBasisDateTime(fSerialValue)));
    }

    if (aFunctionName == u"VALUE" || aFunctionName == u"DATEVALUE" || aFunctionName == u"TIMEVALUE")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        const auto oParsed = sedatetime::parseStandaloneNumberText(aText.maValue);
        if (!oParsed)
            return makeFailure(api::Error::IllegalArgument);

        if (aFunctionName == u"VALUE")
            return makeScalarResult(api::CellValue::number(oParsed->mfValue));

        if (aFunctionName == u"DATEVALUE")
        {
            if (oParsed->meKind != api::NumberParseResult::Kind::Date
                && oParsed->meKind != api::NumberParseResult::Kind::DateTime)
            {
                return makeFailure(api::Error::IllegalArgument);
            }

            return makeScalarResult(api::CellValue::number(
                rtl::math::approxFloor(oParsed->mfValue)));
        }

        if (oParsed->meKind != api::NumberParseResult::Kind::Time
            && oParsed->meKind != api::NumberParseResult::Kind::DateTime)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        return makeScalarResult(api::CellValue::number(
            sedatetime::normalizeTimeFraction(oParsed->mfValue)));
    }

    if (aFunctionName == u"TIME")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aHour
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aHour)
            return aHour;
        EvaluationResult aMinute
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aMinute)
            return aMinute;
        EvaluationResult aSecond
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aSecond)
            return aSecond;

        const auto aHourNumber = coerceToNumber(aHour.maValue.maValue);
        if (!aHourNumber)
            return makeFailure(aHourNumber.meError);
        const auto aMinuteNumber = coerceToNumber(aMinute.maValue.maValue);
        if (!aMinuteNumber)
            return makeFailure(aMinuteNumber.meError);
        const auto aSecondNumber = coerceToNumber(aSecond.maValue.maValue);
        if (!aSecondNumber)
            return makeFailure(aSecondNumber.meError);

        const auto aTimeSerial = api::calendar::makeTimeSerial(
            aHourNumber.maValue, aMinuteNumber.maValue, aSecondNumber.maValue);
        if (!aTimeSerial)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(aTimeSerial.maValue));
    }

    if (aFunctionName == u"DATE")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aYear
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aYear)
            return aYear;
        EvaluationResult aMonth
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aMonth)
            return aMonth;
        EvaluationResult aDay
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aDay)
            return aDay;

        if (aYear.maValue.maValue.isEmpty() || aMonth.maValue.maValue.isEmpty()
            || aDay.maValue.maValue.isEmpty())
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        const auto aYearNumber = coerceToNumber(aYear.maValue.maValue);
        if (!aYearNumber)
            return makeFailure(aYearNumber.meError);
        const auto aMonthNumber = coerceToNumber(aMonth.maValue.maValue);
        if (!aMonthNumber)
            return makeFailure(aMonthNumber.meError);
        const auto aDayNumber = coerceToNumber(aDay.maValue.maValue);
        if (!aDayNumber)
            return makeFailure(aDayNumber.meError);

        const sal_Int16 nYear = static_cast<sal_Int16>(std::trunc(aYearNumber.maValue));
        const sal_Int16 nMonth = static_cast<sal_Int16>(std::trunc(aMonthNumber.maValue));
        const sal_Int16 nDay = static_cast<sal_Int16>(std::trunc(aDayNumber.maValue));
        if (nYear < 0)
            return makeFailure(api::Error::IllegalArgument);

        const auto aDateSerial
            = api::calendar::makeDateSerial(
                sedatetime::defaultNullDate(), nYear, nMonth, nDay, false);
        if (!aDateSerial)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(aDateSerial.maValue));
    }

    if (aFunctionName == u"DATEDIF")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aStart
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aStart)
            return aStart;
        EvaluationResult aEnd
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aEnd)
            return aEnd;
        EvaluationResult aInterval
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aInterval)
            return aInterval;

        const auto oStartDate = sedatetime::coerceToDateSerial(aStart.maValue.maValue);
        const auto oEndDate = sedatetime::coerceToDateSerial(aEnd.maValue.maValue);
        if (!oStartDate || !oEndDate)
            return makeFailure(api::Error::IllegalArgument);

        const auto aIntervalText = coerceToString(aInterval.maValue.maValue);
        if (!aIntervalText)
            return makeFailure(aIntervalText.meError);

        const auto aDateDif
            = api::calendar::dateDif(sedatetime::defaultNullDate(), *oStartDate, *oEndDate,
                aIntervalText.maValue);
        if (!aDateDif)
            return makeFailure(aDateDif.meError);

        return makeScalarResult(api::CellValue::number(aDateDif.maValue));
    }

    if (aFunctionName == u"ROUND" || aFunctionName == u"ROUNDUP" || aFunctionName == u"ROUNDDOWN")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return aValue;

        const auto aValueNumber = coerceToNumber(aValue.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);

        int nDecimals = 0;
        if (rNode.maChildren.size() == 2)
        {
            EvaluationResult aDecimals
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aDecimals)
                return aDecimals;

            const auto aDigitsNumber = coerceToNumber(aDecimals.maValue.maValue);
            if (!aDigitsNumber || !std::isfinite(aDigitsNumber.maValue))
                return makeFailure(api::Error::IllegalArgument);

            const double fTruncatedDigits = std::trunc(aDigitsNumber.maValue);
            if (fTruncatedDigits < static_cast<double>(std::numeric_limits<int>::min())
                || fTruncatedDigits > static_cast<double>(std::numeric_limits<int>::max()))
            {
                return makeFailure(api::Error::IllegalArgument);
            }

            nDecimals = static_cast<int>(fTruncatedDigits);
        }

        api::RoundingMode eMode = api::RoundingMode::Corrected;
        if (aFunctionName == u"ROUNDUP")
            eMode = api::RoundingMode::Up;
        else if (aFunctionName == u"ROUNDDOWN")
            eMode = api::RoundingMode::Down;

        const auto aRounded = semath::evaluateRoundValue(
            aValueNumber.maValue, nDecimals, eMode,
            aFunctionName == u"ROUNDUP" || aFunctionName == u"ROUNDDOWN");
        if (!aRounded)
            return makeFailure(aRounded.meError);
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"CEILING" || aFunctionName == u"FLOOR" || aFunctionName == u"CEILING.XCL"
        || aFunctionName == u"FLOOR.XCL")
    {
        const bool bMicrosoftCompat = usesMicrosoftCompatibilityName(rNode.maPrimaryText)
                                      || aFunctionName == u"CEILING.XCL"
                                      || aFunctionName == u"FLOOR.XCL";
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3
            || (bMicrosoftCompat && rNode.maChildren.size() != 2))
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        double fValue = 0.0;
        if (rNode.maChildren[0]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aValue
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
            if (!aValue)
                return aValue;

            const auto aValueNumber = coerceToNumber(aValue.maValue.maValue);
            if (!aValueNumber)
                return makeFailure(aValueNumber.meError);
            fValue = aValueNumber.maValue;
        }

        double fSignificance = 1.0;
        if (rNode.maChildren.size() >= 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aSignificance
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aSignificance)
                return aSignificance;

            const auto aSignificanceNumber = coerceToNumber(aSignificance.maValue.maValue);
            if (!aSignificanceNumber)
                return makeFailure(aSignificanceNumber.meError);
            fSignificance = aSignificanceNumber.maValue;
        }

        const bool bMissingSignificance
            = rNode.maChildren.size() < 2
              || rNode.maChildren[1]->meKind == formula::NodeKind::EmptyArgument;

        bool bAbs = false;
        if (!bMicrosoftCompat && rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aMode)
                return aMode;

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            bAbs = !::rtl::math::approxEqual(aModeNumber.maValue, 0.0);
        }

        if (!bMicrosoftCompat && bAbs && bMissingSignificance && fValue < 0.0)
            fSignificance = -1.0;

        const auto aRounded = semath::evaluateCeilingFloorValue(
            fValue, fSignificance, bAbs,
            aFunctionName == u"CEILING" || aFunctionName == u"CEILING.XCL",
            bMicrosoftCompat);
        if (!aRounded)
            return makeFailure(aRounded.meError);
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"CEILING.MATH" || aFunctionName == u"FLOOR.MATH")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return aValue;

        const auto aValueNumber = coerceToNumber(aValue.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);

        double fSignificance = 1.0;
        if (rNode.maChildren.size() >= 2)
        {
            EvaluationResult aSignificance
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aSignificance)
                return aSignificance;

            const auto aSignificanceNumber = coerceToNumber(aSignificance.maValue.maValue);
            if (!aSignificanceNumber)
                return makeFailure(aSignificanceNumber.meError);
            fSignificance = aSignificanceNumber.maValue;
        }

        if (fSignificance == 0.0 || aValueNumber.maValue == 0.0)
            return makeScalarResult(api::CellValue::number(0.0));

        double fMode = 0.0;
        if (rNode.maChildren.size() == 3)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aMode)
                return aMode;

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            fMode = aModeNumber.maValue;
        }

        const auto aRounded = semath::evaluateCeilingFloorMathValue(
            aValueNumber.maValue, fSignificance, fMode, aFunctionName == u"CEILING.MATH");
        if (!aRounded)
            return makeFailure(aRounded.meError);
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"CEILING.PRECISE" || aFunctionName == u"FLOOR.PRECISE"
        || aFunctionName == u"ISO.CEILING")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return aValue;

        const auto aValueNumber = coerceToNumber(aValue.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);

        double fSignificance = 1.0;
        if (rNode.maChildren.size() == 2)
        {
            EvaluationResult aSignificance
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aSignificance)
                return aSignificance;

            const auto aSignificanceNumber = coerceToNumber(aSignificance.maValue.maValue);
            if (!aSignificanceNumber)
                return makeFailure(aSignificanceNumber.meError);
            fSignificance = aSignificanceNumber.maValue;
        }

        const auto aRounded = semath::evaluateCeilingFloorPreciseValue(
            aValueNumber.maValue, fSignificance, aFunctionName == u"FLOOR.PRECISE");
        if (!aRounded)
            return makeFailure(aRounded.meError);
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"ROUNDSIG")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return aValue;
        EvaluationResult aDigits
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aDigits)
            return aDigits;

        const auto aValueNumber = coerceToNumber(aValue.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);
        const auto aDigitsNumber = coerceToNumber(aDigits.maValue.maValue);
        if (!aDigitsNumber || !std::isfinite(aDigitsNumber.maValue))
            return makeFailure(api::Error::IllegalArgument);

        const auto aRounded
            = semath::evaluateRoundSigValue(aValueNumber.maValue, aDigitsNumber.maValue);
        if (!aRounded)
            return makeFailure(aRounded.meError);
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"DAYSINMONTH" || aFunctionName == u"DAYSINYEAR"
        || aFunctionName == u"ISLEAPYEAR" || aFunctionName == u"ISOWEEKNUM")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto oDateSerial = sedatetime::coerceToDateSerial(aArgument.maValue.maValue);
        if (!oDateSerial)
            return makeFailure(api::Error::IllegalArgument);

        const api::DateParts aNullDate = sedatetime::defaultNullDate();
        const sal_Int16 nYear = static_cast<sal_Int16>(
            spreadsheetengine::core::datetime::extractYear(aNullDate, *oDateSerial));
        const sal_Int16 nMonth = static_cast<sal_Int16>(
            spreadsheetengine::core::datetime::extractMonth(aNullDate, *oDateSerial));

        if (aFunctionName == u"DAYSINMONTH")
        {
            return makeScalarResult(api::CellValue::number(
                static_cast<double>(spreadsheetengine::core::detail::date::getDaysInMonth(
                    static_cast<sal_uInt16>(nMonth), nYear))));
        }

        const bool bLeapYear = spreadsheetengine::core::detail::date::isLeapYear(nYear);
        if (aFunctionName == u"DAYSINYEAR")
            return makeScalarResult(api::CellValue::number(bLeapYear ? 366.0 : 365.0));

        if (aFunctionName == u"ISOWEEKNUM")
        {
            return makeScalarResult(api::CellValue::number(
                static_cast<double>(spreadsheetengine::api::calendar::isoWeekOfYear(
                    aNullDate, *oDateSerial))));
        }

        return makeScalarResult(api::CellValue::boolean(bLeapYear));
    }

    const auto coerceStandaloneDateTimeNumber = [&](const api::CellValue& rValue)
        -> api::ValueResult<double> {
        switch (rValue.meKind)
        {
            case api::CellValueKind::Empty:
                return api::ValueResult<double>::success(0.0);
            case api::CellValueKind::Number:
            case api::CellValueKind::Boolean:
                return api::ValueResult<double>::success(rValue.mfNumber);
            case api::CellValueKind::Text:
            {
                if (const auto oParsed = sedatetime::parseStandaloneNumberText(rValue.maString))
                    return api::ValueResult<double>::success(oParsed->mfValue);
                if (const auto oStored = sedatetime::parseStoredDateValue(rValue.maString))
                    return api::ValueResult<double>::success(*oStored);
                return api::ValueResult<double>::failure(api::Error::IllegalArgument);
            }
            case api::CellValueKind::Error:
                return api::ValueResult<double>::failure(rValue.meError);
        }

        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    };

    if (aFunctionName == u"YEAR" || aFunctionName == u"MONTH" || aFunctionName == u"DAY")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aDateValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aDateValue)
            return makeScalarResult(api::CellValue::error(aDateValue.meError));
        const auto aDateNumber = coerceStandaloneDateTimeNumber(aDateValue.maValue);
        if (!aDateNumber)
            return makeScalarResult(api::CellValue::error(aDateNumber.meError));
        const api::DateSerial nDateSerial
            = static_cast<api::DateSerial>(rtl::math::approxFloor(aDateNumber.maValue));

        const api::DateParts aNullDate = sedatetime::defaultNullDate();
        if (aFunctionName == u"YEAR")
        {
            return makeScalarResult(api::CellValue::number(
                spreadsheetengine::core::datetime::extractYear(aNullDate, nDateSerial)));
        }
        if (aFunctionName == u"MONTH")
        {
            return makeScalarResult(api::CellValue::number(
                spreadsheetengine::core::datetime::extractMonth(aNullDate, nDateSerial)));
        }

        const auto oDay = spreadsheetengine::core::datetime::extractDay(aNullDate, nDateSerial);
        if (!oDay)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        return makeScalarResult(api::CellValue::number(*oDay));
    }

    if (aFunctionName == u"HOUR" || aFunctionName == u"MINUTE" || aFunctionName == u"SECOND")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aTimeValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aTimeValue)
            return makeScalarResult(api::CellValue::error(aTimeValue.meError));

        const auto aNumber = coerceStandaloneDateTimeNumber(aTimeValue.maValue);
        if (!aNumber)
            return makeScalarResult(api::CellValue::error(aNumber.meError));

        double fComponent = 0.0;
        if (aFunctionName == u"HOUR")
            fComponent = sedatetime::extractHour(aNumber.maValue);
        else if (aFunctionName == u"MINUTE")
            fComponent = sedatetime::extractMinute(aNumber.maValue);
        else
            fComponent = sedatetime::extractSecond(aNumber.maValue);

        return makeScalarResult(api::CellValue::number(fComponent));
    }

    if (aFunctionName == u"EDATE" || aFunctionName == u"EOMONTH")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aStart
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aStart)
            return aStart;
        EvaluationResult aMonths
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aMonths)
            return aMonths;

        const auto oDateSerial = sedatetime::coerceToDateSerial(aStart.maValue.maValue);
        if (!oDateSerial)
            return makeFailure(api::Error::IllegalArgument);

        const auto aMonthNumber = coerceToNumber(aMonths.maValue.maValue);
        if (!aMonthNumber || !std::isfinite(aMonthNumber.maValue))
            return makeFailure(api::Error::IllegalArgument);

        const sal_Int32 nMonthOffset = static_cast<sal_Int32>(std::trunc(aMonthNumber.maValue));
        const auto oShifted = sedatetime::shiftMonthSerial(
            *oDateSerial, nMonthOffset, aFunctionName == u"EOMONTH");
        if (!oShifted)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(*oShifted));
    }

    if (aFunctionName == u"WEEKDAY")
    {
        if (rNode.maChildren.size() < 1 || rNode.maChildren.size() > 2)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aDateValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aDateValue)
            return makeScalarResult(api::CellValue::error(aDateValue.meError));
        const auto oDateSerial = sedatetime::coerceToDateSerial(aDateValue.maValue);
        if (!oDateSerial)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        sal_Int16 nMode = 1;
        if (rNode.maChildren.size() == 2)
        {
            if (rNode.maChildren[1]->meKind == formula::NodeKind::EmptyArgument)
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

            const auto aModeValue = evaluateScalarArgumentValue(*rNode.maChildren[1]);
            if (!aModeValue)
                return makeScalarResult(api::CellValue::error(aModeValue.meError));
            if (aModeValue.maValue.isEmpty())
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

            const auto aModeNumber = coerceToNumber(aModeValue.maValue);
            if (!aModeNumber)
                return makeScalarResult(api::CellValue::error(aModeNumber.meError));
            const auto oWholeMode = toWholeNumber(aModeNumber.maValue);
            if (!oWholeMode)
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
            nMode = static_cast<sal_Int16>(*oWholeMode);
        }

        const auto aWeekday
            = api::calendar::dayOfWeek(sedatetime::defaultNullDate(), *oDateSerial, nMode);
        if (!aWeekday)
            return makeScalarResult(api::CellValue::error(aWeekday.meError));

        return makeScalarResult(api::CellValue::number(static_cast<double>(aWeekday.maValue)));
    }

    if (aFunctionName == u"WEEKNUM")
    {
        if (rNode.maChildren.size() < 1 || rNode.maChildren.size() > 2)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aDateValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aDateValue)
            return makeScalarResult(api::CellValue::error(aDateValue.meError));
        const auto oDateSerial = sedatetime::coerceToDateSerial(aDateValue.maValue);
        if (!oDateSerial)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        sal_Int16 nMode = 1;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aModeValue = evaluateScalarArgumentValue(*rNode.maChildren[1]);
            if (!aModeValue)
                return makeScalarResult(api::CellValue::error(aModeValue.meError));
            if (!aModeValue.maValue.isEmpty())
            {
                const auto aModeNumber = coerceToNumber(aModeValue.maValue);
                if (!aModeNumber)
                    return makeScalarResult(api::CellValue::error(aModeNumber.meError));
                const auto oWholeMode = toWholeNumber(aModeNumber.maValue);
                if (!oWholeMode)
                    return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
                nMode = static_cast<sal_Int16>(*oWholeMode);
            }
        }

        const auto aWeek
            = api::calendar::weekOfYear(sedatetime::defaultNullDate(), *oDateSerial, nMode);
        if (!aWeek)
            return makeScalarResult(api::CellValue::error(aWeek.meError));

        return makeScalarResult(api::CellValue::number(static_cast<double>(aWeek.maValue)));
    }

    if (aFunctionName == u"DAYS360")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aStartValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aStartValue)
            return makeScalarResult(api::CellValue::error(aStartValue.meError));
        const auto aEndValue = evaluateScalarArgumentValue(*rNode.maChildren[1]);
        if (!aEndValue)
            return makeScalarResult(api::CellValue::error(aEndValue.meError));

        const auto oStartDate = sedatetime::coerceToDateSerial(aStartValue.maValue);
        const auto oEndDate = sedatetime::coerceToDateSerial(aEndValue.maValue);
        if (!oStartDate || !oEndDate)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        bool bEuropeanMethod = false;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aMethodValue = evaluateScalarArgumentValue(*rNode.maChildren[2]);
            if (!aMethodValue)
                return makeScalarResult(api::CellValue::error(aMethodValue.meError));
            if (!aMethodValue.maValue.isEmpty())
            {
                const auto aMethod = coerceToBoolean(aMethodValue.maValue);
                if (!aMethod)
                    return makeScalarResult(api::CellValue::error(aMethod.meError));
                bEuropeanMethod = aMethod.maValue;
            }
        }

        return makeScalarResult(api::CellValue::number(api::calendar::diffDate360(
            sedatetime::defaultNullDate(), *oStartDate, *oEndDate, bEuropeanMethod)));
    }

    if (aFunctionName == u"EASTERSUNDAY")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aYearValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aYearValue)
            return makeScalarResult(api::CellValue::error(aYearValue.meError));
        if (aYearValue.maValue.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aYearNumber = coerceToNumber(aYearValue.maValue);
        if (!aYearNumber)
            return makeScalarResult(api::CellValue::error(aYearNumber.meError));

        const auto oWholeYear = toWholeNumber(aYearNumber.maValue);
        if (!oWholeYear || *oWholeYear < std::numeric_limits<sal_Int16>::min()
            || *oWholeYear > std::numeric_limits<sal_Int16>::max())
        {
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        }

        sal_Int16 nYear = static_cast<sal_Int16>(*oWholeYear);
        if (nYear >= 0 && nYear < 100)
        {
            constexpr sal_Int16 nTwoDigitYearStart = 1930;
            if (nYear < (nTwoDigitYearStart % 100))
                nYear = static_cast<sal_Int16>(nYear + (((nTwoDigitYearStart / 100) + 1) * 100));
            else
                nYear = static_cast<sal_Int16>(nYear + ((nTwoDigitYearStart / 100) * 100));
        }

        const auto aEaster = api::calendar::easterSundaySerial(sedatetime::defaultNullDate(), nYear);
        if (!aEaster)
            return makeScalarResult(api::CellValue::error(aEaster.meError));

        return makeScalarResult(api::CellValue::number(aEaster.maValue));
    }

    if (aFunctionName == u"YEARS")
    {
        if (rNode.maChildren.size() != 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aStartValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aStartValue)
            return makeScalarResult(api::CellValue::error(aStartValue.meError));
        const auto aEndValue = evaluateScalarArgumentValue(*rNode.maChildren[1]);
        if (!aEndValue)
            return makeScalarResult(api::CellValue::error(aEndValue.meError));
        const auto aModeValue = evaluateScalarArgumentValue(*rNode.maChildren[2]);
        if (!aModeValue)
            return makeScalarResult(api::CellValue::error(aModeValue.meError));
        if (aModeValue.maValue.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto oStartDate = sedatetime::coerceToDateSerial(aStartValue.maValue);
        const auto oEndDate = sedatetime::coerceToDateSerial(aEndValue.maValue);
        if (!oStartDate || !oEndDate)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aModeNumber = coerceToNumber(aModeValue.maValue);
        if (!aModeNumber)
            return makeScalarResult(api::CellValue::error(aModeNumber.meError));
        const auto oWholeMode = toWholeNumber(aModeNumber.maValue);
        if (!oWholeMode || (*oWholeMode != 0 && *oWholeMode != 1))
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const sal_Int32 nNullDate = sedate::toAbsoluteDays(sedatetime::defaultNullDate());
        const api::DateParts aStartDate = sedate::fromAbsoluteDays(nNullDate + *oStartDate);
        const api::DateParts aEndDate = sedate::fromAbsoluteDays(nNullDate + *oEndDate);

        sal_Int32 nYears = static_cast<sal_Int32>(aEndDate.mnYear) - aStartDate.mnYear;
        if (*oWholeMode == 0)
        {
            sal_Int32 nMonths = static_cast<sal_Int32>(aEndDate.mnMonth) - aStartDate.mnMonth
                                + nYears * 12;
            if (*oStartDate < *oEndDate)
            {
                if (aStartDate.mnDay > aEndDate.mnDay)
                    --nMonths;
            }
            else if (*oStartDate > *oEndDate)
            {
                if (aStartDate.mnDay < aEndDate.mnDay)
                    ++nMonths;
            }
            nYears = nMonths / 12;
        }

        return makeScalarResult(api::CellValue::number(static_cast<double>(nYears)));
    }

    if (aFunctionName == u"WEEKS")
    {
        if (rNode.maChildren.size() != 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto evaluateWeeksArgument = [&](const formula::Node& rArgument) -> EvaluationResult {
            EvaluationResult aArgument = evaluateNode(rArgument, rCurrentAddress);
            if (!aArgument)
                return makeScalarResult(api::CellValue::error(aArgument.meError));
            if (aArgument.maValue.isScalar())
                return aArgument;
            if (!aArgument.maValue.maReference.isSingleCell())
                return makeScalarResult(api::CellValue::error(api::Error::NoValue));

            aArgument = materializeReferenceValue(aArgument.maValue.maReference, 0, 0);
            if (!aArgument)
                return makeScalarResult(api::CellValue::error(aArgument.meError));
            return aArgument;
        };

        EvaluationResult aStart = evaluateWeeksArgument(*rNode.maChildren[0]);
        if (!aStart)
            return aStart;
        EvaluationResult aEnd = evaluateWeeksArgument(*rNode.maChildren[1]);
        if (!aEnd)
            return aEnd;
        EvaluationResult aMode = evaluateWeeksArgument(*rNode.maChildren[2]);
        if (!aMode)
            return aMode;

        const auto oStartDate = sedatetime::coerceToDateSerial(aStart.maValue.maValue);
        const auto oEndDate = sedatetime::coerceToDateSerial(aEnd.maValue.maValue);
        if (!oStartDate || !oEndDate || aMode.maValue.maValue.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
        if (!aModeNumber)
            return makeScalarResult(api::CellValue::error(aModeNumber.meError));

        const auto oWholeMode = toWholeNumber(aModeNumber.maValue);
        if (!oWholeMode)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto oWeeks = sedatetime::computeWeeksDifference(
            *oStartDate, *oEndDate, static_cast<sal_Int16>(*oWholeMode));
        if (!oWeeks)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        return makeScalarResult(api::CellValue::number(*oWeeks));
    }

    if (aFunctionName == u"WEEKSINYEAR")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        EvaluationResult aDateArgument = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aDateArgument)
            return makeScalarResult(api::CellValue::error(aDateArgument.meError));
        if (!aDateArgument.maValue.isScalar())
        {
            if (!aDateArgument.maValue.maReference.isSingleCell())
                return makeScalarResult(api::CellValue::error(api::Error::NoValue));

            aDateArgument = materializeReferenceValue(aDateArgument.maValue.maReference, 0, 0);
            if (!aDateArgument)
                return makeScalarResult(api::CellValue::error(aDateArgument.meError));
        }

        const auto oDateSerial = sedatetime::coerceToDateSerial(aDateArgument.maValue.maValue);
        if (!oDateSerial)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const api::DateParts aDate = sedate::fromAbsoluteDays(
            sedate::toAbsoluteDays(sedatetime::defaultNullDate()) + *oDateSerial);
        const sal_Int32 nJan1WeekDay
            = (sedate::toAbsoluteDays({ aDate.mnYear, 1, 1 }) - 1) % 7;
        const double fWeeksInYear = nJan1WeekDay == 3
                                        ? 53.0
                                        : (nJan1WeekDay == 2 && sedate::isLeapYear(aDate.mnYear)
                                               ? 53.0
                                               : 52.0);
        return makeScalarResult(api::CellValue::number(fWeeksInYear));
    }

    if (aFunctionName == u"WORKDAY.INTL")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aStartNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDaysNumber = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aStartNumber)
            return makeFailure(aStartNumber.meError);
        if (!aDaysNumber)
            return makeFailure(aDaysNumber.meError);

        const auto oStartDate = toWholeNumber(std::trunc(aStartNumber.maValue));
        const auto oDays = toWholeNumber(std::trunc(aDaysNumber.maValue));
        if (!oStartDate || !oDays)
            return makeFailure(api::Error::IllegalArgument);

        const auto aWeekendMask = evaluateWeekendMaskArgument(
            rNode.maChildren.size() >= 3 ? rNode.maChildren[2].get() : nullptr, true);
        if (!aWeekendMask)
            return makeFailure(aWeekendMask.meError);

        const auto aHolidays = collectHolidaySerials(
            rNode.maChildren.size() >= 4 ? rNode.maChildren[3].get() : nullptr);
        if (!aHolidays)
            return makeFailure(aHolidays.meError);

        return makeScalarResult(api::CellValue::number(advanceWorkdayFods(
            static_cast<api::DateSerial>(*oStartDate), static_cast<api::DateSerial>(*oDays),
            aHolidays.maValue, aWeekendMask.maValue)));
    }

    if (aFunctionName == u"NETWORKDAYS.INTL")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aStartNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aEndNumber = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aStartNumber)
            return makeFailure(aStartNumber.meError);
        if (!aEndNumber)
            return makeFailure(aEndNumber.meError);

        const auto oStartDate = toWholeNumber(std::trunc(aStartNumber.maValue));
        const auto oEndDate = toWholeNumber(std::trunc(aEndNumber.maValue));
        if (!oStartDate || !oEndDate)
            return makeFailure(api::Error::IllegalArgument);

        const auto aWeekendMask = evaluateWeekendMaskArgument(
            rNode.maChildren.size() >= 3 ? rNode.maChildren[2].get() : nullptr, false);
        if (!aWeekendMask)
            return makeFailure(aWeekendMask.meError);

        const auto aHolidays = collectHolidaySerials(
            rNode.maChildren.size() >= 4 ? rNode.maChildren[3].get() : nullptr);
        if (!aHolidays)
            return makeFailure(aHolidays.meError);

        return makeScalarResult(api::CellValue::number(countWorkdaysFods(
            static_cast<api::DateSerial>(*oStartDate), static_cast<api::DateSerial>(*oEndDate),
            aHolidays.maValue, aWeekendMask.maValue)));
    }

    if (aFunctionName == u"VLOOKUP" || aFunctionName == u"HLOOKUP")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aLookupValue)
            return aLookupValue;

        EvaluationResult aTable = evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        if (!aTable)
            return aTable;
        if (!aTable.maValue.isMatrixReference())
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aIndex
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aIndex)
            return aIndex;
        if (aIndex.maValue.maValue.isEmpty())
            return makeFailure(api::Error::IllegalArgument);

        const auto aIndexNumber = coerceToNumber(aIndex.maValue.maValue);
        if (!aIndexNumber)
            return makeFailure(aIndexNumber.meError);
        const auto oWholeIndex = toWholeNumber(aIndexNumber.maValue);
        if (!oWholeIndex || *oWholeIndex <= 0)
            return makeFailure(api::Error::IllegalArgument);

        bool bApproximate = true;
        if (rNode.maChildren.size() == 4)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aMode)
                return aMode;

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            bApproximate = !rtl::math::approxEqual(aModeNumber.maValue, 0.0);
        }

        LookupInput aTableInput;
        aTableInput.mbScalar = false;
        aTableInput.maReference = aTable.maValue.maReference;
        const auto aDimensions = aTableInput.maReference.matrixDimensions();
        aTableInput.mnColumns = aDimensions.mnColumns;
        aTableInput.mnRows = aDimensions.mnRows;
        const auto eOrientation = aFunctionName == u"VLOOKUP"
                                      ? api::lookup::VectorOrientation::Column
                                      : api::lookup::VectorOrientation::Row;

        const api::MatrixSize nSearchLength
            = eOrientation == api::lookup::VectorOrientation::Column ? aDimensions.mnRows
                                                                     : aDimensions.mnColumns;
        const api::MatrixSize nResultIndex = *oWholeIndex - 1;
        if (nSearchLength <= 0)
            return makeFailure(api::Error::IllegalArgument);
        if (eOrientation == api::lookup::VectorOrientation::Column
            && nResultIndex >= aDimensions.mnColumns)
        {
            return makeFailure(api::Error::IllegalArgument);
        }
        if (eOrientation == api::lookup::VectorOrientation::Row && nResultIndex >= aDimensions.mnRows)
            return makeFailure(api::Error::IllegalArgument);

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);

        const EvaluatorLookupMaterializer aLookupMaterializer(*this);
        const auto aResolvedIndex = selookup::resolveTabularLookupIndex(aLookupMaterializer,
            rLookup, aTableInput, eOrientation, bApproximate,
            toQuerySearchType(mrWorkbook.meFormulaSearchType));
        if (!aResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        const auto aMatchedSearchValue = selookup::materializeLookupInputValue(
            aLookupMaterializer, aTableInput, eOrientation, aResolvedIndex.maValue);
        if (!aMatchedSearchValue)
            return makeFailure(aMatchedSearchValue.meError);
        if (rLookup.isText() && (aMatchedSearchValue.maValue.isNumber()
                                 || aMatchedSearchValue.maValue.isBoolean()))
        {
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        const auto aResultCoordinate = api::lookup::planTabularLookupResult(
            eOrientation, aResolvedIndex.maValue, nResultIndex, aDimensions);
        if (!aResultCoordinate)
            return makeFailure(aResultCoordinate.meError);

        return materializeReferenceValue(
            aTableInput.maReference, aResultCoordinate.maValue.mnColumn,
            aResultCoordinate.maValue.mnRow);
    }

    if (aFunctionName == u"LOOKUP")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);
        if (rLookup.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        const auto aDataInput = evaluateLookupInputNode(*rNode.maChildren[1]);
        if (!aDataInput)
            return makeFailure(aDataInput.meError);
        const LookupInput& rDataInput = aDataInput.maValue;
        if (rDataInput.mbScalar && rDataInput.maScalar.isError())
            return makeScalarResult(api::CellValue::error(rDataInput.maScalar.meError));

        const auto aDataLayout = selookup::detectLookupLayout(rDataInput, true);
        if (!aDataLayout)
            return makeFailure(aDataLayout.meError);
        const EvaluatorLookupMaterializer aLookupMaterializer(*this);

        std::optional<LookupInput> oResultInput;
        std::optional<api::lookup::VectorLayout> oResultLayout;
        if (rNode.maChildren.size() == 3)
        {
            const auto aResultInput = evaluateLookupInputNode(*rNode.maChildren[2]);
            if (!aResultInput)
                return makeFailure(aResultInput.meError);
            oResultInput = aResultInput.maValue;
            if (oResultInput->mbScalar && oResultInput->maScalar.isError())
                return makeScalarResult(api::CellValue::error(oResultInput->maScalar.meError));

            const auto aResultLayout = selookup::detectLookupLayout(*oResultInput, false);
            if (!aResultLayout)
                return makeFailure(aResultLayout.meError);
            oResultLayout = aResultLayout.maValue;
        }

        auto materializeResultAt = [&](api::MatrixSize nIndex) -> EvaluationResult {
            if (oResultInput)
            {
                if (oResultInput->mbScalar)
                {
                if (nIndex != 0)
                    return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
                return makeScalarResult(oResultInput->maScalar);
            }

                const auto aValue = selookup::materializeLookupInputValue(
                    aLookupMaterializer, *oResultInput, oResultLayout->meOrientation, nIndex);
                if (!aValue)
                    return makeFailure(aValue.meError);
                return makeScalarResult(aValue.maValue);
            }

            if (rDataInput.mbScalar)
                return makeScalarResult(rDataInput.maScalar);

            const api::MatrixDimensions aDimensions { rDataInput.mnColumns, rDataInput.mnRows };
            const api::MatrixSize nResultIndex
                = aDataLayout.maValue.meOrientation == api::lookup::VectorOrientation::Column
                      ? aDimensions.mnColumns - 1
                      : aDimensions.mnRows - 1;
            const auto aResultCoordinate = api::lookup::planTabularLookupResult(
                aDataLayout.maValue.meOrientation, nIndex, nResultIndex, aDimensions);
            if (!aResultCoordinate)
                return makeFailure(aResultCoordinate.meError);

            if (!rDataInput.maValues.empty())
            {
                const sal_Int64 nLinearIndex
                    = static_cast<sal_Int64>(aResultCoordinate.maValue.mnRow) * rDataInput.mnColumns
                      + aResultCoordinate.maValue.mnColumn;
                if (nLinearIndex < 0
                    || static_cast<std::size_t>(nLinearIndex) >= rDataInput.maValues.size())
                {
                    return makeFailure(api::Error::IllegalArgument);
                }

                return makeScalarResult(
                    rDataInput.maValues[static_cast<std::size_t>(nLinearIndex)]);
            }

            return materializeReferenceValue(rDataInput.maReference,
                aResultCoordinate.maValue.mnColumn, aResultCoordinate.maValue.mnRow);
        };

        const auto aResolvedIndex = selookup::resolveLookupIndex(aLookupMaterializer, rLookup,
            rDataInput, toQuerySearchType(mrWorkbook.meFormulaSearchType));
        if (!aResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        const auto aMatchedSearchValue = selookup::materializeLookupInputValue(
            aLookupMaterializer, rDataInput, aDataLayout.maValue.meOrientation,
            aResolvedIndex.maValue);
        if (!aMatchedSearchValue)
            return makeFailure(aMatchedSearchValue.meError);
        if (rLookup.isText() && (aMatchedSearchValue.maValue.isNumber()
                                 || aMatchedSearchValue.maValue.isBoolean()))
        {
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        return materializeResultAt(aResolvedIndex.maValue);
    }

    if (aFunctionName == u"MATCH")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aLookupValue)
            return aLookupValue;
        if (aLookupValue.maValue.isMatrixReference())
        {
            aLookupValue = materializeReferenceValue(aLookupValue.maValue.maReference, 0, 0);
        }
        else
        {
            aLookupValue = ensureScalarValue(*this, std::move(aLookupValue));
        }
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);

        EvaluationResult aSearchValue = evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        const auto oSearchInput = makeLookupInput(aSearchValue);
        if (!oSearchInput)
            return makeFailure(api::Error::IllegalArgument);

        api::lookup::MatchSearchMode aModes;
        if (rNode.maChildren.size() == 3)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);

            const auto aNormalized = api::lookup::normalizeMatchType(aModeNumber.maValue);
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            aModes = aNormalized.maValue;
        }
        else
        {
            aModes.meMatchMode = api::lookup::MatchMode::ExactOrNextSmaller;
            aModes.meSearchMode = api::lookup::SearchMode::BinaryAscending;
        }
        const EvaluatorLookupMaterializer aLookupMaterializer(*this);
        const auto aResolvedIndex = selookup::resolveMatchIndex(aLookupMaterializer, rLookup,
            *oSearchInput, aModes, toQuerySearchType(mrWorkbook.meFormulaSearchType));
        if (!aResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        return makeScalarResult(
            api::CellValue::number(static_cast<double>(aResolvedIndex.maValue + 1)));
    }

    if (aFunctionName == u"XMATCH")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aLookupValue)
            return aLookupValue;
        if (aLookupValue.maValue.isMatrixReference())
        {
            aLookupValue = materializeReferenceValue(aLookupValue.maValue.maReference, 0, 0);
        }
        else
        {
            aLookupValue = ensureScalarValue(*this, std::move(aLookupValue));
        }
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);

        const auto aSearchInput = evaluateLookupInputNode(*rNode.maChildren[1]);
        if (!aSearchInput)
            return makeFailure(aSearchInput.meError);

        api::lookup::MatchMode eMatchMode = api::lookup::MatchMode::ExactOrNotAvailable;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized
                = api::lookup::normalizeExtendedMatchMode(static_cast<sal_Int16>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eMatchMode = aNormalized.maValue;
        }

        api::lookup::SearchMode eSearchMode = api::lookup::SearchMode::Forward;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized
                = api::lookup::normalizeSearchMode(static_cast<sal_Int16>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eSearchMode = aNormalized.maValue;
        }

        const EvaluatorLookupMaterializer aLookupMaterializer(*this);
        const auto aResolvedIndex = selookup::resolveExtendedMatchIndex(
            aLookupMaterializer, rLookup, aSearchInput.maValue, eMatchMode, eSearchMode,
            toQuerySearchType(mrWorkbook.meFormulaSearchType), true);
        if (!aResolvedIndex)
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        return makeScalarResult(
            api::CellValue::number(static_cast<double>(aResolvedIndex.maValue + 1)));
    }

    if (aFunctionName == u"XLOOKUP")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aLookupValue = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aLookupValue)
            return aLookupValue;
        if (aLookupValue.maValue.isMatrixReference())
        {
            aLookupValue = materializeReferenceValue(aLookupValue.maValue.maReference, 0, 0);
        }
        else
        {
            aLookupValue = ensureScalarValue(*this, std::move(aLookupValue));
        }
        if (!aLookupValue)
            return aLookupValue;

        const api::CellValue& rLookup = aLookupValue.maValue.maValue;
        if (rLookup.isError())
            return makeFailure(rLookup.meError);
        if (rLookup.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));

        EvaluationResult aSearchValue = evaluateNode(*rNode.maChildren[1], rCurrentAddress);
        EvaluationResult aReturnValue = evaluateNode(*rNode.maChildren[2], rCurrentAddress);
        const auto oSearchInput = makeLookupInput(aSearchValue);
        const auto oReturnInput = makeLookupInput(aReturnValue);
        if (!oSearchInput || !oReturnInput)
            return makeFailure(api::Error::IllegalArgument);

        const api::MatrixDimensions aSearchDimensions { oSearchInput->mnColumns, oSearchInput->mnRows };
        const api::MatrixDimensions aReturnDimensions { oReturnInput->mnColumns, oReturnInput->mnRows };
        const auto aSearchLayout = selookup::detectLookupLayout(*oSearchInput, false);
        if (!aSearchLayout)
            return makeFailure(aSearchLayout.meError);
        if (aReturnDimensions.isEmpty())
            return makeFailure(api::Error::IllegalArgument);
        if (aSearchLayout.maValue.meOrientation == api::lookup::VectorOrientation::Column)
        {
            if (aReturnDimensions.mnRows != aSearchDimensions.mnRows)
                return makeFailure(api::Error::IllegalArgument);
        }
        else if (aReturnDimensions.mnColumns != aSearchDimensions.mnColumns)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        api::lookup::MatchMode eMatchMode = api::lookup::MatchMode::ExactOrNotAvailable;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[4], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized = api::lookup::normalizeExtendedMatchMode(
                static_cast<sal_Int16>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eMatchMode = aNormalized.maValue;
        }

        api::lookup::SearchMode eSearchMode = api::lookup::SearchMode::Forward;
        if (rNode.maChildren.size() >= 6
            && rNode.maChildren[5]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aMode
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[5], rCurrentAddress));
            if (!aMode)
                return aMode;
            if (aMode.maValue.maValue.isEmpty())
                return makeFailure(api::Error::IllegalArgument);

            const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            const auto oWholeMode = toWholeNumber(std::trunc(aModeNumber.maValue));
            if (!oWholeMode)
                return makeFailure(api::Error::IllegalArgument);

            const auto aNormalized
                = api::lookup::normalizeSearchMode(static_cast<sal_Int16>(*oWholeMode));
            if (!aNormalized)
                return makeFailure(aNormalized.meError);
            eSearchMode = aNormalized.maValue;
        }

        const EvaluatorLookupMaterializer aLookupMaterializer(*this);
        const auto aResolvedIndex = selookup::resolveExtendedMatchIndex(
            aLookupMaterializer, rLookup, *oSearchInput, eMatchMode, eSearchMode,
            toQuerySearchType(mrWorkbook.meFormulaSearchType), false);
        if (!aResolvedIndex)
        {
            if (rNode.maChildren.size() >= 4
                && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
            {
                return evaluateNode(*rNode.maChildren[3], rCurrentAddress);
            }
            return makeScalarResult(api::CellValue::error(api::Error::NotAvailable));
        }

        const auto aResultSlice = api::lookup::planXLookupResultSlice(
            aSearchLayout.maValue.meOrientation, aResolvedIndex.maValue, aReturnDimensions);
        if (!aResultSlice)
            return makeFailure(aResultSlice.meError);

        if (oReturnInput->mbScalar)
            return makeScalarResult(oReturnInput->maScalar);

        api::ResolvedReference aSliceReference;
        aSliceReference.maRange.maStart = oReturnInput->maReference.addressAt(
            aResultSlice.maValue.maStart.mnColumn, aResultSlice.maValue.maStart.mnRow);
        aSliceReference.maRange.maEnd = oReturnInput->maReference.addressAt(
            aResultSlice.maValue.maStart.mnColumn + aResultSlice.maValue.maDimensions.mnColumns - 1,
            aResultSlice.maValue.maStart.mnRow + aResultSlice.maValue.maDimensions.mnRows - 1);

        if (aSliceReference.isSingleCell())
            return materializeReferenceValue(aSliceReference, 0, 0);

        return makeReferenceResult(aSliceReference);
    }

    if (aFunctionName == u"CHOOSECOLS" || aFunctionName == u"CHOOSEROWS")
    {
        if (rNode.maChildren.size() < 2)
            return makeFailure(api::Error::IllegalArgument);

        const bool bChooseColumns = aFunctionName == u"CHOOSECOLS";
        EvaluationResult aSource = evaluateReferenceNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aSource)
            return aSource;
        if (!aSource.maValue.isMatrixReference())
            return makeFailure(api::Error::IllegalArgument);

        const auto aSourceDimensions = aSource.maValue.maReference.matrixDimensions();
        if (aSourceDimensions.mnColumns < 1 || aSourceDimensions.mnRows < 1)
            return makeFailure(api::Error::IllegalArgument);

        std::optional<api::MatrixSize> oFirstSelection;
        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            if (rNode.maChildren[nIndex]->meKind == formula::NodeKind::EmptyArgument)
                return makeFailure(api::Error::IllegalArgument);

            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *rNode.maChildren[nIndex],
                [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);
                    if (rValue.isEmpty() || rValue.isText())
                        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);

                    const auto aNumber = coerceToNumber(rValue);
                    if (!aNumber || !std::isfinite(aNumber.maValue)
                        || aNumber.maValue < static_cast<double>(std::numeric_limits<sal_Int32>::min())
                        || aNumber.maValue > static_cast<double>(std::numeric_limits<sal_Int32>::max()))
                    {
                        return api::ValueResult<bool>::failure(api::Error::IllegalArgument);
                    }

                    const sal_Int32 nRequestedIndex = static_cast<sal_Int32>(aNumber.maValue);
                    const auto aSelection = api::array::normalizeSelectionIndex(
                        nRequestedIndex,
                        bChooseColumns ? aSourceDimensions.mnColumns : aSourceDimensions.mnRows);
                    if (!aSelection)
                        return api::ValueResult<bool>::failure(aSelection.meError);

                    if (!oFirstSelection)
                        oFirstSelection = aSelection.maValue;
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return makeFailure(aVisited.meError);
        }

        if (!oFirstSelection)
            return makeFailure(api::Error::IllegalArgument);

        return materializeReferenceValue(aSource.maValue.maReference,
            bChooseColumns ? *oFirstSelection : 0, bChooseColumns ? 0 : *oFirstSelection);
    }

    if (aFunctionName == u"CHAR")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aCode = coerceToNumber(aArgument.maValue.maValue);
        if (!aCode)
            return makeFailure(aCode.meError);

        const auto oWholeNumber = toWholeNumber(aCode.maValue);
        if (!oWholeNumber || *oWholeNumber < 1 || *oWholeNumber > 255)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCharacter = api::text::charFromValue(
            setext::defaultSingleByteEncodingService(), static_cast<double>(*oWholeNumber));
        if (!aCharacter)
            return makeFailure(aCharacter.meError);
        return makeScalarResult(api::CellValue::text(aCharacter.maValue));
    }

    if (aFunctionName == u"CODE")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);
        if (aText.maValue.empty())
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(static_cast<double>(
            api::text::codeFromText(
                setext::defaultSingleByteEncodingService(), aText.maValue))));
    }

    if (aFunctionName == u"ADDRESS")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        const auto aRowNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aRowNumber)
            return makeFailure(aRowNumber.meError);
        const auto aColumnNumber = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aColumnNumber)
            return makeFailure(aColumnNumber.meError);

        const auto oWholeRow = toWholeNumber(aRowNumber.maValue);
        const auto oWholeColumn = toWholeNumber(aColumnNumber.maValue);
        if (!oWholeRow || !oWholeColumn || *oWholeRow < 1 || *oWholeColumn < 1)
            return makeFailure(api::Error::IllegalArgument);

        sal_Int32 nAbsMode = 1;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aAbsMode = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aAbsMode)
                return makeFailure(aAbsMode.meError);
            const auto oWholeAbsMode = toWholeNumber(aAbsMode.maValue);
            if (!oWholeAbsMode || *oWholeAbsMode < 1 || *oWholeAbsMode > 4)
                return makeFailure(api::Error::IllegalArgument);
            nAbsMode = *oWholeAbsMode;
        }

        bool bA1Style = true;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aA1Argument = evaluateScalarArgumentValue(*rNode.maChildren[3]);
            if (!aA1Argument)
                return makeFailure(aA1Argument.meError);
            if (!aA1Argument.maValue.isEmpty())
            {
                const auto aA1Bool = coerceToBoolean(aA1Argument.maValue);
                if (!aA1Bool)
                    return makeFailure(aA1Bool.meError);
                bA1Style = aA1Bool.maValue;
            }
        }

        api::String aSheetName;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aSheetArgument = evaluateScalarArgumentValue(*rNode.maChildren[4]);
            if (!aSheetArgument)
                return makeFailure(aSheetArgument.meError);
            const auto aSheetText = coerceToString(aSheetArgument.maValue);
            if (!aSheetText)
                return makeFailure(aSheetText.meError);
            aSheetName = aSheetText.maValue;
        }

        return makeScalarResult(api::CellValue::text(formatAddressFunctionResult(
            static_cast<api::RowIndex>(*oWholeRow - 1),
            static_cast<api::ColumnIndex>(*oWholeColumn - 1), nAbsMode, bA1Style, aSheetName)));
    }

    if (aFunctionName == u"EUROCONVERT")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;
        EvaluationResult aFromUnitArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aFromUnitArgument)
            return aFromUnitArgument;
        EvaluationResult aToUnitArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aToUnitArgument)
            return aToUnitArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);
        const auto aFromUnit = coerceToString(aFromUnitArgument.maValue.maValue);
        if (!aFromUnit)
            return makeFailure(aFromUnit.meError);
        const auto aToUnit = coerceToString(aToUnitArgument.maValue.maValue);
        if (!aToUnit)
            return makeFailure(aToUnit.meError);

        bool bFullPrecision = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aFullPrecision = evaluateScalarArgumentValue(*rNode.maChildren[3]);
            if (!aFullPrecision)
                return makeFailure(aFullPrecision.meError);
            if (!aFullPrecision.maValue.isEmpty())
            {
                const auto aBool = coerceToBoolean(aFullPrecision.maValue);
                if (!aBool)
                    return makeFailure(aBool.meError);
                bFullPrecision = aBool.maValue;
            }
        }

        if (rNode.maChildren.size() == 5)
        {
            if (rNode.maChildren[4]->meKind == formula::NodeKind::EmptyArgument)
                return makeFailure(api::Error::IllegalArgument);

            const auto aTriangulationPrecision
                = evaluateNumericArgument(*rNode.maChildren[4], std::nullopt);
            if (!aTriangulationPrecision)
                return makeFailure(aTriangulationPrecision.meError);

            const auto oWholePrecision = toWholeNumber(aTriangulationPrecision.maValue);
            if (!oWholePrecision || *oWholePrecision < 3)
                return makeFailure(api::Error::IllegalArgument);
        }

        const auto aConverted = seconvert::evaluateEuroConvertValue(
            aValueNumber.maValue, aFromUnit.maValue, aToUnit.maValue, true, !bFullPrecision);
        if (!aConverted)
            return makeFailure(aConverted.meError);
        return makeScalarResult(api::CellValue::number(aConverted.maValue));
    }

    if (aFunctionName == u"CONVERT")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 5)
            return makeFailure(api::Error::IllegalArgument);
        if (rNode.maChildren.size() > 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;
        EvaluationResult aFromUnitArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aFromUnitArgument)
            return aFromUnitArgument;
        EvaluationResult aToUnitArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aToUnitArgument)
            return aToUnitArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);
        const auto aFromUnit = coerceToString(aFromUnitArgument.maValue.maValue);
        if (!aFromUnit)
            return makeFailure(aFromUnit.meError);
        const auto aToUnit = coerceToString(aToUnitArgument.maValue.maValue);
        if (!aToUnit)
            return makeFailure(aToUnit.meError);

        const auto aConverted = seconvert::evaluateConvertValue(
            aValueNumber.maValue, aFromUnit.maValue, aToUnit.maValue);
        if (aConverted)
            return makeScalarResult(api::CellValue::number(aConverted.maValue));

        const auto aEuroConverted = seconvert::evaluateEuroConvertValue(
            aValueNumber.maValue, aFromUnit.maValue, aToUnit.maValue, false, false);
        if (aEuroConverted)
        {
            return makeScalarResult(api::CellValue::number(aEuroConverted.maValue));
        }

        return makeFailure(aConverted.meError);
    }

    if (aFunctionName == u"DECIMAL")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aTextArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aTextArgument)
            return aTextArgument;
        EvaluationResult aBaseArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aBaseArgument)
            return aBaseArgument;

        const auto aText = coerceToString(aTextArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);
        const auto aBaseNumber = coerceToNumber(aBaseArgument.maValue.maValue);
        if (!aBaseNumber)
            return makeFailure(aBaseNumber.meError);

        const auto aResult = seconvert::evaluateDecimalValue(aText.maValue, aBaseNumber.maValue);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::number(aResult.maValue));
    }

    if (aFunctionName == u"DEC2HEX")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);

        std::optional<double> oPlaces;
        if (rNode.maChildren.size() == 2)
        {
            EvaluationResult aPlacesArgument
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aPlacesArgument)
                return aPlacesArgument;

            const auto aPlacesNumber = coerceToNumber(aPlacesArgument.maValue.maValue);
            if (!aPlacesNumber)
                return makeFailure(aPlacesNumber.meError);
            oPlaces = aPlacesNumber.maValue;
        }

        const auto aResult = seconvert::evaluateBaseValue(aValueNumber.maValue, 16.0, oPlaces);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::text(aResult.maValue));
    }

    if (aFunctionName == u"BITXOR")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        auto evaluateBitXorArgument = [&](const formula::Node& rArgument,
                                          std::optional<double> oDefaultValue)
            -> api::ValueResult<sal_uInt64> {
            const auto aValue = evaluateNumericArgument(rArgument, oDefaultValue);
            if (!aValue)
                return api::ValueResult<sal_uInt64>::failure(aValue.meError);

            if (!std::isfinite(aValue.maValue) || aValue.maValue < 0.0
                || aValue.maValue > 281474976710655.0)
            {
                return api::ValueResult<sal_uInt64>::failure(api::Error::IllegalArgument);
            }

            const double fRounded = std::round(aValue.maValue);
            if (std::abs(aValue.maValue - fRounded) > 1e-9)
                return api::ValueResult<sal_uInt64>::failure(api::Error::IllegalArgument);
            return api::ValueResult<sal_uInt64>::success(
                static_cast<sal_uInt64>(fRounded));
        };

        const auto aLeft = evaluateBitXorArgument(*rNode.maChildren[0], 0.0);
        if (!aLeft)
            return makeFailure(aLeft.meError);

        if (rNode.maChildren.size() == 1)
            return makeFailure(api::Error::NoValue);

        const auto aRight = evaluateBitXorArgument(*rNode.maChildren[1], 0.0);
        if (!aRight)
            return makeFailure(aRight.meError);

        return makeScalarResult(
            api::CellValue::number(static_cast<double>(aLeft.maValue ^ aRight.maValue)));
    }

    if (aFunctionName == u"BITLSHIFT" || aFunctionName == u"BITRSHIFT")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValueNumber = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);
        const auto aShiftNumber = evaluateNumericArgument(*rNode.maChildren[1], 0.0);
        if (!aShiftNumber)
            return makeFailure(aShiftNumber.meError);

        const auto oWholeValue = toWholeNumber(aValueNumber.maValue);
        const auto oWholeShift = toWholeNumber(aShiftNumber.maValue);
        if (!oWholeValue || !oWholeShift || *oWholeValue < 0)
            return makeFailure(api::Error::IllegalArgument);

        const sal_Int32 nShift = *oWholeShift;
        const sal_uInt64 nValue = static_cast<sal_uInt64>(*oWholeValue);

        if (nShift == 0)
            return makeScalarResult(api::CellValue::number(static_cast<double>(nValue)));

        if (aFunctionName == u"BITLSHIFT")
        {
            if (nShift < 0)
                return makeScalarResult(
                    api::CellValue::number(static_cast<double>(nValue >> (-nShift))));

            if (nShift >= 64)
                return makeFailure(api::Error::IllegalArgument);
            return makeScalarResult(
                api::CellValue::number(static_cast<double>(nValue << nShift)));
        }

        if (nShift < 0)
        {
            const sal_Int32 nLeftShift = -nShift;
            if (nLeftShift >= 64)
                return makeFailure(api::Error::IllegalArgument);
            return makeScalarResult(
                api::CellValue::number(static_cast<double>(nValue << nLeftShift)));
        }

        if (nShift >= 64)
            return makeScalarResult(api::CellValue::number(0.0));
        return makeScalarResult(api::CellValue::number(static_cast<double>(nValue >> nShift)));
    }

    if (aFunctionName == u"LOG")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber || !(aValueNumber.maValue > 0.0))
            return makeFailure(api::Error::IllegalArgument);

        double fBase = 10.0;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aBaseArgument
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aBaseArgument)
                return aBaseArgument;

            const auto aBaseNumber = coerceToNumber(aBaseArgument.maValue.maValue);
            if (!aBaseNumber)
                return makeFailure(aBaseNumber.meError);
            fBase = aBaseNumber.maValue;
        }

        const auto aLogarithm = semath::evaluateLogValue(aValueNumber.maValue, fBase);
        if (!aLogarithm)
            return makeFailure(aLogarithm.meError);
        return makeScalarResult(api::CellValue::number(aLogarithm.maValue));
    }

    if (aFunctionName == u"MROUND")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);
        if (rNode.maChildren[0]->meKind == formula::NodeKind::EmptyArgument
            || rNode.maChildren[1]->meKind == formula::NodeKind::EmptyArgument)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;
        EvaluationResult aMultipleArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aMultipleArgument)
            return aMultipleArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);
        const auto aMultipleNumber = coerceToNumber(aMultipleArgument.maValue.maValue);
        if (!aMultipleNumber)
            return makeFailure(aMultipleNumber.meError);

        const auto aRounded
            = semath::evaluateMroundValue(aValueNumber.maValue, aMultipleNumber.maValue);
        if (!aRounded)
            return makeFailure(aRounded.meError);
        return makeScalarResult(api::CellValue::number(aRounded.maValue));
    }

    if (aFunctionName == u"COMBIN")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aN = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aN)
            return makeFailure(aN.meError);
        const auto aK = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aK)
            return makeFailure(aK.meError);

        const auto aCombin = semath::evaluateCombinValue(aN.maValue, aK.maValue, false);
        if (!aCombin)
            return makeFailure(aCombin.meError);
        return makeScalarResult(api::CellValue::number(aCombin.maValue));
    }

    if (aFunctionName == u"COMBINA")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aN = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aN)
            return makeFailure(aN.meError);
        const auto aK = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aK)
            return makeFailure(aK.meError);

        const auto aCombina = semath::evaluateCombinValue(aN.maValue, aK.maValue, true);
        if (!aCombina)
            return makeFailure(aCombina.meError);
        return makeScalarResult(api::CellValue::number(aCombina.maValue));
    }

    if (aFunctionName == u"MULTINOMIAL")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        std::vector<double> aValues;
        aValues.reserve(rNode.maChildren.size());
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aValue = evaluateNumericArgument(*pChild, std::nullopt);
            if (!aValue)
                return makeFailure(aValue.meError);
            aValues.push_back(aValue.maValue);
        }

        const auto aMultinomial = semath::evaluateMultinomialValue(aValues);
        if (!aMultinomial)
            return makeFailure(aMultinomial.meError);
        return makeScalarResult(api::CellValue::number(aMultinomial.maValue));
    }

    if (aFunctionName == u"CSC")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aValue)
            return makeFailure(aValue.meError);
        if (!aValue.maValue.isNumber())
            return makeFailure(api::Error::IllegalArgument);

        const auto aCsc = semath::evaluateCscValue(aValue.maValue.mfNumber);
        if (!aCsc)
            return makeFailure(aCsc.meError);
        return makeScalarResult(api::CellValue::number(aCsc.maValue));
    }

    if (aFunctionName == u"CSCH")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aValue)
            return makeFailure(aValue.meError);
        if (!aValue.maValue.isNumber())
            return makeFailure(api::Error::IllegalArgument);

        const auto aCsch = semath::evaluateCschValue(aValue.maValue.mfNumber);
        if (!aCsch)
            return makeFailure(aCsch.meError);
        return makeScalarResult(api::CellValue::number(aCsch.maValue));
    }

    if (aFunctionName == u"TRUNC")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        const auto aValue = evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        if (!aValue)
            return makeFailure(aValue.meError);

        sal_Int32 nDigits = 0;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aDigits = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aDigits)
                return makeFailure(aDigits.meError);
            const auto oWholeDigits = toWholeNumber(aDigits.maValue);
            if (!oWholeDigits)
                return makeFailure(api::Error::IllegalArgument);
            nDigits = *oWholeDigits;
        }

        const auto aTruncated = semath::evaluateTruncValue(aValue.maValue, nDigits);
        if (!aTruncated)
            return makeFailure(aTruncated.meError);
        return makeScalarResult(api::CellValue::number(aTruncated.maValue));
    }

    if (aFunctionName == u"UNICHAR")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aCodePoint = coerceToNumber(aArgument.maValue.maValue);
        if (!aCodePoint)
            return makeFailure(aCodePoint.meError);

        const auto oWholeNumber = toWholeNumber(aCodePoint.maValue);
        if (!oWholeNumber || *oWholeNumber < 0)
            return makeFailure(api::Error::IllegalArgument);

        const auto aCharacter
            = api::text::unicharFromCodePoint(static_cast<sal_uInt32>(*oWholeNumber));
        if (!aCharacter)
            return makeFailure(aCharacter.meError);

        return makeScalarResult(api::CellValue::text(aCharacter.maValue));
    }

    if (aFunctionName == u"UPPER" || aFunctionName == u"LOWER")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        const api::String aResult = aFunctionName == u"UPPER"
                                        ? api::text::uppercase(
                                              setext::defaultCaseMappingService(), aText.maValue)
                                        : api::text::lowercase(
                                              setext::defaultCaseMappingService(), aText.maValue);
        return makeScalarResult(api::CellValue::text(aResult));
    }

    if (aFunctionName == u"PROPER")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);
        return makeScalarResult(api::CellValue::text(api::text::propercase(
            setext::defaultCaseMappingService(), aText.maValue)));
    }

    if (aFunctionName == u"ASC" || aFunctionName == u"JIS")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        const api::String aConverted = aFunctionName == u"ASC"
                                           ? api::text::convertIntoHalfWidth(
                                                 setext::defaultWidthConversionService(),
                                                 aText.maValue)
                                           : api::text::convertIntoFullWidth(
                                                 setext::defaultWidthConversionService(),
                                                 aText.maValue);
        return makeScalarResult(api::CellValue::text(aConverted));
    }

    if (aFunctionName == u"LEN")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        return makeScalarResult(
            api::CellValue::number(static_cast<double>(api::text::countCodePoints(aText.maValue))));
    }

    if (aFunctionName == u"LENB")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        return makeScalarResult(api::CellValue::number(
            static_cast<double>(setext::expandDbcsByteText(aText.maValue, false).size())));
    }

    if (aFunctionName == u"FINDB" || aFunctionName == u"SEARCHB")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aNeedleArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aNeedleArgument)
            return aNeedleArgument;
        EvaluationResult aHaystackArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aHaystackArgument)
            return aHaystackArgument;

        const auto aNeedle = coerceToString(aNeedleArgument.maValue.maValue);
        if (!aNeedle)
            return makeFailure(aNeedle.meError);
        const auto aHaystack = coerceToString(aHaystackArgument.maValue.maValue);
        if (!aHaystack)
            return makeFailure(aHaystack.meError);

        std::size_t nStartIndex = 0;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aStart = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aStart)
                return makeFailure(aStart.meError);
            const auto oWholeStart = toWholeNumber(aStart.maValue);
            if (!oWholeStart || *oWholeStart < 1)
                return makeFailure(api::Error::IllegalArgument);
            nStartIndex = static_cast<std::size_t>(*oWholeStart - 1);
        }

        const auto oFoundIndex = setext::findByteText(aNeedle.maValue, aHaystack.maValue,
            nStartIndex, aFunctionName == u"SEARCHB");
        if (!oFoundIndex)
            return makeFailure(api::Error::NotAvailable);

        return makeScalarResult(
            api::CellValue::number(static_cast<double>(*oFoundIndex + 1)));
    }

    if (aFunctionName == u"REPLACEB")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aSourceArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aSourceArgument)
            return aSourceArgument;
        EvaluationResult aStartArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aStartArgument)
            return aStartArgument;
        EvaluationResult aLengthArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aLengthArgument)
            return aLengthArgument;
        EvaluationResult aReplacementArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
        if (!aReplacementArgument)
            return aReplacementArgument;

        const auto aSource = coerceToString(aSourceArgument.maValue.maValue);
        if (!aSource)
            return makeFailure(aSource.meError);
        const auto aStart = coerceToNumber(aStartArgument.maValue.maValue);
        if (!aStart)
            return makeFailure(aStart.meError);
        const auto aLength = coerceToNumber(aLengthArgument.maValue.maValue);
        if (!aLength)
            return makeFailure(aLength.meError);
        const auto aReplacement = coerceToString(aReplacementArgument.maValue.maValue);
        if (!aReplacement)
            return makeFailure(aReplacement.meError);

        const auto oWholeStart = toWholeNumber(aStart.maValue);
        const auto oWholeLength = toWholeNumber(aLength.maValue);
        if (!oWholeStart || !oWholeLength || *oWholeStart < 1 || *oWholeLength < 0)
            return makeFailure(api::Error::IllegalArgument);

        const std::size_t nStartIndex = static_cast<std::size_t>(*oWholeStart - 1);
        const std::size_t nReplaceLength = static_cast<std::size_t>(*oWholeLength);
        const api::String aExpandedSource = setext::expandDbcsByteText(aSource.maValue, false);
        if (nStartIndex >= aExpandedSource.size()
            || nReplaceLength > aExpandedSource.size() - nStartIndex)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        return makeScalarResult(api::CellValue::text(setext::replaceByteText(
            aSource.maValue, nStartIndex, nReplaceLength, aReplacement.maValue)));
    }

    if (aFunctionName == u"SUBSTITUTE")
    {
        if (rNode.maChildren.size() < 3 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aSourceArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aSourceArgument)
            return aSourceArgument;
        EvaluationResult aOldArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aOldArgument)
            return aOldArgument;
        EvaluationResult aNewArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aNewArgument)
            return aNewArgument;

        const auto aSource = coerceToString(aSourceArgument.maValue.maValue);
        if (!aSource)
            return makeFailure(aSource.meError);
        const auto aOldText = coerceToString(aOldArgument.maValue.maValue);
        if (!aOldText)
            return makeFailure(aOldText.meError);
        const auto aNewText = coerceToString(aNewArgument.maValue.maValue);
        if (!aNewText)
            return makeFailure(aNewText.meError);

        if (aOldText.maValue.empty())
            return makeScalarResult(api::CellValue::text(aSource.maValue));

        std::optional<sal_Int32> oInstance;
        if (rNode.maChildren.size() == 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aInstance = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
            if (!aInstance)
                return makeFailure(aInstance.meError);
            const auto oWholeInstance = toWholeNumber(aInstance.maValue);
            if (!oWholeInstance || *oWholeInstance < 1)
                return makeFailure(api::Error::IllegalArgument);
            oInstance = *oWholeInstance;
        }

        return makeScalarResult(api::CellValue::text(
            setext::substituteText(aSource.maValue, aOldText.maValue, aNewText.maValue, oInstance)));
    }

    if (aFunctionName == u"SEARCH" || aFunctionName == u"FIND")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aNeedleArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aNeedleArgument)
            return aNeedleArgument;
        EvaluationResult aHaystackArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aHaystackArgument)
            return aHaystackArgument;

        const auto aNeedle = coerceToString(aNeedleArgument.maValue.maValue);
        if (!aNeedle)
            return makeFailure(aNeedle.meError);
        const auto aHaystack = coerceToString(aHaystackArgument.maValue.maValue);
        if (!aHaystack)
            return makeFailure(aHaystack.meError);

        sal_Int32 nStart = 1;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aStart = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aStart)
                return makeFailure(aStart.meError);
            const auto oWholeStart = toWholeNumber(aStart.maValue);
            if (!oWholeStart || *oWholeStart < 1)
                return makeFailure(api::Error::IllegalArgument);
            nStart = *oWholeStart;
        }

        const auto oFoundIndex = setext::findText(
            aNeedle.maValue, aHaystack.maValue, nStart - 1, aFunctionName == u"SEARCH");
        if (!oFoundIndex)
            return makeFailure(api::Error::NotAvailable);

        return makeScalarResult(api::CellValue::number(static_cast<double>(*oFoundIndex + 1)));
    }

    if (aFunctionName == u"TEXTAFTER")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTextValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aTextValue)
            return makeFailure(aTextValue.meError);
        const auto aText = coerceToString(aTextValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        std::vector<api::String> aDelimiters;
        const auto aDelimiterVisit = visitFlattenedValues(
            visitFlattenedValues, *rNode.maChildren[1],
            [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                const auto aDelimiter = coerceToString(rValue);
                if (!aDelimiter)
                    return api::ValueResult<bool>::failure(aDelimiter.meError);
                aDelimiters.push_back(aDelimiter.maValue);
                return api::ValueResult<bool>::success(true);
            });
        if (!aDelimiterVisit)
            return makeFailure(aDelimiterVisit.meError);
        if (aDelimiters.empty())
            return makeFailure(api::Error::IllegalArgument);

        sal_Int32 nInstance = 1;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aInstance = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aInstance)
                return makeFailure(aInstance.meError);
            const auto oWholeInstance = toWholeNumber(aInstance.maValue);
            if (!oWholeInstance || *oWholeInstance == 0)
                return makeFailure(api::Error::IllegalArgument);
            nInstance = *oWholeInstance;
        }

        bool bCaseInsensitive = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aMatchMode = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
            if (!aMatchMode)
                return makeFailure(aMatchMode.meError);
            const auto oWholeMatchMode = toWholeNumber(aMatchMode.maValue);
            if (!oWholeMatchMode || (*oWholeMatchMode != 0 && *oWholeMatchMode != 1))
                return makeFailure(api::Error::IllegalArgument);
            bCaseInsensitive = *oWholeMatchMode == 1;
        }

        bool bMatchEnd = false;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aMatchEnd = evaluateScalarArgumentValue(*rNode.maChildren[4]);
            if (!aMatchEnd)
                return makeFailure(aMatchEnd.meError);
            const auto aMatchEndBool = coerceToBoolean(aMatchEnd.maValue);
            if (!aMatchEndBool)
                return makeFailure(aMatchEndBool.meError);
            bMatchEnd = aMatchEndBool.maValue;
        }

        const auto handleNotFound = [&]() -> EvaluationResult {
            if (rNode.maChildren.size() >= 6)
                return evaluateNode(*rNode.maChildren[5], rCurrentAddress);
            return makeFailure(api::Error::NotAvailable);
        };
        const auto oResult
            = setext::textAfter(aText.maValue, aDelimiters, nInstance, bCaseInsensitive, bMatchEnd);
        if (!oResult)
            return handleNotFound();
        return makeScalarResult(api::CellValue::text(*oResult));
    }

    if (aFunctionName == u"TEXTBEFORE")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 6)
            return makeFailure(api::Error::IllegalArgument);

        const auto aTextValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aTextValue)
            return makeFailure(aTextValue.meError);
        const auto aText = coerceToString(aTextValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        std::vector<api::String> aDelimiters;
        const auto aDelimiterVisit = visitFlattenedValues(
            visitFlattenedValues, *rNode.maChildren[1],
            [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                if (rValue.isError())
                    return api::ValueResult<bool>::failure(rValue.meError);
                const auto aDelimiter = coerceToString(rValue);
                if (!aDelimiter)
                    return api::ValueResult<bool>::failure(aDelimiter.meError);
                aDelimiters.push_back(aDelimiter.maValue);
                return api::ValueResult<bool>::success(true);
            });
        if (!aDelimiterVisit)
            return makeFailure(aDelimiterVisit.meError);
        if (aDelimiters.empty())
            return makeFailure(api::Error::IllegalArgument);

        sal_Int32 nInstance = 1;
        if (rNode.maChildren.size() >= 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aInstance = evaluateNumericArgument(*rNode.maChildren[2], std::nullopt);
            if (!aInstance)
                return makeFailure(aInstance.meError);
            const auto oWholeInstance = toWholeNumber(aInstance.maValue);
            if (!oWholeInstance || *oWholeInstance == 0)
                return makeFailure(api::Error::IllegalArgument);
            nInstance = *oWholeInstance;
        }

        bool bCaseInsensitive = false;
        if (rNode.maChildren.size() >= 4
            && rNode.maChildren[3]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aMatchMode = evaluateNumericArgument(*rNode.maChildren[3], std::nullopt);
            if (!aMatchMode)
                return makeFailure(aMatchMode.meError);
            const auto oWholeMatchMode = toWholeNumber(aMatchMode.maValue);
            if (!oWholeMatchMode || (*oWholeMatchMode != 0 && *oWholeMatchMode != 1))
                return makeFailure(api::Error::IllegalArgument);
            bCaseInsensitive = *oWholeMatchMode == 1;
        }

        bool bMatchEnd = false;
        if (rNode.maChildren.size() >= 5
            && rNode.maChildren[4]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aMatchEnd = evaluateScalarArgumentValue(*rNode.maChildren[4]);
            if (!aMatchEnd)
                return makeFailure(aMatchEnd.meError);
            const auto aMatchEndBool = coerceToBoolean(aMatchEnd.maValue);
            if (!aMatchEndBool)
                return makeFailure(aMatchEndBool.meError);
            bMatchEnd = aMatchEndBool.maValue;
        }

        const auto handleNotFound = [&]() -> EvaluationResult {
            if (rNode.maChildren.size() >= 6)
                return evaluateNode(*rNode.maChildren[5], rCurrentAddress);
            return makeFailure(api::Error::NotAvailable);
        };
        const auto oResult
            = setext::textBefore(aText.maValue, aDelimiters, nInstance, bCaseInsensitive, bMatchEnd);
        if (!oResult)
            return handleNotFound();
        return makeScalarResult(api::CellValue::text(*oResult));
    }

    if (aFunctionName == u"MID")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aTextArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aTextArgument)
            return aTextArgument;
        EvaluationResult aStartArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aStartArgument)
            return aStartArgument;
        EvaluationResult aLengthArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aLengthArgument)
            return aLengthArgument;

        const auto aText = coerceToString(aTextArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);
        const auto aStart = coerceToNumber(aStartArgument.maValue.maValue);
        if (!aStart)
            return makeFailure(aStart.meError);
        const auto aLength = coerceToNumber(aLengthArgument.maValue.maValue);
        if (!aLength)
            return makeFailure(aLength.meError);

        const auto oWholeStart = toWholeNumber(aStart.maValue);
        const auto oWholeLength = toWholeNumber(aLength.maValue);
        if (!oWholeStart || !oWholeLength || *oWholeStart < 1 || *oWholeLength < 0)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::text(
            setext::sliceText(aText.maValue, *oWholeStart - 1, *oWholeLength)));
    }

    if (aFunctionName == u"REPLACE")
    {
        if (rNode.maChildren.size() != 4)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aSourceArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aSourceArgument)
            return aSourceArgument;
        EvaluationResult aStartArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aStartArgument)
            return aStartArgument;
        EvaluationResult aLengthArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aLengthArgument)
            return aLengthArgument;
        EvaluationResult aReplacementArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[3], rCurrentAddress));
        if (!aReplacementArgument)
            return aReplacementArgument;

        const auto aSource = coerceToString(aSourceArgument.maValue.maValue);
        if (!aSource)
            return makeFailure(aSource.meError);
        const auto aStart = coerceToNumber(aStartArgument.maValue.maValue);
        if (!aStart)
            return makeFailure(aStart.meError);
        const auto aLength = coerceToNumber(aLengthArgument.maValue.maValue);
        if (!aLength)
            return makeFailure(aLength.meError);
        const auto aReplacement = coerceToString(aReplacementArgument.maValue.maValue);
        if (!aReplacement)
            return makeFailure(aReplacement.meError);

        const auto oWholeStart = toWholeNumber(aStart.maValue);
        const auto oWholeLength = toWholeNumber(aLength.maValue);
        if (!oWholeStart || !oWholeLength || *oWholeStart < 1 || *oWholeLength < 0)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::text(setext::replaceText(
            aSource.maValue, *oWholeStart - 1, *oWholeLength, aReplacement.maValue)));
    }

    if (aFunctionName == u"BASE")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;
        EvaluationResult aBaseArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aBaseArgument)
            return aBaseArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);
        const auto aBaseNumber = coerceToNumber(aBaseArgument.maValue.maValue);
        if (!aBaseNumber)
            return makeFailure(aBaseNumber.meError);

        std::optional<double> ofMinLength;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aLengthArgument
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
            if (!aLengthArgument)
                return aLengthArgument;

            const auto aLengthNumber = coerceToNumber(aLengthArgument.maValue.maValue);
            if (!aLengthNumber)
                return makeFailure(aLengthNumber.meError);
            ofMinLength = aLengthNumber.maValue;
        }

        const auto aResult = seconvert::evaluateBaseValue(
            aValueNumber.maValue, aBaseNumber.maValue, ofMinLength);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::text(aResult.maValue));
    }

    if (aFunctionName == u"ROMAN")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValueArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValueArgument)
            return aValueArgument;

        const auto aValueNumber = coerceToNumber(aValueArgument.maValue.maValue);
        if (!aValueNumber)
            return makeFailure(aValueNumber.meError);

        std::optional<double> ofMode;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            EvaluationResult aModeArgument
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aModeArgument)
                return aModeArgument;

            const auto aModeNumber = coerceToNumber(aModeArgument.maValue.maValue);
            if (!aModeNumber)
                return makeFailure(aModeNumber.meError);
            ofMode = aModeNumber.maValue;
        }

        const auto aResult = seconvert::evaluateRomanValue(aValueNumber.maValue, ofMode);
        if (!aResult)
            return makeFailure(aResult.meError);
        return makeScalarResult(api::CellValue::text(aResult.maValue));
    }

    if (aFunctionName == u"LEFT" || aFunctionName == u"RIGHT")
    {
        if (rNode.maChildren.empty() || rNode.maChildren.size() > 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aTextArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aTextArgument)
            return aTextArgument;

        const auto aText = coerceToString(aTextArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        sal_Int32 nLength = 1;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aLength = evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
            if (!aLength)
                return makeFailure(aLength.meError);
            const auto oWholeLength = toWholeNumber(aLength.maValue);
            if (!oWholeLength || *oWholeLength < 0)
                return makeFailure(api::Error::IllegalArgument);
            nLength = *oWholeLength;
        }

        return makeScalarResult(api::CellValue::text(setext::sliceTextLeftRight(
            aText.maValue, nLength, aFunctionName == u"RIGHT")));
    }

    if (aFunctionName == u"TEXTJOIN")
    {
        if (rNode.maChildren.size() < 3)
            return makeFailure(api::Error::IllegalArgument);

        const auto aDelimiterValue = evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aDelimiterValue)
            return makeFailure(aDelimiterValue.meError);
        const auto aDelimiter = coerceToString(aDelimiterValue.maValue);
        if (!aDelimiter)
            return makeFailure(aDelimiter.meError);

        const auto aIgnoreEmptyValue = evaluateScalarArgumentValue(*rNode.maChildren[1]);
        if (!aIgnoreEmptyValue)
            return makeFailure(aIgnoreEmptyValue.meError);
        const auto aIgnoreEmpty = coerceToBoolean(aIgnoreEmptyValue.maValue);
        if (!aIgnoreEmpty)
            return makeFailure(aIgnoreEmpty.meError);

        api::String aResult;
        bool bHaveAny = false;
        for (std::size_t nIndex = 2; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *rNode.maChildren[nIndex],
                [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);
                    if (rValue.isEmpty() && aIgnoreEmpty.maValue)
                        return api::ValueResult<bool>::success(true);

                    const auto aTextValue = coerceToString(rValue);
                    if (!aTextValue)
                        return api::ValueResult<bool>::failure(aTextValue.meError);

                    if (aTextValue.maValue.empty() && aIgnoreEmpty.maValue)
                        return api::ValueResult<bool>::success(true);

                    if (bHaveAny)
                        aResult += aDelimiter.maValue;
                    aResult += aTextValue.maValue;
                    bHaveAny = true;
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return makeFailure(aVisited.meError);
        }

        return makeScalarResult(api::CellValue::text(aResult));
    }

    if (aFunctionName == u"CONCAT")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        api::String aResult;
        for (const auto& pChild : rNode.maChildren)
        {
            const auto aVisited = visitFlattenedValues(
                visitFlattenedValues, *pChild,
                [&](const api::CellValue& rValue, bool) -> api::ValueResult<bool> {
                    if (rValue.isError())
                        return api::ValueResult<bool>::failure(rValue.meError);
                    const auto aTextValue = coerceToString(rValue);
                    if (!aTextValue)
                        return api::ValueResult<bool>::failure(aTextValue.meError);
                    aResult += aTextValue.maValue;
                    return api::ValueResult<bool>::success(true);
                });
            if (!aVisited)
                return makeFailure(aVisited.meError);
        }

        return makeScalarResult(api::CellValue::text(aResult));
    }

    if (aFunctionName == u"T")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aValue
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aValue)
            return aValue;

        if (aValue.maValue.maValue.isError())
            return makeFailure(aValue.maValue.maValue.meError);
        if (aValue.maValue.maValue.isText())
            return makeScalarResult(api::CellValue::text(aValue.maValue.maValue.maString));
        return makeScalarResult(api::CellValue::text({}));
    }

    if (aFunctionName == u"EXACT")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        auto materializeFirstValue = [&](const formula::Node& rArgument) -> EvaluationResult {
            EvaluationResult aValue = evaluateNode(rArgument, rCurrentAddress);
            if (!aValue)
                return aValue;
            if (aValue.maValue.isScalar())
                return aValue;
            return materializeReferenceValue(aValue.maValue.maReference, 0, 0);
        };

        EvaluationResult aLeft = materializeFirstValue(*rNode.maChildren[0]);
        if (!aLeft)
            return aLeft;

        EvaluationResult aRight = materializeFirstValue(*rNode.maChildren[1]);
        if (!aRight)
            return aRight;

        const auto aLeftText = coerceToString(aLeft.maValue.maValue);
        if (!aLeftText)
            return makeFailure(aLeftText.meError);

        const auto aRightText = coerceToString(aRight.maValue.maValue);
        if (!aRightText)
            return makeFailure(aRightText.meError);

        return makeScalarResult(api::CellValue::boolean(
            aLeftText.maValue == aRightText.maValue));
    }

    if (aFunctionName == u"MOD")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aNumerator
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aNumerator)
            return aNumerator;

        EvaluationResult aDenominator
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aDenominator)
            return aDenominator;

        const auto aLeftNumber = coerceToNumber(aNumerator.maValue.maValue);
        if (!aLeftNumber)
            return makeFailure(aLeftNumber.meError);
        const auto aRightNumber = coerceToNumber(aDenominator.maValue.maValue);
        if (!aRightNumber)
            return makeFailure(aRightNumber.meError);

        const auto aModResult = semath::evaluateModValue(
            aLeftNumber.maValue, aRightNumber.maValue);
        if (!aModResult)
            return makeFailure(aModResult.meError);
        return makeScalarResult(api::CellValue::number(aModResult.maValue));
    }

    if (aFunctionName == u"RAWSUBTRACT")
    {
        if (rNode.maChildren.size() < 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aFirst
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aFirst)
            return aFirst;
        const auto aFirstNumber = coerceToNumber(aFirst.maValue.maValue);
        if (!aFirstNumber)
            return makeFailure(aFirstNumber.meError);

        double fResult = aFirstNumber.maValue;
        for (std::size_t nIndex = 1; nIndex < rNode.maChildren.size(); ++nIndex)
        {
            EvaluationResult aNext
                = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[nIndex], rCurrentAddress));
            if (!aNext)
                return aNext;
            const auto aNextNumber = coerceToNumber(aNext.maValue.maValue);
            if (!aNextNumber)
                return makeFailure(aNextNumber.meError);
            fResult -= aNextNumber.maValue;
        }

        return makeScalarResult(api::CellValue::number(fResult));
    }

    if (aFunctionName == u"AND")
    {
        if (rNode.maChildren.empty())
            return makeFailure(api::Error::IllegalArgument);

        bool bResult = true;
        bool bSawValue = false;
        for (const auto& pChild : rNode.maChildren)
        {
            const bool bReferenceLike = pChild->meKind == formula::NodeKind::CellReference
                                        || pChild->meKind == formula::NodeKind::RangeReference
                                        || pChild->meKind == formula::NodeKind::NamedReference;

            EvaluationResult aArgument = bReferenceLike
                                             ? evaluateReferenceNode(*pChild, rCurrentAddress)
                                             : evaluateNode(*pChild, rCurrentAddress);
            if (!aArgument)
                return aArgument;

            if (aArgument.maValue.isMatrixReference())
            {
                const auto& rReference = aArgument.maValue.maReference;
                for (api::RowIndex nRow = 0; nRow < rReference.maRange.rowCount(); ++nRow)
                {
                    for (api::ColumnIndex nCol = 0; nCol < rReference.maRange.columnCount(); ++nCol)
                    {
                        EvaluationResult aCell = materializeReferenceValue(rReference, nCol, nRow);
                        if (!aCell)
                            return aCell;
                        if (aCell.maValue.maValue.isEmpty() || aCell.maValue.maValue.isText())
                            continue;
                        const auto aBool = coerceToBoolean(aCell.maValue.maValue);
                        if (!aBool)
                            return makeFailure(aBool.meError);
                        bResult = bResult && aBool.maValue;
                        bSawValue = true;
                    }
                }
                continue;
            }

            const auto aBool = coerceToBoolean(aArgument.maValue.maValue);
            if (!aBool)
                return makeFailure(aBool.meError);
            bResult = bResult && aBool.maValue;
            bSawValue = true;
        }

        if (!bSawValue)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::boolean(bResult));
    }

    return makeFailure(api::Error::IllegalArgument);
}

EvaluationResult Evaluator::evaluateNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::NumberLiteral:
            return makeScalarResult(api::CellValue::number(rNode.mfNumber));
        case formula::NodeKind::StringLiteral:
            return makeScalarResult(api::CellValue::text(rNode.maPrimaryText));
        case formula::NodeKind::BooleanLiteral:
            return makeScalarResult(api::CellValue::boolean(rNode.mbBoolean));
        case formula::NodeKind::ErrorLiteral:
            return makeScalarResult(api::CellValue::error(mapErrorLiteral(rNode.maPrimaryText)));
        case formula::NodeKind::EmptyArgument:
            return makeScalarResult(api::CellValue::empty());
        case formula::NodeKind::CellReference:
        {
            const auto aReference = resolveReferenceText(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aReference)
                return makeFailure(aReference.meError);
            return materializeReferenceValue(aReference.maValue, 0, 0);
        }
        case formula::NodeKind::RangeReference:
        {
            api::String aReference = rNode.maPrimaryText;
            aReference.push_back(u':');
            aReference += rNode.maSecondaryText;
            const auto aRange = resolveReferenceText(aReference, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            if (aRange.maValue.isSingleCell())
                return materializeReferenceValue(aRange.maValue, 0, 0);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::NamedReference:
        {
            if (const auto* pLocalBinding = lookupLocalBinding(rNode.maPrimaryText))
                return *pLocalBinding;
            const auto aRange = resolveNamedRange(rNode.maPrimaryText, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeFailure(aRange.meError);
            if (aRange.maValue.isSingleCell())
                return materializeReferenceValue(aRange.maValue, 0, 0);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::RangeConstructor:
            return makeFailure(api::Error::IllegalArgument);
        case formula::NodeKind::ReferenceList:
            return makeFailure(api::Error::IllegalArgument);
        case formula::NodeKind::ArrayConstant:
        {
            if (rNode.mnArrayRows != 1 || rNode.mnArrayColumns != 1 || rNode.maChildren.empty())
                return makeFailure(api::Error::IllegalArgument);
            return evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        }
        case formula::NodeKind::UnaryOperation:
        {
            EvaluationResult aChild = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
            if (!aChild)
                return aChild;
            const auto aNumber = coerceToNumber(aChild.maValue.maValue);
            if (!aNumber)
                return makeFailure(aNumber.meError);
            const double fValue = rNode.meUnaryOperator == formula::UnaryOperator::Minus
                                      ? -aNumber.maValue
                                      : aNumber.maValue;
            return makeScalarResult(api::CellValue::number(fValue));
        }
        case formula::NodeKind::BinaryOperation:
        {
            EvaluationResult aLeft = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
            if (!aLeft)
                return aLeft;
            EvaluationResult aRight = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
            if (!aRight)
                return aRight;

            if (rNode.meBinaryOperator == formula::BinaryOperator::Concat)
            {
                const auto aLeftText = coerceToString(aLeft.maValue.maValue);
                if (!aLeftText)
                    return makeFailure(aLeftText.meError);
                const auto aRightText = coerceToString(aRight.maValue.maValue);
                if (!aRightText)
                    return makeFailure(aRightText.meError);
                api::String aValue = aLeftText.maValue;
                aValue += aRightText.maValue;
                return makeScalarResult(api::CellValue::text(aValue));
            }

            if (rNode.meBinaryOperator == formula::BinaryOperator::Equal
                || rNode.meBinaryOperator == formula::BinaryOperator::NotEqual
                || rNode.meBinaryOperator == formula::BinaryOperator::Less
                || rNode.meBinaryOperator == formula::BinaryOperator::LessEqual
                || rNode.meBinaryOperator == formula::BinaryOperator::Greater
                || rNode.meBinaryOperator == formula::BinaryOperator::GreaterEqual)
            {
                if (aLeft.maValue.maValue.isText() && aRight.maValue.maValue.isText())
                {
                    return makeScalarResult(api::CellValue::boolean(evaluateStringComparison(
                        aLeft.maValue.maValue.maString, aRight.maValue.maValue.maString,
                        rNode.meBinaryOperator)));
                }

                const auto aLeftNumber = coerceToNumber(aLeft.maValue.maValue);
                if (!aLeftNumber)
                    return makeFailure(aLeftNumber.meError);
                const auto aRightNumber = coerceToNumber(aRight.maValue.maValue);
                if (!aRightNumber)
                    return makeFailure(aRightNumber.meError);
                return makeScalarResult(api::CellValue::boolean(evaluateNumericComparison(
                    aLeftNumber.maValue, aRightNumber.maValue, rNode.meBinaryOperator)));
            }

            const auto aLeftNumber = coerceToNumber(aLeft.maValue.maValue);
            if (!aLeftNumber)
                return makeFailure(aLeftNumber.meError);
            const auto aRightNumber = coerceToNumber(aRight.maValue.maValue);
            if (!aRightNumber)
                return makeFailure(aRightNumber.meError);

            switch (rNode.meBinaryOperator)
            {
                case formula::BinaryOperator::Add:
                    return makeScalarResult(api::CellValue::number(
                        ::rtl::math::approxAdd(aLeftNumber.maValue, aRightNumber.maValue)));
                case formula::BinaryOperator::Subtract:
                    return makeScalarResult(api::CellValue::number(
                        ::rtl::math::approxSub(aLeftNumber.maValue, aRightNumber.maValue)));
                case formula::BinaryOperator::Multiply:
                    return makeScalarResult(
                        api::CellValue::number(aLeftNumber.maValue * aRightNumber.maValue));
                case formula::BinaryOperator::Divide:
                    if (aRightNumber.maValue == 0.0)
                        return makeFailure(api::Error::DivisionByZero);
                    return makeScalarResult(
                        api::CellValue::number(aLeftNumber.maValue / aRightNumber.maValue));
                case formula::BinaryOperator::Power:
                    return makeScalarResult(api::CellValue::number(
                        std::pow(aLeftNumber.maValue, aRightNumber.maValue)));
                default:
                    return makeFailure(api::Error::IllegalArgument);
            }
        }
        case formula::NodeKind::FunctionCall:
            return evaluateFunction(rNode, rCurrentAddress);
    }

    return makeFailure(api::Error::IllegalArgument);
}

EvaluationResult Evaluator::evaluateFormula(
    api::StringView rFormula, const api::CellAddress& rCurrentAddress)
{
    const formula::ParseResult aParse = formula::parseFormula(rFormula);
    if (!aParse)
        return makeFailure(api::Error::IllegalArgument);
    return evaluateNode(*aParse.mpRoot, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateCompiledFormula(
    const spreadsheetengine::detail::token::CompiledFormula& rFormula,
    const api::CellAddress& rCurrentAddress)
{
    const auto oInflated
        = spreadsheetengine::detail::compiler::inflateCompiledFormulaNode(
            rFormula, mrWorkbook, rCurrentAddress);
    if (!oInflated)
        return makeFailure(api::Error::IllegalArgument);
    return evaluateNode(**oInflated, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateFormulaViaCompiledTokens(
    api::StringView rFormula, const api::CellAddress& rCurrentAddress)
{
    if (rCurrentAddress.mnSheet < 0
        || static_cast<std::size_t>(rCurrentAddress.mnSheet) >= mrWorkbook.maSheets.size())
    {
        return makeFailure(api::Error::IllegalArgument);
    }

    secompiler::WorkbookCompileHost aHost(mrWorkbook);
    const auto oContext = secompiler::makeWorkbookCompileContext(mrWorkbook,
        mrWorkbook.maSheets[static_cast<std::size_t>(rCurrentAddress.mnSheet)].maName,
        rCurrentAddress.mnColumn, rCurrentAddress.mnRow);
    if (!oContext)
        return makeFailure(api::Error::IllegalArgument);

    const auto aLowered = secompiler::lowerFormulaSource(rFormula, aHost, *oContext);
    if (!aLowered)
        return makeFailure(api::Error::IllegalArgument);
    return evaluateCompiledFormula(aLowered.maFormula, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateCellInternal(
    const api::CellAddress& rAddress, ExecutionMode eMode)
{
    if (!getSheet(rAddress.mnSheet))
        return makeFailure(api::Error::IllegalArgument);

    const workbook::Cell* pCell = getCell(rAddress);
    if (!pCell)
        return makeScalarResult(api::CellValue::empty());
    if (!pCell->hasFormula())
    {
        if (const auto oTypedValue = parseTypedStoredCellValue(*pCell))
            return makeScalarResult(*oTypedValue);
        return makeScalarResult(pCell->maValue);
    }

    CacheEntry& rEntry = cacheForMode(eMode)[makeAddressKey(rAddress)];
    if (rEntry.meState == CacheState::Complete)
        return rEntry.maResult;

    if (rEntry.meState == CacheState::Active)
    {
        EvaluationResult aCycle = makeFailure(api::Error::IllegalArgument);
        auto aIt = std::find(maEvaluationStack.begin(), maEvaluationStack.end(), rAddress);
        if (aIt != maEvaluationStack.end())
            aCycle.maCyclePath.assign(aIt, maEvaluationStack.end());
        aCycle.maCyclePath.push_back(rAddress);
        return aCycle;
    }

    rEntry.meState = CacheState::Active;
    maEvaluationStack.push_back(rAddress);

    const auto finalize = [&](EvaluationResult aResult) -> EvaluationResult {
        maEvaluationStack.pop_back();
        rEntry.meState = CacheState::Complete;
        rEntry.maResult = aResult;
        return aResult;
    };

    const ExecutionMode ePreviousMode = meActiveExecutionMode;
    meActiveExecutionMode = eMode;
    EvaluationResult aResult = eMode == ExecutionMode::CompiledToken
                                   ? evaluateFormulaViaCompiledTokens(pCell->maFormula, rAddress)
                                   : evaluateFormula(pCell->maFormula, rAddress);
    if (aResult && aResult.maValue.isMatrixReference())
    {
        // Standalone replay compares the anchor cell stored in FODS for matrix formulas.
        // Materialize the top-left value here instead of treating multi-cell array results as
        // an evaluation failure and falling back to the cached workbook value.
        aResult = materializeReferenceValue(aResult.maValue.maReference, 0, 0);
    }

    if (!aResult && aResult.maCyclePath.empty() && hasCachedFallbackValue(*pCell))
    {
        meActiveExecutionMode = ePreviousMode;
        if (const auto oTypedValue = parseTypedStoredCellValue(*pCell))
            return finalize(makeScalarResult(*oTypedValue, true));
        return finalize(makeScalarResult(pCell->maValue, true));
    }

    meActiveExecutionMode = ePreviousMode;
    return finalize(aResult);
}

EvaluationResult Evaluator::evaluateCell(const api::CellAddress& rAddress)
{
    return evaluateCellInternal(rAddress, ExecutionMode::Ast);
}

EvaluationResult Evaluator::evaluateCellViaCompiledTokens(const api::CellAddress& rAddress)
{
    return evaluateCellInternal(rAddress, ExecutionMode::CompiledToken);
}

} // namespace spreadsheetengine::core::eval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
