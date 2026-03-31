/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <optional>

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/String.hxx>

namespace spreadsheetengine::core::text
{

class CaseMappingService
{
public:
    virtual ~CaseMappingService() = default;

    virtual spreadsheetengine::api::String uppercase(
        spreadsheetengine::api::StringView rInput) const = 0;
    virtual spreadsheetengine::api::String lowercase(
        spreadsheetengine::api::StringView rInput) const = 0;
    virtual bool isLetter(char32_t nCodePoint) const = 0;
};

class WidthConversionService
{
public:
    virtual ~WidthConversionService() = default;

    virtual spreadsheetengine::api::String toHalfWidth(
        spreadsheetengine::api::StringView rInput) const = 0;
    virtual spreadsheetengine::api::String toFullWidth(
        spreadsheetengine::api::StringView rInput) const = 0;
};

class SingleByteEncodingService
{
public:
    virtual ~SingleByteEncodingService() = default;

    virtual sal_Int32 encodeFirstCharacter(
        spreadsheetengine::api::StringView rInput) const = 0;
    virtual std::optional<spreadsheetengine::api::String> decodeSingleByte(
        unsigned char nValue) const = 0;
};

} // namespace spreadsheetengine::core::text

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
