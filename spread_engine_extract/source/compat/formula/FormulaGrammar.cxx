/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/compat/formula/FormulaGrammar.hxx>

#include <cassert>

namespace spreadsheetengine::compat::formula
{

FormulaGrammar::Grammar FormulaGrammar::mapAPItoGrammar(const bool bEnglish, const bool bXML)
{
    Grammar eGrammar;
    if (bEnglish && bXML)
        eGrammar = ::formula::FormulaGrammar::GRAM_PODF;
    else if (bEnglish && !bXML)
        eGrammar = ::formula::FormulaGrammar::GRAM_API;
    else if (!bEnglish && bXML)
        eGrammar = ::formula::FormulaGrammar::GRAM_NATIVE_ODF;
    else
        eGrammar = ::formula::FormulaGrammar::GRAM_NATIVE;
    return eGrammar;
}

bool FormulaGrammar::isSupported(const Grammar eGrammar)
{
    switch (eGrammar)
    {
        case ::formula::FormulaGrammar::GRAM_ODFF:
        case ::formula::FormulaGrammar::GRAM_PODF:
        case ::formula::FormulaGrammar::GRAM_ENGLISH:
        case ::formula::FormulaGrammar::GRAM_NATIVE:
        case ::formula::FormulaGrammar::GRAM_ODFF_UI:
        case ::formula::FormulaGrammar::GRAM_ODFF_A1:
        case ::formula::FormulaGrammar::GRAM_PODF_UI:
        case ::formula::FormulaGrammar::GRAM_PODF_A1:
        case ::formula::FormulaGrammar::GRAM_NATIVE_UI:
        case ::formula::FormulaGrammar::GRAM_NATIVE_ODF:
        case ::formula::FormulaGrammar::GRAM_NATIVE_XL_A1:
        case ::formula::FormulaGrammar::GRAM_NATIVE_XL_R1C1:
        case ::formula::FormulaGrammar::GRAM_ENGLISH_XL_A1:
        case ::formula::FormulaGrammar::GRAM_ENGLISH_XL_R1C1:
        case ::formula::FormulaGrammar::GRAM_ENGLISH_XL_OOX:
        case ::formula::FormulaGrammar::GRAM_OOXML:
        case ::formula::FormulaGrammar::GRAM_API:
            return true;
        default:
            return extractFormulaLanguage(eGrammar) == ::formula::FormulaGrammar::GRAM_EXTERNAL;
    }
}

FormulaGrammar::Grammar FormulaGrammar::setEnglishBit(const Grammar eGrammar, const bool bEnglish)
{
    if (bEnglish)
        return static_cast<Grammar>(eGrammar | kEnglishBit);

    return static_cast<Grammar>(eGrammar & ~kEnglishBit);
}

FormulaGrammar::Grammar FormulaGrammar::mergeToGrammar(const Grammar eGrammar, const AddressConvention eConv)
{
    const bool bEnglish = isEnglish(eGrammar);
    Grammar eGram = static_cast<Grammar>(
        extractFormulaLanguage(eGrammar)
        | ((eConv + kConventionOffset) << kConventionShift));
    eGram = setEnglishBit(eGram, bEnglish);
    assert(isSupported(eGram));
    return eGram;
}

} // namespace spreadsheetengine::compat::formula

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
