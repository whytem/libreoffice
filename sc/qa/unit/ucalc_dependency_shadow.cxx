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
#include <cstdlib>
#include <map>
#include <set>
#include <string>

#include <rtl/string.hxx>

#include <document.hxx>
#include <globalnames.hxx>
#include <listenercontext.hxx>
#include <rangenam.hxx>
#include <scopetools.hxx>
#include <table.hxx>
#include <spreadsheetengine/compat/libreoffice/DependencyShadow.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalShadowMutation.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateObservation.hxx>
#include <spreadsheetengine/compat/libreoffice/DependencyGraphShadowMutation.hxx>
#include <spreadsheetengine/compat/libreoffice/ExecutionIrBuilder.hxx>
#include <spreadsheetengine/compat/libreoffice/ExecutionIrMutation.hxx>
#include <spreadsheetengine/compat/libreoffice/MutationTranslator.hxx>
#include <spreadsheetengine/compat/libreoffice/MutableComputationalSubstrate.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateAuthority.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateLifecycle.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateStructural.hxx>
#include <spreadsheetengine/compat/libreoffice/ComputationalSubstrateWiring.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcAuthority.hxx>
#include <spreadsheetengine/compat/libreoffice/RecalcShadow.hxx>
#include <spreadsheetengine/compat/libreoffice/WorkbookFacade.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/GraphWiringDelta.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIrComparison.hxx>
#include <spreadsheetengine/detail/dependency/DependencySnapshot.hxx>
#include <spreadsheetengine/detail/dependency/InvalidationPlanner.hxx>

namespace
{

class TestDependencyShadow : public ScUcalcTestBase
{
public:
    void tearDown() override
    {
        if (m_pDoc)
            m_pDoc->DiscardFormulaGroupContext();
        ScUcalcTestBase::tearDown();
    }
};

using spreadsheetengine::api::CellAddress;
using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
using spreadsheetengine::compat::libreoffice::dependencyshadow::ScopedInvalidationShadow;
using spreadsheetengine::compat::libreoffice::dependencyshadow::ShadowComparisonKind;
using ComputationalPilotResultKind
    = spreadsheetengine::compat::libreoffice::substrateauthority::PilotResultKind;
using spreadsheetengine::compat::libreoffice::substrateauthority::ScopedComputationalAuthority;
using ComputationalLifecycleResultKind
    = spreadsheetengine::compat::libreoffice::substratelifecycle::LifecycleResultKind;
using spreadsheetengine::compat::libreoffice::substratelifecycle::ScopedComputationalLifecycle;
using ComputationalStructuralResultKind
    = spreadsheetengine::compat::libreoffice::substratestructural::StructuralResultKind;
using spreadsheetengine::compat::libreoffice::substratestructural::ScopedComputationalStructural;
using spreadsheetengine::compat::libreoffice::recalcauthority::PilotResultKind;
using spreadsheetengine::compat::libreoffice::recalcauthority::ScopedRecalcAuthority;
using spreadsheetengine::compat::libreoffice::recalcshadow::ScopedRecalcShadow;
using spreadsheetengine::compat::libreoffice::recalcqueue::FormulaStateSnapshot;
using RecalcShadowComparisonKind
    = spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparisonKind;
using spreadsheetengine::compat::libreoffice::substrateobs::BroadcasterStateSnapshot;
using spreadsheetengine::compat::libreoffice::substrateobs::CellBroadcasterSnapshot;
using spreadsheetengine::compat::libreoffice::substrateobs::ListenerKind;
using spreadsheetengine::compat::libreoffice::substrateobs::LiveComputationalStateSnapshot;
using spreadsheetengine::detail::dependency::DirtyFormulaCell;
using GraphComparisonKind = spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind;
using ExecutionIrComparisonKind
    = spreadsheetengine::detail::substrate::ExecutionIrComparisonKind;

class ScopedEnvironmentOverride
{
    std::string maName;
    std::optional<std::string> moOriginalValue;

public:
    ScopedEnvironmentOverride(const char* pName, const char* pValue)
        : maName(pName)
    {
        if (const char* pOriginal = std::getenv(pName))
            moOriginalValue = pOriginal;

        if (pValue)
            setenv(maName.c_str(), pValue, 1);
        else
            unsetenv(maName.c_str());
    }

    ~ScopedEnvironmentOverride()
    {
        if (moOriginalValue)
            setenv(maName.c_str(), moOriginalValue->c_str(), 1);
        else
            unsetenv(maName.c_str());
    }
};

struct AddressLess
{
    [[nodiscard]] bool operator()(const CellAddress& rLeft, const CellAddress& rRight) const
    {
        if (rLeft.mnSheet != rRight.mnSheet)
            return rLeft.mnSheet < rRight.mnSheet;
        if (rLeft.mnColumn != rRight.mnColumn)
            return rLeft.mnColumn < rRight.mnColumn;
        return rLeft.mnRow < rRight.mnRow;
    }
};

[[nodiscard]] std::set<CellAddress, AddressLess> collectDirtyFormulaAddresses(
    const CalcWorkbookFacade& rFacade)
{
    std::set<CellAddress, AddressLess> aResult;
    rFacade.visitAllFormulaCells([&aResult](const spreadsheetengine::detail::facade::FormulaCellDescriptor& rDesc) {
        if (rDesc.mbDirty || rDesc.mbNeedsRecalc)
            aResult.insert(rDesc.maId.maAddress);
        return true;
    });
    return aResult;
}

[[nodiscard]] std::set<CellAddress, AddressLess> collectDirtyFormulaAddresses(
    const std::vector<DirtyFormulaCell>& rEntries)
{
    std::set<CellAddress, AddressLess> aResult;
    for (const auto& rEntry : rEntries)
        aResult.insert(rEntry.maAddress);
    return aResult;
}

[[nodiscard]] bool isSubsetOf(const std::set<CellAddress, AddressLess>& rSubset,
    const std::set<CellAddress, AddressLess>& rSuperset)
{
    return std::includes(rSuperset.begin(), rSuperset.end(), rSubset.begin(), rSubset.end(),
        AddressLess {});
}

void assertConservativeSuperset(const std::set<CellAddress, AddressLess>& rActual,
    const std::set<CellAddress, AddressLess>& rPredicted)
{
    CPPUNIT_ASSERT(isSubsetOf(rActual, rPredicted));
}

void assertNotUnderInvalidation(
    const std::optional<spreadsheetengine::compat::libreoffice::dependencyshadow::ShadowComparison>&
        oComparison)
{
    CPPUNIT_ASSERT(oComparison.has_value());
    CPPUNIT_ASSERT(oComparison->meKind != ShadowComparisonKind::UnderInvalidation);
}

void assertExactRecalcShadow(
    const std::optional<spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison>&
        oComparison)
{
    CPPUNIT_ASSERT(oComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oComparison->meKind);
}

void forceFormulaTreeOrder(ScDocument& rDoc, std::initializer_list<ScAddress> aOrder)
{
    for (const ScAddress& rAddress : aOrder)
    {
        if (ScFormulaCell* pCell = rDoc.GetFormulaCell(rAddress))
        {
            if (rDoc.IsInFormulaTrack(pCell))
                rDoc.RemoveFromFormulaTrack(pCell);
            if (rDoc.IsInFormulaTree(pCell))
                rDoc.RemoveFromFormulaTree(pCell);
        }
    }

    for (const ScAddress& rAddress : aOrder)
    {
        if (ScFormulaCell* pCell = rDoc.GetFormulaCell(rAddress))
        {
            pCell->SetDirtyVar();
            rDoc.PutInFormulaTree(pCell);
        }
    }
}

void reverseCurrentFormulaTreeOrder(ScDocument& rDoc)
{
    auto aCurrent
        = spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectFormulaTreeAddresses(
            rDoc);
    std::reverse(aCurrent.begin(), aCurrent.end());
    for (const auto& rAddress : aCurrent)
    {
        if (ScFormulaCell* pCell = rDoc.GetFormulaCell(
                ScAddress(rAddress.mnColumn, rAddress.mnRow, rAddress.mnSheet)))
        {
            if (rDoc.IsInFormulaTrack(pCell))
                rDoc.RemoveFromFormulaTrack(pCell);
            if (rDoc.IsInFormulaTree(pCell))
                rDoc.RemoveFromFormulaTree(pCell);
        }
    }

    for (const auto& rAddress : aCurrent)
    {
        if (ScFormulaCell* pCell = rDoc.GetFormulaCell(
                ScAddress(rAddress.mnColumn, rAddress.mnRow, rAddress.mnSheet)))
        {
            pCell->SetDirtyVar();
            rDoc.PutInFormulaTree(pCell);
        }
    }
}

void assertPilotAppliedWithExactQueue(
    const std::optional<spreadsheetengine::compat::libreoffice::recalcauthority::PilotResult>&
        oResult,
    RecalcShadowComparisonKind eExpectedBefore,
    const ScDocument& rDoc)
{
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(PilotResultKind::Applied, oResult->meKind);
    CPPUNIT_ASSERT(oResult->moComparisonBefore.has_value());
    CPPUNIT_ASSERT_EQUAL(eExpectedBefore, oResult->moComparisonBefore->meKind);
    CPPUNIT_ASSERT(oResult->moComparisonAfter.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oResult->moComparisonAfter->meKind);

    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            oResult->maPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(rDoc));
}

void assertPilotAppliedAndExactAfter(
    const std::optional<spreadsheetengine::compat::libreoffice::recalcauthority::PilotResult>&
        oResult,
    const ScDocument& rDoc)
{
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(PilotResultKind::Applied, oResult->meKind);
    CPPUNIT_ASSERT(oResult->moComparisonBefore.has_value());
    CPPUNIT_ASSERT(oResult->moComparisonBefore->meKind
                   != RecalcShadowComparisonKind::UnderScheduling);
    CPPUNIT_ASSERT(oResult->moComparisonAfter.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oResult->moComparisonAfter->meKind);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            oResult->maPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(rDoc));
}

void assertComputationalPilotApplied(
    const std::optional<
        spreadsheetengine::compat::libreoffice::substrateauthority::PilotResult>& oResult,
    const ScDocument& rDoc)
{
    CPPUNIT_ASSERT(oResult.has_value());
    const std::string aResultMessage
        = "unexpected computational pilot result kind="
          + std::to_string(static_cast<int>(oResult->meKind))
          + " queue="
          + (oResult->moQueueComparison
                 ? std::to_string(static_cast<int>(oResult->moQueueComparison->meKind))
                 : std::string("none"))
          + " graph="
          + (oResult->moGraphComparison
                 ? std::to_string(static_cast<int>(oResult->moGraphComparison->meKind))
                       + ":" + (oResult->moGraphComparison->mbFullMatch ? "1" : "0")
                 : std::string("none"))
          + " ir="
          + (oResult->moIrComparison
                 ? std::to_string(static_cast<int>(oResult->moIrComparison->meKind))
                       + ":" + (oResult->moIrComparison->mbFullMatch ? "1" : "0")
                 : std::string("none"));
    CPPUNIT_ASSERT_MESSAGE(
        aResultMessage,
        oResult->meKind == ComputationalPilotResultKind::Applied
            || oResult->meKind == ComputationalPilotResultKind::AppliedNormalizedEquivalent);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oResult->moQueueComparison->meKind);
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());
    CPPUNIT_ASSERT_MESSAGE(
        "graph comparison kind="
            + std::to_string(static_cast<int>(oResult->moGraphComparison->meKind)),
        oResult->moGraphComparison->mbFullMatch);
    CPPUNIT_ASSERT(oResult->moIrComparison.has_value());
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            oResult->maTransition.maRecalcPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(rDoc));
}

void assertComputationalLifecycleApplied(
    const std::optional<
        spreadsheetengine::compat::libreoffice::substratelifecycle::LifecycleResult>& oResult,
    const ScDocument& rDoc)
{
    CPPUNIT_ASSERT(oResult.has_value());
    const std::string aResultMessage
        = "unexpected computational lifecycle result kind="
          + std::to_string(static_cast<int>(oResult->meKind))
          + " queue="
          + (oResult->moQueueComparison
                 ? std::to_string(static_cast<int>(oResult->moQueueComparison->meKind))
                 : std::string("none"))
          + " computational="
          + (oResult->moComputationalComparison
                 ? std::string(oResult->moComputationalComparison->mbFullMatch ? "1" : "0")
                 : std::string("none"))
          + " graph="
          + (oResult->moGraphComparison
                 ? std::to_string(static_cast<int>(oResult->moGraphComparison->meKind))
                       + ":" + (oResult->moGraphComparison->mbFullMatch ? "1" : "0")
                 : std::string("none"))
          + " ir="
          + (oResult->moIrComparison
                 ? std::to_string(static_cast<int>(oResult->moIrComparison->meKind))
                       + ":" + (oResult->moIrComparison->mbFullMatch ? "1" : "0")
                 : std::string("none"));
    CPPUNIT_ASSERT_MESSAGE(
        aResultMessage,
        oResult->meKind == ComputationalLifecycleResultKind::Applied
        || oResult->meKind == ComputationalLifecycleResultKind::AppliedNormalizedEquivalent);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oResult->moQueueComparison->meKind);
    CPPUNIT_ASSERT(oResult->moComputationalComparison.has_value());
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbFullMatch);
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());
    CPPUNIT_ASSERT(oResult->moGraphComparison->mbFullMatch);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            oResult->maTransition.maRecalcPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(rDoc));
}

void assertComputationalStructuralApplied(
    const std::optional<
        spreadsheetengine::compat::libreoffice::substratestructural::StructuralResult>& oResult,
    const ScDocument& rDoc)
{
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_MESSAGE(
        "unexpected computational structural result kind="
            + std::to_string(static_cast<int>(oResult->meKind))
            + " verdict="
            + std::to_string(static_cast<int>(oResult->maTransition.meVerdict))
            + " reason="
            + OUStringToOString(
                  spreadsheetengine::compat::libreoffice::toLibreOfficeString(
                      oResult->maTransition.maReason),
                  RTL_TEXTENCODING_UTF8)
                  .getStr(),
        oResult->meKind == ComputationalStructuralResultKind::Applied
            || oResult->meKind == ComputationalStructuralResultKind::AppliedNormalizedEquivalent);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(RecalcShadowComparisonKind::Exact, oResult->moQueueComparison->meKind);
    CPPUNIT_ASSERT(oResult->moComputationalComparison.has_value());
    CPPUNIT_ASSERT(oResult->moComputationalComparison->mbFullMatch);
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());
    CPPUNIT_ASSERT(oResult->moGraphComparison->mbFullMatch);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            oResult->maTransition.maRecalcPlan)
        == spreadsheetengine::compat::libreoffice::recalcshadow::detail::
            collectFormulaTreeAddresses(rDoc));
}

void assertComputationalStructuralAppliedExactly(
    const std::optional<
        spreadsheetengine::compat::libreoffice::substratestructural::StructuralResult>& oResult,
    const ScDocument& rDoc)
{
    assertComputationalStructuralApplied(oResult, rDoc);
    CPPUNIT_ASSERT_EQUAL(ComputationalStructuralResultKind::Applied, oResult->meKind);
}

[[nodiscard]] const CellBroadcasterSnapshot* findCellBroadcaster(
    const BroadcasterStateSnapshot& rSnapshot, const ScAddress& rAddress)
{
    const auto aTarget = spreadsheetengine::compat::libreoffice::toApiCellAddress(rAddress);
    auto it = std::find_if(rSnapshot.maCellBroadcasters.begin(), rSnapshot.maCellBroadcasters.end(),
        [&aTarget](const CellBroadcasterSnapshot& rEntry) {
            return rEntry.maBroadcaster == aTarget;
        });
    return it == rSnapshot.maCellBroadcasters.end() ? nullptr : &*it;
}

[[nodiscard]] bool hasCellFormulaListener(const BroadcasterStateSnapshot& rSnapshot,
    const ScAddress& rBroadcaster, const ScAddress& rFormula)
{
    const CellBroadcasterSnapshot* pEntry = findCellBroadcaster(rSnapshot, rBroadcaster);
    if (!pEntry)
        return false;

    const auto aFormula = spreadsheetengine::compat::libreoffice::toApiCellAddress(rFormula);
    return std::any_of(pEntry->maListeners.begin(), pEntry->maListeners.end(),
        [&aFormula](const auto& rListener) {
            return rListener.meKind == ListenerKind::FormulaCell && rListener.maAnchor == aFormula;
        });
}

void assertNoCellBroadcaster(const BroadcasterStateSnapshot& rSnapshot, const ScAddress& rAddress)
{
    CPPUNIT_ASSERT(!findCellBroadcaster(rSnapshot, rAddress));
}

void assertHasEmptyCellBroadcaster(const BroadcasterStateSnapshot& rSnapshot, const ScAddress& rAddress)
{
    const CellBroadcasterSnapshot* pEntry = findCellBroadcaster(rSnapshot, rAddress);
    CPPUNIT_ASSERT(pEntry);
    CPPUNIT_ASSERT(pEntry->maListeners.empty());
}

void assertComparableGraph(
    const spreadsheetengine::detail::substrate::DependencyGraphShadowComparison& rComparison)
{
    CPPUNIT_ASSERT(rComparison.mbFullMatch);
    CPPUNIT_ASSERT(rComparison.meKind != GraphComparisonKind::Mismatch);
}

void assertComparableExecutionIr(
    const spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison& rComparison)
{
    CPPUNIT_ASSERT(rComparison.mbFullMatch);
    CPPUNIT_ASSERT(rComparison.meKind != ExecutionIrComparisonKind::Mismatch);
}

void assertFormulaStateEqual(
    const FormulaStateSnapshot& rExpected, const FormulaStateSnapshot& rActual)
{
    CPPUNIT_ASSERT(rExpected.maTreeOrder == rActual.maTreeOrder);
    CPPUNIT_ASSERT(rExpected.maTrackOrder == rActual.maTrackOrder);
    CPPUNIT_ASSERT(rExpected.maDirtyOnly == rActual.maDirtyOnly);
}

[[nodiscard]] bool hasGraphEdge(
    const spreadsheetengine::detail::substrate::DependencyGraphShadow& rGraph,
    const spreadsheetengine::detail::substrate::BroadcasterNodeId& rBroadcaster,
    const spreadsheetengine::detail::substrate::ListenerAnchorId& rListener)
{
    return std::any_of(rGraph.maEdges.begin(), rGraph.maEdges.end(),
        [&rBroadcaster, &rListener](const auto& rEdge) {
            return rEdge.maBroadcaster == rBroadcaster && rEdge.maListenerAnchor == rListener;
        });
}

} // namespace

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDirectAndTransitiveScalarInvalidationShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 0, 1, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr); // C1
    m_pDoc->SetString(3, 0, 0, u"=SUM(A1:A2)"_ustr); // D1

    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 1);
    const auto aSnapshot = buildDependencySnapshot(aBeforeFacade);
    const auto aPlan = planInvalidation(aSnapshot, translateSetScalarValue(ScAddress(0, 0, 0)));

    m_pDoc->SetValue(0, 0, 0, 99.0);

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
    const auto aActual = collectDirtyFormulaAddresses(aAfterFacade);
    const auto aPredicted = collectDirtyFormulaAddresses(aPlan.maDirtyFormulaCells);

    CPPUNIT_ASSERT(aActual == aPredicted);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testNamedRangeInvalidationShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 0, 1, 2.0); // A2

    auto* pGlobalName = new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pGlobalName));

    m_pDoc->SetString(1, 0, 0, u"=SUM(Metrics)"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr); // C1

    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 1);
    const auto aSnapshot = buildDependencySnapshot(aBeforeFacade);
    const auto aPlan = planInvalidation(aSnapshot, translateSetScalarValue(ScAddress(0, 1, 0)));

    m_pDoc->SetValue(0, 1, 0, 9.0);

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
    const auto aActual = collectDirtyFormulaAddresses(aAfterFacade);
    const auto aPredicted = collectDirtyFormulaAddresses(aPlan.maDirtyFormulaCells);

    CPPUNIT_ASSERT(aActual == aPredicted);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testStructuralRowInsertShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    m_pDoc->InsertTab(1, u"Summary"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // Data.A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // Data.A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // Data.B1
    m_pDoc->SetString(2, 0, 0, u"=SUM(A1:A2)"_ustr); // Data.C1
    m_pDoc->SetString(0, 0, 1, u"=Data.B1"_ustr); // Summary.A1

    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 1);
    const auto aSnapshot = buildDependencySnapshot(aBeforeFacade);
    const auto aPlan = planInvalidation(aSnapshot, translateInsertRows(0, 1, 1));

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
    const auto aActual = collectDirtyFormulaAddresses(aAfterFacade);
    const auto aPredicted = collectDirtyFormulaAddresses(aPlan.maDirtyFormulaCells);

    CPPUNIT_ASSERT(aPlan.mbRequiresSnapshotRebuild);
    CPPUNIT_ASSERT(!aPlan.maRebuildScopes.empty());
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::dependency::RebuildScopeKind::Workbook,
        aPlan.maRebuildScopes.front().meKind);
    assertConservativeSuperset(aActual, aPredicted);

    m_pDoc->DeleteTab(1);
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testStructuralDeleteColumnShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 0, 0, 2.0); // B1
    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr); // C1
    m_pDoc->SetString(3, 0, 0, u"=C1"_ustr); // D1

    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 1);
    const auto aSnapshot = buildDependencySnapshot(aBeforeFacade);
    const auto aPlan = planInvalidation(aSnapshot, translateDeleteColumns(0, 1, 1));

    m_pDoc->DeleteCol(0, 0, m_pDoc->MaxRow(), 0, 1, 1);

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
    const auto aActual = collectDirtyFormulaAddresses(aAfterFacade);

    CPPUNIT_ASSERT(aPlan.mbRequiresSnapshotRebuild);
    CPPUNIT_ASSERT(!aPlan.maRebuildScopes.empty());
    CPPUNIT_ASSERT(static_cast<sal_Int32>(aActual.size())
                   <= static_cast<sal_Int32>(aPlan.maDirtyFormulaCells.size()));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testWorkbookScaleShadowCorpus)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    m_pDoc->InsertTab(1, u"Summary"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 5.0); // Data.A1
    m_pDoc->SetValue(0, 1, 0, 7.0); // Data.A2

    auto* pGlobalName = new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pGlobalName));

    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // Data.B1
    m_pDoc->SetString(1, 1, 0, u"=A2*2"_ustr); // Data.B2
    m_pDoc->SetString(2, 0, 0, u"=SUM(A1:A2)"_ustr); // Data.C1
    m_pDoc->SetString(0, 0, 1, u"=Data.C1"_ustr); // Summary.A1
    m_pDoc->SetString(1, 0, 1, u"=SUM(Metrics)"_ustr); // Summary.B1
    m_pDoc->SetString(2, 0, 1, u"=B1"_ustr); // Summary.C1

    m_pDoc->CalcAll();

    {
        const ScopedInvalidationShadow aShadow(*m_pDoc, true);

        m_pDoc->SetValue(0, 0, 0, 9.0);

        assertNotUnderInvalidation(
            aShadow.compare(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))));
    }

    m_pDoc->CalcAll();

    {
        const ScopedInvalidationShadow aShadow(*m_pDoc, true);

        m_pDoc->SetString(2, 0, 1, u"=IF(B1>0;B1;0)"_ustr);

        assertNotUnderInvalidation(
            aShadow.compare(*m_pDoc,
                translateSetFormula(ScAddress(2, 0, 1), u"=IF(B1>0;B1;0)"_ustr)));
    }

    m_pDoc->DeleteTab(1);
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testRuntimeDependencyShadowAudit)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedInvalidationShadow aShadow(*m_pDoc, true);
    CPPUNIT_ASSERT(aShadow.isCaptured());

    m_pDoc->SetValue(0, 0, 0, 99.0);

    const auto oComparison
        = aShadow.compare(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));
    CPPUNIT_ASSERT(oComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(ShadowComparisonKind::Exact, oComparison->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testSetValueRecalcShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    CPPUNIT_ASSERT(aShadow.isCaptured());

    m_pDoc->SetValue(0, 0, 0, 9.0);
    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testSetStringScalarRecalcShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetString(0, 0, 0, u"a"_ustr);
    m_pDoc->SetString(1, 0, 0, u"=LEN(A1)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    m_pDoc->SetString(0, 0, 0, u"alpha"_ustr);

    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testSetFormulaRecalcShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=B1+2"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    m_pDoc->SetString(1, 0, 0, u"=A1*3"_ustr);
    forceFormulaTreeOrder(*m_pDoc,
        { ScAddress(1, 0, 0), ScAddress(2, 0, 0), ScAddress(3, 0, 0) });

    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*3"_ustr)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testClearCellRecalcShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    m_pDoc->SetEmptyCell(ScAddress(0, 0, 0));

    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateClearCell(ScAddress(0, 0, 0))));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testClearRangeRecalcShadow)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearRange;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=SUM(A1:A2)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    ScMarkData aMark(m_pDoc->GetSheetLimits());
    aMark.SelectOneTable(0);
    m_pDoc->DeleteArea(0, 0, 0, 1, aMark, InsertDeleteFlags::CONTENTS);

    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateClearRange(ScRange(0, 0, 0, 0, 1, 0))));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testSetValueRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=A1+5"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.isCaptured());
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->SetValue(0, 0, 0, 9.0);
    forceFormulaTreeOrder(*m_pDoc,
        { ScAddress(2, 0, 0), ScAddress(1, 0, 0), ScAddress(3, 0, 0) });

    assertPilotAppliedWithExactQueue(
        aAuthority.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))),
        RecalcShadowComparisonKind::OrderMismatch, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testSetFormulaRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=B1+2"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->SetString(1, 0, 0, u"=A1*3"_ustr);
    forceFormulaTreeOrder(*m_pDoc,
        { ScAddress(3, 0, 0), ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    assertPilotAppliedWithExactQueue(
        aAuthority.apply(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*3"_ustr)),
        RecalcShadowComparisonKind::OrderMismatch, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testClearRangeRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearRange;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=SUM(A1:A2)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    ScMarkData aMark(m_pDoc->GetSheetLimits());
    aMark.SelectOneTable(0);
    m_pDoc->DeleteArea(0, 0, 0, 1, aMark, InsertDeleteFlags::CONTENTS);
    forceFormulaTreeOrder(*m_pDoc,
        { ScAddress(3, 0, 0), ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    assertPilotAppliedWithExactQueue(
        aAuthority.apply(*m_pDoc, translateClearRange(ScRange(0, 0, 0, 0, 1, 0))),
        RecalcShadowComparisonKind::OrderMismatch, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testRecalcAuthoritySkipsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.isCaptured());
    CPPUNIT_ASSERT(!aAuthority.canApplyAuthority());

    m_pDoc->SetValue(0, 0, 0, 5.0);
    const auto oResult = aAuthority.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(PilotResultKind::SkippedDirtyBaseline, oResult->meKind);
    CPPUNIT_ASSERT(oResult->moComparisonBefore.has_value());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testRuntimeRecalcAuthoritySetValueHook)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    const ScopedEnvironmentOverride aEnv("SPREADSHEET_ENGINE_RECALC_AUTHORITY", "1");
    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcShadow aShadow(*m_pDoc, true);
    m_pDoc->SetValue(0, 0, 0, 12.0);

    assertExactRecalcShadow(
        aShadow.compare(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testInsertRowRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=SUM(A1:A2)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    reverseCurrentFormulaTreeOrder(*m_pDoc);

    assertPilotAppliedAndExactAfter(
        aAuthority.apply(*m_pDoc, translateInsertRows(0, 1, 1)), *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDeleteColumnRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(1, 0, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr);
    m_pDoc->SetString(3, 0, 0, u"=C1"_ustr);
    m_pDoc->SetString(4, 0, 0, u"=D1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->DeleteCol(ScRange(1, 0, 0, 1, m_pDoc->MaxRow(), 0));
    reverseCurrentFormulaTreeOrder(*m_pDoc);

    assertPilotAppliedAndExactAfter(
        aAuthority.apply(*m_pDoc, translateDeleteColumns(0, 1, 1)), *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testNamedRangeRenameRecalcAuthorityPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateRenameNamedRange;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    auto* pName = new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pName));
    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=SUM(Metrics)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedRecalcAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    std::map<OUString, ScRangeName> aRangeMap;
    aRangeMap.emplace(STR_GLOBAL_RANGE_NAME, *m_pDoc->GetRangeName());
    ScRangeName& rUpdated = aRangeMap.find(STR_GLOBAL_RANGE_NAME)->second;
    ScRangeData* pUpdated = rUpdated.findByIndex(pName->GetIndex());
    CPPUNIT_ASSERT(pUpdated);
    pUpdated->SetNewName(u"RenamedMetrics"_ustr);

    m_pDoc->SetAllRangeNames(aRangeMap);
    reverseCurrentFormulaTreeOrder(*m_pDoc);

    assertPilotAppliedAndExactAfter(
        aAuthority.apply(*m_pDoc,
            translateRenameNamedRange(*m_pDoc, *m_pDoc->GetRangeName()->findByIndex(pName->GetIndex()),
                std::nullopt, u"Metrics"_ustr)),
        *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthoritySetValuePilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.isCaptured());
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->SetValue(0, 0, 0, 9.0);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    assertComputationalPilotApplied(
        aAuthority.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0))), *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthoritySetFormulaPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.isCaptured());
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->SetString(1, 0, 0, u"=A1*3"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    assertComputationalPilotApplied(
        aAuthority.apply(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*3"_ustr)),
        *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthorityRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.isCaptured());
    CPPUNIT_ASSERT(!aAuthority.canApplyAuthority());

    m_pDoc->SetValue(0, 0, 0, 5.0);
    const auto oResult = aAuthority.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalPilotResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthorityRejectsValidationOnlyMutation)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::recalcqueue::captureFormulaState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(1, 0, 0, u"=SUM(A1:A2)"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    const auto aBeforeApply = captureFormulaState(*m_pDoc);
    const auto oResult = aAuthority.apply(*m_pDoc, translateInsertRows(0, 1, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalPilotResultKind::RejectedOutOfContract, oResult->meKind);
    assertFormulaStateEqual(aBeforeApply, captureFormulaState(*m_pDoc));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthorityRollsBackVerificationFailure)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::recalcqueue::captureFormulaState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalAuthority aAuthority(*m_pDoc, true);
    CPPUNIT_ASSERT(aAuthority.canApplyAuthority());

    m_pDoc->SetValue(0, 0, 0, 9.0);
    m_pDoc->SetString(2, 0, 0, u"=A1+1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    const auto aBeforeApply = captureFormulaState(*m_pDoc);
    const auto oResult = aAuthority.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalPilotResultKind::RolledBackVerificationFailure, oResult->meKind);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Mismatch, oResult->moGraphComparison->meKind);
    assertFormulaStateEqual(aBeforeApply, captureFormulaState(*m_pDoc));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalAuthorityClassifiesNormalizedEquivalent)
{
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison;
    using spreadsheetengine::compat::libreoffice::substrateauthority::PilotResult;
    using spreadsheetengine::compat::libreoffice::substrateauthority::detail::
        classifyVerifiedPilotResult;

    PilotResult aResult;
    aResult.maTransition.meVerdict
        = spreadsheetengine::detail::substrate::AuthorityPilotVerdict::Applicable;
    aResult.maTransition.maVerification.meQueueMode
        = spreadsheetengine::detail::substrate::AuthorityVerificationMode::Exact;
    aResult.maTransition.maVerification.meGraphMode
        = spreadsheetengine::detail::substrate::AuthorityVerificationMode::Exact;
    aResult.maTransition.maVerification.meIrMode
        = spreadsheetengine::detail::substrate::AuthorityVerificationMode::AllowNormalizedEquivalent;

    ShadowComparison aQueueComparison;
    aQueueComparison.meKind = RecalcShadowComparisonKind::Exact;
    aResult.moQueueComparison = aQueueComparison;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraphComparison;
    aGraphComparison.meKind = GraphComparisonKind::Exact;
    aGraphComparison.mbFullMatch = true;
    aResult.moGraphComparison = aGraphComparison;

    spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison aIrComparison;
    aIrComparison.meKind = ExecutionIrComparisonKind::NormalizedEquivalent;
    aIrComparison.mbFullMatch = true;
    aResult.moIrComparison = aIrComparison;

    CPPUNIT_ASSERT_EQUAL(
        ComputationalPilotResultKind::AppliedNormalizedEquivalent,
        classifyVerifiedPilotResult(aResult));
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleInsertFormulaPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(1, 0, 0, 2.0);
    m_pDoc->CalcAll();

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.isCaptured());
    CPPUNIT_ASSERT(aLifecycle.canApplyLifecycle());

    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0) });

    const auto oResult
        = aLifecycle.apply(*m_pDoc, translateSetFormula(ScAddress(2, 0, 0), u"=A1+B1"_ustr));
    assertComputationalLifecycleApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(2, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleReplaceFormulaPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.canApplyLifecycle());

    m_pDoc->SetString(1, 0, 0, u"=A1*3"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    const auto oResult
        = aLifecycle.apply(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*3"_ustr));
    assertComputationalLifecycleApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(1, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleRemoveFormulaPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.canApplyLifecycle());

    m_pDoc->SetEmptyCell(ScAddress(1, 0, 0));

    const auto oResult = aLifecycle.apply(*m_pDoc, translateClearCell(ScAddress(1, 0, 0)));
    assertComputationalLifecycleApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(!m_pDoc->GetFormulaCell(ScAddress(1, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.isCaptured());
    CPPUNIT_ASSERT(!aLifecycle.canApplyLifecycle());

    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    const auto oResult
        = aLifecycle.apply(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*2"_ustr));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalLifecycleResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleRejectsOutOfContractMutation)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::recalcqueue::captureFormulaState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.canApplyLifecycle());

    m_pDoc->SetValue(0, 0, 0, 5.0);
    const auto aBeforeApply = captureFormulaState(*m_pDoc);
    const auto oResult = aLifecycle.apply(*m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalLifecycleResultKind::RejectedOutOfContract, oResult->meKind);
    assertFormulaStateEqual(aBeforeApply, captureFormulaState(*m_pDoc));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleRollsBackVerificationFailure)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::recalcqueue::captureFormulaState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalLifecycle aLifecycle(*m_pDoc, true);
    CPPUNIT_ASSERT(aLifecycle.canApplyLifecycle());

    m_pDoc->SetString(1, 0, 0, u"=A1*3"_ustr);
    m_pDoc->SetString(2, 0, 0, u"=A1+1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0), ScAddress(1, 0, 0) });

    const auto aBeforeApply = captureFormulaState(*m_pDoc);
    const auto oResult
        = aLifecycle.apply(*m_pDoc, translateSetFormula(ScAddress(1, 0, 0), u"=A1*3"_ustr));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalLifecycleResultKind::RolledBackVerificationFailure, oResult->meKind);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Mismatch, oResult->moGraphComparison->meKind);
    assertFormulaStateEqual(aBeforeApply, captureFormulaState(*m_pDoc));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleClassifiesNormalizedEquivalent)
{
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison;
    using spreadsheetengine::compat::libreoffice::substratelifecycle::LifecycleResult;
    using spreadsheetengine::compat::libreoffice::substratelifecycle::detail::
        classifyVerifiedLifecycleResult;

    LifecycleResult aResult;
    aResult.maTransition.meVerdict
        = spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable;
    aResult.maTransition.maVerification.meComputationalMode
        = spreadsheetengine::detail::substrate::LifecycleVerificationMode::Exact;
    aResult.maTransition.maVerification.meGraphMode
        = spreadsheetengine::detail::substrate::LifecycleVerificationMode::
            AllowNormalizedEquivalent;
    aResult.maTransition.maVerification.mbObserveIrOnly = true;

    ShadowComparison aQueueComparison;
    aQueueComparison.meKind = RecalcShadowComparisonKind::Exact;
    aResult.moQueueComparison = aQueueComparison;

    spreadsheetengine::detail::substrate::ComputationalShadowComparison aComputationalComparison;
    aComputationalComparison.mbFullMatch = true;
    aResult.moComputationalComparison = aComputationalComparison;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraphComparison;
    aGraphComparison.meKind = GraphComparisonKind::NormalizedEquivalent;
    aGraphComparison.mbFullMatch = true;
    aResult.moGraphComparison = aGraphComparison;

    spreadsheetengine::detail::substrate::ExecutionIrWorkbookComparison aIrComparison;
    aIrComparison.meKind = ExecutionIrComparisonKind::Exact;
    aIrComparison.mbFullMatch = true;
    aResult.moIrComparison = aIrComparison;

    CPPUNIT_ASSERT_EQUAL(
        ComputationalLifecycleResultKind::AppliedNormalizedEquivalent,
        classifyVerifiedLifecycleResult(aResult));
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalLifecycleClassifiesRepairDetected)
{
    using spreadsheetengine::compat::libreoffice::recalcshadow::ShadowComparison;
    using spreadsheetengine::compat::libreoffice::substratelifecycle::LifecycleResult;
    using spreadsheetengine::compat::libreoffice::substratelifecycle::detail::
        classifyVerifiedLifecycleResult;

    LifecycleResult aResult;
    aResult.maTransition.meVerdict
        = spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable;
    aResult.maTransition.maVerification.meComputationalMode
        = spreadsheetengine::detail::substrate::LifecycleVerificationMode::Exact;
    aResult.maTransition.maVerification.meGraphMode
        = spreadsheetengine::detail::substrate::LifecycleVerificationMode::Exact;

    ShadowComparison aQueueComparison;
    aQueueComparison.meKind = RecalcShadowComparisonKind::Exact;
    aResult.moQueueComparison = aQueueComparison;

    spreadsheetengine::detail::substrate::ComputationalShadowComparison aComputationalComparison;
    aComputationalComparison.mbFullMatch = false;
    aResult.moComputationalComparison = aComputationalComparison;

    spreadsheetengine::detail::substrate::DependencyGraphShadowComparison aGraphComparison;
    aGraphComparison.meKind = GraphComparisonKind::Exact;
    aGraphComparison.mbFullMatch = true;
    aResult.moGraphComparison = aGraphComparison;

    CPPUNIT_ASSERT_EQUAL(
        ComputationalLifecycleResultKind::RepairDetected,
        classifyVerifiedLifecycleResult(aResult));
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalNarrowRolloutDisabledByDefault)
{
    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "0");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", "0");
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", "0");
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", "0");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    const auto aAuthorityCapture
        = ScopedComputationalAuthority::captureIfRuntimeEnabled(*m_pDoc);
    const auto aLifecycleCapture
        = ScopedComputationalLifecycle::captureIfRuntimeEnabled(*m_pDoc);
    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);

    CPPUNIT_ASSERT(!aAuthorityCapture.isCaptured());
    CPPUNIT_ASSERT(!aLifecycleCapture.isCaptured());
    CPPUNIT_ASSERT(!aStructuralCapture.isCaptured());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalNarrowRolloutLifecycleEnabledByUmbrella)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(1, 0, 0, 2.0);
    m_pDoc->CalcAll();

    const auto aAuthorityCapture
        = ScopedComputationalAuthority::captureIfRuntimeEnabled(*m_pDoc);
    const auto aLifecycleCapture
        = ScopedComputationalLifecycle::captureIfRuntimeEnabled(*m_pDoc);
    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aAuthorityCapture.isCaptured());
    CPPUNIT_ASSERT(aLifecycleCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aLifecycleCapture.canApplyLifecycle());

    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0) });

    const auto oResult = aLifecycleCapture.apply(
        *m_pDoc, translateSetFormula(ScAddress(2, 0, 0), u"=A1+B1"_ustr));
    assertComputationalLifecycleApplied(oResult, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralInsertRowPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.isCaptured());
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 3, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateInsertRows(0, 1, 1));
    assertComputationalStructuralApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(0, 3, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalNarrowRolloutStructuralEnabledByUmbrella)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 3, 0) });

    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateInsertRows(0, 1, 1));
    assertComputationalStructuralApplied(oResult, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralDeleteColumnPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(1, 0, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=$B$1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->DeleteCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(1, 0, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateDeleteColumns(0, 0, 1));
    assertComputationalStructuralApplied(oResult, *m_pDoc);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(1, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralDeleteRowPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->DeleteRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 1, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateDeleteRows(0, 0, 1));
    assertComputationalStructuralAppliedExactly(oResult, *m_pDoc);
    CPPUNIT_ASSERT(oResult->maTransition.maContract.isAdmitted());
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(0, 1, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralInsertColumnPilot)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=$A$1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateInsertColumns(0, 0, 1));
    assertComputationalStructuralAppliedExactly(oResult, *m_pDoc);
    CPPUNIT_ASSERT(oResult->maTransition.maContract.isAdmitted());
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(2, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalNarrowRolloutDeleteRowEnabledByUmbrella)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteRows;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->DeleteRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 1, 0) });

    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateDeleteRows(0, 0, 1));
    assertComputationalStructuralAppliedExactly(oResult, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalNarrowRolloutInsertColumnEnabledByUmbrella)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=$A$1+1"_ustr);
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0) });

    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateInsertColumns(0, 0, 1));
    assertComputationalStructuralAppliedExactly(oResult, *m_pDoc);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalNarrowRolloutStructuralOverrideBeatsUmbrella)
{
    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", "0");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    const auto aAuthorityCapture
        = ScopedComputationalAuthority::captureIfRuntimeEnabled(*m_pDoc);
    const auto aLifecycleCapture
        = ScopedComputationalLifecycle::captureIfRuntimeEnabled(*m_pDoc);
    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);

    CPPUNIT_ASSERT(aAuthorityCapture.isCaptured());
    CPPUNIT_ASSERT(aLifecycleCapture.isCaptured());
    CPPUNIT_ASSERT(!aStructuralCapture.isCaptured());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalNarrowRolloutGlobalNamedRangeStaysRejectedWithoutCandidateGate)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);
    ScopedEnvironmentOverride aGlobalNamedRange(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_GLOBAL_NAMED_RANGE", "0");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalNarrowRolloutGlobalNamedRangeEnabledByCandidateGate)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);
    ScopedEnvironmentOverride aGlobalNamedRange(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_GLOBAL_NAMED_RANGE", "1");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(3, 0, 0) });

    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::substrate::StructuralMutationClass::Admitted,
        oResult->maTransition.maContract.meMutationClass);
    CPPUNIT_ASSERT(oResult->meKind
        != ComputationalStructuralResultKind::RejectedOutOfContract);
    CPPUNIT_ASSERT(oResult->meKind
        != ComputationalStructuralResultKind::RejectedDirtyBaseline);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value());
    CPPUNIT_ASSERT(oResult->moComputationalComparison.has_value());
    CPPUNIT_ASSERT(oResult->moGraphComparison.has_value());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalNarrowRolloutGlobalNamedRangeOffSheetConsumerCandidate)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    ScopedEnvironmentOverride aRollout(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_NARROW_ROLLOUT", "1");
    ScopedEnvironmentOverride aAuthority(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_AUTHORITY", nullptr);
    ScopedEnvironmentOverride aLifecycle(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_LIFECYCLE", nullptr);
    ScopedEnvironmentOverride aStructural(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_STRUCTURAL", nullptr);
    ScopedEnvironmentOverride aGlobalNamedRange(
        "SPREADSHEET_ENGINE_COMPUTATIONAL_GLOBAL_NAMED_RANGE", "1");

    m_pDoc->InsertTab(0, u"Data"_ustr);
    m_pDoc->InsertTab(1, u"Summary"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(0, 0, 1, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const auto aStructuralCapture
        = ScopedComputationalStructural::captureIfRuntimeEnabled(*m_pDoc);
    CPPUNIT_ASSERT(aStructuralCapture.isCaptured());
    CPPUNIT_ASSERT(aStructuralCapture.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 0, 1) });

    const auto oResult = aStructuralCapture.apply(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(1);
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(0, 2, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.isCaptured());
    CPPUNIT_ASSERT(!aStructural.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    const auto oResult = aStructural.apply(*m_pDoc, translateInsertRows(0, 1, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralRejectsUnsupportedMutation)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateMoveRange;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    const auto oResult = aStructural.apply(*m_pDoc,
        translateMoveRange(ScRange(0, 0, 0, 0, 0, 0), ScRange(1, 0, 0, 1, 0, 0)));
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(0, 2, 0)));
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralDeleteRowRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(0, 2, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.isCaptured());
    CPPUNIT_ASSERT(!aStructural.canApplyStructural());

    m_pDoc->DeleteRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    const auto oResult = aStructural.apply(*m_pDoc, translateDeleteRows(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralInsertColumnRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=$A$1+1"_ustr);
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.isCaptured());
    CPPUNIT_ASSERT(!aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.apply(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralDeleteRowRejectsNamedRangeSlice)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->DeleteRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    const auto oResult = aStructural.apply(*m_pDoc, translateDeleteRows(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralInsertColumnRejectsNamedRangeSlice)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=$A$1+1"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$1"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.apply(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralValidateGlobalNamedRangeSlice)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.validateCandidate(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::substrate::StructuralMutationClass::ValidationOnly,
        oResult->maTransition.maContract.meMutationClass);
    CPPUNIT_ASSERT(oResult->meKind != ComputationalStructuralResultKind::RejectedOutOfContract);
    CPPUNIT_ASSERT(oResult->meKind != ComputationalStructuralResultKind::RejectedDirtyBaseline);
    CPPUNIT_ASSERT(oResult->moQueueComparison.has_value()
        || oResult->meKind == ComputationalStructuralResultKind::RepairDetected);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralValidateGlobalNamedRangeRejectsDirtyBaseline)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(2, 0, 0));
    CPPUNIT_ASSERT(pFormula);
    pFormula->SetDirtyVar();
    m_pDoc->PutInFormulaTree(pFormula);

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.isCaptured());
    CPPUNIT_ASSERT(!aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.validateCandidate(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedDirtyBaseline, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralValidateGlobalNamedRangeRepairDetected)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    m_pDoc->SetString(3, 0, 0, u"=SUM(Metrics)+1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(3, 0, 0) });

    const auto oResult = aStructural.validateCandidate(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RepairDetected, oResult->meKind);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(2, 0, 0)));
    CPPUNIT_ASSERT(!m_pDoc->GetFormulaCell(ScAddress(3, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralLocalNamedRangeStaysDeferred)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 4.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(LocalMetric)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName(0)->insert(
        new ScRangeData(*m_pDoc, u"LocalMetric"_ustr, u"$A$1:$A$1"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.validateCandidate(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralValidateMultiAreaNamedRangeRejects)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetValue(0, 1, 0, 2.0);
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(
        new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$A$1:$A$1~$A$2:$A$2"_ustr)));
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    const auto oResult = aStructural.validateCandidate(*m_pDoc, translateInsertColumns(0, 0, 1));

    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RejectedOutOfContract, oResult->meKind);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalStructuralRepairDetectedRollback)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    m_pDoc->SetString(0, 3, 0, u"=$A$2*1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 3, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateInsertRows(0, 1, 1));
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RepairDetected, oResult->meKind);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(0, 2, 0)));
    CPPUNIT_ASSERT(!m_pDoc->GetFormulaCell(ScAddress(0, 3, 0)));
    CPPUNIT_ASSERT_EQUAL(20.0, m_pDoc->GetValue(ScAddress(0, 1, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralDeleteRowRepairDetectedRollback)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteRows;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0);
    m_pDoc->SetValue(0, 1, 0, 20.0);
    m_pDoc->SetString(0, 2, 0, u"=$A$2*1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->DeleteRow(ScRange(0, 0, 0, m_pDoc->MaxCol(), 0, 0));
    m_pDoc->SetString(0, 1, 0, u"=$A$2*1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(0, 1, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateDeleteRows(0, 0, 1));
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RepairDetected, oResult->meKind);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(0, 2, 0)));
    CPPUNIT_ASSERT(!m_pDoc->GetFormulaCell(ScAddress(0, 1, 0)));
    CPPUNIT_ASSERT_EQUAL(10.0, m_pDoc->GetValue(ScAddress(0, 0, 0)));
    CPPUNIT_ASSERT_EQUAL(20.0, m_pDoc->GetValue(ScAddress(0, 1, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow,
    testComputationalStructuralInsertColumnRepairDetectedRollback)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertColumns;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0);
    m_pDoc->SetString(1, 0, 0, u"=$A$1+1"_ustr);
    m_pDoc->CalcAll();

    const ScopedComputationalStructural aStructural(*m_pDoc, true);
    CPPUNIT_ASSERT(aStructural.canApplyStructural());

    m_pDoc->InsertCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    m_pDoc->SetString(2, 0, 0, u"=$A$1+1"_ustr);
    forceFormulaTreeOrder(*m_pDoc, { ScAddress(2, 0, 0) });

    const auto oResult = aStructural.apply(*m_pDoc, translateInsertColumns(0, 0, 1));
    CPPUNIT_ASSERT(oResult.has_value());
    CPPUNIT_ASSERT_EQUAL(
        ComputationalStructuralResultKind::RepairDetected, oResult->meKind);
    CPPUNIT_ASSERT(m_pDoc->GetFormulaCell(ScAddress(1, 0, 0)));
    CPPUNIT_ASSERT(!m_pDoc->GetFormulaCell(ScAddress(2, 0, 0)));
    CPPUNIT_ASSERT_EQUAL(1.0, m_pDoc->GetValue(ScAddress(0, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalSubstrateScalarEditCapture)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::dependency::buildDependencySnapshot;
    using spreadsheetengine::detail::dependency::buildRecalcPlan;
    using spreadsheetengine::detail::dependency::planInvalidation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr); // C1
    m_pDoc->SetString(3, 0, 0, u"=SUM(A1:A2)"_ustr); // D1
    m_pDoc->CalcAll();

    const LiveComputationalStateSnapshot aBefore = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(hasCellFormulaListener(aBefore.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));
    CPPUNIT_ASSERT(hasCellFormulaListener(aBefore.maBroadcasters, ScAddress(1, 0, 0),
        ScAddress(2, 0, 0)));

    const CalcWorkbookFacade aFacade(*m_pDoc, 1);
    const auto aDependencySnapshot = buildDependencySnapshot(aFacade);
    const auto aInvalidationPlan
        = planInvalidation(aDependencySnapshot, translateSetScalarValue(ScAddress(0, 0, 0)));
    const auto aRecalcPlan = buildRecalcPlan(aDependencySnapshot, aInvalidationPlan);

    m_pDoc->SetValue(0, 0, 0, 99.0);

    const LiveComputationalStateSnapshot aAfter = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(aBefore.maBroadcasters == aAfter.maBroadcasters);
    CPPUNIT_ASSERT(
        spreadsheetengine::compat::libreoffice::recalcshadow::detail::collectPredictedQueueAddresses(
            aRecalcPlan)
        == aAfter.maFormulaTree);
    CPPUNIT_ASSERT(aAfter.maFormulaTrack.empty());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalSubstrateFormulaEditCapture)
{
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    const LiveComputationalStateSnapshot aBefore = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(hasCellFormulaListener(aBefore.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));

    m_pDoc->SetString(1, 0, 0, u"=A2"_ustr);

    const LiveComputationalStateSnapshot aAfter = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(!hasCellFormulaListener(aAfter.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));
    CPPUNIT_ASSERT(hasCellFormulaListener(aAfter.maBroadcasters, ScAddress(0, 1, 0),
        ScAddress(1, 0, 0)));
    CPPUNIT_ASSERT(aAfter.maFormulaTrack.empty());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalSubstrateDelayedStartListeningCapture)
{
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);

    {
        sc::EndListeningContext aEndCxt(*m_pDoc);
        pFormula->EndListeningTo(aEndCxt);
        aEndCxt.purgeEmptyBroadcasters();
    }

    const LiveComputationalStateSnapshot aDetached = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(!hasCellFormulaListener(aDetached.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));

    ScTable* pTable = m_pDoc->FetchTable(0);
    CPPUNIT_ASSERT(pTable);
    ScColumn& rFormulaColumn = pTable->CreateColumnIfNotExists(1);

    m_pDoc->EnableDelayStartListeningFormulaCells(&rFormulaColumn, true);
    rFormulaColumn.StartListeningUnshared({ 0, 0 });

    const LiveComputationalStateSnapshot aDelayed = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(!hasCellFormulaListener(aDelayed.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));

    m_pDoc->EnableDelayStartListeningFormulaCells(&rFormulaColumn, false);

    const LiveComputationalStateSnapshot aRestored = collectLiveComputationalState(*m_pDoc);
    CPPUNIT_ASSERT(hasCellFormulaListener(aRestored.maBroadcasters, ScAddress(0, 0, 0),
        ScAddress(1, 0, 0)));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalSubstrateDelayedBroadcasterDeletionCapture)
{
    using spreadsheetengine::compat::libreoffice::substrateobs::collectBroadcasterStateSnapshot;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);

    ScTable* pTable = m_pDoc->FetchTable(0);
    CPPUNIT_ASSERT(pTable);
    ScColumn& rSourceColumn = pTable->CreateColumnIfNotExists(0);

    {
        sc::DelayDeletingBroadcasters aDelay(*m_pDoc);
        rSourceColumn.EndListening(*pFormula, 0);

        const BroadcasterStateSnapshot aDelayed = collectBroadcasterStateSnapshot(*m_pDoc);
        assertHasEmptyCellBroadcaster(aDelayed, ScAddress(0, 0, 0));
    }

    const BroadcasterStateSnapshot aAfter = collectBroadcasterStateSnapshot(*m_pDoc);
    assertNoCellBroadcaster(aAfter, ScAddress(0, 0, 0));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalShadowRebuildAfterSafeMutations)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::rebuildComputationalShadowAfterMutation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    auto* pName = new ScRangeData(*m_pDoc, u"Metrics"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pName));
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metrics)"_ustr); // C1
    m_pDoc->CalcAll();

    m_pDoc->SetValue(0, 0, 0, 5.0);
    auto aScalarState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 1), *m_pDoc, translateSetScalarValue(ScAddress(0, 0, 0)));
    const auto* pA1 = aScalarState.maShadow.findCell({ 0, 0, 0 });
    CPPUNIT_ASSERT(pA1);
    CPPUNIT_ASSERT(pA1->maCell.maValue.isNumber());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, pA1->maCell.maValue.mfNumber, 1e-12);

    m_pDoc->SetString(1, 0, 0, u"=A1*4"_ustr);
    auto aFormulaEditState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 2), *m_pDoc,
        translateSetFormula(ScAddress(1, 0, 0), u"=A1*4"_ustr));
    const auto* pB1 = aFormulaEditState.maShadow.findCell({ 0, 1, 0 });
    CPPUNIT_ASSERT(pB1);
    CPPUNIT_ASSERT(pB1->moFormula.has_value());
    CPPUNIT_ASSERT(pB1->moFormula->maFormulaSource == spreadsheetengine::api::String(u"=A1*4"));

    m_pDoc->SetString(3, 0, 0, u"=B1+C1"_ustr);
    auto aFormulaInsertState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 3), *m_pDoc,
        translateSetFormula(ScAddress(3, 0, 0), u"=B1+C1"_ustr));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(3), aFormulaInsertState.maShadow.getFormulaCellCount());
    CPPUNIT_ASSERT(aFormulaInsertState.maShadow.findCell({ 0, 3, 0 }));

    m_pDoc->SetEmptyCell(ScAddress(0, 1, 0));
    auto aClearState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 4), *m_pDoc, translateClearCell(ScAddress(0, 1, 0)));
    CPPUNIT_ASSERT(!aClearState.maShadow.findCell({ 0, 0, 1 }));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalShadowRepresentativeStructuralWidening)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::rebuildComputationalShadowAfterMutation;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 1, 0, 2.0); // B2
    m_pDoc->SetString(2, 1, 0, u"=A1+B2"_ustr); // C2
    m_pDoc->CalcAll();

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    auto aRowInsertState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 1), *m_pDoc, translateInsertRows(0, 1, 1));
    CPPUNIT_ASSERT(aRowInsertState.maShadow.findCell({ 0, 0, 0 }));
    CPPUNIT_ASSERT(aRowInsertState.maShadow.findCell({ 0, 1, 2 }));
    CPPUNIT_ASSERT(aRowInsertState.maShadow.findCell({ 0, 2, 2 }));
    CPPUNIT_ASSERT(!aRowInsertState.maShadow.findCell({ 0, 1, 1 }));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aRowInsertState.maShadow.getFormulaCellCount());

    m_pDoc->DeleteCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    auto aDeleteColumnState = rebuildComputationalShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 2), *m_pDoc, translateDeleteColumns(0, 0, 1));
    CPPUNIT_ASSERT(aDeleteColumnState.maShadow.findCell({ 0, 0, 2 }));
    CPPUNIT_ASSERT(aDeleteColumnState.maShadow.findCell({ 0, 1, 2 }));
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aDeleteColumnState.maShadow.getFormulaCellCount());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalShadowDifferentialValidation)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::rebuildComputationalShadowAfterMutation;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    auto assertFullMatch = [&](const spreadsheetengine::detail::facade::MutationEvent& rMutation,
                               sal_Int64 nGeneration) {
        const CalcWorkbookFacade aFacade(*m_pDoc, nGeneration);
        const auto aState = rebuildComputationalShadowAfterMutation(aFacade, *m_pDoc, rMutation);
        const auto aComparison = compareComputationalShadow(aState.maShadow, aFacade, aState.maObservation);
        CPPUNIT_ASSERT(aComparison.mbFullMatch);
    };

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    m_pDoc->SetValue(0, 0, 0, 7.0);
    assertFullMatch(translateSetScalarValue(ScAddress(0, 0, 0)), 1);

    m_pDoc->SetString(1, 0, 0, u"=A2*3"_ustr);
    assertFullMatch(translateSetFormula(ScAddress(1, 0, 0), u"=A2*3"_ustr), 2);

    m_pDoc->SetEmptyCell(ScAddress(0, 1, 0));
    assertFullMatch(translateClearCell(ScAddress(0, 1, 0)), 3);

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    assertFullMatch(translateInsertRows(0, 1, 1), 4);

    m_pDoc->DeleteCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    assertFullMatch(translateDeleteColumns(0, 0, 1), 5);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testMutableComputationalSubstrateTracksAdmittedSlice)
{
    using spreadsheetengine::compat::libreoffice::bootstrapMutableComputationalSubstrateState;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::applyMutableAuthorityTransition;
    using spreadsheetengine::detail::substrate::applyMutableLifecycleTransition;
    using spreadsheetengine::detail::substrate::applyMutableStructuralTransition;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildLifecyclePilotTransition;
    using spreadsheetengine::detail::substrate::buildStructuralPilotTransition;
    using spreadsheetengine::detail::substrate::compareComputationalShadow;
    using spreadsheetengine::detail::substrate::authoritybuilddetail::buildAuthorityExecutionIrShadow;
    using spreadsheetengine::detail::substrate::buildAuthorityPilotTransition;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 0, 0, 2.0); // B1
    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr); // C1
    m_pDoc->CalcAll();

    auto aMutableState = bootstrapMutableComputationalSubstrateState(*m_pDoc, 0);
    CPPUNIT_ASSERT(aMutableState.mbBootstrapped);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aMutableState.maShadow.getFormulaCellCount());

    m_pDoc->SetValue(0, 0, 0, 5.0);
    {
        spreadsheetengine::detail::substrate::AuthorityPilotInput aInput;
        aInput.maComputationalShadow = aMutableState.maShadow;
        aInput.maGraphShadow = buildDependencyGraphShadow(
            aMutableState.maShadow, aMutableState.maObservation);
        aInput.maIrShadow
            = buildAuthorityExecutionIrShadow(aMutableState.maShadow, aMutableState.maFacade);
        aInput.maMutation = translateSetScalarValue(ScAddress(0, 0, 0));
        aInput.moScalarValueAfter = spreadsheetengine::api::CellValue::number(5.0);
        aInput.mbCleanBaseline = true;

        const auto aTransition = buildAuthorityPilotTransition(aInput);
        CPPUNIT_ASSERT(applyMutableAuthorityTransition(aMutableState, aTransition));
        const auto* pA1 = aMutableState.maShadow.findCell({ 0, 0, 0 });
        CPPUNIT_ASSERT(pA1);
        CPPUNIT_ASSERT(pA1->maCell.maValue.isNumber());
        CPPUNIT_ASSERT_DOUBLES_EQUAL(5.0, pA1->maCell.maValue.mfNumber, 1e-12);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aMutableState.mnAppliedMutationCount);
    }

    m_pDoc->SetString(1, 1, 0, u"=C1*2"_ustr); // B2
    {
        spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
        aInput.maComputationalShadow = aMutableState.maShadow;
        aInput.maGraphShadow = buildDependencyGraphShadow(
            aMutableState.maShadow, aMutableState.maObservation);
        aInput.maIrShadow
            = buildAuthorityExecutionIrShadow(aMutableState.maShadow, aMutableState.maFacade);
        aInput.maMutation = translateSetFormula(ScAddress(1, 1, 0), u"=C1*2"_ustr);
        aInput.mbCleanBaseline = true;

        const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
        const auto oFormula = aAfterFacade.getFormulaCellDescriptor({ 0, 1, 1 });
        CPPUNIT_ASSERT(oFormula);
        aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;

        const auto aTransition = buildLifecyclePilotTransition(aInput);
        CPPUNIT_ASSERT(applyMutableLifecycleTransition(aMutableState, aTransition));
        CPPUNIT_ASSERT(aMutableState.maShadow.findCell({ 0, 1, 1 }));
        CPPUNIT_ASSERT(aMutableState.maFacade.getFormulaCellDescriptor({ 0, 1, 1 }).has_value());
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(2), aMutableState.mnAppliedMutationCount);
    }

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDependencyGraphShadowRebuildAfterSafeMutations)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::rebuildDependencyGraphShadowAfterMutation;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=A1+A2"_ustr); // C1
    m_pDoc->CalcAll();

    auto assertGraphMatch = [&](const spreadsheetengine::detail::facade::MutationEvent& rMutation,
                                sal_Int64 nGeneration) {
        const CalcWorkbookFacade aFacade(*m_pDoc, nGeneration);
        const auto aState = rebuildDependencyGraphShadowAfterMutation(aFacade, *m_pDoc, rMutation);
        const auto aComparison = compareDependencyGraphShadow(
            aState.maGraphShadow, aState.maComputationalShadow, aState.maObservation);
        CPPUNIT_ASSERT(aComparison.mbFullMatch);
        CPPUNIT_ASSERT(aComparison.meKind != GraphComparisonKind::Mismatch);
    };

    m_pDoc->SetValue(0, 0, 0, 7.0);
    assertGraphMatch(translateSetScalarValue(ScAddress(0, 0, 0)), 1);

    m_pDoc->SetString(1, 0, 0, u"=A2*3"_ustr);
    assertGraphMatch(translateSetFormula(ScAddress(1, 0, 0), u"=A2*3"_ustr), 2);

    m_pDoc->SetString(3, 0, 0, u"=B1+C1"_ustr);
    assertGraphMatch(translateSetFormula(ScAddress(3, 0, 0), u"=B1+C1"_ustr), 3);

    m_pDoc->SetEmptyCell(ScAddress(0, 1, 0));
    assertGraphMatch(translateClearCell(ScAddress(0, 1, 0)), 4);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testGraphWiringDeltaCapturesLifecycleAdds)
{
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildGraphWiringDelta;
    using spreadsheetengine::detail::substrate::buildLifecyclePilotTransition;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr); // C1
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);

    spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maMutation = translateSetFormula(ScAddress(2, 0, 0), u"=B1+1"_ustr);
    aInput.mbCleanBaseline = true;

    const auto oFormula = aAfterFacade.getFormulaCellDescriptor({ 0, 2, 0 });
    CPPUNIT_ASSERT(oFormula);
    aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;

    const auto aTransition = buildLifecyclePilotTransition(aInput);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable,
        aTransition.meVerdict);

    const auto aDelta = buildGraphWiringDelta(aTransition);
    CPPUNIT_ASSERT_EQUAL(spreadsheetengine::detail::facade::MutationKind::SetFormula,
        aDelta.maMutation.meKind);
    CPPUNIT_ASSERT(!aDelta.maFormulaTreeDeltas.empty());
    CPPUNIT_ASSERT(!aDelta.maBroadcasterNodeDeltas.empty());
    CPPUNIT_ASSERT(!aDelta.maListenerEdgeDeltas.empty());
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), aDelta.getRemoveCount());
    CPPUNIT_ASSERT(!aDelta.maRecalcPlan.maQueue.empty());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalWiringApplyRebuildsLifecycleState)
{
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResultKind;
    using spreadsheetengine::compat::libreoffice::substratewiring::rebuildAdmittedLiveWiring;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildGraphWiringDelta;
    using spreadsheetengine::detail::substrate::buildLifecyclePilotTransition;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->SetString(2, 0, 0, u"=B1+1"_ustr); // C1
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);

    spreadsheetengine::detail::substrate::LifecyclePilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maMutation = translateSetFormula(ScAddress(2, 0, 0), u"=B1+1"_ustr);
    aInput.mbCleanBaseline = true;
    const auto oFormula = aAfterFacade.getFormulaCellDescriptor({ 0, 2, 0 });
    CPPUNIT_ASSERT(oFormula);
    aInput.moFormulaCachedValueAfter = oFormula->maCachedValue;

    const auto aTransition = buildLifecyclePilotTransition(aInput);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::LifecyclePilotVerdict::Applicable,
        aTransition.meVerdict);

    const auto aDelta = buildGraphWiringDelta(aTransition);
    const auto aApply = rebuildAdmittedLiveWiring(*m_pDoc, aDelta);
    CPPUNIT_ASSERT_EQUAL(WiringApplyResultKind::Applied, aApply.meKind);

    const auto aLiveObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aLiveShadow = buildComputationalWorkbookShadow(aAfterFacade, aLiveObservation);
    const auto aComparison
        = compareDependencyGraphShadow(aTransition.maGraphAfter, aLiveShadow, aLiveObservation);
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Exact, aComparison.meKind);
    CPPUNIT_ASSERT(aComparison.mbFullMatch);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testComputationalWiringApplyRebuildsStructuralState)
{
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::compat::libreoffice::substratewiring::WiringApplyResultKind;
    using spreadsheetengine::compat::libreoffice::substratewiring::rebuildAdmittedLiveWiring;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::buildGraphWiringDelta;
    using spreadsheetengine::detail::substrate::buildStructuralPilotTransition;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 10.0); // A1
    m_pDoc->SetValue(0, 1, 0, 20.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A2*2"_ustr); // B1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aBeforeFacade(*m_pDoc, 0);
    const auto aBeforeObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
    const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
    const auto aBeforeIr = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
        aBeforeShadow, *m_pDoc);

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 1);
    const auto aAfterObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));

    spreadsheetengine::detail::substrate::StructuralPilotInput aInput;
    aInput.maComputationalShadow = aBeforeShadow;
    aInput.maGraphShadow = aBeforeGraph;
    aInput.maIrShadow = aBeforeIr;
    aInput.maObservedAfterComputationalShadow
        = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
    aInput.maObservedAfterIrShadow
        = spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow(
            aInput.maObservedAfterComputationalShadow, *m_pDoc);
    aInput.maMutation = translateInsertRows(0, 1, 1);
    aInput.mbCleanBaseline = true;

    const auto aTransition = buildStructuralPilotTransition(aInput, aAfterFacade, aAfterObservation);
    CPPUNIT_ASSERT_EQUAL(
        spreadsheetengine::detail::substrate::StructuralPilotVerdict::Applicable,
        aTransition.meVerdict);

    const auto aDelta = buildGraphWiringDelta(aTransition);
    const auto aApply = rebuildAdmittedLiveWiring(*m_pDoc, aDelta);
    CPPUNIT_ASSERT_EQUAL(WiringApplyResultKind::Applied, aApply.meKind);

    const auto aLiveObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aLiveShadow = buildComputationalWorkbookShadow(aAfterFacade, aLiveObservation);
    const auto aComparison
        = compareDependencyGraphShadow(aTransition.maGraphAfter, aLiveShadow, aLiveObservation);
    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::Exact, aComparison.meKind);
    CPPUNIT_ASSERT(aComparison.mbFullMatch);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDependencyGraphShadowNormalizedComparison)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::graphmapping::GraphComparisonKind;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=B1"_ustr); // C1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aFacade(*m_pDoc, 1);
    auto aObservation = makeComputationalObservationState(collectLiveComputationalState(*m_pDoc));
    aObservation.maFormulaTree = { { 0, 2, 0 }, { 0, 1, 0 } };
    const auto aShadow = buildComputationalWorkbookShadow(aFacade, aObservation);
    const auto aGraph = buildDependencyGraphShadow(aShadow, aObservation);
    const auto aComparison = compareDependencyGraphShadow(aGraph, aShadow, aObservation);

    CPPUNIT_ASSERT_EQUAL(GraphComparisonKind::NormalizedEquivalent, aComparison.meKind);
    CPPUNIT_ASSERT(aComparison.mbFullMatch);
    CPPUNIT_ASSERT(!aComparison.mbFormulaTreeExactMatch);
    CPPUNIT_ASSERT(aComparison.mbFormulaTreeNormalizedMatch);

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDependencyGraphShadowDelayedListenerCoverage)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::BroadcasterNodeId;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;
    namespace graphmapping = spreadsheetengine::detail::substrate::graphmapping;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);

    {
        sc::EndListeningContext aEndCxt(*m_pDoc);
        pFormula->EndListeningTo(aEndCxt);
        aEndCxt.purgeEmptyBroadcasters();
    }

    ScTable* pTable = m_pDoc->FetchTable(0);
    CPPUNIT_ASSERT(pTable);
    ScColumn& rFormulaColumn = pTable->CreateColumnIfNotExists(1);

    m_pDoc->EnableDelayStartListeningFormulaCells(&rFormulaColumn, true);
    rFormulaColumn.StartListeningUnshared({ 0, 0 });

    const CalcWorkbookFacade aDelayedFacade(*m_pDoc, 1);
    const auto aDelayedObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aDelayedShadow = buildComputationalWorkbookShadow(aDelayedFacade, aDelayedObservation);
    const auto aDelayedGraph = buildDependencyGraphShadow(aDelayedShadow, aDelayedObservation);
    const auto aDelayedComparison
        = compareDependencyGraphShadow(aDelayedGraph, aDelayedShadow, aDelayedObservation);
    assertComparableGraph(aDelayedComparison);

    const auto aBroadcaster = BroadcasterNodeId::forCell({ 0, 0, 0 });
    const auto aFormulaAnchor
        = graphmapping::makeGraphFormulaCellListenerAnchorId({ 0, 1, 0 });
    CPPUNIT_ASSERT(!hasGraphEdge(aDelayedGraph, aBroadcaster, aFormulaAnchor));

    m_pDoc->EnableDelayStartListeningFormulaCells(&rFormulaColumn, false);

    const CalcWorkbookFacade aRestoredFacade(*m_pDoc, 2);
    const auto aRestoredObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aRestoredShadow = buildComputationalWorkbookShadow(aRestoredFacade, aRestoredObservation);
    const auto aRestoredGraph = buildDependencyGraphShadow(aRestoredShadow, aRestoredObservation);
    const auto aRestoredComparison
        = compareDependencyGraphShadow(aRestoredGraph, aRestoredShadow, aRestoredObservation);
    assertComparableGraph(aRestoredComparison);
    CPPUNIT_ASSERT(hasGraphEdge(aRestoredGraph, aBroadcaster, aFormulaAnchor));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDependencyGraphShadowDelayedBroadcasterDeletionCoverage)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::makeComputationalObservationState;
    using spreadsheetengine::compat::libreoffice::substrateobs::collectLiveComputationalState;
    using spreadsheetengine::detail::substrate::BroadcasterNodeId;
    using spreadsheetengine::detail::substrate::buildComputationalWorkbookShadow;
    using spreadsheetengine::detail::substrate::buildDependencyGraphShadow;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->CalcAll();

    ScFormulaCell* pFormula = m_pDoc->GetFormulaCell(ScAddress(1, 0, 0));
    CPPUNIT_ASSERT(pFormula);

    ScTable* pTable = m_pDoc->FetchTable(0);
    CPPUNIT_ASSERT(pTable);
    ScColumn& rSourceColumn = pTable->CreateColumnIfNotExists(0);
    const auto aBroadcaster = BroadcasterNodeId::forCell({ 0, 0, 0 });

    {
        sc::DelayDeletingBroadcasters aDelay(*m_pDoc);
        rSourceColumn.EndListening(*pFormula, 0);

        const CalcWorkbookFacade aDelayedFacade(*m_pDoc, 1);
        const auto aDelayedObservation = makeComputationalObservationState(
            collectLiveComputationalState(*m_pDoc));
        const auto aDelayedShadow
            = buildComputationalWorkbookShadow(aDelayedFacade, aDelayedObservation);
        const auto aDelayedGraph
            = buildDependencyGraphShadow(aDelayedShadow, aDelayedObservation);
        const auto aDelayedComparison
            = compareDependencyGraphShadow(aDelayedGraph, aDelayedShadow, aDelayedObservation);
        assertComparableGraph(aDelayedComparison);
        const auto* pBroadcaster = aDelayedGraph.findBroadcasterNode(aBroadcaster);
        CPPUNIT_ASSERT(pBroadcaster);
        CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(0), pBroadcaster->mnListenerCount);
    }

    const CalcWorkbookFacade aAfterFacade(*m_pDoc, 2);
    const auto aAfterObservation = makeComputationalObservationState(
        collectLiveComputationalState(*m_pDoc));
    const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
    const auto aAfterGraph = buildDependencyGraphShadow(aAfterShadow, aAfterObservation);
    const auto aAfterComparison
        = compareDependencyGraphShadow(aAfterGraph, aAfterShadow, aAfterObservation);
    assertComparableGraph(aAfterComparison);
    CPPUNIT_ASSERT(!aAfterGraph.findBroadcasterNode(aBroadcaster));

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testDependencyGraphShadowStructuralGateCoverage)
{
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::rebuildDependencyGraphShadowAfterMutation;
    using spreadsheetengine::detail::substrate::compareDependencyGraphShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 1, 0, 2.0); // B2
    m_pDoc->SetString(2, 1, 0, u"=A1+B2"_ustr); // C2
    m_pDoc->CalcAll();

    m_pDoc->InsertRow(ScRange(0, 1, 0, m_pDoc->MaxCol(), 1, 0));
    auto aRowInsertState = rebuildDependencyGraphShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 1), *m_pDoc, translateInsertRows(0, 1, 1));
    const auto aRowInsertComparison = compareDependencyGraphShadow(
        aRowInsertState.maGraphShadow, aRowInsertState.maComputationalShadow, aRowInsertState.maObservation);
    assertComparableGraph(aRowInsertComparison);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aRowInsertState.maGraphShadow.getFormulaNodeCount());

    m_pDoc->DeleteCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    auto aDeleteColumnState = rebuildDependencyGraphShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 2), *m_pDoc, translateDeleteColumns(0, 0, 1));
    const auto aDeleteColumnComparison = compareDependencyGraphShadow(
        aDeleteColumnState.maGraphShadow, aDeleteColumnState.maComputationalShadow,
        aDeleteColumnState.maObservation);
    assertComparableGraph(aDeleteColumnComparison);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aDeleteColumnState.maGraphShadow.getFormulaNodeCount());

    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testExecutionIrShadowRebuildAfterSafeMutations)
{
    using spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow;
    using spreadsheetengine::compat::libreoffice::mutation::translateClearCell;
    using spreadsheetengine::compat::libreoffice::mutation::translateRenameNamedRange;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetFormula;
    using spreadsheetengine::compat::libreoffice::mutation::translateSetScalarValue;
    using spreadsheetengine::compat::libreoffice::rebuildExecutionIrShadowAfterMutation;
    using spreadsheetengine::detail::substrate::compareExecutionIrWorkbookShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(0, 1, 0, 2.0); // A2
    auto* pGlobalName
        = new ScRangeData(*m_pDoc, u"Metric"_ustr, u"$Data.$A$1:$A$2"_ustr);
    CPPUNIT_ASSERT(m_pDoc->GetRangeName()->insert(pGlobalName));
    m_pDoc->SetString(1, 0, 0, u"=A1"_ustr); // B1
    m_pDoc->SetString(2, 0, 0, u"=SUM(Metric)"_ustr); // C1
    m_pDoc->CalcAll();

    auto assertIrMatch = [&](const spreadsheetengine::detail::facade::MutationEvent& rMutation,
                             sal_Int64 nGeneration) {
        const CalcWorkbookFacade aFacade(*m_pDoc, nGeneration);
        const auto aState = rebuildExecutionIrShadowAfterMutation(aFacade, *m_pDoc, rMutation);
        const auto aExpected = buildExecutionIrWorkbookShadow(aState.maComputationalShadow, *m_pDoc);
        const auto aComparison = compareExecutionIrWorkbookShadow(aState.maIrShadow, aExpected);
        assertComparableExecutionIr(aComparison);
        CPPUNIT_ASSERT(aState.maIrShadow.maBuildFailures.empty());
    };

    m_pDoc->SetValue(0, 0, 0, 7.0);
    assertIrMatch(translateSetScalarValue(ScAddress(0, 0, 0)), 1);

    m_pDoc->SetString(1, 0, 0, u"=A2*3"_ustr);
    assertIrMatch(translateSetFormula(ScAddress(1, 0, 0), u"=A2*3"_ustr), 2);

    m_pDoc->SetString(3, 0, 0, u"=B1+C1"_ustr);
    assertIrMatch(translateSetFormula(ScAddress(3, 0, 0), u"=B1+C1"_ustr), 3);

    m_pDoc->SetEmptyCell(ScAddress(0, 1, 0));
    assertIrMatch(translateClearCell(ScAddress(0, 1, 0)), 4);

    pGlobalName->SetNewName(u"MetricRenamed"_ustr);
    assertIrMatch(
        translateRenameNamedRange(*m_pDoc, *pGlobalName, std::nullopt, u"Metric"_ustr), 5);

    m_pDoc->DiscardFormulaGroupContext();
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testExecutionIrShadowNormalizedComparison)
{
    using spreadsheetengine::compat::libreoffice::CalcWorkbookFacade;
    using spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow;
    using spreadsheetengine::detail::substrate::compareExecutionIrWorkbookShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 0, 0, 2.0); // B1
    m_pDoc->SetString(2, 0, 0, u"=A1+B1"_ustr); // C1
    m_pDoc->SetString(3, 0, 0, u"=SUM(A1:B1)"_ustr); // D1
    m_pDoc->CalcAll();

    const CalcWorkbookFacade aFacade(*m_pDoc, 1);
    auto aIrShadow = buildExecutionIrWorkbookShadow(aFacade, *m_pDoc);
    std::reverse(aIrShadow.maFormulaRecords.begin(), aIrShadow.maFormulaRecords.end());

    const auto aExpected = buildExecutionIrWorkbookShadow(aFacade, *m_pDoc);
    const auto aComparison = compareExecutionIrWorkbookShadow(aIrShadow, aExpected);

    CPPUNIT_ASSERT_EQUAL(ExecutionIrComparisonKind::NormalizedEquivalent, aComparison.meKind);
    CPPUNIT_ASSERT(aComparison.mbFullMatch);
    CPPUNIT_ASSERT(!aComparison.mbFormulaRecordExactMatch);
    CPPUNIT_ASSERT(aComparison.mbFormulaRecordNormalizedMatch);

    m_pDoc->DiscardFormulaGroupContext();
    m_pDoc->DeleteTab(0);
}

CPPUNIT_TEST_FIXTURE(TestDependencyShadow, testExecutionIrShadowStructuralGateCoverage)
{
    using spreadsheetengine::compat::libreoffice::buildExecutionIrWorkbookShadow;
    using spreadsheetengine::compat::libreoffice::mutation::translateDeleteColumns;
    using spreadsheetengine::compat::libreoffice::mutation::translateInsertRows;
    using spreadsheetengine::compat::libreoffice::rebuildExecutionIrShadowAfterMutation;
    using spreadsheetengine::detail::substrate::compareExecutionIrWorkbookShadow;

    m_pDoc->InsertTab(0, u"Data"_ustr);
    sc::AutoCalcSwitch aACSwitch(*m_pDoc, false);

    m_pDoc->SetValue(0, 0, 0, 1.0); // A1
    m_pDoc->SetValue(1, 1, 0, 2.0); // B2
    m_pDoc->SetString(2, 1, 0, u"=A1+B2"_ustr); // C2
    m_pDoc->CalcAll();

    auto aRowInsertState = rebuildExecutionIrShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 1), *m_pDoc, translateInsertRows(0, 1, 1));
    const auto aRowInsertExpected
        = buildExecutionIrWorkbookShadow(aRowInsertState.maComputationalShadow, *m_pDoc);
    const auto aRowInsertComparison
        = compareExecutionIrWorkbookShadow(aRowInsertState.maIrShadow, aRowInsertExpected);
    assertComparableExecutionIr(aRowInsertComparison);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aRowInsertState.maIrShadow.getFormulaCount());

    m_pDoc->DeleteCol(ScRange(0, 0, 0, 0, m_pDoc->MaxRow(), 0));
    auto aDeleteColumnState = rebuildExecutionIrShadowAfterMutation(
        CalcWorkbookFacade(*m_pDoc, 2), *m_pDoc, translateDeleteColumns(0, 0, 1));
    const auto aDeleteColumnExpected
        = buildExecutionIrWorkbookShadow(aDeleteColumnState.maComputationalShadow, *m_pDoc);
    const auto aDeleteColumnComparison = compareExecutionIrWorkbookShadow(
        aDeleteColumnState.maIrShadow, aDeleteColumnExpected);
    assertComparableExecutionIr(aDeleteColumnComparison);
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_Int32>(1), aDeleteColumnState.maIrShadow.getFormulaCount());

    m_pDoc->DiscardFormulaGroupContext();
    m_pDoc->DeleteTab(0);
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
