/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#pragma once

#include <optional>

#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/runtime/TextServices.hxx>

namespace spreadsheetengine::standalone::test
{

class AsciiCaseMappingService final : public spreadsheetengine::core::text::CaseMappingService
{
public:
    spreadsheetengine::api::String uppercase(
        spreadsheetengine::api::StringView rInput) const override
    {
        spreadsheetengine::api::String aResult(rInput);
        for (auto& c : aResult)
        {
            if (u'a' <= c && c <= u'z')
                c -= (u'a' - u'A');
        }
        return aResult;
    }

    spreadsheetengine::api::String lowercase(
        spreadsheetengine::api::StringView rInput) const override
    {
        spreadsheetengine::api::String aResult(rInput);
        for (auto& c : aResult)
        {
            if (u'A' <= c && c <= u'Z')
                c += (u'a' - u'A');
        }
        return aResult;
    }

    bool isLetter(char32_t nCodePoint) const override
    {
        return (U'A' <= nCodePoint && nCodePoint <= U'Z')
               || (U'a' <= nCodePoint && nCodePoint <= U'z');
    }
};

class MockWidthConversionService final
    : public spreadsheetengine::core::text::WidthConversionService
{
public:
    spreadsheetengine::api::String toHalfWidth(
        spreadsheetengine::api::StringView rInput) const override
    {
        spreadsheetengine::api::String aResult(rInput);
        for (auto& c : aResult)
        {
            if (c == u'Ａ')
                c = u'A';
        }
        return aResult;
    }

    spreadsheetengine::api::String toFullWidth(
        spreadsheetengine::api::StringView rInput) const override
    {
        spreadsheetengine::api::String aResult(rInput);
        for (auto& c : aResult)
        {
            if (c == u'A')
                c = u'Ａ';
        }
        return aResult;
    }
};

class Latin1EncodingService final
    : public spreadsheetengine::core::text::SingleByteEncodingService
{
public:
    sal_Int32 encodeFirstCharacter(spreadsheetengine::api::StringView rInput) const override
    {
        if (rInput.empty())
            return 0;
        return static_cast<unsigned char>(rInput.front() & 0x00FF);
    }

    std::optional<spreadsheetengine::api::String> decodeSingleByte(
        unsigned char nValue) const override
    {
        return spreadsheetengine::api::String(1, static_cast<char16_t>(nValue));
    }
};

} // namespace spreadsheetengine::standalone::test

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
