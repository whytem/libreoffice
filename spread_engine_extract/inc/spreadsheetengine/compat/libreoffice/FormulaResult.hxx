/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <formularesult.hxx>

#include <spreadsheetengine/api/FormulaResult.hxx>
#include <spreadsheetengine/compat/libreoffice/Error.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline sc::FormulaResultValue toLibreOfficeFormulaResultValue(
    const spreadsheetengine::api::formulavalue::FormulaResultValue& rValue)
{
    namespace seformula = spreadsheetengine::api::formulavalue;
    switch (rValue.meType)
    {
        case seformula::ValueType::Value:
            return sc::FormulaResultValue(rValue.mfValue);
        case seformula::ValueType::String:
            return sc::FormulaResultValue(
                svl::SharedString(toLibreOfficeString(rValue.maString)), rValue.mbMultiLine);
        case seformula::ValueType::Error:
            return sc::FormulaResultValue(toFormulaError(rValue.meError));
        case seformula::ValueType::Invalid:
        default:
            return sc::FormulaResultValue();
    }
}

inline spreadsheetengine::api::formulavalue::FormulaResultValue makeApiErrorFormulaResult(
    FormulaError eError)
{
    return spreadsheetengine::api::formulavalue::makeErrorResult(toApiError(eError));
}

inline spreadsheetengine::api::formulavalue::FormulaResultValue makeApiStringFormulaResult(
    const svl::SharedString& rValue, bool bMultiLine)
{
    return spreadsheetengine::api::formulavalue::makeStringResult(
        toApiString(rValue.getString()), bMultiLine);
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
