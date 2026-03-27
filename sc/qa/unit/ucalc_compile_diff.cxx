/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "helper/qahelper.hxx"

#include <dbdata.hxx>
#include <docoptio.hxx>
#include <formula/grammar.hxx>
#include <spreadsheetengine/compat/libreoffice/CompileHost.hxx>
#include <spreadsheetengine/compat/libreoffice/CompilerDiff.hxx>

namespace
{

class TestCompileDiff : public ScUcalcTestBase
{
};

formula::FormulaGrammar::Grammar getEnglishOooGrammar()
{
    return static_cast<formula::FormulaGrammar::Grammar>(
        css::sheet::FormulaLanguage::ENGLISH
        | ((formula::FormulaGrammar::CONV_OOO + formula::FormulaGrammar::kConventionOffset)
           << formula::FormulaGrammar::kConventionShift)
        | formula::FormulaGrammar::kEnglishBit);
}

spreadsheetengine::detail::compiler::CompileRequest makeRequest(
    spreadsheetengine::compat::libreoffice::DocumentCompileHost& rHost, const ScAddress& rPos,
    const OUString& rFormula, formula::FormulaGrammar::Grammar eGrammar,
    const OUString& rNamespace = OUString())
{
    spreadsheetengine::detail::compiler::CompileRequest aRequest;
    aRequest.maSource.maFormula = std::u16string_view(rFormula.getStr(), rFormula.getLength());
    if (!rNamespace.isEmpty())
        aRequest.maSource.maNamespace = std::u16string_view(rNamespace.getStr(), rNamespace.getLength());
    aRequest.maContext = spreadsheetengine::compat::libreoffice::makeCompileContext(rPos, eGrammar);
    aRequest.maHosts = rHost.hosts();
    return aRequest;
}

OUString describeDiffFailure(const spreadsheetengine::compat::libreoffice::CompilerDiffArtifacts& rArtifacts)
{
    if (!rArtifacts.maMismatchMessage.isEmpty())
        return rArtifacts.maMismatchMessage;
    if (!rArtifacts.maLegacy)
        return u"legacy compile/import failed: "_ustr + rArtifacts.maLegacy.maImported.maFailureMessage;
    if (!rArtifacts.maShadow)
        return u"shadow compile failed: "_ustr
               + spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                   rArtifacts.maShadow.maStatus.maFailureMessage);
    if (!rArtifacts.maRoundTripExport)
        return u"roundtrip export failed: "_ustr + rArtifacts.maRoundTripExport.maFailureMessage;
    if (!rArtifacts.maRoundTripImport)
        return u"roundtrip import failed: "_ustr + rArtifacts.maRoundTripImport.maFailureMessage;
    return u"compiler diff failed"_ustr;
}

void assertShadowDiff(
    ScDocument& rDoc, const spreadsheetengine::detail::compiler::CompileRequest& rRequest,
    std::u16string_view rLabel,
    const css::uno::Sequence<css::sheet::ExternalLinkInfo>& rExternalLinks = {})
{
    const auto aDiff = spreadsheetengine::compat::libreoffice::diffShadowCompileAgainstLegacy(
        rDoc, rRequest, rExternalLinks);
    const OUString aMessage = OUString(rLabel) + u": "_ustr + describeDiffFailure(aDiff);
    CPPUNIT_ASSERT_MESSAGE(aMessage.toUtf8().getStr(), static_cast<bool>(aDiff));
}

} // namespace

CPPUNIT_TEST_FIXTURE(TestCompileDiff, testSyntheticCompilerDiffCorpus)
{
    ScDocument* pDoc = m_pDoc;
    CPPUNIT_ASSERT(pDoc);

    auto pDbData = std::make_unique<ScDBData>(u"SalesTable"_ustr, 0, 0, 0, 3, 5);
    pDbData->SetTableColumnNames({ u"Region"_ustr, u"Amount"_ustr, u"Delta"_ustr, u"Flag"_ustr });
    CPPUNIT_ASSERT(pDoc->GetDBCollection()->getNamedDBs().insert(std::move(pDbData)));

    pDoc->SetString(0, 0, 0, u"Revenue"_ustr);
    pDoc->SetValue(0, 1, 0, 10.0);
    pDoc->SetValue(0, 2, 0, 11.0);
    pDoc->GetColNameRanges()->Append(ScRangePair(
        ScRange(0, 0, 0, 0, 0, 0), ScRange(0, 1, 0, 0, 2, 0)));
    ScDocOptions aOptions = pDoc->GetDocOptions();
    aOptions.SetLookUpColRowNames(true);
    pDoc->SetDocOptions(aOptions);

    spreadsheetengine::compat::libreoffice::DocumentCompileHost aHost(*pDoc);

    assertShadowDiff(*pDoc, makeRequest(aHost, ScAddress(0, 0, 0), u"=1+2"_ustr, getEnglishOooGrammar()),
        u"plain arithmetic");
    assertShadowDiff(*pDoc,
        makeRequest(aHost, ScAddress(0, 0, 0), u"=SUM( A1 : A3 )"_ustr, getEnglishOooGrammar()),
        u"whitespace-sensitive sum");
    assertShadowDiff(*pDoc,
        makeRequest(aHost, ScAddress(0, 0, 0), u"=SUM({1;2;3})"_ustr, getEnglishOooGrammar()),
        u"array literal");
    assertShadowDiff(*pDoc,
        makeRequest(aHost, ScAddress(1, 1, 0), u"=SUM(SalesTable)"_ustr, getEnglishOooGrammar()),
        u"database range");
    assertShadowDiff(*pDoc,
        makeRequest(aHost, ScAddress(1, 1, 0), u"=SUM(SalesTable[#Data])"_ustr,
                    formula::FormulaGrammar::GRAM_ENGLISH_XL_A1),
        u"table reference");
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
