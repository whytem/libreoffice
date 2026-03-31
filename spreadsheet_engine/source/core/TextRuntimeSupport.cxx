/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/runtime/TextRuntimeSupport.hxx>
#include <cstdint>

#include <algorithm>
#include <memory>

#include <unicode/uchar.h>
#include <unicode/translit.h>
#include <unicode/unistr.h>

namespace spreadsheetengine::core::text
{
namespace
{

[[nodiscard]] spreadsheetengine::api::String fromUnicodeString(const icu::UnicodeString& rText)
{
    spreadsheetengine::api::String aResult;
    aResult.reserve(static_cast<std::size_t>(rText.length()));
    for (int32_t nIndex = 0; nIndex < rText.length(); ++nIndex)
        aResult.push_back(static_cast<char16_t>(rText[nIndex]));
    return aResult;
}

class DefaultCaseMappingService final : public CaseMappingService
{
public:
    [[nodiscard]] spreadsheetengine::api::String uppercase(
        spreadsheetengine::api::StringView rInput) const override
    {
        icu::UnicodeString aText(
            reinterpret_cast<const UChar*>(rInput.data()), static_cast<int32_t>(rInput.size()));
        aText.toUpper();
        return fromUnicodeString(aText);
    }

    [[nodiscard]] spreadsheetengine::api::String lowercase(
        spreadsheetengine::api::StringView rInput) const override
    {
        icu::UnicodeString aText(
            reinterpret_cast<const UChar*>(rInput.data()), static_cast<int32_t>(rInput.size()));
        aText.toLower();
        return fromUnicodeString(aText);
    }

    [[nodiscard]] bool isLetter(char32_t nCodePoint) const override
    {
        return u_isalpha(static_cast<UChar32>(nCodePoint));
    }
};

class DefaultSingleByteEncodingService final : public SingleByteEncodingService
{
public:
    [[nodiscard]] std::int32_t encodeFirstCharacter(
        spreadsheetengine::api::StringView rInput) const override
    {
        if (rInput.empty())
            return 0;
        return static_cast<unsigned char>(rInput.front() & 0x00FF);
    }

    [[nodiscard]] std::optional<spreadsheetengine::api::String> decodeSingleByte(
        unsigned char nValue) const override
    {
        return spreadsheetengine::api::String(1, static_cast<char16_t>(nValue));
    }
};

class DefaultWidthConversionService final : public WidthConversionService
{
    [[nodiscard]] static spreadsheetengine::api::String transliterate(
        spreadsheetengine::api::StringView rInput, spreadsheetengine::api::StringView rId)
    {
        static std::unique_ptr<icu::Transliterator> xFullToHalf = [] {
            UErrorCode eCreateStatus = U_ZERO_ERROR;
            return std::unique_ptr<icu::Transliterator>(icu::Transliterator::createInstance(
                icu::UnicodeString::fromUTF8("Fullwidth-Halfwidth"), UTRANS_FORWARD,
                eCreateStatus));
        }();
        static std::unique_ptr<icu::Transliterator> xHalfToFull = [] {
            UErrorCode eCreateStatus = U_ZERO_ERROR;
            return std::unique_ptr<icu::Transliterator>(icu::Transliterator::createInstance(
                icu::UnicodeString::fromUTF8("Halfwidth-Fullwidth"), UTRANS_FORWARD,
                eCreateStatus));
        }();

        icu::Transliterator* pTransliterator = nullptr;
        if (rId == u"Fullwidth-Halfwidth")
            pTransliterator = xFullToHalf.get();
        else if (rId == u"Halfwidth-Fullwidth")
            pTransliterator = xHalfToFull.get();

        if (!pTransliterator)
            return spreadsheetengine::api::String(rInput);

        spreadsheetengine::api::String aNormalizedInput;
        aNormalizedInput.reserve(rInput.size());
        if (rId == u"Fullwidth-Halfwidth")
        {
            for (const char16_t cChar : rInput)
            {
                if (cChar == u'\u3000')
                    aNormalizedInput.push_back(u' ');
                else if (cChar == u'\u2018')
                    aNormalizedInput.push_back(u'`');
                else if (cChar == u'\u2019')
                    aNormalizedInput.push_back(u'\'');
                else if (cChar == u'\u201D')
                    aNormalizedInput.push_back(u'"');
                else if (cChar == u'\u3002')
                    aNormalizedInput.push_back(u'\uFF61');
                else if (cChar == u'\u300C')
                    aNormalizedInput.push_back(u'\uFF62');
                else if (cChar == u'\u300D')
                    aNormalizedInput.push_back(u'\uFF63');
                else if (cChar == u'\u3001')
                    aNormalizedInput.push_back(u'\uFF64');
                else if (cChar == u'\u30FB')
                    aNormalizedInput.push_back(u'\uFF65');
                else if (cChar == u'\u309B')
                    aNormalizedInput.push_back(u'\uFF9E');
                else if (cChar == u'\u309C')
                    aNormalizedInput.push_back(u'\uFF9F');
                else if (cChar == u'\u30FC' || cChar == u'\u2015')
                    aNormalizedInput.push_back(u'\uFF70');
                else if (cChar == u'\uFFE5')
                    aNormalizedInput.push_back(u'\\');
                else if (cChar >= u'\uFF01' && cChar <= u'\uFF5E')
                    aNormalizedInput.push_back(static_cast<char16_t>(cChar - 0xFEE0));
                else
                    aNormalizedInput.push_back(cChar);
            }
        }
        else
        {
            aNormalizedInput.assign(rInput);
        }

        if (rId == u"Fullwidth-Halfwidth")
        {
            bool bNeedsKanaTransliteration = false;
            for (const char16_t cChar : aNormalizedInput)
            {
                if ((cChar >= u'\u30A0' && cChar <= u'\u30FF')
                    || (cChar >= u'\uFF61' && cChar <= u'\uFF9F'))
                {
                    bNeedsKanaTransliteration = true;
                    break;
                }
            }
            if (!bNeedsKanaTransliteration)
                return aNormalizedInput;
        }

        icu::UnicodeString aText(reinterpret_cast<const UChar*>(aNormalizedInput.data()),
            static_cast<int32_t>(aNormalizedInput.size()));
        pTransliterator->transliterate(aText);

        spreadsheetengine::api::String aResult = fromUnicodeString(aText);
        if (rId == u"Halfwidth-Fullwidth")
        {
            spreadsheetengine::api::String aNormalizedResult;
            aNormalizedResult.reserve(aResult.size() * 2);
            for (const char16_t cOriginalChar : aResult)
            {
                char16_t cChar = cOriginalChar;
                if (cChar == u'\uFF02')
                    cChar = u'\u201D';
                else if (cChar == u'\uFF07')
                    cChar = u'\u2019';
                else if (cChar == u'\uFF40')
                    cChar = u'\u2018';
                else if (cChar == u'\uFF3C')
                    cChar = u'\uFFE5';
                if (cChar == u'\u30F4')
                {
                    aNormalizedResult.push_back(u'\u30A6');
                    aNormalizedResult.push_back(u'\u309B');
                    continue;
                }
                if (cChar == u'\u3099')
                    cChar = u'\u309B';
                else if (cChar == u'\u309A')
                    cChar = u'\u309C';
                aNormalizedResult.push_back(cChar);
            }
            aResult = std::move(aNormalizedResult);
        }

        return aResult;
    }

public:
    [[nodiscard]] spreadsheetengine::api::String toHalfWidth(
        spreadsheetengine::api::StringView rInput) const override
    {
        return transliterate(rInput, u"Fullwidth-Halfwidth");
    }

    [[nodiscard]] spreadsheetengine::api::String toFullWidth(
        spreadsheetengine::api::StringView rInput) const override
    {
        return transliterate(rInput, u"Halfwidth-Fullwidth");
    }
};

[[nodiscard]] icu::UnicodeString toUnicodeString(spreadsheetengine::api::StringView rText)
{
    return icu::UnicodeString(
        reinterpret_cast<const UChar*>(rText.data()), static_cast<int32_t>(rText.size()));
}

[[nodiscard]] std::int32_t clampCodePointIndex(const icu::UnicodeString& rText, std::int32_t nIndex)
{
    if (nIndex <= 0)
        return 0;

    std::int32_t nOffset = 0;
    std::int32_t nRemaining = nIndex;
    while (nRemaining > 0 && nOffset < rText.length())
    {
        nOffset = rText.moveIndex32(nOffset, 1);
        --nRemaining;
    }
    return nOffset;
}

} // namespace

const CaseMappingService& defaultCaseMappingService()
{
    static const DefaultCaseMappingService aService;
    return aService;
}

const SingleByteEncodingService& defaultSingleByteEncodingService()
{
    static const DefaultSingleByteEncodingService aService;
    return aService;
}

const WidthConversionService& defaultWidthConversionService()
{
    static const DefaultWidthConversionService aService;
    return aService;
}

spreadsheetengine::api::String substringByCodePoints(
    spreadsheetengine::api::StringView rText, std::int32_t nCodePointStart,
    std::int32_t nCodePointLength)
{
    icu::UnicodeString aText = toUnicodeString(rText);
    const std::int32_t nStartOffset = clampCodePointIndex(aText, nCodePointStart);
    const std::int32_t nEndOffset = clampCodePointIndex(
        aText, nCodePointStart + std::max<std::int32_t>(0, nCodePointLength));
    return fromUnicodeString(aText.tempSubStringBetween(nStartOffset, nEndOffset));
}

spreadsheetengine::api::String replaceByCodePoints(spreadsheetengine::api::StringView rText,
    std::int32_t nCodePointStart, std::int32_t nCodePointLength,
    spreadsheetengine::api::StringView rReplacement)
{
    icu::UnicodeString aText = toUnicodeString(rText);
    const std::int32_t nStartOffset = clampCodePointIndex(aText, nCodePointStart);
    const std::int32_t nEndOffset = clampCodePointIndex(
        aText, nCodePointStart + std::max<std::int32_t>(0, nCodePointLength));
    const icu::UnicodeString aReplacement(
        reinterpret_cast<const UChar*>(rReplacement.data()),
        static_cast<int32_t>(rReplacement.size()));
    aText.replaceBetween(nStartOffset, nEndOffset, aReplacement);
    return fromUnicodeString(aText);
}

std::optional<std::int32_t> findTextCodePointIndex(spreadsheetengine::api::StringView rNeedle,
    spreadsheetengine::api::StringView rHaystack, std::int32_t nCodePointStart,
    bool bCaseInsensitive)
{
    icu::UnicodeString aNeedle = toUnicodeString(rNeedle);
    icu::UnicodeString aHaystack = toUnicodeString(rHaystack);
    if (bCaseInsensitive)
    {
        aNeedle.foldCase();
        aHaystack.foldCase();
    }

    const std::int32_t nStartOffset = clampCodePointIndex(aHaystack, std::max<std::int32_t>(0, nCodePointStart));
    const std::int32_t nFoundOffset = aHaystack.indexOf(aNeedle, nStartOffset);
    if (nFoundOffset < 0)
        return std::nullopt;

    return aHaystack.countChar32(0, nFoundOffset);
}

spreadsheetengine::api::String expandDbcsByteText(
    spreadsheetengine::api::StringView rText, bool bFoldAscii)
{
    spreadsheetengine::api::String aExpanded;
    for (const char16_t cChar : rText)
    {
        char16_t cSlot = cChar;
        if (bFoldAscii && cSlot >= u'a' && cSlot <= u'z')
            cSlot = static_cast<char16_t>(cSlot - (u'a' - u'A'));

        aExpanded.push_back(cSlot);
        const int nEastAsianWidth = u_getIntPropertyValue(cChar, UCHAR_EAST_ASIAN_WIDTH);
        if (nEastAsianWidth == U_EA_FULLWIDTH || nEastAsianWidth == U_EA_WIDE)
            aExpanded.push_back(cSlot);
    }
    return aExpanded;
}

spreadsheetengine::api::String collapseDbcsByteText(
    spreadsheetengine::api::StringView rExpandedText)
{
    spreadsheetengine::api::String aCollapsed;
    for (std::size_t nIndex = 0; nIndex < rExpandedText.size();)
    {
        const char16_t cChar = rExpandedText[nIndex];
        if (cChar <= 0x7F)
        {
            aCollapsed.push_back(cChar);
            ++nIndex;
            continue;
        }

        const int nEastAsianWidth = u_getIntPropertyValue(cChar, UCHAR_EAST_ASIAN_WIDTH);
        if (nEastAsianWidth != U_EA_FULLWIDTH && nEastAsianWidth != U_EA_WIDE)
        {
            aCollapsed.push_back(cChar);
            ++nIndex;
            continue;
        }

        if (nIndex + 1 < rExpandedText.size() && rExpandedText[nIndex + 1] == cChar)
        {
            aCollapsed.push_back(cChar);
            nIndex += 2;
            continue;
        }

        aCollapsed.push_back(u' ');
        ++nIndex;
    }

    return aCollapsed;
}

std::optional<std::size_t> findDbcsExpandedText(spreadsheetengine::api::StringView rNeedle,
    spreadsheetengine::api::StringView rHaystack, std::size_t nStartIndex, bool bFoldAscii)
{
    const spreadsheetengine::api::String aNeedle = expandDbcsByteText(rNeedle, bFoldAscii);
    const spreadsheetengine::api::String aHaystack = expandDbcsByteText(rHaystack, bFoldAscii);
    if (nStartIndex > aHaystack.size())
        return std::nullopt;
    if (aNeedle.empty())
        return nStartIndex;

    const std::size_t nPos = aHaystack.find(aNeedle, nStartIndex);
    if (nPos == spreadsheetengine::api::String::npos)
        return std::nullopt;
    return nPos;
}

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
