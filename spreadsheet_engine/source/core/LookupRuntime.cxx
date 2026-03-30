/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/LookupRuntime.hxx>

#include <rtl/math.hxx>

#include <spreadsheetengine/runtime/QueryRuntime.hxx>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

#include <unicode/coll.h>
#include <unicode/unistr.h>

namespace spreadsheetengine::core::lookup
{
namespace
{

namespace sequery = spreadsheetengine::core::query;

[[nodiscard]] std::optional<double> parseAsciiDouble(api::StringView rValue)
{
    if (rValue.empty())
        return std::nullopt;

    std::string aAscii;
    aAscii.reserve(rValue.size());
    for (const char16_t cChar : rValue)
    {
        if (cChar > 0x7f)
            return std::nullopt;
        aAscii.push_back(static_cast<char>(cChar));
    }

    char* pEnd = nullptr;
    const double fValue = std::strtod(aAscii.c_str(), &pEnd);
    if (!pEnd || *pEnd != '\0')
        return std::nullopt;

    return fValue;
}

[[nodiscard]] api::ValueResult<double> coerceToNumber(const api::CellValue& rValue)
{
    switch (rValue.meKind)
    {
        case api::CellValueKind::Empty:
            return api::ValueResult<double>::success(0.0);
        case api::CellValueKind::Number:
        case api::CellValueKind::Boolean:
            return api::ValueResult<double>::success(rValue.mfNumber);
        case api::CellValueKind::Text:
        {
            if (const auto oValue = parseAsciiDouble(rValue.maString))
                return api::ValueResult<double>::success(*oValue);
            return api::ValueResult<double>::failure(api::Error::IllegalArgument);
        }
        case api::CellValueKind::Error:
            return api::ValueResult<double>::failure(rValue.meError);
    }

    return api::ValueResult<double>::failure(api::Error::IllegalArgument);
}

[[nodiscard]] api::ValueResult<int> compareLookupText(
    api::StringView rLeft, api::StringView rRight)
{
    static std::unique_ptr<icu::Collator> xCollator = [] {
        UErrorCode eStatus = U_ZERO_ERROR;
        std::unique_ptr<icu::Collator> xInstance(
            icu::Collator::createInstance(icu::Locale(), eStatus));
        if (!xInstance || U_FAILURE(eStatus))
            return std::unique_ptr<icu::Collator>();
        xInstance->setStrength(icu::Collator::SECONDARY);
        return xInstance;
    }();

    if (!xCollator)
        return api::ValueResult<int>::failure(api::Error::IllegalArgument);

    UErrorCode eStatus = U_ZERO_ERROR;
    const icu::UnicodeString aLeft(
        reinterpret_cast<const UChar*>(rLeft.data()), static_cast<int32_t>(rLeft.size()));
    const icu::UnicodeString aRight(
        reinterpret_cast<const UChar*>(rRight.data()), static_cast<int32_t>(rRight.size()));
    const UCollationResult eCompare = xCollator->compare(aLeft, aRight, eStatus);
    if (U_FAILURE(eStatus))
        return api::ValueResult<int>::failure(api::Error::IllegalArgument);

    if (eCompare == UCOL_EQUAL)
        return api::ValueResult<int>::success(0);
    return api::ValueResult<int>::success(eCompare == UCOL_LESS ? -1 : 1);
}

[[nodiscard]] api::ValueResult<api::CellValue> materializeCoordinate(
    const LookupMaterializer& rMaterializer, const LookupInput& rInput,
    api::MatrixCoordinate aCoordinate)
{
    if (rInput.mbScalar)
    {
        if (aCoordinate.mnColumn != 0 || aCoordinate.mnRow != 0)
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::CellValue>::success(rInput.maScalar);
    }

    return rMaterializer.materialize(rInput, aCoordinate);
}

[[nodiscard]] api::ValueResult<api::CellValue> loadTabularSearchCandidate(
    const LookupMaterializer& rMaterializer, const LookupInput& rInput,
    api::lookup::VectorOrientation eOrientation, api::MatrixSize nSearchIndex)
{
    const api::MatrixCoordinate aSearchCoordinate
        = eOrientation == api::lookup::VectorOrientation::Column
              ? api::MatrixCoordinate { 0, nSearchIndex }
              : api::MatrixCoordinate { nSearchIndex, 0 };
    return materializeCoordinate(rMaterializer, rInput, aSearchCoordinate);
}

[[nodiscard]] api::MatrixSize trimTrailingEmptyLookupLength(
    const LookupMaterializer& rMaterializer, const LookupInput& rInput,
    api::lookup::VectorOrientation eOrientation, api::MatrixSize nLength)
{
    while (nLength > 0)
    {
        const auto aTailCandidate
            = materializeLookupInputValue(rMaterializer, rInput, eOrientation, nLength - 1);
        if (!aTailCandidate || !aTailCandidate.maValue.isEmpty())
            break;
        --nLength;
    }

    return nLength;
}

[[nodiscard]] bool isExactLookupMatch(const api::CellValue& rLookup,
    const api::CellValue& rCandidate, api::query::SearchType eSearchType)
{
    if (rLookup.isText())
    {
        if (!rCandidate.isText())
            return false;

        return sequery::matchesWholeCellLookupText(
            rLookup.maString, rCandidate.maString, eSearchType);
    }

    if (rCandidate.isText() || rCandidate.isEmpty())
        return false;

    const auto aLookupNumber = coerceToNumber(rLookup);
    const auto aCandidateNumber = coerceToNumber(rCandidate);
    if (!aLookupNumber || !aCandidateNumber)
        return false;

    return rtl::math::approxEqual(aLookupNumber.maValue, aCandidateNumber.maValue);
}

[[nodiscard]] bool isPatternExactLookupMatch(const api::CellValue& rLookup,
    const api::CellValue& rCandidate, api::lookup::MatchMode eMatchMode,
    api::query::SearchType eSearchType)
{
    if (rLookup.isEmpty())
        return rCandidate.isEmpty();

    if (rLookup.isText())
    {
        if (eMatchMode == api::lookup::MatchMode::ExactOrNotAvailable)
        {
            if (!rCandidate.isText())
                return false;
            return sequery::matchesWholeCellLookupText(
                rLookup.maString, rCandidate.maString, eSearchType);
        }

        if (!(rCandidate.isText() || rCandidate.isEmpty()))
            return false;

        const api::StringView aCandidateText
            = rCandidate.isText() ? api::StringView(rCandidate.maString) : api::StringView();
        return sequery::matchesWholeCellLookupText(
            rLookup.maString, aCandidateText, eSearchType);
    }

    if (rCandidate.isText() || rCandidate.isEmpty())
        return false;

    const auto aLookupNumber = coerceToNumber(rLookup);
    const auto aCandidateNumber = coerceToNumber(rCandidate);
    if (!aLookupNumber || !aCandidateNumber)
        return false;

    return rtl::math::approxEqual(aLookupNumber.maValue, aCandidateNumber.maValue);
}

[[nodiscard]] std::optional<api::MatrixSize> findExtendedExactIndex(
    const LookupMaterializer& rMaterializer, const LookupInput& rSearchInput,
    api::lookup::VectorOrientation eOrientation, api::MatrixSize nSearchLength,
    const api::CellValue& rLookup, api::lookup::MatchMode eMatchMode,
    api::lookup::SearchMode eSearchMode, api::query::SearchType eSearchType)
{
    const bool bReverse = eSearchMode == api::lookup::SearchMode::Reverse
                          || eSearchMode == api::lookup::SearchMode::BinaryDescending;
    if (bReverse)
    {
        for (api::MatrixSize nSearchIndex = nSearchLength; nSearchIndex > 0; --nSearchIndex)
        {
            const auto aCandidate = materializeLookupInputValue(
                rMaterializer, rSearchInput, eOrientation, nSearchIndex - 1);
            if (!aCandidate)
                continue;
            if (isPatternExactLookupMatch(
                    rLookup, aCandidate.maValue, eMatchMode, eSearchType))
            {
                return nSearchIndex - 1;
            }
        }
        return std::nullopt;
    }

    for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
    {
        const auto aCandidate
            = materializeLookupInputValue(rMaterializer, rSearchInput, eOrientation, nSearchIndex);
        if (!aCandidate)
            continue;
        if (isPatternExactLookupMatch(rLookup, aCandidate.maValue, eMatchMode, eSearchType))
            return nSearchIndex;
    }

    return std::nullopt;
}

} // namespace

api::ValueResult<api::lookup::VectorLayout> detectLookupLayout(
    const LookupInput& rInput, bool bAllowMajorVector)
{
    if (rInput.mbScalar)
    {
        return api::ValueResult<api::lookup::VectorLayout>::success(
            { api::lookup::VectorOrientation::Column, 1 });
    }

    const api::MatrixDimensions aDimensions { rInput.mnColumns, rInput.mnRows };
    if (const auto aVectorLayout = api::lookup::detectVectorLayout(aDimensions))
        return aVectorLayout;

    if (!bAllowMajorVector)
        return api::ValueResult<api::lookup::VectorLayout>::failure(api::Error::IllegalArgument);

    return api::ValueResult<api::lookup::VectorLayout>::success(
        api::lookup::majorVectorLayout(aDimensions));
}

api::ValueResult<api::CellValue> materializeLookupInputValue(
    const LookupMaterializer& rMaterializer, const LookupInput& rInput,
    api::lookup::VectorOrientation eOrientation, api::MatrixSize nIndex)
{
    if (rInput.mbScalar)
    {
        if (nIndex != 0)
            return api::ValueResult<api::CellValue>::failure(api::Error::IllegalArgument);
        return api::ValueResult<api::CellValue>::success(rInput.maScalar);
    }

    const auto aCoordinate = api::lookup::planVectorElement(
        eOrientation, nIndex, { rInput.mnColumns, rInput.mnRows });
    if (!aCoordinate)
        return api::ValueResult<api::CellValue>::failure(aCoordinate.meError);

    return materializeCoordinate(rMaterializer, rInput, aCoordinate.maValue);
}

api::ValueResult<api::MatrixSize> resolveTabularLookupIndex(
    const LookupMaterializer& rMaterializer, const api::CellValue& rLookup,
    const LookupInput& rTableInput, api::lookup::VectorOrientation eSearchOrientation,
    bool bApproximate, api::query::SearchType eSearchType)
{
    if (rLookup.isError())
        return api::ValueResult<api::MatrixSize>::failure(rLookup.meError);

    const api::MatrixSize nSearchLength = eSearchOrientation == api::lookup::VectorOrientation::Column
                                              ? rTableInput.mnRows
                                              : rTableInput.mnColumns;
    if (nSearchLength <= 0)
        return api::ValueResult<api::MatrixSize>::failure(api::Error::IllegalArgument);

    std::optional<api::MatrixSize> oResolvedIndex;
    if (bApproximate)
    {
        if (rLookup.isText())
        {
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                const auto aCandidate = loadTabularSearchCandidate(
                    rMaterializer, rTableInput, eSearchOrientation, nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue;
                if (rCandidate.isText() || rCandidate.isEmpty())
                {
                    const api::StringView aCandidateText
                        = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                              : api::StringView();
                    const auto aTextCompare = compareLookupText(aCandidateText, rLookup.maString);
                    if (!aTextCompare)
                        continue;
                    if (aTextCompare.maValue <= 0)
                    {
                        oResolvedIndex = nSearchIndex;
                    }
                    else if (nSearchIndex > 0)
                    {
                        break;
                    }
                }
                else
                {
                    oResolvedIndex = nSearchIndex;
                }
            }
        }
        else
        {
            const auto aLookupNumber = coerceToNumber(rLookup);
            if (!aLookupNumber)
                return api::ValueResult<api::MatrixSize>::failure(aLookupNumber.meError);

            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                const auto aCandidate = loadTabularSearchCandidate(
                    rMaterializer, rTableInput, eSearchOrientation, nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue;
                if (rCandidate.isText() || rCandidate.isEmpty())
                    continue;

                const auto aCandidateNumber = coerceToNumber(rCandidate);
                if (!aCandidateNumber)
                    continue;

                if (aCandidateNumber.maValue < aLookupNumber.maValue
                    || rtl::math::approxEqual(
                        aCandidateNumber.maValue, aLookupNumber.maValue))
                {
                    oResolvedIndex = nSearchIndex;
                }
                else
                {
                    break;
                }
            }
        }
    }
    else
    {
        for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
        {
            const auto aCandidate = loadTabularSearchCandidate(
                rMaterializer, rTableInput, eSearchOrientation, nSearchIndex);
            if (!aCandidate)
                continue;
            if (isExactLookupMatch(rLookup, aCandidate.maValue, eSearchType))
            {
                oResolvedIndex = nSearchIndex;
                break;
            }
        }
    }

    if (!oResolvedIndex)
        return api::ValueResult<api::MatrixSize>::failure(api::Error::NotAvailable);
    return api::ValueResult<api::MatrixSize>::success(*oResolvedIndex);
}

api::ValueResult<api::MatrixSize> resolveLookupIndex(const LookupMaterializer& rMaterializer,
    const api::CellValue& rLookup, const LookupInput& rSearchInput,
    api::query::SearchType eSearchType)
{
    if (rLookup.isError())
        return api::ValueResult<api::MatrixSize>::failure(rLookup.meError);
    if (rLookup.isEmpty())
        return api::ValueResult<api::MatrixSize>::failure(api::Error::NotAvailable);

    const auto aSearchLayout = detectLookupLayout(rSearchInput, true);
    if (!aSearchLayout)
        return api::ValueResult<api::MatrixSize>::failure(aSearchLayout.meError);

    auto compareExactTypeMatch = [&](const api::CellValue& rCandidate) {
        if (rLookup.isText())
        {
            if (!rCandidate.isText())
                return false;
            return sequery::matchesWholeCellLookupText(
                rLookup.maString, rCandidate.maString, eSearchType);
        }

        if (rCandidate.isText() || rCandidate.isEmpty())
            return false;

        const auto aCandidateNumber = coerceToNumber(rCandidate);
        if (!aCandidateNumber)
            return false;
        const auto aLookupNumber = coerceToNumber(rLookup);
        if (!aLookupNumber)
            return false;

        return rtl::math::approxEqual(aCandidateNumber.maValue, aLookupNumber.maValue);
    };

    if (rSearchInput.mbScalar)
    {
        if (compareExactTypeMatch(rSearchInput.maScalar))
            return api::ValueResult<api::MatrixSize>::success(0);

        if (rLookup.isText())
            return api::ValueResult<api::MatrixSize>::failure(api::Error::NotAvailable);

        const auto aLookupNumber = coerceToNumber(rLookup);
        const auto aDataNumber = coerceToNumber(rSearchInput.maScalar);
        if (!aLookupNumber || !aDataNumber || aDataNumber.maValue > aLookupNumber.maValue)
            return api::ValueResult<api::MatrixSize>::failure(api::Error::NotAvailable);
        return api::ValueResult<api::MatrixSize>::success(0);
    }

    std::optional<api::MatrixSize> oResolvedIndex;
    if (rLookup.isText())
    {
        bool bSeenExactTextMatch = false;
        for (api::MatrixSize nSearchIndex = 0; nSearchIndex < aSearchLayout.maValue.mnLength;
             ++nSearchIndex)
        {
            const auto aCandidate = materializeLookupInputValue(
                rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation, nSearchIndex);
            if (!aCandidate)
                continue;

            const api::CellValue& rCandidate = aCandidate.maValue;
            if (rCandidate.isText() || rCandidate.isEmpty())
            {
                const api::StringView aCandidateText
                    = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                          : api::StringView();
                const sal_Int32 nCompare
                    = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                if (nCompare == 0)
                {
                    oResolvedIndex = nSearchIndex;
                    bSeenExactTextMatch = true;
                    continue;
                }
                if (bSeenExactTextMatch)
                    break;
                if (nCompare < 0)
                {
                    oResolvedIndex = nSearchIndex;
                }
                else if (nSearchIndex > 0)
                {
                    break;
                }
            }
            else
            {
                oResolvedIndex = nSearchIndex;
            }
        }
    }
    else
    {
        const auto aLookupNumber = coerceToNumber(rLookup);
        if (!aLookupNumber)
            return api::ValueResult<api::MatrixSize>::failure(aLookupNumber.meError);

        bool bSeenExactNumericMatch = false;
        for (api::MatrixSize nSearchIndex = 0; nSearchIndex < aSearchLayout.maValue.mnLength;
             ++nSearchIndex)
        {
            const auto aCandidate = materializeLookupInputValue(
                rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation, nSearchIndex);
            if (!aCandidate)
                continue;

            const api::CellValue& rCandidate = aCandidate.maValue;
            if (rCandidate.isText() || rCandidate.isEmpty())
                continue;

            const auto aCandidateNumber = coerceToNumber(rCandidate);
            if (!aCandidateNumber)
                continue;

            if (rtl::math::approxEqual(aCandidateNumber.maValue, aLookupNumber.maValue))
            {
                oResolvedIndex = nSearchIndex;
                bSeenExactNumericMatch = true;
            }
            else if (bSeenExactNumericMatch)
            {
                break;
            }
            else if (aCandidateNumber.maValue < aLookupNumber.maValue)
            {
                oResolvedIndex = nSearchIndex;
            }
            else
            {
                break;
            }
        }
    }

    if (!oResolvedIndex)
        return api::ValueResult<api::MatrixSize>::failure(api::Error::NotAvailable);
    return api::ValueResult<api::MatrixSize>::success(*oResolvedIndex);
}

api::ValueResult<api::MatrixSize> resolveMatchIndex(const LookupMaterializer& rMaterializer,
    const api::CellValue& rLookup, const LookupInput& rSearchInput,
    const api::lookup::MatchSearchMode& rModes, api::query::SearchType eSearchType)
{
    if (rLookup.isError())
        return api::ValueResult<api::MatrixSize>::failure(rLookup.meError);

    const auto aSearchLayout = detectLookupLayout(rSearchInput, true);
    if (!aSearchLayout)
        return api::ValueResult<api::MatrixSize>::failure(aSearchLayout.meError);

    api::MatrixSize nSearchLength = trimTrailingEmptyLookupLength(
        rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
        aSearchLayout.maValue.mnLength);

    auto isExactMatch = [&](const api::CellValue& rCandidate) {
        return isExactLookupMatch(rLookup, rCandidate, eSearchType);
    };

    std::optional<api::MatrixSize> oResolvedIndex;
    if (rModes.meMatchMode == api::lookup::MatchMode::ExactOrNotAvailable)
    {
        for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
        {
            const auto aCandidate = materializeLookupInputValue(
                rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation, nSearchIndex);
            if (!aCandidate)
                continue;
            if (isExactMatch(aCandidate.maValue))
            {
                oResolvedIndex = nSearchIndex;
                break;
            }
        }
    }
    else if (rModes.meMatchMode == api::lookup::MatchMode::ExactOrNextSmaller)
    {
        if (rLookup.isText())
        {
            bool bSeenExactTextMatch = false;
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                const auto aCandidate = materializeLookupInputValue(
                    rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                    nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue;
                if (rCandidate.isText() || rCandidate.isEmpty())
                {
                    const api::StringView aCandidateText
                        = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                              : api::StringView();
                    const sal_Int32 nCompare
                        = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                    if (nCompare == 0)
                    {
                        oResolvedIndex = nSearchIndex;
                        bSeenExactTextMatch = true;
                        continue;
                    }
                    if (bSeenExactTextMatch)
                        break;
                    if (nCompare < 0)
                        oResolvedIndex = nSearchIndex;
                    else if (nSearchIndex > 0)
                        break;
                }
            }
        }
        else
        {
            const auto aLookupNumber = coerceToNumber(rLookup);
            if (!aLookupNumber)
                return api::ValueResult<api::MatrixSize>::failure(aLookupNumber.meError);

            bool bSeenExactNumericMatch = false;
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                const auto aCandidate = materializeLookupInputValue(
                    rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                    nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue;
                if (rCandidate.isText() || rCandidate.isEmpty())
                    continue;

                const auto aCandidateNumber = coerceToNumber(rCandidate);
                if (!aCandidateNumber)
                    continue;

                if (rtl::math::approxEqual(aCandidateNumber.maValue, aLookupNumber.maValue))
                {
                    oResolvedIndex = nSearchIndex;
                    bSeenExactNumericMatch = true;
                }
                else if (bSeenExactNumericMatch)
                {
                    break;
                }
                else if (aCandidateNumber.maValue < aLookupNumber.maValue)
                {
                    oResolvedIndex = nSearchIndex;
                }
                else
                {
                    break;
                }
            }
        }
    }
    else if (rModes.meMatchMode == api::lookup::MatchMode::ExactOrNextLarger)
    {
        if (rLookup.isText())
        {
            bool bSeenExactTextMatch = false;
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                const auto aCandidate = materializeLookupInputValue(
                    rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                    nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue;
                if (rCandidate.isText() || rCandidate.isEmpty())
                {
                    const api::StringView aCandidateText
                        = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                              : api::StringView();
                    const sal_Int32 nCompare
                        = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                    if (nCompare == 0)
                    {
                        oResolvedIndex = nSearchIndex;
                        bSeenExactTextMatch = true;
                        continue;
                    }
                    if (bSeenExactTextMatch)
                        break;
                    if (nCompare > 0)
                        oResolvedIndex = nSearchIndex;
                    else
                        break;
                }
            }
        }
        else
        {
            const auto aLookupNumber = coerceToNumber(rLookup);
            if (!aLookupNumber)
                return api::ValueResult<api::MatrixSize>::failure(aLookupNumber.meError);

            bool bSeenExactNumericMatch = false;
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                const auto aCandidate = materializeLookupInputValue(
                    rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                    nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue;
                if (rCandidate.isText() || rCandidate.isEmpty())
                    continue;

                const auto aCandidateNumber = coerceToNumber(rCandidate);
                if (!aCandidateNumber)
                    continue;

                if (rtl::math::approxEqual(aCandidateNumber.maValue, aLookupNumber.maValue))
                {
                    oResolvedIndex = nSearchIndex;
                    bSeenExactNumericMatch = true;
                }
                else if (bSeenExactNumericMatch)
                {
                    break;
                }
                else if (aCandidateNumber.maValue > aLookupNumber.maValue)
                {
                    oResolvedIndex = nSearchIndex;
                }
                else
                {
                    break;
                }
            }
        }
    }

    if (!oResolvedIndex)
        return api::ValueResult<api::MatrixSize>::failure(api::Error::NotAvailable);
    return api::ValueResult<api::MatrixSize>::success(*oResolvedIndex);
}

api::ValueResult<api::MatrixSize> resolveExtendedMatchIndex(
    const LookupMaterializer& rMaterializer, const api::CellValue& rLookup,
    const LookupInput& rSearchInput, api::lookup::MatchMode eMatchMode,
    api::lookup::SearchMode eSearchMode, api::query::SearchType eSearchType,
    bool bAllowPatternMatch)
{
    const auto aSearchLayout = detectLookupLayout(rSearchInput, false);
    if (!aSearchLayout)
        return api::ValueResult<api::MatrixSize>::failure(aSearchLayout.meError);

    if (!bAllowPatternMatch && (eMatchMode == api::lookup::MatchMode::Wildcard
                                || eMatchMode == api::lookup::MatchMode::Regex))
    {
        return api::ValueResult<api::MatrixSize>::failure(api::Error::NoValue);
    }

    if ((eMatchMode == api::lookup::MatchMode::Wildcard
         || eMatchMode == api::lookup::MatchMode::Regex)
        && api::lookup::isBinarySearchMode(eSearchMode))
    {
        return api::ValueResult<api::MatrixSize>::failure(api::Error::NoValue);
    }

    api::MatrixSize nSearchLength = trimTrailingEmptyLookupLength(
        rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
        aSearchLayout.maValue.mnLength);

    std::optional<api::MatrixSize> oResolvedIndex;
    if (eMatchMode == api::lookup::MatchMode::ExactOrNotAvailable
        || eMatchMode == api::lookup::MatchMode::Wildcard
        || eMatchMode == api::lookup::MatchMode::Regex)
    {
        oResolvedIndex = findExtendedExactIndex(rMaterializer, rSearchInput,
            aSearchLayout.maValue.meOrientation, nSearchLength, rLookup, eMatchMode, eSearchMode,
            eSearchType);
    }
    else if (eMatchMode == api::lookup::MatchMode::ExactOrNextSmaller)
    {
        oResolvedIndex = findExtendedExactIndex(rMaterializer, rSearchInput,
            aSearchLayout.maValue.meOrientation, nSearchLength, rLookup,
            api::lookup::MatchMode::ExactOrNotAvailable, eSearchMode, eSearchType);
        if (oResolvedIndex)
        {
            // exact hit wins even if the lookup vector is not sorted
        }
        else if (eSearchMode == api::lookup::SearchMode::Forward
                 || eSearchMode == api::lookup::SearchMode::Reverse)
        {
            const bool bReverse = eSearchMode == api::lookup::SearchMode::Reverse;
            if (rLookup.isText())
            {
                api::String aBestText;
                bool bHaveBestText = false;
                for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                {
                    const api::MatrixSize nSearchIndex
                        = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                    const auto aCandidate = materializeLookupInputValue(
                        rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                        nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue;
                    if (!(rCandidate.isText() || rCandidate.isEmpty()))
                        continue;

                    const api::StringView aCandidateText
                        = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                              : api::StringView();
                    if (sequery::compareFoldedText(aCandidateText, rLookup.maString) >= 0)
                        continue;

                    if (!bHaveBestText
                        || sequery::compareFoldedText(aCandidateText, aBestText) > 0)
                    {
                        aBestText = api::String(aCandidateText);
                        bHaveBestText = true;
                        oResolvedIndex = nSearchIndex;
                    }
                }
            }
            else
            {
                const auto aLookupNumber = coerceToNumber(rLookup);
                if (!aLookupNumber)
                    return api::ValueResult<api::MatrixSize>::failure(aLookupNumber.meError);

                std::optional<double> ofBestNumber;
                for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                {
                    const api::MatrixSize nSearchIndex
                        = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                    const auto aCandidate = materializeLookupInputValue(
                        rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                        nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                        continue;

                    const auto aCandidateNumber = coerceToNumber(rCandidate);
                    if (!aCandidateNumber
                        || !(aCandidateNumber.maValue < aLookupNumber.maValue))
                    {
                        continue;
                    }

                    if (!ofBestNumber || aCandidateNumber.maValue > *ofBestNumber)
                    {
                        ofBestNumber = aCandidateNumber.maValue;
                        oResolvedIndex = nSearchIndex;
                    }
                }
            }
        }
        else if (rLookup.isText())
        {
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                const auto aCandidate = materializeLookupInputValue(
                    rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                    nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue;
                if (rCandidate.isText() || rCandidate.isEmpty())
                {
                    const api::StringView aCandidateText
                        = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                              : api::StringView();
                    const sal_Int32 nCompare
                        = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                    if (nCompare < 0)
                        oResolvedIndex = nSearchIndex;
                    else if (nSearchIndex > 0)
                        break;
                }
            }
        }
        else
        {
            const auto aLookupNumber = coerceToNumber(rLookup);
            if (!aLookupNumber)
                return api::ValueResult<api::MatrixSize>::failure(aLookupNumber.meError);

            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                const auto aCandidate = materializeLookupInputValue(
                    rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                    nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue;
                if (rCandidate.isText() || rCandidate.isEmpty())
                    continue;

                const auto aCandidateNumber = coerceToNumber(rCandidate);
                if (!aCandidateNumber)
                    continue;

                if (aCandidateNumber.maValue < aLookupNumber.maValue)
                {
                    oResolvedIndex = nSearchIndex;
                }
                else
                {
                    break;
                }
            }
        }
    }
    else if (eMatchMode == api::lookup::MatchMode::ExactOrNextLarger)
    {
        oResolvedIndex = findExtendedExactIndex(rMaterializer, rSearchInput,
            aSearchLayout.maValue.meOrientation, nSearchLength, rLookup,
            api::lookup::MatchMode::ExactOrNotAvailable, eSearchMode, eSearchType);
        if (oResolvedIndex)
        {
            // exact hit wins even if the lookup vector is not sorted
        }
        else if (eSearchMode == api::lookup::SearchMode::Forward
                 || eSearchMode == api::lookup::SearchMode::Reverse)
        {
            const bool bReverse = eSearchMode == api::lookup::SearchMode::Reverse;
            if (rLookup.isText())
            {
                api::String aBestText;
                bool bHaveBestText = false;
                for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                {
                    const api::MatrixSize nSearchIndex
                        = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                    const auto aCandidate = materializeLookupInputValue(
                        rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                        nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue;
                    if (!(rCandidate.isText() || rCandidate.isEmpty()))
                        continue;

                    const api::StringView aCandidateText
                        = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                              : api::StringView();
                    if (sequery::compareFoldedText(aCandidateText, rLookup.maString) <= 0)
                        continue;

                    if (!bHaveBestText
                        || sequery::compareFoldedText(aCandidateText, aBestText) < 0)
                    {
                        aBestText = api::String(aCandidateText);
                        bHaveBestText = true;
                        oResolvedIndex = nSearchIndex;
                    }
                }
            }
            else
            {
                const auto aLookupNumber = coerceToNumber(rLookup);
                if (!aLookupNumber)
                    return api::ValueResult<api::MatrixSize>::failure(aLookupNumber.meError);

                std::optional<double> ofBestNumber;
                for (api::MatrixSize nOffset = 0; nOffset < nSearchLength; ++nOffset)
                {
                    const api::MatrixSize nSearchIndex
                        = bReverse ? (nSearchLength - 1 - nOffset) : nOffset;
                    const auto aCandidate = materializeLookupInputValue(
                        rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                        nSearchIndex);
                    if (!aCandidate)
                        continue;

                    const api::CellValue& rCandidate = aCandidate.maValue;
                    if (rCandidate.isText() || rCandidate.isEmpty())
                        continue;

                    const auto aCandidateNumber = coerceToNumber(rCandidate);
                    if (!aCandidateNumber
                        || !(aCandidateNumber.maValue > aLookupNumber.maValue))
                    {
                        continue;
                    }

                    if (!ofBestNumber || aCandidateNumber.maValue < *ofBestNumber)
                    {
                        ofBestNumber = aCandidateNumber.maValue;
                        oResolvedIndex = nSearchIndex;
                    }
                }
            }
        }
        else if (rLookup.isText())
        {
            const bool bDescending = eSearchMode == api::lookup::SearchMode::BinaryDescending;
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                const auto aCandidate = materializeLookupInputValue(
                    rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                    nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue;
                if (!(rCandidate.isText() || rCandidate.isEmpty()))
                    continue;

                const api::StringView aCandidateText
                    = rCandidate.isText() ? api::StringView(rCandidate.maString)
                                          : api::StringView();
                const sal_Int32 nCompare
                    = sequery::compareFoldedText(aCandidateText, rLookup.maString);
                if ((!bDescending && nCompare > 0) || (bDescending && nCompare < 0))
                {
                    oResolvedIndex = nSearchIndex;
                    if (!bDescending)
                        break;
                }
                else if (bDescending)
                {
                    break;
                }
            }
        }
        else
        {
            const auto aLookupNumber = coerceToNumber(rLookup);
            if (!aLookupNumber)
                return api::ValueResult<api::MatrixSize>::failure(aLookupNumber.meError);

            const bool bDescending = eSearchMode == api::lookup::SearchMode::BinaryDescending;
            for (api::MatrixSize nSearchIndex = 0; nSearchIndex < nSearchLength; ++nSearchIndex)
            {
                const auto aCandidate = materializeLookupInputValue(
                    rMaterializer, rSearchInput, aSearchLayout.maValue.meOrientation,
                    nSearchIndex);
                if (!aCandidate)
                    continue;

                const api::CellValue& rCandidate = aCandidate.maValue;
                if (rCandidate.isText() || rCandidate.isEmpty())
                    continue;

                const auto aCandidateNumber = coerceToNumber(rCandidate);
                if (!aCandidateNumber)
                    continue;

                if ((!bDescending && aCandidateNumber.maValue > aLookupNumber.maValue)
                    || (bDescending && aCandidateNumber.maValue < aLookupNumber.maValue))
                {
                    oResolvedIndex = nSearchIndex;
                    if (!bDescending)
                        break;
                }
                else if (bDescending)
                {
                    break;
                }
            }
        }
    }

    if (!oResolvedIndex)
        return api::ValueResult<api::MatrixSize>::failure(api::Error::NotAvailable);
    return api::ValueResult<api::MatrixSize>::success(*oResolvedIndex);
}

} // namespace spreadsheetengine::core::lookup

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
