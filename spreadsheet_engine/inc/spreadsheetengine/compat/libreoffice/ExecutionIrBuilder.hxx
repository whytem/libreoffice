/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <spreadsheetengine/compat/libreoffice/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/ShadowCompiler.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrBuilder.hxx>

namespace spreadsheetengine::compat::libreoffice
{

namespace irexecdetail
{

[[nodiscard]] inline spreadsheetengine::detail::substrate::ExecutionIrCompileArtifacts
compileExecutionIrFormula(ScDocument& rDoc,
    spreadsheetengine::compat::libreoffice::DocumentCompileHost& rHost,
    const spreadsheetengine::detail::substrate::ShadowCellRecord& rCell,
    formula::FormulaGrammar::Grammar eGrammar,
    const css::uno::Sequence<css::sheet::ExternalLinkInfo>& rExternalLinks = {})
{
    spreadsheetengine::detail::substrate::ExecutionIrCompileArtifacts aArtifacts;
    if (!rCell.moFormula)
    {
        aArtifacts.maFailureMessage = u"non-formula cell passed to execution IR compiler";
        return aArtifacts;
    }

    spreadsheetengine::detail::compiler::CompileRequest aRequest;
    aRequest.maSource.maFormula = rCell.moFormula->maFormulaSource;
    aRequest.maContext = makeCompileContext(toLibreOfficeAddress(rCell.maId.maAddress), eGrammar);
    aRequest.maHosts = rHost.hosts();

    const auto aShadowArtifacts = shadowCompileFormula(rDoc, aRequest, rExternalLinks);
    if (!aShadowArtifacts)
    {
        aArtifacts.mnFailureIndex = aShadowArtifacts.maStatus.mnFailureIndex;
        aArtifacts.maFailureMessage = aShadowArtifacts.maStatus.maFailureMessage;
        return aArtifacts;
    }

    aArtifacts.moFormula = aShadowArtifacts.maStatus.maFormula;
    return aArtifacts;
}

[[nodiscard]] inline auto makeCalcExecutionIrCompileCallback(ScDocument& rDoc,
    spreadsheetengine::compat::libreoffice::DocumentCompileHost& rHost,
    formula::FormulaGrammar::Grammar eGrammar,
    const css::uno::Sequence<css::sheet::ExternalLinkInfo>& rExternalLinks = {})
{
    return [&rDoc, &rHost, eGrammar, rExternalLinks](
               const spreadsheetengine::detail::substrate::ShadowCellRecord& rCell) {
        return compileExecutionIrFormula(rDoc, rHost, rCell, eGrammar, rExternalLinks);
    };
}

} // namespace irexecdetail

[[nodiscard]] inline spreadsheetengine::detail::substrate::ExecutionIrWorkbookShadow
buildExecutionIrWorkbookShadow(
    const spreadsheetengine::detail::substrate::ComputationalWorkbookShadow& rShadow,
    ScDocument& rDoc,
    const css::uno::Sequence<css::sheet::ExternalLinkInfo>& rExternalLinks = {})
{
    spreadsheetengine::compat::libreoffice::DocumentCompileHost aHost(rDoc);
    const auto eGrammar = toLibreOfficeGrammar(rShadow.maGrammar);
    return spreadsheetengine::detail::substrate::buildExecutionIrWorkbookShadow(
        rShadow, irexecdetail::makeCalcExecutionIrCompileCallback(
                     rDoc, aHost, eGrammar, rExternalLinks));
}

[[nodiscard]] inline spreadsheetengine::detail::substrate::ExecutionIrWorkbookShadow
buildExecutionIrWorkbookShadow(
    const spreadsheetengine::detail::facade::WorkbookFacade& rFacade, ScDocument& rDoc,
    const css::uno::Sequence<css::sheet::ExternalLinkInfo>& rExternalLinks = {})
{
    const auto aShadow = buildComputationalWorkbookShadow(rFacade, rDoc);
    return buildExecutionIrWorkbookShadow(aShadow, rDoc, rExternalLinks);
}

} // namespace spreadsheetengine::compat::libreoffice

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
