/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <spreadsheetengine/compat/formula/FormulaGrammar.hxx>

namespace spreadsheetengine::compat::formula
{

FormulaGrammar::Grammar FormulaGrammar::mapAPItoGrammar(const bool bEnglish, const bool bXML)
{
    Grammar eGrammar;
    if (bEnglish && bXML)
        eGrammar = { FormulaLanguage::Odf11, AddressConvention::OdfA1, true };
    else if (bEnglish && !bXML)
        eGrammar = { FormulaLanguage::Api, AddressConvention::OooA1, true };
    else if (!bEnglish && bXML)
        eGrammar = { FormulaLanguage::Native, AddressConvention::OdfA1, false };
    else
        eGrammar = { FormulaLanguage::Native, AddressConvention::OooA1, false };
    return eGrammar;
}

bool FormulaGrammar::isSupported(const Grammar& rGrammar)
{
    switch (rGrammar.meLanguage)
    {
        case FormulaLanguage::Odff:
        case FormulaLanguage::Odf11:
            return rGrammar.mbEnglish
                   && (rGrammar.meAddressConvention == AddressConvention::OdfA1
                       || rGrammar.meAddressConvention == AddressConvention::Unknown
                       || rGrammar.meAddressConvention == AddressConvention::OooA1);
        case FormulaLanguage::English:
        case FormulaLanguage::Api:
            return rGrammar.mbEnglish
                   && rGrammar.meAddressConvention == AddressConvention::OooA1;
        case FormulaLanguage::Native:
            return !rGrammar.mbEnglish
                   && (rGrammar.meAddressConvention == AddressConvention::OooA1
                       || rGrammar.meAddressConvention == AddressConvention::Unknown
                       || rGrammar.meAddressConvention == AddressConvention::OdfA1
                       || rGrammar.meAddressConvention == AddressConvention::XlA1
                       || rGrammar.meAddressConvention == AddressConvention::XlR1C1);
        case FormulaLanguage::XlEnglish:
            return rGrammar.mbEnglish
                   && (rGrammar.meAddressConvention == AddressConvention::XlA1
                       || rGrammar.meAddressConvention == AddressConvention::XlR1C1
                       || rGrammar.meAddressConvention == AddressConvention::XlOox);
        case FormulaLanguage::Ooxml:
            return rGrammar.mbEnglish
                   && rGrammar.meAddressConvention == AddressConvention::XlOox;
        case FormulaLanguage::External:
            return true;
        default:
            return false;
    }
}

FormulaGrammar::Grammar FormulaGrammar::setEnglishBit(Grammar eGrammar, const bool bEnglish)
{
    eGrammar.mbEnglish = bEnglish;
    return eGrammar;
}

FormulaGrammar::Grammar FormulaGrammar::mergeToGrammar(Grammar eGrammar, const AddressConvention eConv)
{
    eGrammar.meAddressConvention = eConv;
    return eGrammar;
}

} // namespace spreadsheetengine::compat::formula

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
