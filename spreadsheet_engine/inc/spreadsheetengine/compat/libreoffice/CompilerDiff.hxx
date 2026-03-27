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

#include <spreadsheetengine/compat/libreoffice/Address.hxx>
#include <spreadsheetengine/compat/libreoffice/ShadowCompiler.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/compat/libreoffice/TokenBridge.hxx>
#include <spreadsheetengine/detail/TokenStringifier.hxx>

namespace spreadsheetengine::compat::libreoffice
{

struct LegacyCompileArtifacts
{
    TokenImportStatus maImported;
    std::unique_ptr<ScTokenArray> mxTokenArray;

    explicit operator bool() const
    {
        return static_cast<bool>(maImported) && static_cast<bool>(mxTokenArray);
    }
};

struct CompilerDiffArtifacts
{
    LegacyCompileArtifacts maLegacy;
    ShadowCompileArtifacts maShadow;
    TokenExportStatus maRoundTripExport;
    TokenImportStatus maRoundTripImport;
    OUString maMismatchMessage;
    bool mbCanonicalMatch = false;
    bool mbHashMatch = false;
    bool mbLegacyRoundTripTokenEqual = false;
    bool mbMetadataMatch = false;

    explicit operator bool() const
    {
        return static_cast<bool>(maLegacy) && static_cast<bool>(maShadow)
               && static_cast<bool>(maRoundTripExport) && static_cast<bool>(maRoundTripImport)
               && maMismatchMessage.isEmpty() && mbCanonicalMatch && mbHashMatch
               && mbLegacyRoundTripTokenEqual && mbMetadataMatch;
    }
};

inline LegacyCompileArtifacts compileLegacyFormula(
    ScDocument& rDocument, const spreadsheetengine::detail::compiler::CompileRequest& rRequest,
    const css::uno::Sequence<css::sheet::ExternalLinkInfo>& rExternalLinks = {})
{
    LegacyCompileArtifacts aArtifacts;

    const formula::FormulaGrammar::Grammar eGrammar = toLibreOfficeGrammar(rRequest.maContext.maGrammar);
    if (eGrammar == formula::FormulaGrammar::GRAM_UNSPECIFIED)
    {
        aArtifacts.maImported.maFailureMessage = u"legacy compile request has unspecified grammar"_ustr;
        return aArtifacts;
    }

    aArtifacts.mxTokenArray = detail::compileLegacyString(rDocument, rRequest, rExternalLinks);

    if (!aArtifacts.mxTokenArray)
    {
        aArtifacts.maImported.maFailureMessage = u"legacy Calc compiler returned no token array"_ustr;
        return aArtifacts;
    }

    aArtifacts.maImported = importCompiledFormula(*aArtifacts.mxTokenArray);
    return aArtifacts;
}

inline CompilerDiffArtifacts diffShadowCompileAgainstLegacy(
    ScDocument& rDocument, const spreadsheetengine::detail::compiler::CompileRequest& rRequest,
    const css::uno::Sequence<css::sheet::ExternalLinkInfo>& rExternalLinks = {})
{
    using spreadsheetengine::detail::token::hashCompiledFormula;

    CompilerDiffArtifacts aArtifacts;
    aArtifacts.maLegacy = compileLegacyFormula(rDocument, rRequest, rExternalLinks);
    if (!aArtifacts.maLegacy)
    {
        aArtifacts.maMismatchMessage = u"legacy compile/import failed: "_ustr
                                      + aArtifacts.maLegacy.maImported.maFailureMessage;
        return aArtifacts;
    }

    aArtifacts.maShadow = shadowCompileFormula(rDocument, rRequest, rExternalLinks);
    if (!aArtifacts.maShadow)
    {
        aArtifacts.maMismatchMessage = u"shadow compile failed: "_ustr
                                      + toLibreOfficeString(aArtifacts.maShadow.maStatus.maFailureMessage);
        return aArtifacts;
    }

    aArtifacts.mbCanonicalMatch
        = aArtifacts.maLegacy.maImported.maFormula == aArtifacts.maShadow.maStatus.maFormula;
    if (!aArtifacts.mbCanonicalMatch)
    {
        aArtifacts.maMismatchMessage
            = u"canonical compiled formula mismatch: legacy="_ustr
              + toLibreOfficeString(
                  spreadsheetengine::detail::tokenstringifier::compiledFormulaToDiagnosticString(
                      aArtifacts.maLegacy.maImported.maFormula))
              + u" shadow="_ustr
              + toLibreOfficeString(
                  spreadsheetengine::detail::tokenstringifier::compiledFormulaToDiagnosticString(
                      aArtifacts.maShadow.maStatus.maFormula));
        return aArtifacts;
    }

    aArtifacts.mbHashMatch
        = hashCompiledFormula(aArtifacts.maLegacy.maImported.maFormula)
          == hashCompiledFormula(aArtifacts.maShadow.maStatus.maFormula);
    if (!aArtifacts.mbHashMatch)
    {
        aArtifacts.maMismatchMessage = u"canonical compiled formula hash mismatch"_ustr;
        return aArtifacts;
    }

    aArtifacts.maRoundTripExport
        = exportCompiledFormula(aArtifacts.maShadow.maStatus.maFormula, rDocument);
    if (!aArtifacts.maRoundTripExport)
    {
        aArtifacts.maMismatchMessage = u"canonical export failed: "_ustr
                                      + aArtifacts.maRoundTripExport.maFailureMessage;
        return aArtifacts;
    }

    aArtifacts.mbLegacyRoundTripTokenEqual
        = tokenArraysEqualForBridge(*aArtifacts.maLegacy.mxTokenArray,
                                    *aArtifacts.maRoundTripExport.mxTokenArray);
    if (!aArtifacts.mbLegacyRoundTripTokenEqual)
    {
        aArtifacts.maMismatchMessage = u"legacy and roundtrip token arrays differ"_ustr;
        return aArtifacts;
    }

    aArtifacts.mbMetadataMatch
        = aArtifacts.maLegacy.mxTokenArray->GetCodeError()
              == aArtifacts.maRoundTripExport.mxTokenArray->GetCodeError()
          && aArtifacts.maLegacy.mxTokenArray->GetRecalcMode()
                 == aArtifacts.maRoundTripExport.mxTokenArray->GetRecalcMode()
          && aArtifacts.maLegacy.mxTokenArray->IsHyperLink()
                 == aArtifacts.maRoundTripExport.mxTokenArray->IsHyperLink()
          && aArtifacts.maLegacy.mxTokenArray->IsFromRangeName()
                 == aArtifacts.maRoundTripExport.mxTokenArray->IsFromRangeName()
          && aArtifacts.maLegacy.mxTokenArray->IsShareable()
                 == aArtifacts.maRoundTripExport.mxTokenArray->IsShareable();
    if (!aArtifacts.mbMetadataMatch)
    {
        aArtifacts.maMismatchMessage = u"legacy and roundtrip token metadata differ"_ustr;
        return aArtifacts;
    }

    aArtifacts.maRoundTripImport = importCompiledFormula(*aArtifacts.maRoundTripExport.mxTokenArray);
    if (!aArtifacts.maRoundTripImport)
    {
        aArtifacts.maMismatchMessage = u"roundtrip import failed: "_ustr
                                      + aArtifacts.maRoundTripImport.maFailureMessage;
        return aArtifacts;
    }

    if (!(aArtifacts.maRoundTripImport.maFormula == aArtifacts.maShadow.maStatus.maFormula))
    {
        aArtifacts.maMismatchMessage
            = u"roundtrip canonical import mismatch: shadow="_ustr
              + toLibreOfficeString(
                  spreadsheetengine::detail::tokenstringifier::compiledFormulaToDiagnosticString(
                      aArtifacts.maShadow.maStatus.maFormula))
              + u" roundtrip="_ustr
              + toLibreOfficeString(
                  spreadsheetengine::detail::tokenstringifier::compiledFormulaToDiagnosticString(
                      aArtifacts.maRoundTripImport.maFormula));
        return aArtifacts;
    }

    return aArtifacts;
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
