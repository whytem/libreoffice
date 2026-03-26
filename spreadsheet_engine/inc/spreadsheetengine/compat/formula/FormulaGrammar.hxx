/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/api/Grammar.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::compat::formula
{

/**
 * Calc-owned copy of the FormulaGrammar helper logic.
 *
 * This layer now uses spreadsheetengine-owned grammar enums so it can be
 * built and tested outside LibreOffice. Calc bridges to the legacy
 * ::formula::FormulaGrammar values at the boundary.
 */
class SPREADSHEETENGINE_DLLPUBLIC FormulaGrammar
{
public:
    using AddressConvention = spreadsheetengine::api::AddressConvention;
    using FormulaLanguage = spreadsheetengine::api::FormulaLanguage;
    using Grammar = spreadsheetengine::api::Grammar;

    static bool isEnglish(const Grammar& rGrammar)
    {
        return rGrammar.mbEnglish;
    }

    static Grammar mapAPItoGrammar(bool bEnglish, bool bXML);
    static bool isSupported(const Grammar& rGrammar);

    static FormulaLanguage extractFormulaLanguage(const Grammar& rGrammar)
    {
        return rGrammar.meLanguage;
    }

    static AddressConvention extractRefConvention(const Grammar& rGrammar)
    {
        return rGrammar.meAddressConvention;
    }

    static Grammar setEnglishBit(Grammar eGrammar, bool bEnglish);
    static Grammar mergeToGrammar(Grammar eGrammar, AddressConvention eConv);

    static bool isPODF(const Grammar& rGrammar)
    {
        return extractFormulaLanguage(rGrammar) == FormulaLanguage::Odf11;
    }

    static bool isODFF(const Grammar& rGrammar)
    {
        return extractFormulaLanguage(rGrammar) == FormulaLanguage::Odff;
    }

    static bool isOOXML(const Grammar& rGrammar)
    {
        return extractFormulaLanguage(rGrammar) == FormulaLanguage::Ooxml;
    }

    static bool isRefConventionOOXML(const Grammar& rGrammar)
    {
        return extractRefConvention(rGrammar) == AddressConvention::XlOox;
    }

    static bool isExcelSyntax(const Grammar& rGrammar)
    {
        switch (extractRefConvention(rGrammar))
        {
            case AddressConvention::XlA1:
            case AddressConvention::XlR1C1:
            case AddressConvention::XlOox:
                return true;
            default:
                return false;
        }
    }
};

} // namespace spreadsheetengine::compat::formula

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
