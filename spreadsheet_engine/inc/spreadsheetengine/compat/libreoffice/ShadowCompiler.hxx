/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <compiler.hxx>
#include <formula/grammar.hxx>
#include <tokenarray.hxx>

#include <com/sun/star/sheet/ExternalLinkInfo.hpp>

#include <memory>
#include <utility>

#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/CompileHost.hxx>
#include <spreadsheetengine/compat/libreoffice/Grammar.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/compat/libreoffice/TokenBridge.hxx>
#include <spreadsheetengine/detail/CompilerPipeline.hxx>

namespace spreadsheetengine::compat::libreoffice
{

struct ShadowCompileArtifacts
{
    spreadsheetengine::detail::compiler::CompileStatus maStatus;
    std::unique_ptr<ScTokenArray> mxLegacyTokenArray;

    explicit operator bool() const
    {
        return static_cast<bool>(maStatus) && static_cast<bool>(mxLegacyTokenArray);
    }
};

inline ShadowCompileArtifacts shadowCompileFormula(
    ScDocument& rDocument, const spreadsheetengine::detail::compiler::CompileRequest& rRequest,
    const css::uno::Sequence<css::sheet::ExternalLinkInfo>& rExternalLinks = {})
{
    using spreadsheetengine::detail::compiler::CompileStatus;

    ShadowCompileArtifacts aArtifacts;
    if (!spreadsheetengine::detail::compiler::hasCompleteHostBundle(rRequest.maHosts))
    {
        aArtifacts.maStatus.maFailureMessage = u"incomplete compile host bundle";
        return aArtifacts;
    }

    const formula::FormulaGrammar::Grammar eGrammar = toLibreOfficeGrammar(rRequest.maContext.maGrammar);
    if (eGrammar == formula::FormulaGrammar::GRAM_UNSPECIFIED)
    {
        aArtifacts.maStatus.maFailureMessage = u"compile request has unspecified grammar";
        return aArtifacts;
    }

    const ScAddress aBaseAddress = toLibreOfficeAddress(rRequest.maContext.maBaseAddress);
    ScCompiler aCompiler(rDocument, aBaseAddress, eGrammar);
    if (rExternalLinks.hasElements())
        aCompiler.SetExternalLinks(rExternalLinks);

    aArtifacts.mxLegacyTokenArray
        = rRequest.maSource.hasNamespace()
              ? aCompiler.CompileString(
                    toLibreOfficeString(rRequest.maSource.maFormula),
                    toLibreOfficeString(rRequest.maSource.maNamespace))
              : aCompiler.CompileString(toLibreOfficeString(rRequest.maSource.maFormula));

    if (!aArtifacts.mxLegacyTokenArray)
    {
        aArtifacts.maStatus.maFailureMessage = u"legacy Calc compiler returned no token array";
        return aArtifacts;
    }

    const auto aImported = importCompiledFormula(*aArtifacts.mxLegacyTokenArray);
    if (!aImported)
    {
        aArtifacts.maStatus.mnFailureIndex = aImported.mnFailureIndex;
        aArtifacts.maStatus.maFailureMessage = toApiString(aImported.maFailureMessage);
        aArtifacts.mxLegacyTokenArray.reset();
        return aArtifacts;
    }

    aArtifacts.maStatus.maFormula = aImported.maFormula;
    aArtifacts.maStatus.mbUsedLegacyBackend = true;
    return aArtifacts;
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
