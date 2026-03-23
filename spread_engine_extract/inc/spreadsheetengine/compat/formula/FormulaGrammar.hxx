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
#include <sal/types.h>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::compat::formula
{

/**
 * Calc-owned copy of the FormulaGrammar helper logic.
 *
 * During the early extraction phases we keep using the existing
 * ::formula::FormulaGrammar enums and bit layout so Calc can be retargeted in
 * small slices without forcing a broad public type migration.
 */
class SPREADSHEETENGINE_DLLPUBLIC FormulaGrammar
{
public:
    using AddressConvention = ::formula::FormulaGrammar::AddressConvention;
    using Grammar = ::formula::FormulaGrammar::Grammar;

    static constexpr int kConventionOffset = ::formula::FormulaGrammar::kConventionOffset;
    static constexpr int kConventionShift = ::formula::FormulaGrammar::kConventionShift;
    static constexpr int kEnglishBit = ::formula::FormulaGrammar::kEnglishBit;
    static constexpr int kFlagMask = ::formula::FormulaGrammar::kFlagMask;

    static bool isEnglish(Grammar eGrammar)
    {
        return (eGrammar & kEnglishBit) != 0;
    }

    static Grammar mapAPItoGrammar(bool bEnglish, bool bXML);
    static bool isSupported(Grammar eGrammar);

    static sal_Int32 extractFormulaLanguage(Grammar eGrammar)
    {
        return eGrammar & kFlagMask;
    }

    static AddressConvention extractRefConvention(Grammar eGrammar)
    {
        return static_cast<AddressConvention>(
            ((eGrammar & ~kEnglishBit) >> kConventionShift) - kConventionOffset);
    }

    static Grammar setEnglishBit(Grammar eGrammar, bool bEnglish);
    static Grammar mergeToGrammar(Grammar eGrammar, AddressConvention eConv);

    static bool isPODF(Grammar eGrammar)
    {
        return extractFormulaLanguage(eGrammar) == css::sheet::FormulaLanguage::ODF_11;
    }

    static bool isODFF(Grammar eGrammar)
    {
        return extractFormulaLanguage(eGrammar) == css::sheet::FormulaLanguage::ODFF;
    }

    static bool isOOXML(Grammar eGrammar)
    {
        return extractFormulaLanguage(eGrammar) == css::sheet::FormulaLanguage::OOXML;
    }

    static bool isRefConventionOOXML(Grammar eGrammar)
    {
        return extractRefConvention(eGrammar)
            == AddressConvention::CONV_XL_OOX;
    }

    static bool isExcelSyntax(Grammar eGrammar)
    {
        switch (extractRefConvention(eGrammar))
        {
            case AddressConvention::CONV_XL_A1:
            case AddressConvention::CONV_XL_R1C1:
            case AddressConvention::CONV_XL_OOX:
                return true;
            default:
                return false;
        }
    }
};

} // namespace spreadsheetengine::compat::formula

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
