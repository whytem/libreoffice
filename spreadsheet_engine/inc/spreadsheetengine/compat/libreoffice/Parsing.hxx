/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <document.hxx>
#include <interpretercontext.hxx>

#include <spreadsheetengine/api/Parsing.hxx>
#include <spreadsheetengine/compat/libreoffice/Host.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>

namespace spreadsheetengine::compat::libreoffice
{

inline spreadsheetengine::api::ValueResult<double> parseValueFromText(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const OUString& rValue)
{
    DocumentEvaluationHost aHost(rDoc, rContext);
    return spreadsheetengine::api::parsing::valueFromText(aHost, toApiString(rValue));
}

inline spreadsheetengine::api::ValueResult<double> parseDateValueFromText(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const OUString& rValue)
{
    DocumentEvaluationHost aHost(rDoc, rContext);
    return spreadsheetengine::api::parsing::dateValueFromText(aHost, toApiString(rValue));
}

inline spreadsheetengine::api::ValueResult<double> parseTimeValueFromText(
    const ScDocument& rDoc, ScInterpreterContext& rContext, const OUString& rValue)
{
    DocumentEvaluationHost aHost(rDoc, rContext);
    return spreadsheetengine::api::parsing::timeValueFromText(aHost, toApiString(rValue));
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
