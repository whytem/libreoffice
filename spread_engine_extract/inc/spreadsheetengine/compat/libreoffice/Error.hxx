/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <formula/errorcodes.hxx>

#include <spreadsheetengine/api/Error.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::Error toApiError(FormulaError eError)
{
    switch (eError)
    {
        case FormulaError::IllegalArgument:
            return spreadsheetengine::api::Error::IllegalArgument;
        case FormulaError::DivisionByZero:
            return spreadsheetengine::api::Error::DivisionByZero;
        case FormulaError::IllegalFPOperation:
            return spreadsheetengine::api::Error::Domain;
        case FormulaError::StringOverflow:
            return spreadsheetengine::api::Error::StringOverflow;
        case FormulaError::NoValue:
            return spreadsheetengine::api::Error::NoValue;
        case FormulaError::NoConvergence:
            return spreadsheetengine::api::Error::NoConvergence;
        case FormulaError::NotAvailable:
            return spreadsheetengine::api::Error::NotAvailable;
        case FormulaError::NONE:
        default:
            return spreadsheetengine::api::Error::None;
    }
}

inline FormulaError toFormulaError(spreadsheetengine::api::Error eError)
{
    switch (eError)
    {
        case spreadsheetengine::api::Error::IllegalArgument:
            return FormulaError::IllegalArgument;
        case spreadsheetengine::api::Error::DivisionByZero:
            return FormulaError::DivisionByZero;
        case spreadsheetengine::api::Error::Domain:
            return FormulaError::IllegalFPOperation;
        case spreadsheetengine::api::Error::StringOverflow:
            return FormulaError::StringOverflow;
        case spreadsheetengine::api::Error::NoValue:
            return FormulaError::NoValue;
        case spreadsheetengine::api::Error::NoConvergence:
            return FormulaError::NoConvergence;
        case spreadsheetengine::api::Error::NotAvailable:
            return FormulaError::NotAvailable;
        case spreadsheetengine::api::Error::None:
        default:
            return FormulaError::NONE;
    }
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
