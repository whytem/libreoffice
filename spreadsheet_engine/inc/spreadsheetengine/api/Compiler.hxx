/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Types.hxx>

namespace spreadsheetengine::api
{

enum class CompilerCharFlags : sal_uInt32
{
    NONE = 0x00000000,
    Illegal = 0x00000000,
    Char = 0x00000001,
    CharBool = 0x00000002,
    CharWord = 0x00000004,
    CharValue = 0x00000008,
    CharString = 0x00000010,
    CharDontCare = 0x00000020,
    Bool = 0x00000040,
    Word = 0x00000080,
    WordSep = 0x00000100,
    Value = 0x00000200,
    ValueSep = 0x00000400,
    ValueExp = 0x00000800,
    ValueSign = 0x00001000,
    ValueValue = 0x00002000,
    StringSep = 0x00004000,
    NameSep = 0x00008000,
    CharIdent = 0x00010000,
    Ident = 0x00020000,
    OdfLBracket = 0x00040000,
    OdfRBracket = 0x00080000,
    OdfLabelOp = 0x00100000,
    OdfNameMarker = 0x00200000,
    CharName = 0x00400000,
    Name = 0x00800000,
    CharErrConst = 0x01000000,
};

constexpr CompilerCharFlags operator|(CompilerCharFlags eLeft, CompilerCharFlags eRight)
{
    return static_cast<CompilerCharFlags>(
        static_cast<sal_uInt32>(eLeft) | static_cast<sal_uInt32>(eRight));
}

constexpr CompilerCharFlags operator&(CompilerCharFlags eLeft, CompilerCharFlags eRight)
{
    return static_cast<CompilerCharFlags>(
        static_cast<sal_uInt32>(eLeft) & static_cast<sal_uInt32>(eRight));
}

constexpr CompilerCharFlags operator~(CompilerCharFlags eValue)
{
    return static_cast<CompilerCharFlags>(~static_cast<sal_uInt32>(eValue));
}

constexpr CompilerCharFlags& operator|=(CompilerCharFlags& reLeft, CompilerCharFlags eRight)
{
    reLeft = reLeft | eRight;
    return reLeft;
}

constexpr CompilerCharFlags& operator&=(CompilerCharFlags& reLeft, CompilerCharFlags eRight)
{
    reLeft = reLeft & eRight;
    return reLeft;
}

constexpr bool hasAnyCompilerCharFlag(CompilerCharFlags eValue, CompilerCharFlags eMask)
{
    return static_cast<sal_uInt32>(eValue & eMask) != 0;
}

} // namespace spreadsheetengine::api

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
