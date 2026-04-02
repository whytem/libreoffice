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
#include <spreadsheetengine/api/ReferenceData.hxx>
#include <spreadsheetengine/compat/libreoffice/CompileHost.hxx>
#include <spreadsheetengine/compat/libreoffice/TokenBridge.hxx>
#include <spreadsheetengine/detail/BuiltinExternalNames.hxx>

namespace
{

class TestCompileHost : public ScUcalcTestBase
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

spreadsheetengine::api::refdata::SheetLimits getSheetLimits(const ScDocument& rDoc)
{
    return { rDoc.MaxCol(), rDoc.MaxRow(), static_cast<SCTAB>(rDoc.GetTableCount() - 1) };
}

} // namespace

CPPUNIT_TEST_FIXTURE(TestCompileHost, testRangeNameAndDatabaseLookup)
{
    using spreadsheetengine::compat::libreoffice::DocumentCompileHost;
    using spreadsheetengine::compat::libreoffice::makeCompileContext;

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
    const auto aHosts = aHost.hosts();
    CPPUNIT_ASSERT(aHosts.mpNameResolver);
    CPPUNIT_ASSERT(aHosts.mpDatabaseRangeResolver);

    const auto aSheet0Context = makeCompileContext(ScAddress(2, 3, 0), getEnglishOooGrammar());
    const auto aSheet1Context = makeCompileContext(ScAddress(2, 3, 1), getEnglishOooGrammar());

    const auto aGlobal
        = aHosts.mpNameResolver->lookupRangeName(u"GlobalMetric", std::nullopt, aSheet0Context);
    CPPUNIT_ASSERT(aGlobal);
    CPPUNIT_ASSERT_EQUAL(sal_Int16(-1), aGlobal->mnSheet);

    const auto aLocalOnSheet
        = aHosts.mpNameResolver->lookupRangeName(
            u"LocalMetric", spreadsheetengine::api::SheetId(1), aSheet0Context);
    CPPUNIT_ASSERT(aLocalOnSheet);
    CPPUNIT_ASSERT_EQUAL(sal_Int16(1), aLocalOnSheet->mnSheet);

    const auto aLocalOnBase
        = aHosts.mpNameResolver->lookupRangeName(u"LocalMetric", std::nullopt, aSheet1Context);
    CPPUNIT_ASSERT(aLocalOnBase);
    CPPUNIT_ASSERT_EQUAL(sal_Int16(1), aLocalOnBase->mnSheet);

    const auto aMissingLocal
        = aHosts.mpNameResolver->lookupRangeName(u"LocalMetric", std::nullopt, aSheet0Context);
    CPPUNIT_ASSERT(!aMissingLocal);

    const auto aDbRange
        = aHosts.mpDatabaseRangeResolver->lookupDatabaseRange(u"SalesTable", aSheet0Context);
    CPPUNIT_ASSERT(aDbRange);
    CPPUNIT_ASSERT_EQUAL(pInserted->GetIndex(), aDbRange->mnIndex);

    m_pDoc->DeleteTab(1);
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestCompileHost, testTableReferenceAndExternalNameLookup)
{
    using spreadsheetengine::compat::libreoffice::DocumentCompileHost;
    using spreadsheetengine::compat::libreoffice::makeCompileContext;

    m_pDoc->InsertTab(0, u"Sheet1"_ustr);

    auto pDbData = std::make_unique<ScDBData>(u"SalesTable"_ustr, 0, 0, 0, 3, 5);
    CPPUNIT_ASSERT(m_pDoc->GetDBCollection()->getNamedDBs().insert(std::move(pDbData)));
    ScDBData* pInserted = m_pDoc->GetDBCollection()->getNamedDBs().findByUpperName(u"SALESTABLE"_ustr);
    CPPUNIT_ASSERT(pInserted);

    const auto aContext = makeCompileContext(ScAddress(0, 0, 0), getEnglishOooGrammar());
    DocumentCompileHost aHost(*m_pDoc);

    ScCompiler aCompiler(*m_pDoc, ScAddress(0, 0, 0), getEnglishOooGrammar());
    ScCompiler::OpCodeMapPtr xSymbols
        = aCompiler.GetOpCodeMap(css::sheet::FormulaLanguage::ENGLISH);
    CPPUNIT_ASSERT(xSymbols);
    const OUString aDataItem = xSymbols->getSymbol(ocTableRefItemData);

    const auto aTableOnly = aHost.lookupTableReference(u"SalesTable", u"", aContext);
    CPPUNIT_ASSERT(aTableOnly);
    CPPUNIT_ASSERT_EQUAL(pInserted->GetIndex(), aTableOnly->mnIndex);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::token::TableRefItem::Table, aTableOnly->meItem);

    const auto aTableData = aHost.lookupTableReference(
        u"SalesTable", spreadsheetengine::compat::libreoffice::toApiString(aDataItem), aContext);
    CPPUNIT_ASSERT(aTableData);
    CPPUNIT_ASSERT_EQUAL(pInserted->GetIndex(), aTableData->mnIndex);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::token::TableRefItem::Data, aTableData->meItem);

    static OUString constexpr aExternalFile(u"file:///token-compile-host-external.fake"_ustr);
    ScExternalRefManager* pRefMgr = m_pDoc->GetExternalRefManager();
    CPPUNIT_ASSERT(pRefMgr);
    const sal_uInt16 nFileId = pRefMgr->getExternalFileId(aExternalFile);

    ScTokenArray aRangeTokens(*m_pDoc);
    aRangeTokens.AddDouble(42.0);
    pRefMgr->storeRangeNameTokens(nFileId, u"ExternalMetric"_ustr, aRangeTokens);

    const ScCompiler::Convention* pConvention
        = ScCompiler::GetRefConvention(formula::FormulaGrammar::CONV_OOO);
    CPPUNIT_ASSERT(pConvention);
    const OUString aSymbol
        = pConvention->makeExternalNameStr(nFileId, aExternalFile, u"ExternalMetric"_ustr);

    const auto aExternalName = aHost.lookupExternalName(
        spreadsheetengine::compat::libreoffice::toApiString(aSymbol), aContext);
    CPPUNIT_ASSERT(aExternalName);
    CPPUNIT_ASSERT_EQUAL(nFileId, aExternalName->mnFileId);
    CPPUNIT_ASSERT_EQUAL(std::size_t(14), aExternalName->maName.size());
    CPPUNIT_ASSERT_EQUAL(
        u"ExternalMetric"_ustr,
        OUString(aExternalName->maName.data(), static_cast<sal_Int32>(aExternalName->maName.size())));

    const auto aBuiltinWorkday = aHost.lookupExternalName(u"WORKDAY", aContext);
    CPPUNIT_ASSERT(aBuiltinWorkday);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::compiler::kBuiltinExternalNameCatalogId,
        aBuiltinWorkday->mnFileId);
    CPPUNIT_ASSERT_EQUAL(
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETWORKDAY"_ustr,
        OUString(aBuiltinWorkday->maName.data(),
            static_cast<sal_Int32>(aBuiltinWorkday->maName.size())));

    const auto aBuiltinConvertAlias = aHost.lookupExternalName(u"org.openoffice.convert", aContext);
    CPPUNIT_ASSERT(aBuiltinConvertAlias);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::compiler::kBuiltinExternalNameCatalogId,
        aBuiltinConvertAlias->mnFileId);
    CPPUNIT_ASSERT_EQUAL(
        u"COM.SUN.STAR.SHEET.ADDIN.ANALYSIS.GETCONVERT"_ustr,
        OUString(aBuiltinConvertAlias->maName.data(),
            static_cast<sal_Int32>(aBuiltinConvertAlias->maName.size())));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestCompileHost, testColRowNameLookup)
{
    using spreadsheetengine::compat::libreoffice::DocumentCompileHost;
    using spreadsheetengine::compat::libreoffice::makeCompileContext;

    m_pDoc->InsertTab(0, u"Sheet1"_ustr);
    m_pDoc->SetString(0, 0, 0, u"Revenue"_ustr);
    m_pDoc->SetValue(0, 1, 0, 10.0);
    m_pDoc->SetValue(0, 2, 0, 11.0);

    m_pDoc->GetColNameRanges()->Append(ScRangePair(
        ScRange(0, 0, 0, 0, 0, 0), ScRange(0, 1, 0, 0, 2, 0)));

    DocumentCompileHost aHost(*m_pDoc);
    const auto aContext = makeCompileContext(ScAddress(2, 2, 0), getEnglishOooGrammar());

    const auto aReference = aHost.lookupColRowName(u"Revenue", aContext);
    CPPUNIT_ASSERT(aReference);
    CPPUNIT_ASSERT(aReference->maFlags.mbColumnRelative);
    CPPUNIT_ASSERT(!aReference->maFlags.mbRowRelative);

    const auto aAbsolute = spreadsheetengine::api::refdata::toAbsoluteAddress(
        *aReference, getSheetLimits(*m_pDoc), aContext.maBaseAddress);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::SheetId(0), aAbsolute.mnSheet);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::ColumnIndex(0), aAbsolute.mnColumn);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::RowIndex(0), aAbsolute.mnRow);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestCompileHost, testCompileHelperBuildsExternalNameRpn)
{
    using spreadsheetengine::compat::libreoffice::compilehost::compileFormulaText;
    using spreadsheetengine::compat::libreoffice::compilehost::lowerTokenArray;

    m_pDoc->InsertTab(0, u"Sheet1"_ustr);

    static OUString constexpr aExternalFile(u"file:///token-compile-helper-external.fake"_ustr);
    ScExternalRefManager* pRefMgr = m_pDoc->GetExternalRefManager();
    CPPUNIT_ASSERT(pRefMgr);
    const sal_uInt16 nFileId = pRefMgr->getExternalFileId(aExternalFile);

    ScTokenArray aRangeTokens(*m_pDoc);
    aRangeTokens.AddDouble(42.0);
    pRefMgr->storeRangeNameTokens(nFileId, u"ExternalMetric"_ustr, aRangeTokens);

    const ScCompiler::Convention* pConvention
        = ScCompiler::GetRefConvention(formula::FormulaGrammar::CONV_OOO);
    CPPUNIT_ASSERT(pConvention);
    const OUString aSymbol
        = pConvention->makeExternalNameStr(nFileId, aExternalFile, u"ExternalMetric"_ustr);

    std::unique_ptr<ScTokenArray> xTokens(compileFormulaText(*m_pDoc, ScAddress(0, 0, 0),
        getEnglishOooGrammar(), formula::FormulaGrammar::CONV_OOO, aSymbol));
    CPPUNIT_ASSERT(xTokens);
    CPPUNIT_ASSERT_EQUAL(FormulaError::NONE, xTokens->GetCodeError());
    CPPUNIT_ASSERT(xTokens->GetLen() > 0);
    CPPUNIT_ASSERT(xTokens->FirstToken());
    CPPUNIT_ASSERT_EQUAL(formula::svExternalName, xTokens->FirstToken()->GetType());

    ScCompiler aLegacyCompiler(*m_pDoc, ScAddress(0, 0, 0), getEnglishOooGrammar());
    aLegacyCompiler.SetRefConvention(formula::FormulaGrammar::CONV_OOO);
    std::unique_ptr<ScTokenArray> xLegacyTokens(aLegacyCompiler.CompileString(aSymbol));
    CPPUNIT_ASSERT(xLegacyTokens);
    CPPUNIT_ASSERT_EQUAL(FormulaError::NONE, xLegacyTokens->GetCodeError());

    lowerTokenArray(*m_pDoc, ScAddress(0, 0, 0), getEnglishOooGrammar(),
        formula::FormulaGrammar::CONV_OOO, *xTokens);
    aLegacyCompiler.CompileTokenArray();
    CPPUNIT_ASSERT_EQUAL(xLegacyTokens->GetCodeLen(), xTokens->GetCodeLen());
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::tokenArraysEqualForBridge(*xLegacyTokens, *xTokens));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestCompileHost, testStringifyTokenArrayHelperMatchesLegacyCompiler)
{
    using spreadsheetengine::compat::libreoffice::compilehost::createFormulaStringFromTokenArray;

    m_pDoc->InsertTab(0, u"Sheet1"_ustr);

    static OUString constexpr aExternalFile(u"file:///token-stringify-helper-external.fake"_ustr);
    ScExternalRefManager* pRefMgr = m_pDoc->GetExternalRefManager();
    CPPUNIT_ASSERT(pRefMgr);
    const sal_uInt16 nFileId = pRefMgr->getExternalFileId(aExternalFile);

    ScSingleRefData aReference;
    aReference.InitAddress(ScAddress(3, 4, 0));

    ScTokenArray aLegacyTokens(*m_pDoc);
    aLegacyTokens.AddExternalSingleReference(
        nFileId, svl::SharedString(u"ExtSheet"_ustr), aReference);
    ScCompiler aCompiler(*m_pDoc, ScAddress(1, 1, 0), aLegacyTokens,
        formula::FormulaGrammar::GRAM_ODFF_A1);
    OUString aLegacyString;
    aCompiler.CreateStringFromTokenArray(aLegacyString);

    ScTokenArray aHelperTokens(*m_pDoc);
    aHelperTokens.AddExternalSingleReference(
        nFileId, svl::SharedString(u"ExtSheet"_ustr), aReference);
    const OUString aHelperString = createFormulaStringFromTokenArray(
        *m_pDoc, ScAddress(1, 1, 0), aHelperTokens, formula::FormulaGrammar::GRAM_ODFF_A1);

    CPPUNIT_ASSERT_EQUAL(aLegacyString, aHelperString);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
