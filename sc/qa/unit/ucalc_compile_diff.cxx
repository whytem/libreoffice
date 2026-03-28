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
#include <externalrefmgr.hxx>
#include <formula/grammar.hxx>
#include <spreadsheetengine/compat/libreoffice/CompileHost.hxx>
#include <spreadsheetengine/compat/libreoffice/CompilerDiff.hxx>
#include <spreadsheetengine/compat/libreoffice/Grammar.hxx>
#include <spreadsheetengine/compat/libreoffice/ShadowCompiler.hxx>
#include <spreadsheetengine/detail/TokenStringifier.hxx>
#include <spreadsheetengine/detail/WorkbookCompileHost.hxx>
#include <spreadsheetengine/detail/WorkbookCompilerLowering.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>

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

spreadsheetengine::core::workbook::Workbook makeStandaloneWorkbook()
{
    spreadsheetengine::core::workbook::Workbook aWorkbook;
    spreadsheetengine::core::workbook::Sheet aSheet;
    aSheet.maName = u"Sheet1";
    aWorkbook.maSheets.push_back(std::move(aSheet));
    return aWorkbook;
}

void addStandaloneNamedRange(spreadsheetengine::core::workbook::Workbook& rWorkbook,
    std::u16string_view rName, std::u16string_view rRange)
{
    spreadsheetengine::core::workbook::NamedRange aRange;
    aRange.maName = rName;
    aRange.maCellRangeAddress = rRange;
    rWorkbook.maNamedRanges.push_back(std::move(aRange));
}

void assertStandaloneLowerMetadataMatchesCalc(ScDocument& rDoc,
    spreadsheetengine::compat::libreoffice::DocumentCompileHost& rCalcHost,
    const spreadsheetengine::core::workbook::Workbook& rWorkbook, const ScAddress& rPos,
    std::u16string_view rFormula, formula::FormulaGrammar::Grammar eGrammar,
    std::u16string_view rLabel)
{
    using spreadsheetengine::compat::libreoffice::shadowCompileFormula;
    using spreadsheetengine::detail::compiler::WorkbookCompileHost;
    using spreadsheetengine::detail::compiler::lowerFormulaSource;

    const auto aShadow = shadowCompileFormula(
        rDoc, makeRequest(rCalcHost, rPos, OUString(rFormula), eGrammar));
    CPPUNIT_ASSERT_MESSAGE(OUString(rLabel).toUtf8().getStr(), static_cast<bool>(aShadow));

    WorkbookCompileHost aWorkbookHost(rWorkbook);
    const auto aContext = spreadsheetengine::detail::compiler::makeWorkbookCompileContext(
        { static_cast<spreadsheetengine::api::SheetId>(rPos.Tab()),
          static_cast<spreadsheetengine::api::ColumnIndex>(rPos.Col()),
          static_cast<spreadsheetengine::api::RowIndex>(rPos.Row()) },
        spreadsheetengine::compat::libreoffice::toApiGrammar(eGrammar));
    const auto aLowered
        = lowerFormulaSource(rFormula, aWorkbookHost, aContext);
    CPPUNIT_ASSERT_MESSAGE(OUString(rLabel).toUtf8().getStr(), static_cast<bool>(aLowered));
    CPPUNIT_ASSERT_EQUAL(
        aShadow.maStatus.maFormula.mnCodeError, aLowered.maFormula.mnCodeError);
    CPPUNIT_ASSERT_EQUAL(aShadow.maStatus.maFormula.moXmlFormulaSource.has_value(),
        aLowered.maFormula.moXmlFormulaSource.has_value());
    CPPUNIT_ASSERT(!aShadow.maStatus.maFormula.maTokens.empty());
    CPPUNIT_ASSERT(!aLowered.maFormula.maTokens.empty());
}

void assertStandaloneLowerTokensExactlyMatchCalc(ScDocument& rDoc,
    spreadsheetengine::compat::libreoffice::DocumentCompileHost& rCalcHost,
    const spreadsheetengine::core::workbook::Workbook& rWorkbook, const ScAddress& rPos,
    std::u16string_view rFormula, formula::FormulaGrammar::Grammar eGrammar,
    std::u16string_view rLabel)
{
    using spreadsheetengine::compat::libreoffice::shadowCompileFormula;
    using spreadsheetengine::detail::compiler::WorkbookCompileHost;
    using spreadsheetengine::detail::compiler::lowerFormulaSourceLexical;
    using spreadsheetengine::detail::tokenstringifier::compiledFormulaToDiagnosticString;

    const auto aShadow = shadowCompileFormula(
        rDoc, makeRequest(rCalcHost, rPos, OUString(rFormula), eGrammar));
    CPPUNIT_ASSERT_MESSAGE(OUString(rLabel).toUtf8().getStr(), static_cast<bool>(aShadow));

    WorkbookCompileHost aWorkbookHost(rWorkbook);
    const auto aContext = spreadsheetengine::detail::compiler::makeWorkbookCompileContext(
        { static_cast<spreadsheetengine::api::SheetId>(rPos.Tab()),
          static_cast<spreadsheetengine::api::ColumnIndex>(rPos.Col()),
          static_cast<spreadsheetengine::api::RowIndex>(rPos.Row()) },
        spreadsheetengine::compat::libreoffice::toApiGrammar(eGrammar));
    const auto aLowered = lowerFormulaSourceLexical(rFormula, aWorkbookHost, aContext);
    CPPUNIT_ASSERT_MESSAGE(OUString(rLabel).toUtf8().getStr(), static_cast<bool>(aLowered));

    const bool bEqual = aShadow.maStatus.maFormula.maTokens == aLowered.maFormula.maTokens;
    const OUString aMessage = OUString(rLabel) + u"\ncalc="_ustr
                              + spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                                  compiledFormulaToDiagnosticString(aShadow.maStatus.maFormula))
                              + u"\nstandalone="_ustr
                              + spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                                  compiledFormulaToDiagnosticString(aLowered.maFormula));
    CPPUNIT_ASSERT_MESSAGE(aMessage.toUtf8().getStr(), bEqual);
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
    if (pDoc->GetTableCount() == 0)
        CPPUNIT_ASSERT(pDoc->InsertTab(0, u"Sheet1"_ustr));

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

    static OUString constexpr aExternalFile(u"file:///compile-diff-external.fake"_ustr);
    ScExternalRefManager* pRefMgr = pDoc->GetExternalRefManager();
    CPPUNIT_ASSERT(pRefMgr);
    const sal_uInt16 nFileId = pRefMgr->getExternalFileId(aExternalFile);
    ScTokenArray aRangeTokens(*pDoc);
    aRangeTokens.AddDouble(42.0);
    pRefMgr->storeRangeNameTokens(nFileId, u"ExternalMetric"_ustr, aRangeTokens);
    const ScCompiler::Convention* pConvention
        = ScCompiler::GetRefConvention(formula::FormulaGrammar::CONV_OOO);
    CPPUNIT_ASSERT(pConvention);
    const OUString aExternalSymbol
        = pConvention->makeExternalNameStr(nFileId, aExternalFile, u"ExternalMetric"_ustr);

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
        makeRequest(aHost, ScAddress(0, 0, 0), u"=SUM( { 1 ; 2 ; 3 } )"_ustr,
                    getEnglishOooGrammar()),
        u"whitespace-sensitive array literal");
    assertShadowDiff(*pDoc,
        makeRequest(aHost, ScAddress(1, 1, 0), u"=SUM(SalesTable)"_ustr, getEnglishOooGrammar()),
        u"database range");
    assertShadowDiff(*pDoc,
        makeRequest(aHost, ScAddress(1, 1, 0), u"=SUM(SalesTable[#Data])"_ustr,
                    formula::FormulaGrammar::GRAM_ENGLISH_XL_A1),
        u"table reference");
    assertShadowDiff(*pDoc,
        makeRequest(aHost, ScAddress(2, 2, 0), u"='Revenue'"_ustr, getEnglishOooGrammar()),
        u"col-row name");
    assertShadowDiff(*pDoc,
        makeRequest(aHost, ScAddress(1, 1, 0), aExternalSymbol, getEnglishOooGrammar()),
        u"external name");
}

CPPUNIT_TEST_FIXTURE(TestCompileDiff, testEnabledFodsFormulaSmoke)
{
    ScDocument* pDoc = m_pDoc;
    CPPUNIT_ASSERT(pDoc);
    if (pDoc->GetTableCount() == 0)
        CPPUNIT_ASSERT(pDoc->InsertTab(0, u"Sheet1"_ustr));

    spreadsheetengine::compat::libreoffice::DocumentCompileHost aHost(*pDoc);

    const struct
    {
        std::u16string_view maLabel;
        ScAddress maPosition;
        formula::FormulaGrammar::Grammar meGrammar;
        std::u16string_view maFormula;
    } aSamples[] = {
        { u"logical/and.fods :: of:=AND(1;1)", ScAddress(0, 0, 0),
            getEnglishOooGrammar(), u"=AND(1;1)" },
        { u"mathematical/add.fods :: of:=-0.3+0.2+0.1", ScAddress(0, 0, 0),
            getEnglishOooGrammar(), u"=-0.3+0.2+0.1" },
        { u"text/concat.fods :: of:=COM.MICROSOFT.CONCAT(\"Good \";\"Morning \";\"Mrs. \";\"Doe\")",
            ScAddress(0, 0, 0), getEnglishOooGrammar(),
            u"=COM.MICROSOFT.CONCAT(\"Good \";\"Morning \";\"Mrs. \";\"Doe\")" },
        { u"date_time/datevalue.fods :: of:=DATEVALUE(\"Jan1, 2015\")", ScAddress(0, 0, 0),
            getEnglishOooGrammar(), u"=DATEVALUE(\"Jan1, 2015\")" },
        { u"information/formula.fods :: of:=FORMULA(A1)", ScAddress(0, 0, 0),
            getEnglishOooGrammar(), u"=FORMULA(A1)" },
        { u"spreadsheet/vlookup.fods :: of:=VLOOKUP(\"Cat\";A1:B2;2;0)", ScAddress(0, 0, 0),
            getEnglishOooGrammar(), u"=VLOOKUP(\"Cat\";A1:B2;2;0)" },
    };

    for (const auto& rSample : aSamples)
    {
        assertShadowDiff(*pDoc,
            makeRequest(aHost, rSample.maPosition, OUString(rSample.maFormula), rSample.meGrammar),
            rSample.maLabel);
    }
}

CPPUNIT_TEST_FIXTURE(TestCompileDiff, testStandaloneLoweringMetadataSmoke)
{
    ScDocument* pDoc = m_pDoc;
    CPPUNIT_ASSERT(pDoc);
    if (pDoc->GetTableCount() == 0)
        CPPUNIT_ASSERT(pDoc->InsertTab(0, u"Sheet1"_ustr));

    CPPUNIT_ASSERT(pDoc->GetRangeName()->insert(
        new ScRangeData(*pDoc, u"GlobalMetric"_ustr, u"$Sheet1.$A$1"_ustr)));

    auto aWorkbook = makeStandaloneWorkbook();
    addStandaloneNamedRange(aWorkbook, u"GlobalMetric", u"Sheet1.A1");

    spreadsheetengine::compat::libreoffice::DocumentCompileHost aHost(*pDoc);

    const struct
    {
        std::u16string_view maLabel;
        std::u16string_view maFormula;
    } aSamples[] = {
        { u"arithmetic", u"=-0.3+0.2+0.1" },
        { u"concat_operator", u"=\"Good \"&\"Morning\"" },
        { u"single_refs", u"=A1+B1" },
        { u"array_literal", u"={1;2;3}" },
        { u"error_literal", u"=#N/A" },
        { u"range_name", u"=GlobalMetric+1" },
    };

    for (const auto& rSample : aSamples)
    {
        assertStandaloneLowerMetadataMatchesCalc(*pDoc, aHost, aWorkbook, ScAddress(0, 0, 0),
            rSample.maFormula, getEnglishOooGrammar(), rSample.maLabel);
    }
}

CPPUNIT_TEST_FIXTURE(TestCompileDiff, testStandaloneLoweringOperatorTokenParitySmoke)
{
    ScDocument* pDoc = m_pDoc;
    CPPUNIT_ASSERT(pDoc);
    if (pDoc->GetTableCount() == 0)
        CPPUNIT_ASSERT(pDoc->InsertTab(0, u"Sheet1"_ustr));

    CPPUNIT_ASSERT(pDoc->GetRangeName()->insert(
        new ScRangeData(*pDoc, u"GlobalMetric"_ustr, u"$Sheet1.$A$1"_ustr)));

    auto aWorkbook = makeStandaloneWorkbook();
    addStandaloneNamedRange(aWorkbook, u"GlobalMetric", u"Sheet1.A1");

    spreadsheetengine::compat::libreoffice::DocumentCompileHost aHost(*pDoc);

    const struct
    {
        std::u16string_view maLabel;
        std::u16string_view maFormula;
    } aSamples[] = {
        { u"single_ref_add", u"=A1+B1" },
        { u"concat_operator", u"=A1&B1" },
        { u"compare_equal", u"=A1=B1" },
        { u"compare_less_equal", u"=A1<=B1" },
        { u"unary_minus", u"=-A1" },
        { u"double_ref_push", u"=A1:B2" },
        { u"range_name_add", u"=GlobalMetric+1" },
    };

    for (const auto& rSample : aSamples)
    {
        assertStandaloneLowerTokensExactlyMatchCalc(*pDoc, aHost, aWorkbook, ScAddress(0, 0, 0),
            rSample.maFormula, getEnglishOooGrammar(), rSample.maLabel);
    }
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
