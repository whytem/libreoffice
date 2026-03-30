/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/TextFunctionRuntime.hxx>

#include <algorithm>

#include <spreadsheetengine/runtime/TextRuntimeSupport.hxx>
#include <spreadsheetengine/runtime/TextScalar.hxx>

namespace spreadsheetengine::core::text
{
namespace
{

struct TextDelimiterMatch
{
    sal_Int32 mnCodePointIndex = 0;
    sal_Int32 mnCodePointLength = 0;
    std::size_t mnDelimiterOrder = 0;
};

[[nodiscard]] std::optional<TextDelimiterMatch> findNextTextDelimiterMatch(
    spreadsheetengine::api::StringView rText,
    const std::vector<spreadsheetengine::api::String>& rDelimiters, sal_Int32 nCodePointStart,
    bool bCaseInsensitive)
{
    std::optional<TextDelimiterMatch> oBestMatch;
    for (std::size_t nIndex = 0; nIndex < rDelimiters.size(); ++nIndex)
    {
        if (rDelimiters[nIndex].empty())
            continue;

        const auto oFound = findTextCodePointIndex(
            rDelimiters[nIndex], rText, nCodePointStart, bCaseInsensitive);
        if (!oFound)
            continue;

        const TextDelimiterMatch aCandidate {
            *oFound,
            countCodePoints(rDelimiters[nIndex]),
            nIndex,
        };
        if (!oBestMatch || aCandidate.mnCodePointIndex < oBestMatch->mnCodePointIndex
            || (aCandidate.mnCodePointIndex == oBestMatch->mnCodePointIndex
                && aCandidate.mnDelimiterOrder < oBestMatch->mnDelimiterOrder))
        {
            oBestMatch = aCandidate;
        }
    }

    return oBestMatch;
}

[[nodiscard]] std::vector<TextDelimiterMatch> collectTextDelimiterMatches(
    spreadsheetengine::api::StringView rText,
    const std::vector<spreadsheetengine::api::String>& rDelimiters, bool bCaseInsensitive)
{
    std::vector<TextDelimiterMatch> aMatches;
    sal_Int32 nSearchStart = 0;
    while (true)
    {
        const auto oMatch = findNextTextDelimiterMatch(
            rText, rDelimiters, nSearchStart, bCaseInsensitive);
        if (!oMatch)
            break;

        aMatches.push_back(*oMatch);
        nSearchStart = oMatch->mnCodePointIndex
                       + std::max<sal_Int32>(oMatch->mnCodePointLength, 1);
    }
    return aMatches;
}

[[nodiscard]] std::optional<spreadsheetengine::api::String> resolveDelimitedTextSlice(
    spreadsheetengine::api::StringView rText,
    const std::vector<spreadsheetengine::api::String>& rDelimiters, sal_Int32 nInstance,
    bool bCaseInsensitive, bool bMatchEnd, bool bReturnAfter)
{
    const auto aMatches = collectTextDelimiterMatches(rText, rDelimiters, bCaseInsensitive);
    const sal_Int32 nTextLength = countCodePoints(rText);

    if (bReturnAfter)
    {
        sal_Int32 nSliceStart = 0;
        if (nInstance > 0)
        {
            const std::size_t nRequested = static_cast<std::size_t>(nInstance);
            if (aMatches.size() >= nRequested)
            {
                const auto& rMatch = aMatches[nRequested - 1];
                nSliceStart = rMatch.mnCodePointIndex + rMatch.mnCodePointLength;
            }
            else if (bMatchEnd)
            {
                if (aMatches.empty())
                    nSliceStart = 0;
                else
                {
                    const auto& rMatch = aMatches.back();
                    nSliceStart = rMatch.mnCodePointIndex + rMatch.mnCodePointLength;
                }
            }
            else
                return std::nullopt;
        }
        else
        {
            const std::size_t nRequested = static_cast<std::size_t>(-nInstance);
            if (aMatches.size() >= nRequested)
            {
                const auto& rMatch = aMatches[aMatches.size() - nRequested];
                nSliceStart = rMatch.mnCodePointIndex + rMatch.mnCodePointLength;
            }
            else if (bMatchEnd)
            {
                if (aMatches.empty())
                    nSliceStart = 0;
                else
                {
                    const auto& rMatch = aMatches.front();
                    nSliceStart = rMatch.mnCodePointIndex + rMatch.mnCodePointLength;
                }
            }
            else
                return std::nullopt;
        }

        return sliceText(rText, nSliceStart, nTextLength - nSliceStart);
    }

    sal_Int32 nSliceLength = 0;
    if (nInstance > 0)
    {
        const std::size_t nRequested = static_cast<std::size_t>(nInstance);
        if (aMatches.size() >= nRequested)
            nSliceLength = aMatches[nRequested - 1].mnCodePointIndex;
        else if (bMatchEnd)
            nSliceLength = nTextLength;
        else
            return std::nullopt;
    }
    else
    {
        const std::size_t nRequested = static_cast<std::size_t>(-nInstance);
        if (aMatches.size() >= nRequested)
            nSliceLength = aMatches[aMatches.size() - nRequested].mnCodePointIndex;
        else if (bMatchEnd)
            nSliceLength = nTextLength;
        else
            return std::nullopt;
    }

    return sliceText(rText, 0, nSliceLength);
}

}

std::optional<sal_Int32> findText(spreadsheetengine::api::StringView rNeedle,
    spreadsheetengine::api::StringView rHaystack, sal_Int32 nStart, bool bCaseInsensitive)
{
    return findTextCodePointIndex(rNeedle, rHaystack, nStart, bCaseInsensitive);
}

std::optional<std::size_t> findByteText(spreadsheetengine::api::StringView rNeedle,
    spreadsheetengine::api::StringView rHaystack, std::size_t nStartIndex,
    bool bCaseInsensitive)
{
    return findDbcsExpandedText(rNeedle, rHaystack, nStartIndex, bCaseInsensitive);
}

spreadsheetengine::api::String sliceText(
    spreadsheetengine::api::StringView rText, sal_Int32 nCodePointStart,
    sal_Int32 nCodePointLength)
{
    return substringByCodePoints(rText, nCodePointStart, nCodePointLength);
}

spreadsheetengine::api::String sliceTextLeftRight(
    spreadsheetengine::api::StringView rText, sal_Int32 nLength, bool bFromRight)
{
    const sal_Int32 nCodePointCount = countCodePoints(rText);
    const sal_Int32 nSliceLength = std::min(nLength, nCodePointCount);
    const sal_Int32 nSliceStart
        = bFromRight ? std::max<sal_Int32>(0, nCodePointCount - nSliceLength) : 0;
    return sliceText(rText, nSliceStart, nSliceLength);
}

spreadsheetengine::api::String replaceText(spreadsheetengine::api::StringView rSource,
    sal_Int32 nCodePointStart, sal_Int32 nCodePointLength,
    spreadsheetengine::api::StringView rReplacement)
{
    return replaceByCodePoints(rSource, nCodePointStart, nCodePointLength, rReplacement);
}

spreadsheetengine::api::String replaceByteText(
    spreadsheetengine::api::StringView rSource, std::size_t nStartIndex,
    std::size_t nReplaceLength, spreadsheetengine::api::StringView rReplacement)
{
    spreadsheetengine::api::String aExpandedSource = expandDbcsByteText(rSource, false);
    const spreadsheetengine::api::String aExpandedReplacement
        = expandDbcsByteText(rReplacement, false);
    aExpandedSource.replace(nStartIndex, nReplaceLength, aExpandedReplacement);
    return collapseDbcsByteText(aExpandedSource);
}

spreadsheetengine::api::String substituteText(spreadsheetengine::api::StringView rSource,
    spreadsheetengine::api::StringView rOldText, spreadsheetengine::api::StringView rNewText,
    std::optional<sal_Int32> oInstance)
{
    if (rOldText.empty())
        return spreadsheetengine::api::String(rSource);

    spreadsheetengine::api::String aResult;
    std::size_t nSearchOffset = 0;
    sal_Int32 nMatchCount = 0;
    while (nSearchOffset <= rSource.size())
    {
        const std::size_t nFound = rSource.find(rOldText, nSearchOffset);
        if (nFound == spreadsheetengine::api::String::npos)
        {
            aResult.append(rSource.substr(nSearchOffset));
            break;
        }

        aResult.append(rSource.substr(nSearchOffset, nFound - nSearchOffset));
        ++nMatchCount;
        if (!oInstance || *oInstance == nMatchCount)
            aResult.append(rNewText);
        else
            aResult.append(rOldText);

        nSearchOffset = nFound + rOldText.size();
    }

    return aResult;
}

std::optional<spreadsheetengine::api::String> textAfter(
    spreadsheetengine::api::StringView rText,
    const std::vector<spreadsheetengine::api::String>& rDelimiters, sal_Int32 nInstance,
    bool bCaseInsensitive, bool bMatchEnd)
{
    return resolveDelimitedTextSlice(
        rText, rDelimiters, nInstance, bCaseInsensitive, bMatchEnd, true);
}

std::optional<spreadsheetengine::api::String> textBefore(
    spreadsheetengine::api::StringView rText,
    const std::vector<spreadsheetengine::api::String>& rDelimiters, sal_Int32 nInstance,
    bool bCaseInsensitive, bool bMatchEnd)
{
    return resolveDelimitedTextSlice(
        rText, rDelimiters, nInstance, bCaseInsensitive, bMatchEnd, false);
}

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
