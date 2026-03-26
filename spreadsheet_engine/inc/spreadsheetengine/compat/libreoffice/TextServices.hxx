/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <comphelper/processfactory.hxx>
#include <i18nutil/transliteration.hxx>
#include <osl/thread.h>
#include <rtl/character.hxx>
#include <rtl/textenc.h>
#include <rtl/ustring.hxx>
#include <unotools/charclass.hxx>
#include <unotools/transliterationwrapper.hxx>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/runtime/TextCase.hxx>
#include <spreadsheetengine/runtime/TextScalar.hxx>
#include <spreadsheetengine/runtime/TextServices.hxx>
#include <spreadsheetengine/runtime/TextWidth.hxx>

namespace spreadsheetengine::compat::libreoffice
{

class CharClassCaseMappingService final : public spreadsheetengine::core::text::CaseMappingService
{
public:
    explicit CharClassCaseMappingService(const CharClass& rCharClass)
        : mrCharClass(rCharClass)
    {
    }

    spreadsheetengine::api::String uppercase(
        spreadsheetengine::api::StringView rInput) const override
    {
        return toApiString(mrCharClass.uppercase(toLibreOfficeString(spreadsheetengine::api::String(rInput))));
    }

    spreadsheetengine::api::String lowercase(
        spreadsheetengine::api::StringView rInput) const override
    {
        return toApiString(mrCharClass.lowercase(toLibreOfficeString(spreadsheetengine::api::String(rInput))));
    }

    bool isLetter(char32_t nCodePoint) const override
    {
        sal_uInt32 nUnicode = static_cast<sal_uInt32>(nCodePoint);
        OUString aChar(&nUnicode, 1);
        return mrCharClass.isLetter(aChar, 0);
    }

private:
    const CharClass& mrCharClass;
};

class TransliterationWidthConversionService final
    : public spreadsheetengine::core::text::WidthConversionService
{
public:
    spreadsheetengine::api::String toHalfWidth(
        spreadsheetengine::api::StringView rInput) const override
    {
        auto init = []() -> utl::TransliterationWrapper&
        {
            static utl::TransliterationWrapper trans(
                comphelper::getProcessComponentContext(), static_cast<TransliterationFlags>(0));
            trans.loadModuleByImplName(u"FULLWIDTH_HALFWIDTH_LIKE_ASC"_ustr, LANGUAGE_SYSTEM);
            return trans;
        };
        static utl::TransliterationWrapper& rTrans(init());
        const OUString aValue = toLibreOfficeString(spreadsheetengine::api::String(rInput));
        return toApiString(rTrans.transliterate(aValue, 0, sal_uInt16(aValue.getLength())));
    }

    spreadsheetengine::api::String toFullWidth(
        spreadsheetengine::api::StringView rInput) const override
    {
        auto init = []() -> utl::TransliterationWrapper&
        {
            static utl::TransliterationWrapper trans(
                comphelper::getProcessComponentContext(), static_cast<TransliterationFlags>(0));
            trans.loadModuleByImplName(u"HALFWIDTH_FULLWIDTH_LIKE_JIS"_ustr, LANGUAGE_SYSTEM);
            return trans;
        };
        static utl::TransliterationWrapper& rTrans(init());
        const OUString aValue = toLibreOfficeString(spreadsheetengine::api::String(rInput));
        return toApiString(rTrans.transliterate(aValue, 0, sal_uInt16(aValue.getLength())));
    }
};

class SystemTextEncodingService final : public spreadsheetengine::core::text::SingleByteEncodingService
{
public:
    sal_Int32 encodeFirstCharacter(spreadsheetengine::api::StringView rInput) const override
    {
        if (rInput.empty())
            return 0;

        const sal_uInt32 nConvertFlags = RTL_UNICODETOTEXT_FLAGS_NONSPACING_IGNORE
                                         | RTL_UNICODETOTEXT_FLAGS_CONTROL_IGNORE
                                         | RTL_UNICODETOTEXT_FLAGS_FLUSH
                                         | RTL_UNICODETOTEXT_FLAGS_UNDEFINED_DEFAULT
                                         | RTL_UNICODETOTEXT_FLAGS_INVALID_DEFAULT
                                         | RTL_UNICODETOTEXT_FLAGS_UNDEFINED_REPLACE;
        return static_cast<unsigned char>(
            OUStringToOString(OUStringChar(rInput[0]), osl_getThreadTextEncoding(), nConvertFlags)
                .toChar());
    }

    std::optional<spreadsheetengine::api::String> decodeSingleByte(
        unsigned char nValue) const override
    {
        const sal_uInt32 nConvertFlags = RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_DEFAULT
                                         | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_DEFAULT
                                         | RTL_TEXTTOUNICODE_FLAGS_INVALID_DEFAULT;
        const char cEncodedChar = static_cast<char>(nValue);
        return toApiString(OUString(&cEncodedChar, 1, osl_getThreadTextEncoding(), nConvertFlags));
    }
};

inline SystemTextEncodingService& getSystemTextEncodingService()
{
    static SystemTextEncodingService aService;
    return aService;
}

inline TransliterationWidthConversionService& getWidthConversionService()
{
    static TransliterationWidthConversionService aService;
    return aService;
}

inline OUString trimRepeatedSpaces(const OUString& rInput)
{
    return toLibreOfficeString(spreadsheetengine::core::text::trimRepeatedSpaces(toApiString(rInput)));
}

inline OUString uppercase(const CharClass& rCharClass, const OUString& rInput)
{
    const CharClassCaseMappingService aCaseService(rCharClass);
    return toLibreOfficeString(
        spreadsheetengine::core::text::uppercase(aCaseService, toApiString(rInput)));
}

inline OUString propercase(const CharClass& rCharClass, const OUString& rInput)
{
    const CharClassCaseMappingService aCaseService(rCharClass);
    return toLibreOfficeString(
        spreadsheetengine::core::text::propercase(aCaseService, toApiString(rInput)));
}

inline OUString lowercase(const CharClass& rCharClass, const OUString& rInput)
{
    const CharClassCaseMappingService aCaseService(rCharClass);
    return toLibreOfficeString(
        spreadsheetengine::core::text::lowercase(aCaseService, toApiString(rInput)));
}

inline sal_Int32 countCodePoints(const OUString& rInput)
{
    return spreadsheetengine::core::text::countCodePoints(toApiString(rInput));
}

inline spreadsheetengine::api::ValueResult<double> parseNumberValue(const OUString& rInput,
    const std::optional<OUString>& roDecimalSeparator,
    const std::optional<OUString>& roGroupSeparator, bool bEmptyStringAsZero)
{
    const auto aResult = spreadsheetengine::core::text::parseNumberValue(toApiString(rInput),
        roDecimalSeparator ? std::optional(toApiString(*roDecimalSeparator)) : std::nullopt,
        roGroupSeparator ? std::optional(toApiString(*roGroupSeparator)) : std::nullopt,
        bEmptyStringAsZero);
    switch (aResult.meStatus)
    {
        case spreadsheetengine::core::text::NumberValueStatus::Ok:
            return spreadsheetengine::api::ValueResult<double>::success(aResult.mfValue);
        case spreadsheetengine::core::text::NumberValueStatus::NoValue:
            return spreadsheetengine::api::ValueResult<double>::failure(
                spreadsheetengine::api::Error::NoValue);
        case spreadsheetengine::core::text::NumberValueStatus::IllegalArgument:
            return spreadsheetengine::api::ValueResult<double>::failure(
                spreadsheetengine::api::Error::IllegalArgument);
    }

    return spreadsheetengine::api::ValueResult<double>::failure(
        spreadsheetengine::api::Error::IllegalArgument);
}

inline OUString cleanPrintable(const OUString& rInput)
{
    return toLibreOfficeString(spreadsheetengine::core::text::cleanPrintable(toApiString(rInput)));
}

inline sal_Int32 codeFromText(const OUString& rInput)
{
    return spreadsheetengine::core::text::codeFromText(
        getSystemTextEncodingService(), toApiString(rInput));
}

inline std::optional<OUString> charFromValue(double fValue)
{
    if (auto oValue = spreadsheetengine::core::text::charFromValue(
            getSystemTextEncodingService(), fValue))
    {
        return toLibreOfficeString(*oValue);
    }
    return std::nullopt;
}

inline OUString convertIntoFullWidth(const OUString& rInput)
{
    return toLibreOfficeString(spreadsheetengine::core::text::convertIntoFullWidth(
        getWidthConversionService(), toApiString(rInput)));
}

inline OUString convertIntoHalfWidth(const OUString& rInput)
{
    return toLibreOfficeString(spreadsheetengine::core::text::convertIntoHalfWidth(
        getWidthConversionService(), toApiString(rInput)));
}

inline std::optional<double> unicodeFromText(const OUString& rInput)
{
    return spreadsheetengine::core::text::unicodeFromText(toApiString(rInput));
}

inline std::optional<OUString> unicharFromCodePoint(sal_uInt32 nCodePoint)
{
    if (auto oValue = spreadsheetengine::core::text::unicharFromCodePoint(nCodePoint))
        return toLibreOfficeString(*oValue);
    return std::nullopt;
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
