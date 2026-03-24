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

#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/core/TextServices.hxx>

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

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
