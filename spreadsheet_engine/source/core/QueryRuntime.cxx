/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/QueryRuntime.hxx>
#include <cstdint>

#include <spreadsheetengine/runtime/FloatingPoint.hxx>

#include <optional>
#include <string>

#include <unicode/regex.h>
#include <unicode/ustring.h>

namespace spreadsheetengine::core::query
{
namespace
{

[[nodiscard]] bool isRegexMetaCharacter(char16_t cChar)
{
    switch (cChar)
    {
        case u'.':
        case u'^':
        case u'$':
        case u'|':
        case u'(':
        case u')':
        case u'[':
        case u']':
        case u'{':
        case u'}':
        case u'+':
        case u'?':
        case u'*':
        case u'\\':
            return true;
        default:
            return false;
    }
}

[[nodiscard]] bool mayBeRegExp(api::StringView rValue)
{
    if (rValue.empty() || (rValue.size() == 1 && rValue.front() != u'.'))
        return false;

    return rValue.find_first_of(u"?*+.[]^$\\<>()|") != api::StringView::npos;
}

[[nodiscard]] bool mayBeWildcard(api::StringView rValue)
{
    return rValue.find_first_of(u"*?~") != api::StringView::npos;
}

[[nodiscard]] std::string toUtf8String(api::StringView rText)
{
    if (rText.empty())
        return {};

    UErrorCode eStatus = U_ZERO_ERROR;
    int32_t nLength = 0;
    u_strToUTF8(nullptr, 0, &nLength, reinterpret_cast<const UChar*>(rText.data()),
        static_cast<int32_t>(rText.size()), &eStatus);
    if (eStatus != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(eStatus))
        return {};

    std::string aResult(static_cast<std::size_t>(nLength), '\0');
    eStatus = U_ZERO_ERROR;
    u_strToUTF8(aResult.data(), nLength, nullptr, reinterpret_cast<const UChar*>(rText.data()),
        static_cast<int32_t>(rText.size()), &eStatus);
    if (U_FAILURE(eStatus))
        return {};

    return aResult;
}

[[nodiscard]] std::string makeWholeCellRegexPattern(
    api::StringView rPattern, api::query::SearchType eSearchType)
{
    std::string aPattern;
    aPattern.reserve(rPattern.size() * 2 + 2);
    aPattern.push_back('^');

    if (eSearchType == api::query::SearchType::Wildcard)
    {
        bool bEscapeNext = false;
        for (const char16_t cChar : rPattern)
        {
            if (bEscapeNext)
            {
                if (isRegexMetaCharacter(cChar))
                    aPattern.push_back('\\');
                aPattern += toUtf8String(api::StringView(&cChar, 1));
                bEscapeNext = false;
                continue;
            }

            if (cChar == u'~')
            {
                bEscapeNext = true;
                continue;
            }

            if (cChar == u'*')
            {
                aPattern += ".*";
                continue;
            }

            if (cChar == u'?')
            {
                aPattern.push_back('.');
                continue;
            }

            if (isRegexMetaCharacter(cChar))
                aPattern.push_back('\\');
            aPattern += toUtf8String(api::StringView(&cChar, 1));
        }

        if (bEscapeNext)
            aPattern += "~";
    }
    else
    {
        aPattern += toUtf8String(rPattern);
    }

    aPattern.push_back('$');
    return aPattern;
}

[[nodiscard]] std::string makeQueryRegexPattern(
    api::StringView rPattern, api::query::SearchType eSearchType, bool bMatchWholeCell)
{
    std::string aPattern;
    aPattern.reserve(rPattern.size() * 2 + 2);
    if (bMatchWholeCell)
        aPattern.push_back('^');

    if (eSearchType == api::query::SearchType::Wildcard)
    {
        bool bEscapeNext = false;
        for (const char16_t cChar : rPattern)
        {
            if (bEscapeNext)
            {
                if (isRegexMetaCharacter(cChar))
                    aPattern.push_back('\\');
                aPattern += toUtf8String(api::StringView(&cChar, 1));
                bEscapeNext = false;
                continue;
            }

            if (cChar == u'~')
            {
                bEscapeNext = true;
                continue;
            }

            if (cChar == u'*')
            {
                aPattern += ".*";
                continue;
            }

            if (cChar == u'?')
            {
                aPattern.push_back('.');
                continue;
            }

            if (isRegexMetaCharacter(cChar))
                aPattern.push_back('\\');
            aPattern += toUtf8String(api::StringView(&cChar, 1));
        }

        if (bEscapeNext)
            aPattern += "~";
    }
    else
    {
        aPattern += toUtf8String(rPattern);
    }

    if (bMatchWholeCell)
        aPattern.push_back('$');
    return aPattern;
}

[[nodiscard]] bool matchesIcuRegexPattern(
    std::string_view rPatternUtf8, api::StringView rCandidateText, bool bMatchWholeCell)
{
    UErrorCode eStatus = U_ZERO_ERROR;
    UParseError aParseError {};
    const icu::UnicodeString aPattern
        = icu::UnicodeString::fromUTF8(icu::StringPiece(rPatternUtf8.data(),
            static_cast<int32_t>(rPatternUtf8.size())));
    std::unique_ptr<icu::RegexPattern> xPattern(
        icu::RegexPattern::compile(aPattern, UREGEX_CASE_INSENSITIVE, aParseError, eStatus));
    if (!xPattern || U_FAILURE(eStatus))
        return false;

    const icu::UnicodeString aCandidate(false, reinterpret_cast<const UChar*>(rCandidateText.data()),
        static_cast<int32_t>(rCandidateText.size()));
    std::unique_ptr<icu::RegexMatcher> xMatcher(xPattern->matcher(aCandidate, eStatus));
    if (!xMatcher || U_FAILURE(eStatus))
        return false;

    xMatcher->setTimeLimit(23 * 1000, eStatus);
    if (U_FAILURE(eStatus))
        return false;

    const bool bMatched = bMatchWholeCell ? xMatcher->matches(eStatus) : xMatcher->find(eStatus);
    return U_SUCCESS(eStatus) && bMatched;
}

[[nodiscard]] bool containsFoldedText(api::StringView rHaystack, api::StringView rNeedle)
{
    if (rNeedle.empty())
        return true;

    icu::UnicodeString aHaystack(
        reinterpret_cast<const UChar*>(rHaystack.data()), static_cast<int32_t>(rHaystack.size()));
    icu::UnicodeString aNeedle(
        reinterpret_cast<const UChar*>(rNeedle.data()), static_cast<int32_t>(rNeedle.size()));
    return aHaystack.foldCase().indexOf(aNeedle.foldCase()) >= 0;
}

[[nodiscard]] std::optional<formula::BinaryOperator> parseCriteriaOperator(
    api::StringView rText, api::StringView& rOperandText)
{
    if (rText.size() >= 2)
    {
        if (rText[0] == u'<' && rText[1] == u'>')
        {
            rOperandText = rText.substr(2);
            return formula::BinaryOperator::NotEqual;
        }
        if (rText[0] == u'<' && rText[1] == u'=')
        {
            rOperandText = rText.substr(2);
            return formula::BinaryOperator::LessEqual;
        }
        if (rText[0] == u'>' && rText[1] == u'=')
        {
            rOperandText = rText.substr(2);
            return formula::BinaryOperator::GreaterEqual;
        }
    }

    if (!rText.empty())
    {
        if (rText[0] == u'<')
        {
            rOperandText = rText.substr(1);
            return formula::BinaryOperator::Less;
        }
        if (rText[0] == u'>')
        {
            rOperandText = rText.substr(1);
            return formula::BinaryOperator::Greater;
        }
        if (rText[0] == u'=')
        {
            rOperandText = rText.substr(1);
            return formula::BinaryOperator::Equal;
        }
    }

    rOperandText = rText;
    return formula::BinaryOperator::Equal;
}

[[nodiscard]] std::optional<double> coerceCriteriaComparisonNumber(
    const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return std::nullopt;
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
            return rValue.mfNumber;
        case api::CellValueKind::Text:
            return std::nullopt;
        case api::CellValueKind::Error:
            return std::nullopt;
    }

    return std::nullopt;
}

[[nodiscard]] bool isCriteriaEmptyValue(const api::CellValue& rValue)
{
    return rValue.isEmpty() || (rValue.isText() && rValue.maString.empty());
}

[[nodiscard]] std::optional<double> coerceCriteriaAggregateNumber(
    const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
            return rValue.mfNumber;
        case api::CellValueKind::Text:
        case api::CellValueKind::Empty:
        case api::CellValueKind::Error:
            return std::nullopt;
    }

    return std::nullopt;
}

[[nodiscard]] bool sameCriteriaAggregateShape(
    const CriteriaAggregateInput& rLeft, const CriteriaAggregateInput& rRight)
{
    return rLeft.mnColumns == rRight.mnColumns && rLeft.mnRows == rRight.mnRows;
}

} // namespace

std::int32_t compareFoldedText(api::StringView rLeft, api::StringView rRight)
{
    UErrorCode eStatus = U_ZERO_ERROR;
    return u_strCaseCompare(reinterpret_cast<const UChar*>(rLeft.data()),
        static_cast<int32_t>(rLeft.size()), reinterpret_cast<const UChar*>(rRight.data()),
        static_cast<int32_t>(rRight.size()), 0, &eStatus);
}

bool matchesWholeCellLookupText(
    api::StringView rLookupText, api::StringView rCandidateText, api::query::SearchType eSearchType)
{
    switch (eSearchType)
    {
        case api::query::SearchType::Regex:
            if (!mayBeRegExp(rLookupText))
                return compareFoldedText(rCandidateText, rLookupText) == 0;
            break;
        case api::query::SearchType::Wildcard:
            if (!mayBeWildcard(rLookupText))
                return compareFoldedText(rCandidateText, rLookupText) == 0;
            break;
        case api::query::SearchType::Normal:
            return compareFoldedText(rCandidateText, rLookupText) == 0;
    }

    return matchesIcuRegexPattern(
        makeWholeCellRegexPattern(rLookupText, eSearchType), rCandidateText, true);
}

bool matchesQueryText(api::StringView rLookupText, api::StringView rCandidateText,
    api::query::SearchType eSearchType, bool bMatchWholeCell)
{
    switch (eSearchType)
    {
        case api::query::SearchType::Regex:
            if (!mayBeRegExp(rLookupText))
                return bMatchWholeCell ? compareFoldedText(rCandidateText, rLookupText) == 0
                                       : containsFoldedText(rCandidateText, rLookupText);
            break;
        case api::query::SearchType::Wildcard:
            if (!mayBeWildcard(rLookupText))
                return bMatchWholeCell ? compareFoldedText(rCandidateText, rLookupText) == 0
                                       : containsFoldedText(rCandidateText, rLookupText);
            break;
        case api::query::SearchType::Normal:
            return bMatchWholeCell ? compareFoldedText(rCandidateText, rLookupText) == 0
                                   : containsFoldedText(rCandidateText, rLookupText);
    }

    return matchesIcuRegexPattern(
        makeQueryRegexPattern(rLookupText, eSearchType, bMatchWholeCell), rCandidateText,
        bMatchWholeCell);
}

std::optional<CriteriaPredicate> makeCriteriaPredicate(const api::CellValue& rCriteriaValue,
    CriteriaNumberTextParser pParseNumberText, CriteriaAsciiDoubleParser pParseAsciiDouble)
{
    CriteriaPredicate aPredicate;
    if (rCriteriaValue.isError())
        return std::nullopt;

    if (rCriteriaValue.isEmpty())
        return aPredicate;

    if (rCriteriaValue.isNumber() || rCriteriaValue.isBoolean())
    {
        aPredicate.meOperandKind = CriteriaPredicate::OperandKind::Number;
        aPredicate.mfNumber = rCriteriaValue.mfNumber;
        return aPredicate;
    }

    api::StringView aOperandText;
    const auto oOperator = parseCriteriaOperator(rCriteriaValue.maString, aOperandText);
    if (!oOperator)
        return std::nullopt;
    aPredicate.meOperator = *oOperator;

    if (aOperandText.empty())
    {
        aPredicate.mbOperatorOnlyTextCriterion = !rCriteriaValue.maString.empty();
        return aPredicate;
    }

    if (pParseNumberText)
    {
        if (const auto oParsed = pParseNumberText(aOperandText))
        {
            aPredicate.meOperandKind = CriteriaPredicate::OperandKind::Number;
            aPredicate.mfNumber = oParsed->mfValue;
            aPredicate.maNumericText = api::String(aOperandText);
            aPredicate.mbNumberOriginatedFromText = true;
            return aPredicate;
        }
    }
    if (pParseAsciiDouble)
    {
        if (const auto oNumber = pParseAsciiDouble(aOperandText))
        {
            aPredicate.meOperandKind = CriteriaPredicate::OperandKind::Number;
            aPredicate.mfNumber = *oNumber;
            aPredicate.maNumericText = api::String(aOperandText);
            aPredicate.mbNumberOriginatedFromText = true;
            return aPredicate;
        }
    }

    aPredicate.meOperandKind = CriteriaPredicate::OperandKind::Text;
    aPredicate.maText = api::String(aOperandText);
    return aPredicate;
}

bool matchesCriteriaPredicate(const CriteriaPredicate& rPredicate, const api::CellValue& rCandidate,
    api::query::SearchType eSearchType, bool bMatchWholeCell)
{
    switch (rPredicate.meOperandKind)
    {
        case CriteriaPredicate::OperandKind::Empty:
        {
            const bool bCandidateEmpty = isCriteriaEmptyValue(rCandidate);
            const bool bCandidateBlankCell = rCandidate.isEmpty();
            const bool bCandidateZeroLike
                = (rCandidate.isNumber() || rCandidate.isBoolean())
                  && fp::approxEqual(rCandidate.mfNumber, 0.0);
            switch (rPredicate.meOperator)
            {
                case formula::BinaryOperator::Equal:
                    if (rPredicate.mbOperatorOnlyTextCriterion)
                        return bCandidateBlankCell;
                    return bCandidateEmpty || bCandidateZeroLike;
                case formula::BinaryOperator::NotEqual:
                    if (rPredicate.mbOperatorOnlyTextCriterion)
                        return !bCandidateBlankCell;
                case formula::BinaryOperator::Greater:
                case formula::BinaryOperator::GreaterEqual:
                    return !(bCandidateEmpty || bCandidateZeroLike);
                case formula::BinaryOperator::Less:
                case formula::BinaryOperator::LessEqual:
                    return false;
                default:
                    return false;
            }
        }
        case CriteriaPredicate::OperandKind::Number:
        {
            const bool bCandidateEmpty = isCriteriaEmptyValue(rCandidate);
            if (bCandidateEmpty)
            {
                if (rPredicate.meOperator == formula::BinaryOperator::NotEqual)
                    return true;
                if (rPredicate.meOperator != formula::BinaryOperator::Equal)
                    return false;
            }

            const auto oCandidateNumber = coerceCriteriaComparisonNumber(rCandidate);
            if (!oCandidateNumber)
            {
                if (rPredicate.mbNumberOriginatedFromText && rCandidate.isText()
                    && (rPredicate.meOperator == formula::BinaryOperator::Equal
                        || rPredicate.meOperator == formula::BinaryOperator::NotEqual))
                {
                    const bool bMatch = matchesQueryText(
                        rPredicate.maNumericText, rCandidate.maString, eSearchType, bMatchWholeCell);
                    return rPredicate.meOperator == formula::BinaryOperator::NotEqual ? !bMatch
                                                                                      : bMatch;
                }
                return false;
            }
            switch (rPredicate.meOperator)
            {
                case formula::BinaryOperator::Equal:
                    return fp::approxEqual(*oCandidateNumber, rPredicate.mfNumber);
                case formula::BinaryOperator::NotEqual:
                    return !fp::approxEqual(*oCandidateNumber, rPredicate.mfNumber);
                case formula::BinaryOperator::Less:
                    return *oCandidateNumber < rPredicate.mfNumber;
                case formula::BinaryOperator::LessEqual:
                    return *oCandidateNumber < rPredicate.mfNumber
                           || fp::approxEqual(*oCandidateNumber, rPredicate.mfNumber);
                case formula::BinaryOperator::Greater:
                    return *oCandidateNumber > rPredicate.mfNumber;
                case formula::BinaryOperator::GreaterEqual:
                    return *oCandidateNumber > rPredicate.mfNumber
                           || fp::approxEqual(*oCandidateNumber, rPredicate.mfNumber);
                default:
                    return false;
            }
        }
        case CriteriaPredicate::OperandKind::Text:
        {
            if (rCandidate.isError())
                return false;

            api::String aCandidateText;
            if (rCandidate.isText())
                aCandidateText = rCandidate.maString;
            else if (rCandidate.isEmpty())
                aCandidateText.clear();
            else
                return false;

            if (rPredicate.meOperator == formula::BinaryOperator::Equal
                || rPredicate.meOperator == formula::BinaryOperator::NotEqual)
            {
                const bool bMatch = matchesQueryText(
                    rPredicate.maText, aCandidateText, eSearchType, bMatchWholeCell);
                return rPredicate.meOperator == formula::BinaryOperator::NotEqual ? !bMatch : bMatch;
            }

            if (aCandidateText.empty())
                return false;

            const std::int32_t nCompare = compareFoldedText(aCandidateText, rPredicate.maText);
            switch (rPredicate.meOperator)
            {
                case formula::BinaryOperator::Less:
                    return nCompare < 0;
                case formula::BinaryOperator::LessEqual:
                    return nCompare <= 0;
                case formula::BinaryOperator::Greater:
                    return nCompare > 0;
                case formula::BinaryOperator::GreaterEqual:
                    return nCompare >= 0;
                default:
                    return false;
            }
        }
    }

    return false;
}

api::ValueResult<api::CellValue> evaluateCriteriaAggregate(
    const CriteriaAggregateMaterializer& rMaterializer,
    const std::vector<CriteriaAggregateInput>& rCriteriaRanges,
    const std::vector<CriteriaPredicate>& rCriteria, const CriteriaAggregateInput* pTargetRange,
    CriteriaAggregateKind eAggregateKind, api::query::SearchType eSearchType,
    bool bMatchWholeCell)
{
    if (rCriteriaRanges.empty() || rCriteriaRanges.size() != rCriteria.size())
        return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);

    const CriteriaAggregateInput& rBaseRange = rCriteriaRanges.front();
    for (std::size_t nIndex = 1; nIndex < rCriteriaRanges.size(); ++nIndex)
    {
        if (!sameCriteriaAggregateShape(rBaseRange, rCriteriaRanges[nIndex]))
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
    }
    if (pTargetRange && !sameCriteriaAggregateShape(rBaseRange, *pTargetRange))
        return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);

    std::size_t nCount = 0;
    double fSum = 0.0;
    double fProduct = 1.0;
    double fBest = 0.0;
    bool bHasBest = false;

    for (api::MatrixSize nRow = 0; nRow < rBaseRange.mnRows; ++nRow)
    {
        for (api::MatrixSize nCol = 0; nCol < rBaseRange.mnColumns; ++nCol)
        {
            const api::MatrixCoordinate aCoordinate { nCol, nRow };
            bool bMatch = true;
            for (std::size_t nIndex = 0; nIndex < rCriteriaRanges.size(); ++nIndex)
            {
                const auto aCandidate = rMaterializer.materialize(rCriteriaRanges[nIndex], aCoordinate);
                if (!aCandidate
                    || !matchesCriteriaPredicate(
                        rCriteria[nIndex], aCandidate.maValue, eSearchType, bMatchWholeCell))
                {
                    bMatch = false;
                    break;
                }
            }

            if (!bMatch)
                continue;

            if (eAggregateKind == CriteriaAggregateKind::Count)
            {
                ++nCount;
                continue;
            }

            const auto aTarget = pTargetRange ? rMaterializer.materialize(*pTargetRange, aCoordinate)
                                              : rMaterializer.materialize(rBaseRange, aCoordinate);
            if (!aTarget)
                return aTarget;

            if (eAggregateKind == CriteriaAggregateKind::Count2)
            {
                // Count every matched row with a non-empty field value.
                // Empty cells and empty-text cells do not contribute.
                if (aTarget.maValue.meKind == api::CellValueKind::Empty)
                    continue;
                if (aTarget.maValue.meKind == api::CellValueKind::Text
                    && aTarget.maValue.maString.empty())
                    continue;
                ++nCount;
                continue;
            }

            const auto oNumber = coerceCriteriaAggregateNumber(aTarget.maValue);
            if (!oNumber)
                continue;

            switch (eAggregateKind)
            {
                case CriteriaAggregateKind::Count:
                case CriteriaAggregateKind::Count2:
                    break;
                case CriteriaAggregateKind::CountNumeric:
                    ++nCount;
                    break;
                case CriteriaAggregateKind::Sum:
                case CriteriaAggregateKind::Average:
                    fSum = fp::approxAdd(fSum, *oNumber);
                    ++nCount;
                    break;
                case CriteriaAggregateKind::Max:
                    if (!bHasBest || *oNumber > fBest)
                    {
                        fBest = *oNumber;
                        bHasBest = true;
                    }
                    break;
                case CriteriaAggregateKind::Min:
                    if (!bHasBest || *oNumber < fBest)
                    {
                        fBest = *oNumber;
                        bHasBest = true;
                    }
                    break;
                case CriteriaAggregateKind::Product:
                    fProduct = fProduct * *oNumber;
                    ++nCount;
                    break;
            }
        }
    }

    switch (eAggregateKind)
    {
        case CriteriaAggregateKind::Count:
        case CriteriaAggregateKind::Count2:
        case CriteriaAggregateKind::CountNumeric:
            return api::ValueResult<api::CellValue>::success(
                api::CellValue::number(static_cast<double>(nCount)));
        case CriteriaAggregateKind::Sum:
            return api::ValueResult<api::CellValue>::success(api::CellValue::number(fSum));
        case CriteriaAggregateKind::Average:
            if (nCount == 0)
                return api::ValueResult<api::CellValue>::failure(api::Error::DivisionByZero);
            return api::ValueResult<api::CellValue>::success(
                api::CellValue::number(fSum / static_cast<double>(nCount)));
        case CriteriaAggregateKind::Max:
        case CriteriaAggregateKind::Min:
            return api::ValueResult<api::CellValue>::success(
                api::CellValue::number(bHasBest ? fBest : 0.0));
        case CriteriaAggregateKind::Product:
            // Empty product: legacy DBProduct returns 0 when no row matches.
            return api::ValueResult<api::CellValue>::success(
                api::CellValue::number(nCount == 0 ? 0.0 : fProduct));
    }

    return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::query

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
