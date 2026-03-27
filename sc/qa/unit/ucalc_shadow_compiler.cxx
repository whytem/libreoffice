/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "helper/qahelper.hxx"

#include <compiler.hxx>
#include <dbdata.hxx>
#include <docoptio.hxx>
#include <externalrefmgr.hxx>
#include <formula/grammar.hxx>
#include <rangelst.hxx>
#include <rangenam.hxx>
#include <spreadsheetengine/compat/libreoffice/CompileHost.hxx>
#include <spreadsheetengine/compat/libreoffice/ShadowCompiler.hxx>

namespace
{

class TestShadowCompiler : public ScUcalcTestBase
{
};

template <typename Predicate>
const spreadsheetengine::detail::token::Token* findTokenIf(
    const spreadsheetengine::detail::token::CompiledFormula& rFormula, Predicate aPredicate)
{
    for (const auto& rToken : rFormula.maTokens)
    {
        if (aPredicate(rToken))
            return &rToken;
    }
    return nullptr;
}

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
    const OUString& rFormula, formula::FormulaGrammar::Grammar eGrammar)
{
    spreadsheetengine::detail::compiler::CompileRequest aRequest;
    aRequest.maSource.maFormula = std::u16string_view(rFormula.getStr(), rFormula.getLength());
    aRequest.maContext = spreadsheetengine::compat::libreoffice::makeCompileContext(rPos, eGrammar);
    aRequest.maHosts = rHost.hosts();
    return aRequest;
}

} // namespace

CPPUNIT_TEST_FIXTURE(TestShadowCompiler, testRejectsIncompleteHostBundle)
{
    using spreadsheetengine::compat::libreoffice::shadowCompileFormula;

    m_pDoc->InsertTab(0, u"Sheet1"_ustr);

    spreadsheetengine::detail::compiler::CompileRequest aRequest;
    aRequest.maSource.maFormula = u"=1+2";
    aRequest.maContext
        = spreadsheetengine::compat::libreoffice::makeCompileContext(
            ScAddress(0, 0, 0), getEnglishOooGrammar());

    const auto aArtifacts = shadowCompileFormula(*m_pDoc, aRequest);
    CPPUNIT_ASSERT(!aArtifacts);
    CPPUNIT_ASSERT(aArtifacts.maStatus.maFailureMessage == u"incomplete compile host bundle");

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestShadowCompiler, testShadowCompileNamesAndDbRanges)
{
    using spreadsheetengine::compat::libreoffice::DocumentCompileHost;
    using spreadsheetengine::compat::libreoffice::shadowCompileFormula;
    using spreadsheetengine::detail::token::Kind;
    using spreadsheetengine::detail::token::NameData;
    using spreadsheetengine::detail::token::DatabaseRangeData;

    m_pDoc->InsertTab(0, u"Sheet1"_ustr);
    m_pDoc->InsertTab(1, u"Sheet2"_ustr);

    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"GlobalMetric"_ustr, u"$Sheet1.$A$1"_ustr)));
    CPPUNIT_ASSERT(m_pDoc->GetRangeName(1)->insert(
        new ScRangeData(*m_pDoc, u"LocalMetric"_ustr, u"$Sheet2.$B$2"_ustr)));

    auto pDbData = std::make_unique<ScDBData>(u"SalesTable"_ustr, 0, 0, 0, 3, 5);
    CPPUNIT_ASSERT(m_pDoc->GetDBCollection()->getNamedDBs().insert(std::move(pDbData)));
    ScDBData* pInserted = m_pDoc->GetDBCollection()->getNamedDBs().findByUpperName(u"SALESTABLE"_ustr);
    CPPUNIT_ASSERT(pInserted);

    DocumentCompileHost aHost(*m_pDoc);

    const auto aGlobalArtifacts
        = shadowCompileFormula(*m_pDoc, makeRequest(aHost, ScAddress(1, 1, 0), u"=GlobalMetric"_ustr,
                                      getEnglishOooGrammar()));
    CPPUNIT_ASSERT(aGlobalArtifacts);
    CPPUNIT_ASSERT(aGlobalArtifacts.maStatus.mbUsedLegacyBackend);
    CPPUNIT_ASSERT(aGlobalArtifacts.mxLegacyTokenArray);
    const auto* pGlobalName = findTokenIf(
        aGlobalArtifacts.maStatus.maFormula, [](const auto& rToken) { return rToken.meKind == Kind::RangeName; });
    CPPUNIT_ASSERT(pGlobalName);
    const auto& rGlobalData = std::get<NameData>(pGlobalName->maPayload);
    CPPUNIT_ASSERT_EQUAL(sal_Int16(-1), rGlobalData.mnSheet);

    const auto aLocalArtifacts
        = shadowCompileFormula(*m_pDoc, makeRequest(aHost, ScAddress(1, 1, 1), u"=LocalMetric"_ustr,
                                      getEnglishOooGrammar()));
    CPPUNIT_ASSERT(aLocalArtifacts);
    const auto* pLocalName = findTokenIf(
        aLocalArtifacts.maStatus.maFormula, [](const auto& rToken) { return rToken.meKind == Kind::RangeName; });
    CPPUNIT_ASSERT(pLocalName);
    const auto& rLocalData = std::get<NameData>(pLocalName->maPayload);
    CPPUNIT_ASSERT_EQUAL(sal_Int16(1), rLocalData.mnSheet);

    const auto aDbArtifacts
        = shadowCompileFormula(*m_pDoc, makeRequest(aHost, ScAddress(1, 1, 0), u"=SUM(SalesTable)"_ustr,
                                      getEnglishOooGrammar()));
    CPPUNIT_ASSERT(aDbArtifacts);
    const auto* pDbToken = findTokenIf(
        aDbArtifacts.maStatus.maFormula,
        [](const auto& rToken) { return rToken.meKind == Kind::DatabaseRange; });
    CPPUNIT_ASSERT(pDbToken);
    const auto& rDbData = std::get<DatabaseRangeData>(pDbToken->maPayload);
    CPPUNIT_ASSERT_EQUAL(pInserted->GetIndex(), rDbData.mnIndex);

    m_pDoc->DeleteTab(1);
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestShadowCompiler, testShadowCompileLookupSpecialCases)
{
    using spreadsheetengine::compat::libreoffice::DocumentCompileHost;
    using spreadsheetengine::compat::libreoffice::shadowCompileFormula;
    using spreadsheetengine::detail::token::ExternalNameData;
    using spreadsheetengine::detail::token::Kind;
    using spreadsheetengine::detail::token::TableRefData;

    m_pDoc->InsertTab(0, u"Sheet1"_ustr);

    auto pDbData = std::make_unique<ScDBData>(u"SalesTable"_ustr, 0, 0, 0, 3, 5);
    pDbData->SetTableColumnNames({ u"Region"_ustr, u"Amount"_ustr, u"Delta"_ustr, u"Flag"_ustr });
    CPPUNIT_ASSERT(m_pDoc->GetDBCollection()->getNamedDBs().insert(std::move(pDbData)));

    m_pDoc->SetString(0, 0, 0, u"Revenue"_ustr);
    m_pDoc->SetValue(0, 1, 0, 10.0);
    m_pDoc->SetValue(0, 2, 0, 11.0);
    m_pDoc->GetColNameRanges()->Append(ScRangePair(
        ScRange(0, 0, 0, 0, 0, 0), ScRange(0, 1, 0, 0, 2, 0)));
    ScDocOptions aOptions = m_pDoc->GetDocOptions();
    aOptions.SetLookUpColRowNames(true);
    m_pDoc->SetDocOptions(aOptions);

    static OUString constexpr aExternalFile(u"file:///shadow-compile-external.fake"_ustr);
    ScExternalRefManager* pRefMgr = m_pDoc->GetExternalRefManager();
    CPPUNIT_ASSERT(pRefMgr);
    const sal_uInt16 nFileId = pRefMgr->getExternalFileId(aExternalFile);
    ScTokenArray aRangeTokens(*m_pDoc);
    aRangeTokens.AddDouble(42.0);
    pRefMgr->storeRangeNameTokens(nFileId, u"ExternalMetric"_ustr, aRangeTokens);
    const ScCompiler::Convention* pConvention
        = ScCompiler::GetRefConvention(formula::FormulaGrammar::CONV_OOO);
    CPPUNIT_ASSERT(pConvention);
    const OUString aExternalSymbol
        = pConvention->makeExternalNameStr(nFileId, aExternalFile, u"ExternalMetric"_ustr);

    DocumentCompileHost aHost(*m_pDoc);

    const auto aTableArtifacts
        = shadowCompileFormula(*m_pDoc, makeRequest(aHost, ScAddress(1, 1, 0),
                                      u"=SUM(SalesTable[#Data])"_ustr,
                                      formula::FormulaGrammar::GRAM_ENGLISH_XL_A1));
    CPPUNIT_ASSERT(aTableArtifacts);
    const auto* pTableToken = findTokenIf(
        aTableArtifacts.maStatus.maFormula,
        [](const auto& rToken) { return rToken.meKind == Kind::TableRef; });
    CPPUNIT_ASSERT(pTableToken);
    const auto& rTableData = std::get<TableRefData>(pTableToken->maPayload);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::token::TableRefItem::Data, rTableData.meItem);

    const auto aColRowArtifacts
        = shadowCompileFormula(*m_pDoc, makeRequest(aHost, ScAddress(1, 1, 0), u"='Revenue'"_ustr,
                                      getEnglishOooGrammar()));
    CPPUNIT_ASSERT(aColRowArtifacts);
    CPPUNIT_ASSERT(findTokenIf(
        aColRowArtifacts.maStatus.maFormula,
        [](const auto& rToken) { return rToken.meKind == Kind::ColRowName; }));

    const auto aExternalArtifacts
        = shadowCompileFormula(*m_pDoc, makeRequest(aHost, ScAddress(1, 1, 0), aExternalSymbol,
                                      getEnglishOooGrammar()));
    CPPUNIT_ASSERT(aExternalArtifacts);
    const auto* pExternalToken = findTokenIf(
        aExternalArtifacts.maStatus.maFormula,
        [](const auto& rToken) { return rToken.meKind == Kind::ExternalName; });
    CPPUNIT_ASSERT(pExternalToken);
    const auto& rExternalData = std::get<ExternalNameData>(pExternalToken->maPayload);
    CPPUNIT_ASSERT_EQUAL(nFileId, rExternalData.mnFileId);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
