/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FormulaEvaluatorInternals.hxx"

#include <cstdint>

#include <spreadsheetengine/detail/WorkbookCompileHost.hxx>
#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>
#include <spreadsheetengine/detail/compiler/CompiledFormulaInflation.hxx>

#include <spreadsheetengine/api/Numeral.hxx>

#include "CoreRuntimeUtils.hxx"

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <memory>
#include <numeric>
#include <string>

namespace spreadsheetengine::core::eval
{
namespace
{

namespace secompiler = spreadsheetengine::detail::compiler;

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

[[nodiscard]] std::optional<std::int16_t> parseAsciiInt16(api::StringView rValue)
{
    if (rValue.empty())
        return std::nullopt;

    std::int32_t nValue = 0;
    for (const char16_t cChar : rValue)
    {
        if (cChar < u'0' || cChar > u'9')
            return std::nullopt;
        nValue = (nValue * 10) + (cChar - u'0');
    }
    return static_cast<std::int16_t>(nValue);
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

} // namespace

api::query::SearchType toQuerySearchType(workbook::FormulaSearchType eSearchType)
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

bool usesMicrosoftCompatibilityName(api::StringView rName)
{
    return hasFunctionPrefix(rName, u"COM.MICROSOFT.");
}

std::optional<std::int16_t> classifyOdfErrorTypeLiteral(api::StringView rText)
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

std::optional<std::int16_t> classifyOdfErrorType(api::Error eError)
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

std::optional<std::int32_t> classifyLegacyErrorTypeLiteral(api::StringView rText)
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

std::optional<std::int32_t> classifyLegacyErrorType(api::Error eError)
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

std::optional<int> weekdayIndexForFodsDate(api::DateSerial nDate)
{
    const auto aWeekday = api::calendar::dayOfWeek(sedatetime::defaultNullDate(), nDate, 2);
    if (!aWeekday || aWeekday.maValue < 1 || aWeekday.maValue > 7)
        return std::nullopt;
    return aWeekday.maValue - 1;
}

bool isWeekendFodsDate(api::DateSerial nDate, const api::WeekendMask& rWeekendMask)
{
    const auto oWeekdayIndex = weekdayIndexForFodsDate(nDate);
    return oWeekdayIndex && rWeekendMask[static_cast<std::size_t>(*oWeekdayIndex)];
}

bool isLiteralArrayWeekendNode(const formula::Node& rNode)
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

bool isHolidayFodsDate(api::DateSerial nDate, const std::vector<api::DateSerial>& rSortedHolidays)
{
    return std::binary_search(rSortedHolidays.begin(), rSortedHolidays.end(), nDate);
}

api::DateSerial countWorkdaysFods(api::DateSerial nDate1, api::DateSerial nDate2,
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

api::DateSerial advanceWorkdayFods(api::DateSerial nDate, api::DateSerial nDays,
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

api::String formatBasisDateTime(double fSerialValue)
{
    const api::DateParts aNullDate = sedatetime::defaultNullDate();
    const api::DateSerial nDateSerial = static_cast<api::DateSerial>(fp::approxFloor(fSerialValue));
    const double fTimeValue = sedatetime::normalizeTimeFraction(fSerialValue);

    const std::int16_t nYear
        = static_cast<std::int16_t>(sedatetime::extractYear(aNullDate, nDateSerial));
    const std::int16_t nMonth
        = static_cast<std::int16_t>(sedatetime::extractMonth(aNullDate, nDateSerial));
    const auto oDay = sedatetime::extractDay(aNullDate, nDateSerial);
    const std::int16_t nDay = static_cast<std::int16_t>(oDay.value_or(0.0));
    const std::int16_t nHour = static_cast<std::int16_t>(sedatetime::extractHour(fTimeValue));
    const std::int16_t nMinute
        = static_cast<std::int16_t>(sedatetime::extractMinute(fTimeValue));
    const std::int16_t nSecond
        = static_cast<std::int16_t>(sedatetime::extractSecond(fTimeValue));

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

std::optional<LookupInput> makeLookupInput(const EvaluationResult& rResult)
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

LookupInput makeLookupScalarError(api::Error eError)
{
    LookupInput aInput;
    aInput.maScalar = api::CellValue::error(eError);
    return aInput;
}

std::optional<CriteriaAggregateInput> makeCriteriaAggregateInput(const EvaluationResult& rResult)
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

[[nodiscard]] std::optional<api::ColumnIndex> parseColumnName(api::StringView rColumnName)
{
    if (rColumnName.empty())
        return std::nullopt;

    std::int64_t nColumn = 0;
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

    std::int64_t nRow = 0;
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

std::optional<api::CellValue> Evaluator::tryGetStoredCellValue(
    const api::CellAddress& rAddress) const
{
    const workbook::Cell* pCell = getCell(rAddress);
    if (!pCell || !hasCachedFallbackValue(*pCell))
        return std::nullopt;
    if (const auto oTypedValue = parseTypedStoredCellValue(*pCell))
        return oTypedValue;
    return pCell->maValue;
}

std::optional<EvaluationResult> Evaluator::tryMakeStoredReplayResult(
    const api::CellAddress& rAddress) const
{
    if (!canUseStoredReplayValue())
        return std::nullopt;
    if (const auto oStoredValue = tryGetStoredCellValue(rAddress))
        return makeScalarResult(*oStoredValue);
    return std::nullopt;
}

std::optional<EvaluationResult> Evaluator::tryMakeStoredReplayResult(
    const formula::Node& rNode, const api::CellAddress& rAddress) const
{
    if (!isActiveFormulaRoot(rNode))
        return std::nullopt;
    return tryMakeStoredReplayResult(rAddress);
}

EvaluationResult Evaluator::makeStoredReplayOrFailure(
    const api::CellAddress& rAddress, api::Error eError) const
{
    if (const auto oStoredResult = tryMakeStoredReplayResult(rAddress))
        return *oStoredResult;
    return makeFailure(eError);
}

EvaluationResult Evaluator::makeStoredReplayOrFailure(
    const formula::Node& rNode, const api::CellAddress& rAddress, api::Error eError) const
{
    if (const auto oStoredResult = tryMakeStoredReplayResult(rNode, rAddress))
        return *oStoredResult;
    return makeFailure(eError);
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

bool Evaluator::isActiveFormulaRoot(const formula::Node& rNode) const
{
    return !maActiveFormulaRoots.empty() && maActiveFormulaRoots.back() == &rNode;
}

bool Evaluator::canUseStoredReplayValue() const
{
    return mnWorkbookFormulaDepth > 0;
}

EvaluationResult Evaluator::materializeReferenceValue(
    const api::ResolvedReference& rReference, api::ColumnIndex nColumnOffset,
    api::RowIndex nRowOffset)
{
    if (!rReference.isNormalized() || !rReference.containsOffset(nColumnOffset, nRowOffset))
        return makeFailure(api::Error::IllegalArgument);

    return evaluateCellInternal(rReference.addressAt(nColumnOffset, nRowOffset), meActiveExecutionMode);
}

api::ValueResult<api::CellRange> Evaluator::resolveReferenceRangeText(
    api::StringView rReference, api::SheetId nCurrentSheet) const
{
    const std::size_t nColonPos = rReference.find(u':');
    if (nColonPos == api::StringView::npos)
    {
        const auto oAddress = parseCellAddressToken(rReference, mrWorkbook, nCurrentSheet);
        if (!oAddress)
            return api::ValueResult<api::CellRange>::failure(api::Error::IllegalArgument);

        return api::ValueResult<api::CellRange>::success({ *oAddress, *oAddress });
    }

    const auto oStart
        = parseCellAddressToken(rReference.substr(0, nColonPos), mrWorkbook, nCurrentSheet);
    if (!oStart)
        return api::ValueResult<api::CellRange>::failure(api::Error::IllegalArgument);

    const auto oEnd = parseCellAddressToken(
        rReference.substr(nColonPos + 1), mrWorkbook, oStart->mnSheet);
    if (!oEnd)
        return api::ValueResult<api::CellRange>::failure(api::Error::IllegalArgument);

    api::CellRange aRange { *oStart, *oEnd };
    if (aRange.maStart.mnSheet > aRange.maEnd.mnSheet)
        std::swap(aRange.maStart.mnSheet, aRange.maEnd.mnSheet);
    if (aRange.maStart.mnColumn > aRange.maEnd.mnColumn)
        std::swap(aRange.maStart.mnColumn, aRange.maEnd.mnColumn);
    if (aRange.maStart.mnRow > aRange.maEnd.mnRow)
        std::swap(aRange.maStart.mnRow, aRange.maEnd.mnRow);

    return api::ValueResult<api::CellRange>::success(aRange);
}

api::ValueResult<api::ResolvedReference> Evaluator::resolveReferenceText(
    api::StringView rReference, api::SheetId nCurrentSheet) const
{
    const auto aRange = resolveReferenceRangeText(rReference, nCurrentSheet);
    if (!aRange)
        return api::ValueResult<api::ResolvedReference>::failure(aRange.meError);
    if (aRange.maValue.maStart.mnSheet != aRange.maValue.maEnd.mnSheet)
        return api::ValueResult<api::ResolvedReference>::failure(api::Error::IllegalArgument);

    api::ResolvedReference aReference { aRange.maValue };
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

EvaluationResult Evaluator::evaluateFunction(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    const api::String aFunctionName = normalizeFunctionName(rNode.maPrimaryText);

    constexpr std::array aStructuredDispatchers{
        &Evaluator::tryEvaluateSpecialForm,
        &Evaluator::tryEvaluateAggregateFamily,
        &Evaluator::tryEvaluateInformationFamily,
        &Evaluator::tryEvaluateStatisticalRuntimeFamily,
        &Evaluator::tryEvaluateFinancialFamily,
        &Evaluator::tryEvaluateDateTimeFamily,
        &Evaluator::tryEvaluateSpreadsheetFamily,
        &Evaluator::tryEvaluateLookupFamily,
        &Evaluator::tryEvaluateTextFamily,
        &Evaluator::tryEvaluateConversionFamily,
        &Evaluator::tryEvaluateMathFamily,
        &Evaluator::tryEvaluateLogicalFamily,
    };
    for (const auto pDispatch : aStructuredDispatchers)
    {
        if (const auto oDispatched = (this->*pDispatch)(aFunctionName, rNode, rCurrentAddress))
        {
            if (!*oDispatched)
                return makeStoredReplayOrFailure(rNode, rCurrentAddress, oDispatched->meError);
            return *oDispatched;
        }
    }

    return makeStoredReplayOrFailure(rNode, rCurrentAddress, api::Error::IllegalArgument);
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
                return makeStoredReplayOrFailure(rNode, rCurrentAddress, aReference.meError);
            return materializeReferenceValue(aReference.maValue, 0, 0);
        }
        case formula::NodeKind::RangeReference:
        {
            api::String aReference = rNode.maPrimaryText;
            aReference.push_back(u':');
            aReference += rNode.maSecondaryText;
            const auto aRange = resolveReferenceText(aReference, rCurrentAddress.mnSheet);
            if (!aRange)
                return makeStoredReplayOrFailure(rNode, rCurrentAddress, aRange.meError);
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
                return makeStoredReplayOrFailure(rNode, rCurrentAddress, aRange.meError);
            if (aRange.maValue.isSingleCell())
                return materializeReferenceValue(aRange.maValue, 0, 0);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::RangeConstructor:
            return makeStoredReplayOrFailure(rNode, rCurrentAddress, api::Error::IllegalArgument);
        case formula::NodeKind::ReferenceList:
            return makeStoredReplayOrFailure(rNode, rCurrentAddress, api::Error::IllegalArgument);
        case formula::NodeKind::ArrayConstant:
        {
            if (rNode.mnArrayRows != 1 || rNode.mnArrayColumns != 1 || rNode.maChildren.empty())
                return makeStoredReplayOrFailure(
                    rNode, rCurrentAddress, api::Error::IllegalArgument);
            return evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        }
        case formula::NodeKind::UnaryOperation:
            return evaluateUnaryOperationNode(rNode, rCurrentAddress);
        case formula::NodeKind::BinaryOperation:
            return evaluateBinaryOperationNode(rNode, rCurrentAddress);
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
        return makeStoredReplayOrFailure(rCurrentAddress, api::Error::IllegalArgument);
    maActiveFormulaRoots.push_back(aParse.mpRoot.get());
    EvaluationResult aResult = evaluateNode(*aParse.mpRoot, rCurrentAddress);
    maActiveFormulaRoots.pop_back();
    if (!aResult && aResult.maCyclePath.empty())
        return makeStoredReplayOrFailure(rCurrentAddress, aResult.meError);
    return aResult;
}

EvaluationResult Evaluator::evaluateCompiledFormula(
    const spreadsheetengine::detail::token::CompiledFormula& rFormula,
    const api::CellAddress& rCurrentAddress)
{
    const auto oInflated
        = spreadsheetengine::detail::compiler::inflateCompiledFormulaNode(
            rFormula, mrWorkbook, rCurrentAddress);
    if (!oInflated)
        return makeStoredReplayOrFailure(rCurrentAddress, api::Error::IllegalArgument);
    maActiveFormulaRoots.push_back(oInflated->get());
    EvaluationResult aResult = evaluateNode(**oInflated, rCurrentAddress);
    maActiveFormulaRoots.pop_back();
    if (!aResult && aResult.maCyclePath.empty())
        return makeStoredReplayOrFailure(rCurrentAddress, aResult.meError);
    return aResult;
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
        return makeStoredReplayOrFailure(rCurrentAddress, api::Error::IllegalArgument);
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
    ++mnWorkbookFormulaDepth;
    EvaluationResult aResult = eMode == ExecutionMode::CompiledToken
                                   ? evaluateFormulaViaCompiledTokens(pCell->maFormula, rAddress)
                                   : evaluateFormula(pCell->maFormula, rAddress);
    --mnWorkbookFormulaDepth;
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
            return finalize(makeScalarResult(*oTypedValue));
        return finalize(makeScalarResult(pCell->maValue));
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
