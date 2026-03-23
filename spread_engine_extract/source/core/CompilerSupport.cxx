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

constexpr std::array<ScCharFlags, 128> makeCommonCharTable()
{
    std::array<ScCharFlags, 128> a;
    a.fill(ScCharFlags::Illegal);

    a['\t'] = ScCharFlags::CharDontCare | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['\n'] = ScCharFlags::CharDontCare | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['\r'] = ScCharFlags::CharDontCare | ScCharFlags::WordSep | ScCharFlags::ValueSep;

    a[' '] = ScCharFlags::CharDontCare | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['!'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['"'] = ScCharFlags::CharString | ScCharFlags::StringSep;
    a['#'] = ScCharFlags::WordSep | ScCharFlags::CharErrConst;
    a['$'] = ScCharFlags::CharWord | ScCharFlags::Word | ScCharFlags::CharIdent | ScCharFlags::Ident;
    a['%'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['&'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['\''] = ScCharFlags::NameSep;
    a['('] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a[')'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['*'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['+'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueExp | ScCharFlags::ValueSign;
    a[','] = ScCharFlags::CharValue | ScCharFlags::Value;
    a['-'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueExp | ScCharFlags::ValueSign;
    a['.'] = ScCharFlags::Word | ScCharFlags::CharValue | ScCharFlags::Value | ScCharFlags::Ident
             | ScCharFlags::Name;
    a['/'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;

    for (int i = '0'; i <= '9'; ++i)
        a[i] = ScCharFlags::CharValue | ScCharFlags::Word | ScCharFlags::Value
               | ScCharFlags::ValueExp | ScCharFlags::ValueValue | ScCharFlags::Ident
               | ScCharFlags::Name;

    a[':'] = ScCharFlags::Char | ScCharFlags::Word;
    a[';'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['<'] = ScCharFlags::CharBool | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['='] = ScCharFlags::Char | ScCharFlags::Bool | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['>'] = ScCharFlags::CharBool | ScCharFlags::Bool | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['?'] = ScCharFlags::CharWord | ScCharFlags::Word | ScCharFlags::Name;

    for (int i = 'A'; i <= 'Z'; ++i)
        a[i] = ScCharFlags::CharWord | ScCharFlags::Word | ScCharFlags::CharIdent
               | ScCharFlags::Ident | ScCharFlags::CharName | ScCharFlags::Name;

    a['^'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['_'] = ScCharFlags::CharWord | ScCharFlags::Word | ScCharFlags::CharIdent
             | ScCharFlags::Ident | ScCharFlags::CharName | ScCharFlags::Name;

    for (int i = 'a'; i <= 'z'; ++i)
        a[i] = ScCharFlags::CharWord | ScCharFlags::Word | ScCharFlags::CharIdent
               | ScCharFlags::Ident | ScCharFlags::CharName | ScCharFlags::Name;

    a['{'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['|'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['}'] = ScCharFlags::Char | ScCharFlags::WordSep | ScCharFlags::ValueSep;
    a['~'] = ScCharFlags::Char;

    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_OOO_A1()
{
    auto a = makeCommonCharTable();
    a['['] = ScCharFlags::Char;
    a[']'] = ScCharFlags::Char;
    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_OOO_A1_ODF()
{
    auto a = makeCommonCharTable();
    a['!'] |= ScCharFlags::OdfLabelOp;
    a['$'] |= ScCharFlags::OdfNameMarker;
    a['['] = ScCharFlags::OdfLBracket;
    a[']'] = ScCharFlags::OdfRBracket;
    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_XL()
{
    auto a = makeCommonCharTable();
    a[' '] |= ScCharFlags::Word;
    a['!'] |= ScCharFlags::Ident | ScCharFlags::Word;
    a['"'] |= ScCharFlags::Word;
    a['#'] &= ~ScCharFlags::WordSep;
    a['#'] |= ScCharFlags::Word;
    a['%'] |= ScCharFlags::Word;
    a['&'] |= ScCharFlags::Word;
    a['\''] |= ScCharFlags::Word;
    a['('] |= ScCharFlags::Word;
    a[')'] |= ScCharFlags::Word;
    a['*'] |= ScCharFlags::Word;
    a['+'] |= ScCharFlags::Word;
    a[','] |= ScCharFlags::Word;
    a['-'] |= ScCharFlags::Word;
    a[';'] |= ScCharFlags::Word;
    a['<'] |= ScCharFlags::Word;
    a['='] |= ScCharFlags::Word;
    a['>'] |= ScCharFlags::Word;
    a['@'] |= ScCharFlags::Word;
    a['['] = ScCharFlags::Word;
    a[']'] = ScCharFlags::Word;
    a['{'] |= ScCharFlags::Word;
    a['|'] |= ScCharFlags::Word;
    a['}'] |= ScCharFlags::Word;
    a['~'] |= ScCharFlags::Word;
    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_XL_A1()
{
    auto a = makeCharTable_XL();
    a['['] |= ScCharFlags::Char;
    a[']'] |= ScCharFlags::Char;
    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_XL_OOX()
{
    auto a = makeCharTable_XL_A1();
    a['['] |= ScCharFlags::CharIdent;
    a[']'] |= ScCharFlags::Ident;
    return a;
}

constexpr std::array<ScCharFlags, 128> makeCharTable_XL_R1C1()
{
    auto a = makeCharTable_XL();
    a['['] |= ScCharFlags::Ident;
    a[']'] |= ScCharFlags::Ident;
    return a;
}

} // namespace

const std::array<ScCharFlags, 128>&
getCharTable(formula::FormulaGrammar::AddressConvention eConv)
{
    switch (eConv)
    {
        case formula::FormulaGrammar::CONV_OOO:
        {
            static constexpr auto table_OOO_A1 = makeCharTable_OOO_A1();
            return table_OOO_A1;
        }
        case formula::FormulaGrammar::CONV_ODF:
        {
            static constexpr auto table_OOO_A1_ODF = makeCharTable_OOO_A1_ODF();
            return table_OOO_A1_ODF;
        }
        case formula::FormulaGrammar::CONV_XL_A1:
        {
            static constexpr auto table_XL_A1 = makeCharTable_XL_A1();
            return table_XL_A1;
        }
        case formula::FormulaGrammar::CONV_XL_R1C1:
        {
            static constexpr auto table_XL_R1C1 = makeCharTable_XL_R1C1();
            return table_XL_R1C1;
        }
        case formula::FormulaGrammar::CONV_XL_OOX:
        {
            static constexpr auto table_XL_OOX = makeCharTable_XL_OOX();
            return table_XL_OOX;
        }
        case formula::FormulaGrammar::CONV_UNSPECIFIED:
        default:
            assert(!"Unimplemented convention");
            std::abort();
    }
}

} // namespace spreadsheetengine::core::compiler

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
