/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <formula/grammar.hxx>

#include <spreadsheetengine/api/Grammar.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::AddressConvention toApiAddressConvention(
    formula::FormulaGrammar::AddressConvention eConv)
{
    switch (eConv)
    {
        case formula::FormulaGrammar::CONV_OOO:
            return spreadsheetengine::api::AddressConvention::OooA1;
        case formula::FormulaGrammar::CONV_ODF:
            return spreadsheetengine::api::AddressConvention::OdfA1;
        case formula::FormulaGrammar::CONV_XL_A1:
            return spreadsheetengine::api::AddressConvention::XlA1;
        case formula::FormulaGrammar::CONV_XL_R1C1:
            return spreadsheetengine::api::AddressConvention::XlR1C1;
        case formula::FormulaGrammar::CONV_XL_OOX:
            return spreadsheetengine::api::AddressConvention::XlOox;
        default:
            return spreadsheetengine::api::AddressConvention::Unknown;
    }
}

inline formula::FormulaGrammar::AddressConvention toLibreOfficeAddressConvention(
    spreadsheetengine::api::AddressConvention eConv)
{
    switch (eConv)
    {
        case spreadsheetengine::api::AddressConvention::OooA1:
            return formula::FormulaGrammar::CONV_OOO;
        case spreadsheetengine::api::AddressConvention::OdfA1:
            return formula::FormulaGrammar::CONV_ODF;
        case spreadsheetengine::api::AddressConvention::XlA1:
            return formula::FormulaGrammar::CONV_XL_A1;
        case spreadsheetengine::api::AddressConvention::XlR1C1:
            return formula::FormulaGrammar::CONV_XL_R1C1;
        case spreadsheetengine::api::AddressConvention::XlOox:
            return formula::FormulaGrammar::CONV_XL_OOX;
        case spreadsheetengine::api::AddressConvention::Unknown:
        default:
            return formula::FormulaGrammar::CONV_UNSPECIFIED;
    }
}

inline spreadsheetengine::api::FormulaLanguage toApiFormulaLanguage(sal_Int32 nLanguage)
{
    switch (nLanguage)
    {
        case css::sheet::FormulaLanguage::NATIVE:
            return spreadsheetengine::api::FormulaLanguage::Native;
        case css::sheet::FormulaLanguage::ENGLISH:
            return spreadsheetengine::api::FormulaLanguage::English;
        case css::sheet::FormulaLanguage::XL_ENGLISH:
            return spreadsheetengine::api::FormulaLanguage::XlEnglish;
        case css::sheet::FormulaLanguage::API:
            return spreadsheetengine::api::FormulaLanguage::Api;
        case css::sheet::FormulaLanguage::ODF_11:
            return spreadsheetengine::api::FormulaLanguage::Odf11;
        case css::sheet::FormulaLanguage::ODFF:
            return spreadsheetengine::api::FormulaLanguage::Odff;
        case css::sheet::FormulaLanguage::OOXML:
            return spreadsheetengine::api::FormulaLanguage::Ooxml;
        case formula::FormulaGrammar::GRAM_EXTERNAL:
            return spreadsheetengine::api::FormulaLanguage::External;
        default:
            return spreadsheetengine::api::FormulaLanguage::Unknown;
    }
}

inline sal_Int32 toLibreOfficeFormulaLanguage(spreadsheetengine::api::FormulaLanguage eLanguage)
{
    switch (eLanguage)
    {
        case spreadsheetengine::api::FormulaLanguage::Native:
            return css::sheet::FormulaLanguage::NATIVE;
        case spreadsheetengine::api::FormulaLanguage::English:
            return css::sheet::FormulaLanguage::ENGLISH;
        case spreadsheetengine::api::FormulaLanguage::XlEnglish:
            return css::sheet::FormulaLanguage::XL_ENGLISH;
        case spreadsheetengine::api::FormulaLanguage::Api:
            return css::sheet::FormulaLanguage::API;
        case spreadsheetengine::api::FormulaLanguage::Odf11:
            return css::sheet::FormulaLanguage::ODF_11;
        case spreadsheetengine::api::FormulaLanguage::Odff:
            return css::sheet::FormulaLanguage::ODFF;
        case spreadsheetengine::api::FormulaLanguage::Ooxml:
            return css::sheet::FormulaLanguage::OOXML;
        case spreadsheetengine::api::FormulaLanguage::External:
            return formula::FormulaGrammar::GRAM_EXTERNAL;
        default:
            return formula::FormulaGrammar::GRAM_UNSPECIFIED;
    }
}

inline spreadsheetengine::api::Grammar toApiGrammar(formula::FormulaGrammar::Grammar eGrammar)
{
    if (eGrammar == formula::FormulaGrammar::GRAM_UNSPECIFIED)
        return {};

    return { toApiFormulaLanguage(formula::FormulaGrammar::extractFormulaLanguage(eGrammar)),
        toApiAddressConvention(formula::FormulaGrammar::extractRefConvention(eGrammar)),
        formula::FormulaGrammar::isEnglish(eGrammar) };
}

inline formula::FormulaGrammar::Grammar
toLibreOfficeGrammar(const spreadsheetengine::api::Grammar& rGrammar)
{
    const sal_Int32 nLanguage = toLibreOfficeFormulaLanguage(rGrammar.meLanguage);
    if (nLanguage == formula::FormulaGrammar::GRAM_UNSPECIFIED)
        return formula::FormulaGrammar::GRAM_UNSPECIFIED;

    return static_cast<formula::FormulaGrammar::Grammar>(
        nLanguage
        | ((toLibreOfficeAddressConvention(rGrammar.meAddressConvention)
            + formula::FormulaGrammar::kConventionOffset)
           << formula::FormulaGrammar::kConventionShift)
        | (rGrammar.mbEnglish ? formula::FormulaGrammar::kEnglishBit : 0));
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
