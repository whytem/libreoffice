/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "helper/qahelper.hxx"

#include <algorithm>

#include <document.hxx>
#include <formulacell.hxx>
#include <rangenam.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/DependencyGraphShadowBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/ExecutionIrBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/MutationTranslator.hxx>
#include <spreadsheetengine/compat/libreoffice/String.hxx>
#include <spreadsheetengine/compat/libreoffice/WorkbookFacade.hxx>
#include <spreadsheetengine/detail/workbook/FacadeConsumers.hxx>

namespace
{

class TestWorkbookFacade : public ScUcalcTestBase
{
};

} // namespace

CPPUNIT_TEST_FIXTURE(TestWorkbookFacade, testCalcFacadeReadSurfaceAndConsumers)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    namespace consumers = spreadsheetengine::detail::facade::consumers;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    m_pDoc->InsertTab(1, u"Summary"_ustr);
    m_pDoc->InsertTab(2, u"Hidden"_ustr);
    m_pDoc->SetVisible(2, false);

    m_pDoc->SetValue(0, 0, 0, 100.0);
    m_pDoc->SetString(0, 1, 0, u"hello"_ustr);
    m_pDoc->SetValue(0, 2, 0, 7.0);

    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(1, 1, 0, u"=A2*2"_ustr);
    m_pDoc->SetString(0, 0, 1, u"=Data.A1"_ustr);

    const ScFormulaCell* pSharedMember = m_pDoc->GetFormulaCell(ScAddress(1, 1, 0));
    CPPUNIT_ASSERT(pSharedMember);
    CPPUNIT_ASSERT(pSharedMember->IsShared());
    CPPUNIT_ASSERT_EQUAL(static_cast<SCROW>(0), pSharedMember->GetSharedTopRow());
    CPPUNIT_ASSERT_EQUAL(static_cast<SCROW>(2), pSharedMember->GetSharedLength());

    auto* pGlobalName
        = new ScRangeData(*m_pDoc, u"GlobalMetric"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pGlobalName));

    auto* pLocalName
        = new ScRangeData(*m_pDoc, u"LocalMetric"_ustr, u"$A$1:$B$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName(0)->insert(pLocalName));

    CalcWorkbookFacade aFacade(*m_pDoc, 17);

    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(3), aFacade.getSheetCount());
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::SheetId(0),
        *aFacade.findSheetId(u"Data"));

    const auto oSheet = aFacade.getSheetDescriptor(2);
    CPPUNIT_ASSERT(oSheet);
    CPPUNIT_ASSERT(oSheet->mbHidden);

    const auto aSnapshot = aFacade.getSnapshotInfo();
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int64>(17), aSnapshot.mnGeneration);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(3), aSnapshot.mnSheetCount);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(3), aSnapshot.mnFormulaCellCount);

    const auto aScalar = aFacade.getCellDescriptor({ 0, 0, 0 });
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::facade::CellKind::Scalar, aScalar.meKind);
    CPPUNIT_ASSERT(aScalar.maValue.isNumber());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(100.0, aScalar.maValue.mfNumber, 1e-12);

    const auto oFormula = aFacade.getFormulaCellDescriptor({ 0, 1, 0 });
    CPPUNIT_ASSERT(oFormula);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::FormulaCellKind::SharedGroupMember,
        oFormula->meKind);

    sal_Int32 nWholeCellCount = 0;
    aFacade.visitAllCells([&nWholeCellCount](
                              const spreadsheetengine::detail::facade::CellDescriptor&) {
        ++nWholeCellCount;
        return true;
    });
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(6), nWholeCellCount);

    const auto oGlobal = aFacade.findNamedRange(u"GlobalMetric", std::nullopt);
    CPPUNIT_ASSERT(oGlobal);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::NamedRangeScope::Global,
        oGlobal->meScope);

    const auto oLocal
        = aFacade.findNamedRange(u"LocalMetric", spreadsheetengine::detail::facade::SheetId(0));
    CPPUNIT_ASSERT(oLocal);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::NamedRangeScope::SheetLocal,
        oLocal->meScope);
    CPPUNIT_ASSERT(oLocal->moScopeSheet.has_value());
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::SheetId(0), *oLocal->moScopeSheet);

    const auto oGroup = aFacade.getFormulaGroupDescriptor({ 0, 1, 1 });
    CPPUNIT_ASSERT(oGroup);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::api::RowIndex(0), oGroup->maAnchor.mnRow);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), oGroup->mnLength);

    const auto aEnum = consumers::enumerateFormulaCells(aFacade);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(3), aEnum.mnTotalFormulaCells);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), aEnum.mnSharedGroupMembers);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aEnum.mnOrdinaryCells);

    const auto aGroups = consumers::summarizeFormulaGroups(aFacade);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aGroups.mnGroupCount);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), aGroups.mnTotalGroupLength);

    const auto aInventory = consumers::inventoryNamedRanges(aFacade);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aInventory.mnGlobalCount);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aInventory.mnSheetLocalCount);

    const auto aCorpus = consumers::collectFormulaCorpus(aFacade);
    CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(3), aCorpus.size());

    m_pDoc->DeleteTab(2);
    m_pDoc->DeleteTab(1);
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestWorkbookFacade, testNamedRangeMutationTranslatorKeepsDescriptors)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateAddNamedRange;
    using spreadsheetengine::compat::libreoffice::mutation::translateRemoveNamedRange;
    using spreadsheetengine::compat::libreoffice::mutation::translateRenameNamedRange;
    using spreadsheetengine::compat::libreoffice::toApiString;

    m_pDoc->InsertTab(0, u"Data"_ustr);

    auto* pGlobalName
        = new ScRangeData(*m_pDoc, u"GlobalMetric"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pGlobalName));

    auto* pLocalName
        = new ScRangeData(*m_pDoc, u"LocalMetric"_ustr, u"$A$1:$B$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName(0)->insert(pLocalName));

    const auto aAdd = translateAddNamedRange(*m_pDoc, *pGlobalName);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::MutationKind::AddNamedRange,
        aAdd.meKind);
    CPPUNIT_ASSERT(!aAdd.moNamedRangeBefore.has_value());
    CPPUNIT_ASSERT(aAdd.moNamedRangeAfter.has_value());
    CPPUNIT_ASSERT(aAdd.moNamedRangeAfter->maName == spreadsheetengine::api::String(u"GlobalMetric"));
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::NamedRangeScope::Global,
        aAdd.moNamedRangeAfter->meScope);

    const auto aRemove = translateRemoveNamedRange(*m_pDoc, *pLocalName, 0);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::MutationKind::RemoveNamedRange,
        aRemove.meKind);
    CPPUNIT_ASSERT(aRemove.moNamedRangeBefore.has_value());
    CPPUNIT_ASSERT(!aRemove.moNamedRangeAfter.has_value());
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::NamedRangeScope::SheetLocal,
        aRemove.moNamedRangeBefore->meScope);
    CPPUNIT_ASSERT(aRemove.moNamedRangeBefore->moScopeSheet.has_value());
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::SheetId(0),
        *aRemove.moNamedRangeBefore->moScopeSheet);
    CPPUNIT_ASSERT(aRemove.moNamedRangeBefore->maTargetExpression
        == toApiString(pLocalName->GetSymbol(m_pDoc->GetGrammar())));

    pLocalName->SetNewName(u"RenamedLocal"_ustr);
    const auto aRename = translateRenameNamedRange(*m_pDoc, *pLocalName, 0, u"LocalMetric"_ustr);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::MutationKind::RenameNamedRange,
        aRename.meKind);
    CPPUNIT_ASSERT(aRename.moNamedRangeBefore.has_value());
    CPPUNIT_ASSERT(aRename.moNamedRangeAfter.has_value());
    CPPUNIT_ASSERT(aRename.moNamedRangeBefore->maName == spreadsheetengine::api::String(u"LocalMetric"));
    CPPUNIT_ASSERT(aRename.moNamedRangeAfter->maName == spreadsheetengine::api::String(u"RenamedLocal"));
    CPPUNIT_ASSERT(aRename.moNamedRangeBefore->maTargetExpression
        == aRename.moNamedRangeAfter->maTargetExpression);
    CPPUNIT_ASSERT(aRename.moNamedRangeBefore->maBaseAddress
        == aRename.moNamedRangeAfter->maBaseAddress);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestWorkbookFacade, testComputationalShadowBuildFromCalcDocument)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::buildComputationalWorkbookShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);

    m_pDoc->SetValue(0, 0, 0, 100.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(1, 1, 0, u"=A1*3"_ustr);
    m_pDoc->CalcAll();

    auto* pGlobalName
        = new ScRangeData(*m_pDoc, u"Metric"_ustr, u"$Data.$A$1:$B$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pGlobalName));

    CalcWorkbookFacade aFacade(*m_pDoc, 23);
    const auto aLive
        = spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState(
            *m_pDoc);
    const auto aShadow = buildComputationalWorkbookShadow(aFacade, *m_pDoc);

    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int64>(23), aShadow.maSnapshot.mnGeneration);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(3), aShadow.getCellCount());
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), aShadow.getFormulaCellCount());
    CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(1), aShadow.maNamedRanges.size());
    CPPUNIT_ASSERT_EQUAL(aLive.maFormulaTree.size(), aShadow.maFormulaTree.size());
    CPPUNIT_ASSERT_EQUAL(aLive.maFormulaTrack.size(), aShadow.maFormulaTrack.size());
    CPPUNIT_ASSERT_EQUAL(aLive.maBroadcasters.maCellBroadcasters.size(),
        aShadow.maCellBroadcasters.size());

    const auto* pFormula = aShadow.findCell({ 0, 1, 0 });
    CPPUNIT_ASSERT(pFormula);
    CPPUNIT_ASSERT(pFormula->hasFormula());
    CPPUNIT_ASSERT_EQUAL(
        std::find(aLive.maFormulaTree.begin(), aLive.maFormulaTree.end(),
            spreadsheetengine::api::CellAddress { 0, 1, 0 })
            != aLive.maFormulaTree.end(),
        pFormula->mbInFormulaTree);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestWorkbookFacade, testDependencyGraphShadowBuildFromCalcDocument)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::BroadcasterNodeId;
    using spreadsheetengine::detail::substrate::ListenerAnchorKind;
    namespace mapping = spreadsheetengine::detail::substrate::mapping;

    m_pDoc->InsertTab(0, u"Data"_ustr);

    m_pDoc->SetValue(0, 0, 0, 100.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(1, 1, 0, u"=A1*3"_ustr);
    m_pDoc->CalcAll();

    CalcWorkbookFacade aFacade(*m_pDoc, 29);
    const auto aGraph = buildDependencyGraphShadow(aFacade, *m_pDoc);

    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int64>(29), aGraph.maSnapshot.mnGeneration);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), aGraph.getFormulaNodeCount());
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), aGraph.getListenerAnchorCount());
    CPPUNIT_ASSERT(aGraph.getBroadcasterNodeCount() >= 1);
    CPPUNIT_ASSERT(aGraph.getEdgeCount() >= 1);
    CPPUNIT_ASSERT(aGraph.findBroadcasterNode(BroadcasterNodeId::forCell({ 0, 0, 0 })));

    const auto aAnchor
        = mapping::makeListenerAnchorId(ListenerAnchorKind::FormulaCell, { 0, 1, 0 }, 1);
    CPPUNIT_ASSERT(aGraph.findListenerAnchor(aAnchor));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestWorkbookFacade, testExecutionIrShadowBuildFromCalcDocument)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow;
    using spreadsheetengine::detail::substrate::ExecutionIrInstructionKind;

    m_pDoc->InsertTab(0, u"Data"_ustr);

    m_pDoc->SetValue(0, 0, 0, 100.0);
    m_pDoc->SetValue(0, 1, 0, 50.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(1, 1, 0, u"=A2*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metric)"_ustr);
    m_pDoc->CalcAll();

    auto* pGlobalName
        = new ScRangeData(*m_pDoc, u"Metric"_ustr, u"$Data.$A$1:$B$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pGlobalName));

    CalcWorkbookFacade aFacade(*m_pDoc, 41);
    const auto aIrShadow = buildExecutionIrWorkbookShadow(aFacade, *m_pDoc);

    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int64>(41), aIrShadow.maSnapshot.mnGeneration);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(3), aIrShadow.getFormulaCount());
    CPPUNIT_ASSERT(aIrShadow.maBuildFailures.empty());
    CPPUNIT_ASSERT_EQUAL(static_cast<std::size_t>(1), aIrShadow.maFormulaGroups.size());

    const auto* pSumFormula = aIrShadow.findFormula({ 0, 2, 0 });
    CPPUNIT_ASSERT(pSumFormula);
    CPPUNIT_ASSERT(!pSumFormula->maInstructions.empty());
    CPPUNIT_ASSERT(std::any_of(pSumFormula->maInstructions.begin(), pSumFormula->maInstructions.end(),
        [](const auto& rInstruction) {
            return rInstruction.meKind == ExecutionIrInstructionKind::RangeNameReference;
        }));

    m_pDoc->DiscardFormulaGroupContext();
    m_pDoc->DeleteTab(0);
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
