/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <tokenstringcontext.hxx>
#include <compiler.hxx>
#include <document.hxx>
#include <dbdata.hxx>
#include <externalrefmgr.hxx>
#include <globstr.hrc>
#include <scresid.hxx>
#include <spreadsheetengine/compat/libreoffice/Grammar.hxx>
#include <spreadsheetengine/compat/formula/FormulaGrammar.hxx>

using namespace com::sun::star;
namespace selibreoffice = spreadsheetengine::compat::libreoffice;
namespace seformula = spreadsheetengine::compat::formula;

namespace sc {

namespace {

void insertAllNames( TokenStringContext::IndexNameMapType& rMap, const ScRangeName& rNames )
{
    for (auto const& it : rNames)
    {
        const ScRangeData *const pData = it.second.get();
        rMap.emplace(pData->GetIndex(), pData->GetName());
    }
}

formula::FormulaGrammar::AddressConvention toCalcRefConvention(
    formula::FormulaGrammar::Grammar eGrammar)
{
    return selibreoffice::toLibreOfficeAddressConvention(
        seformula::FormulaGrammar::extractRefConvention(selibreoffice::toApiGrammar(eGrammar)));
}

sal_Int32 toCalcFormulaLanguage(formula::FormulaGrammar::Grammar eGrammar)
{
    return selibreoffice::toLibreOfficeFormulaLanguage(
        seformula::FormulaGrammar::extractFormulaLanguage(selibreoffice::toApiGrammar(eGrammar)));
}

}

TokenStringContext::TokenStringContext( const ScDocument& rDoc, formula::FormulaGrammar::Grammar eGram ) :
    meGram(eGram),
    mpRefConv(ScCompiler::GetRefConvention(toCalcRefConvention(eGram)))
{
    formula::FormulaCompiler aComp;
    mxOpCodeMap = aComp.GetOpCodeMap(toCalcFormulaLanguage(eGram));
    if (mxOpCodeMap)
        maErrRef = mxOpCodeMap->getSymbol(ocErrRef);
    else
    {
        assert(!"TokenStringContext - no OpCodeMap?!?");
        maErrRef = ScResId(STR_NO_REF_TABLE);
    }

    // Fetch all sheet names.
    maTabNames = rDoc.GetAllTableNames();
    {
        for (auto& rTabName : maTabNames)
            ScCompiler::CheckTabQuotes(rTabName, toCalcRefConvention(eGram));
    }

    // Fetch all named range names.
    const ScRangeName* pNames = rDoc.GetRangeName();
    if (pNames)
        // global names
        insertAllNames(maGlobalRangeNames, *pNames);

    {
        ScRangeName::TabNameCopyMap aTabRangeNames;
        rDoc.GetAllTabRangeNames(aTabRangeNames);
        for (const auto& [nTab, pSheetNames] : aTabRangeNames)
        {
            if (!pSheetNames)
                continue;

            IndexNameMapType aNames;
            insertAllNames(aNames, *pSheetNames);
            maSheetRangeNames.emplace(nTab, aNames);
        }
    }

    // Fetch all named database ranges names.
    const ScDBCollection* pDBs = rDoc.GetDBCollection();
    if (pDBs)
    {
        const ScDBCollection::NamedDBs& rNamedDBs = pDBs->getNamedDBs();
        for (const auto& rxNamedDB : rNamedDBs)
        {
            const ScDBData& rData = *rxNamedDB;
            maNamedDBs.emplace(rData.GetIndex(), rData.GetName());
        }
    }

    // Fetch all relevant bits for external references.
    if (!rDoc.HasExternalRefManager())
        return;

    const ScExternalRefManager* pRefMgr = rDoc.GetExternalRefManager();
    maExternalFileNames = pRefMgr->getAllCachedExternalFileNames();
    for (size_t i = 0, n = maExternalFileNames.size(); i < n; ++i)
    {
        sal_uInt16 nFileId = static_cast<sal_uInt16>(i);
        std::vector<OUString> aTabNames;
        pRefMgr->getAllCachedTableNames(nFileId, aTabNames);
        if (!aTabNames.empty())
            maExternalCachedTabNames.emplace(nFileId, aTabNames);
    }
}

CompileFormulaContext::CompileFormulaContext( ScDocument& rDoc ) :
    mrDoc(rDoc), meGram(rDoc.GetGrammar())
{
    updateTabNames();
}

CompileFormulaContext::CompileFormulaContext( ScDocument& rDoc, formula::FormulaGrammar::Grammar eGram ) :
    mrDoc(rDoc), meGram(eGram)
{
    updateTabNames();
}

void CompileFormulaContext::updateTabNames()
{
    // Fetch all sheet names.
    maTabNames = mrDoc.GetAllTableNames();
    {
        for (auto& rTabName : maTabNames)
            ScCompiler::CheckTabQuotes(rTabName, toCalcRefConvention(meGram));
    }
}

void CompileFormulaContext::setGrammar( formula::FormulaGrammar::Grammar eGram )
{
    bool bUpdate = (meGram != eGram);
    meGram = eGram;
    if (bUpdate)
        updateTabNames();
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
