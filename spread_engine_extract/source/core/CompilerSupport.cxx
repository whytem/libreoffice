/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/core/CompilerSupport.hxx>

#include <cassert>
#include <cstdlib>

namespace spreadsheetengine::core::compiler
{
namespace
{

using spreadsheetengine::api::AddressConvention;
using spreadsheetengine::api::CompilerCharFlags;

constexpr std::array<CompilerCharFlags, 128> makeCommonCharTable()
{
    std::array<CompilerCharFlags, 128> a;
    a.fill(CompilerCharFlags::Illegal);

    a['\t'] = CompilerCharFlags::CharDontCare | CompilerCharFlags::WordSep
              | CompilerCharFlags::ValueSep;
    a['\n'] = CompilerCharFlags::CharDontCare | CompilerCharFlags::WordSep
              | CompilerCharFlags::ValueSep;
    a['\r'] = CompilerCharFlags::CharDontCare | CompilerCharFlags::WordSep
              | CompilerCharFlags::ValueSep;

    a[' '] = CompilerCharFlags::CharDontCare | CompilerCharFlags::WordSep
             | CompilerCharFlags::ValueSep;
    a['!'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;
    a['"'] = CompilerCharFlags::CharString | CompilerCharFlags::StringSep;
    a['#'] = CompilerCharFlags::WordSep | CompilerCharFlags::CharErrConst;
    a['$'] = CompilerCharFlags::CharWord | CompilerCharFlags::Word
             | CompilerCharFlags::CharIdent | CompilerCharFlags::Ident;
    a['%'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;
    a['&'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;
    a['\''] = CompilerCharFlags::NameSep;
    a['('] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;
    a[')'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;
    a['*'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;
    a['+'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep
             | CompilerCharFlags::ValueExp | CompilerCharFlags::ValueSign;
    a[','] = CompilerCharFlags::CharValue | CompilerCharFlags::Value;
    a['-'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep
             | CompilerCharFlags::ValueExp | CompilerCharFlags::ValueSign;
    a['.'] = CompilerCharFlags::Word | CompilerCharFlags::CharValue | CompilerCharFlags::Value
             | CompilerCharFlags::Ident | CompilerCharFlags::Name;
    a['/'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;

    for (int i = '0'; i <= '9'; ++i)
        a[i] = CompilerCharFlags::CharValue | CompilerCharFlags::Word | CompilerCharFlags::Value
               | CompilerCharFlags::ValueExp | CompilerCharFlags::ValueValue
               | CompilerCharFlags::Ident | CompilerCharFlags::Name;

    a[':'] = CompilerCharFlags::Char | CompilerCharFlags::Word;
    a[';'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;
    a['<'] = CompilerCharFlags::CharBool | CompilerCharFlags::WordSep
             | CompilerCharFlags::ValueSep;
    a['='] = CompilerCharFlags::Char | CompilerCharFlags::Bool | CompilerCharFlags::WordSep
             | CompilerCharFlags::ValueSep;
    a['>'] = CompilerCharFlags::CharBool | CompilerCharFlags::Bool | CompilerCharFlags::WordSep
             | CompilerCharFlags::ValueSep;
    a['?'] = CompilerCharFlags::CharWord | CompilerCharFlags::Word | CompilerCharFlags::Name;

    for (int i = 'A'; i <= 'Z'; ++i)
        a[i] = CompilerCharFlags::CharWord | CompilerCharFlags::Word
               | CompilerCharFlags::CharIdent | CompilerCharFlags::Ident
               | CompilerCharFlags::CharName | CompilerCharFlags::Name;

    a['^'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;
    a['_'] = CompilerCharFlags::CharWord | CompilerCharFlags::Word
             | CompilerCharFlags::CharIdent | CompilerCharFlags::Ident
             | CompilerCharFlags::CharName | CompilerCharFlags::Name;

    for (int i = 'a'; i <= 'z'; ++i)
        a[i] = CompilerCharFlags::CharWord | CompilerCharFlags::Word
               | CompilerCharFlags::CharIdent | CompilerCharFlags::Ident
               | CompilerCharFlags::CharName | CompilerCharFlags::Name;

    a['{'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;
    a['|'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;
    a['}'] = CompilerCharFlags::Char | CompilerCharFlags::WordSep | CompilerCharFlags::ValueSep;
    a['~'] = CompilerCharFlags::Char;

    return a;
}

constexpr std::array<CompilerCharFlags, 128> makeCharTable_OOO_A1()
{
    auto a = makeCommonCharTable();
    a['['] = CompilerCharFlags::Char;
    a[']'] = CompilerCharFlags::Char;
    return a;
}

constexpr std::array<CompilerCharFlags, 128> makeCharTable_OOO_A1_ODF()
{
    auto a = makeCommonCharTable();
    a['!'] |= CompilerCharFlags::OdfLabelOp;
    a['$'] |= CompilerCharFlags::OdfNameMarker;
    a['['] = CompilerCharFlags::OdfLBracket;
    a[']'] = CompilerCharFlags::OdfRBracket;
    return a;
}

constexpr std::array<CompilerCharFlags, 128> makeCharTable_XL()
{
    auto a = makeCommonCharTable();
    a[' '] |= CompilerCharFlags::Word;
    a['!'] |= CompilerCharFlags::Ident | CompilerCharFlags::Word;
    a['"'] |= CompilerCharFlags::Word;
    a['#'] &= ~CompilerCharFlags::WordSep;
    a['#'] |= CompilerCharFlags::Word;
    a['%'] |= CompilerCharFlags::Word;
    a['&'] |= CompilerCharFlags::Word;
    a['\''] |= CompilerCharFlags::Word;
    a['('] |= CompilerCharFlags::Word;
    a[')'] |= CompilerCharFlags::Word;
    a['*'] |= CompilerCharFlags::Word;
    a['+'] |= CompilerCharFlags::Word;
    a[','] |= CompilerCharFlags::Word;
    a['-'] |= CompilerCharFlags::Word;
    a[';'] |= CompilerCharFlags::Word;
    a['<'] |= CompilerCharFlags::Word;
    a['='] |= CompilerCharFlags::Word;
    a['>'] |= CompilerCharFlags::Word;
    a['@'] |= CompilerCharFlags::Word;
    a['['] = CompilerCharFlags::Word;
    a[']'] = CompilerCharFlags::Word;
    a['{'] |= CompilerCharFlags::Word;
    a['|'] |= CompilerCharFlags::Word;
    a['}'] |= CompilerCharFlags::Word;
    a['~'] |= CompilerCharFlags::Word;
    return a;
}

constexpr std::array<CompilerCharFlags, 128> makeCharTable_XL_A1()
{
    auto a = makeCharTable_XL();
    a['['] |= CompilerCharFlags::Char;
    a[']'] |= CompilerCharFlags::Char;
    return a;
}

constexpr std::array<CompilerCharFlags, 128> makeCharTable_XL_OOX()
{
    auto a = makeCharTable_XL_A1();
    a['['] |= CompilerCharFlags::CharIdent;
    a[']'] |= CompilerCharFlags::Ident;
    return a;
}

constexpr std::array<CompilerCharFlags, 128> makeCharTable_XL_R1C1()
{
    auto a = makeCharTable_XL();
    a['['] |= CompilerCharFlags::Ident;
    a[']'] |= CompilerCharFlags::Ident;
    return a;
}

} // namespace

const std::array<CompilerCharFlags, 128>& getCharTable(AddressConvention eConv)
{
    switch (eConv)
    {
        case AddressConvention::OooA1:
        {
            static constexpr auto table_OOO_A1 = makeCharTable_OOO_A1();
            return table_OOO_A1;
        }
        case AddressConvention::OdfA1:
        {
            static constexpr auto table_OOO_A1_ODF = makeCharTable_OOO_A1_ODF();
            return table_OOO_A1_ODF;
        }
        case AddressConvention::XlA1:
        {
            static constexpr auto table_XL_A1 = makeCharTable_XL_A1();
            return table_XL_A1;
        }
        case AddressConvention::XlR1C1:
        {
            static constexpr auto table_XL_R1C1 = makeCharTable_XL_R1C1();
            return table_XL_R1C1;
        }
        case AddressConvention::XlOox:
        {
            static constexpr auto table_XL_OOX = makeCharTable_XL_OOX();
            return table_XL_OOX;
        }
        case AddressConvention::Unknown:
        default:
            assert(!"Unimplemented convention");
            std::abort();
    }
}

} // namespace spreadsheetengine::core::compiler

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
