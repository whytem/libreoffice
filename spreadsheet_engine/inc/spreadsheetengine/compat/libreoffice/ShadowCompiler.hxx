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

namespace detail
{

inline void configureCompilerFromContext(
    ScCompiler& rCompiler, const spreadsheetengine::detail::compiler::CompileContext& rContext)
{
    switch (rContext.meExtendedErrorDetection)
    {
        case spreadsheetengine::detail::compiler::ExtendedErrorDetection::NameBreak:
            rCompiler.SetExtendedErrorDetection(
                ScCompiler::ExtendedErrorDetection::EXTENDED_ERROR_DETECTION_NAME_BREAK);
            break;
        case spreadsheetengine::detail::compiler::ExtendedErrorDetection::NameNoBreak:
            rCompiler.SetExtendedErrorDetection(
                ScCompiler::ExtendedErrorDetection::EXTENDED_ERROR_DETECTION_NAME_NO_BREAK);
            break;
        case spreadsheetengine::detail::compiler::ExtendedErrorDetection::None:
        default:
            break;
    }
}

inline std::unique_ptr<ScTokenArray> compileLegacyString(
    ScDocument& rDocument, const spreadsheetengine::detail::compiler::CompileRequest& rRequest,
    const css::uno::Sequence<css::sheet::ExternalLinkInfo>& rExternalLinks)
{
    const formula::FormulaGrammar::Grammar eGrammar = toLibreOfficeGrammar(rRequest.maContext.maGrammar);
    const ScAddress aBaseAddress = toLibreOfficeAddress(rRequest.maContext.maBaseAddress);
    ScCompiler aCompiler(rDocument, aBaseAddress, eGrammar,
        rRequest.maContext.mbComputeImplicitIntersection, rRequest.maContext.mbMatrixFormula);
    configureCompilerFromContext(aCompiler, rRequest.maContext);
    if (rExternalLinks.hasElements())
        aCompiler.SetExternalLinks(rExternalLinks);

    return rRequest.maSource.hasNamespace()
               ? aCompiler.CompileString(
                     toLibreOfficeString(rRequest.maSource.maFormula),
                     toLibreOfficeString(rRequest.maSource.maNamespace))
               : aCompiler.CompileString(toLibreOfficeString(rRequest.maSource.maFormula));
}

} // namespace detail

struct ShadowCompileArtifacts
{
    spreadsheetengine::detail::compiler::CompileStatus maStatus;
    std::unique_ptr<ScTokenArray> mxLegacyTokenArray;

    explicit operator bool() const
    {
        return static_cast<bool>(maStatus) && static_cast<bool>(mxLegacyTokenArray);
    }
};

struct BridgedCompileArtifacts
{
    spreadsheetengine::detail::compiler::CompileStatus maStatus;
    std::unique_ptr<ScTokenArray> mxTokenArray;

    explicit operator bool() const
    {
        return static_cast<bool>(maStatus) && static_cast<bool>(mxTokenArray);
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

    aArtifacts.mxLegacyTokenArray = detail::compileLegacyString(rDocument, rRequest, rExternalLinks);

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

inline BridgedCompileArtifacts compileFormulaToTokenArray(
    ScDocument& rDocument, const spreadsheetengine::detail::compiler::CompileRequest& rRequest,
    const css::uno::Sequence<css::sheet::ExternalLinkInfo>& rExternalLinks = {})
{
    BridgedCompileArtifacts aArtifacts;
    auto aShadowArtifacts = shadowCompileFormula(rDocument, rRequest, rExternalLinks);
    aArtifacts.maStatus = aShadowArtifacts.maStatus;
    if (!aShadowArtifacts)
        return aArtifacts;

    auto aExported = exportCompiledFormula(aShadowArtifacts.maStatus.maFormula, rDocument);
    if (!aExported)
    {
        aArtifacts.maStatus.mnFailureIndex = aExported.mnFailureIndex;
        aArtifacts.maStatus.maFailureMessage = toApiString(aExported.maFailureMessage);
        return aArtifacts;
    }

    aArtifacts.mxTokenArray = std::move(aExported.mxTokenArray);
    return aArtifacts;
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
