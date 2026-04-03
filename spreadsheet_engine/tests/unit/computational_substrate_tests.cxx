/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/detail/substrate/AuthorityPilot.hxx>
#include <spreadsheetengine/detail/substrate/AuthorityPilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/LifecyclePilot.hxx>
#include <spreadsheetengine/detail/substrate/LifecyclePilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/StructuralPilot.hxx>
#include <spreadsheetengine/detail/substrate/StructuralPilotBuilder.hxx>
#include <spreadsheetengine/detail/substrate/ExecutionIr.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowComparison.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowMapping.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadowBuilder.hxx>
#include <spreadsheetengine/detail/substrate/DependencyGraphShadow.hxx>
#include <spreadsheetengine/detail/substrate/ComputationalShadowMutation.hxx>
#include <spreadsheetengine/detail/workbook/InMemoryWorkbookFacade.hxx>

#include "TestSupport.hxx"

int main()
{
    using namespace spreadsheetengine::detail::facade;
    using namespace spreadsheetengine::detail::substrate;
    using spreadsheetengine::detail::substrate::authoritydetail::classifyAuthorityMutation;
    using spreadsheetengine::detail::substrate::authoritydetail::makeAuthorityVerification;
    using spreadsheetengine::detail::substrate::lifecycledetail::classifyLifecycleMutation;
    using spreadsheetengine::detail::substrate::lifecycledetail::makeLifecycleVerification;
    using spreadsheetengine::detail::substrate::structuraldetail::classifyStructuralMutation;
    using spreadsheetengine::detail::substrate::structuraldetail::makeStructuralVerification;
    namespace mapping = spreadsheetengine::detail::substrate::mapping;
    using spreadsheetengine::api::CellValue;
    using spreadsheetengine::standalone::test::fail;

    {
        ExecutionIrInstruction aInstruction {
            ExecutionIrInstructionKind::SingleReference,
            spreadsheetengine::detail::token::kOpCodePush,
            spreadsheetengine::api::refdata::SingleRefData {}
        };
        if (!aInstruction.carriesReference())
            return fail("computational_substrate", "execution ir reference classification mismatch");

        ExecutionIrFormulaRecord aFormula;
        aFormula.maId = { { 0, 1, 2 } };
        aFormula.maSource = { u"=A1", {} };
        aFormula.maInstructions.push_back(aInstruction);
        aFormula.maInstructions.push_back({
            ExecutionIrInstructionKind::PlainOpcode,
            spreadsheetengine::detail::token::kOpCodeAdd,
            {}
        });
        aFormula.mbInFormulaTree = true;

        ExecutionIrWorkbookShadow aIrShadow;
        aIrShadow.maFormulaRecords.push_back(aFormula);
        if (aIrShadow.getFormulaCount() != 1 || aIrShadow.getInstructionCount() != 2
            || !aIrShadow.findFormula({ 0, 1, 2 }) || !aIrShadow.isFullyLowered()
            || aIrShadow.findFormula({ 0, 2, 2 }))
        {
            return fail("computational_substrate", "execution ir schema lookup mismatch");
        }
        if (aIrShadow.findFormula({ 0, 1, 2 })->getReferenceInstructionCount() != 1)
            return fail("computational_substrate", "execution ir reference count mismatch");

        aIrShadow.maBuildFailures.push_back({ { { 0, 3, 4 } }, u"=BAD()", u"lower failed" });
        if (aIrShadow.isFullyLowered())
            return fail("computational_substrate", "execution ir failure tracking mismatch");
    }

    if (!(mapping::makeShadowCellId({ 3, 4, 5 }) == ShadowCellId { { 3, 4, 5 } }))
        return fail("computational_substrate", "shadow cell id mapping mismatch");

    {
        const auto aCellBroadcaster = BroadcasterNodeId::forCell({ 0, 2, 1 });
        if (aCellBroadcaster.meKind != BroadcasterNodeKind::Cell
            || !(aCellBroadcaster.maCellAddress == spreadsheetengine::api::CellAddress { 0, 2, 1 }))
        {
            return fail("computational_substrate", "cell broadcaster id mapping mismatch");
        }

        const auto aAreaBroadcaster
            = BroadcasterNodeId::forArea({ { 1, 3, 4 }, { 1, 5, 6 } });
        if (aAreaBroadcaster.meKind != BroadcasterNodeKind::Area
            || !(aAreaBroadcaster.maAreaRange
                 == spreadsheetengine::api::CellRange { { 1, 3, 4 }, { 1, 5, 6 } }))
        {
            return fail("computational_substrate", "area broadcaster id mapping mismatch");
        }

        DependencyGraphShadow aGraph;
        aGraph.maListenerAnchors.push_back({ mapping::makeListenerAnchorId(
            ListenerAnchorKind::FormulaCell, { 0, 2, 1 }, 1) });
        if (aGraph.getListenerAnchorCount() != 1
            || !aGraph.findListenerAnchor(mapping::makeListenerAnchorId(
                   ListenerAnchorKind::FormulaCell, { 0, 2, 1 }, 1)))
        {
            return fail("computational_substrate", "graph schema lookup mismatch");
        }
    }

    {
        const FormulaGroupDescriptor aDescriptor { { 0, 1, 2 }, 4, true };
        if (!(mapping::makeShadowFormulaGroupId(aDescriptor)
              == ShadowFormulaGroupId { { 0, 1, 2 }, 4 }))
        {
            return fail("computational_substrate", "shadow group id mapping mismatch");
        }
    }

    InMemoryWorkbookFacade aFacade;
    aFacade.setGrammar({ spreadsheetengine::api::FormulaLanguage::Odff,
        spreadsheetengine::api::AddressConvention::OdfA1, false });
    aFacade.setGeneration(9);

    const auto nData = aFacade.addSheet(u"Data");
    aFacade.addSheet(u"Summary");

    aFacade.setCell({ nData, 0, 0 }, CellValue::number(10.0));
    aFacade.setCell({ nData, 0, 1 }, CellValue::text(u"seed"));
    aFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*2", CellValue::number(20.0));
    aFacade.setFormulaCell({ nData, 1, 1 }, u"=SUM(A1:B1)", CellValue::number(30.0),
        FormulaCellKind::SharedGroupMember, true, true);
    aFacade.addFormulaGroup({ nData, 1, 0 }, 2, true);
    aFacade.addNamedRange(u"Metric", std::nullopt, { nData, 0, 0 }, u"$Data.$A$1:$B$2");

    ComputationalObservationState aObservation;
    aObservation.maFormulaTree = { { nData, 1, 0 }, { nData, 1, 1 } };
    aObservation.maFormulaTrack = { { nData, 1, 1 } };
    aObservation.maCellBroadcasters.push_back({
        { nData, 0, 0 },
        { { ListenerAnchorKind::FormulaCell, { nData, 1, 0 }, 1 },
            { ListenerAnchorKind::FormulaGroup, { nData, 1, 0 }, 2 } } });

    const ComputationalWorkbookShadow aShadow
        = buildComputationalWorkbookShadow(aFacade, aObservation);
    const auto aInitialComparison = compareComputationalShadow(aShadow, aFacade, aObservation);

    if (aShadow.maSnapshot.mnGeneration != 9)
        return fail("computational_substrate", "snapshot generation mismatch");
    if (aShadow.getCellCount() != 4)
        return fail("computational_substrate", "cell count mismatch");
    if (aShadow.getFormulaCellCount() != 2)
        return fail("computational_substrate", "formula cell count mismatch");
    if (aShadow.maNamedRanges.size() != 1)
        return fail("computational_substrate", "named range count mismatch");
    if (aShadow.maFormulaGroups.size() != 1)
        return fail("computational_substrate", "formula group count mismatch");
    if (aShadow.maCellBroadcasters.size() != 1)
        return fail("computational_substrate", "cell broadcaster count mismatch");

    const ShadowCellRecord* pScalar = aShadow.findCell({ nData, 0, 0 });
    if (!pScalar || pScalar->hasFormula() || pScalar->mbInFormulaTree)
        return fail("computational_substrate", "scalar cell projection mismatch");

    const ShadowCellRecord* pFormula = aShadow.findCell({ nData, 1, 1 });
    if (!pFormula || !pFormula->hasFormula() || !pFormula->mbInFormulaTrack
        || !pFormula->moFormulaGroup.has_value() || !pFormula->moFormula->mbDirty
        || !pFormula->moFormula->mbNeedsRecalc)
    {
        return fail("computational_substrate", "formula cell projection mismatch");
    }

        if (aShadow.maFormulaGroups.front().maMembers.size() != 2)
            return fail("computational_substrate", "formula group membership mismatch");
        if (!aInitialComparison.mbFullMatch)
            return fail("computational_substrate", "initial shadow comparison mismatch");

        const auto aAdmittedContract
            = classifyAuthorityMutation(MutationEvent::setFormula({ nData, 1, 0 }, u"=A1*4"));
        if (!aAdmittedContract.isAdmitted() || !aAdmittedContract.mbRequiresCleanBaseline)
            return fail("computational_substrate", "authority contract admission mismatch");

        const auto aValidationOnlyContract = classifyAuthorityMutation(
            MutationEvent::renameNamedRange(aFacade.getNamedRangeDescriptors().front(),
                aFacade.getNamedRangeDescriptors().front()));
        if (aValidationOnlyContract.meMutationClass != AuthorityMutationClass::ValidationOnly)
            return fail("computational_substrate", "authority contract validation-only mismatch");

        const auto aRejectedContract
            = classifyAuthorityMutation(MutationEvent::clearRange({ { nData, 0, 0 }, { nData, 1, 1 } }));
        if (aRejectedContract.meMutationClass != AuthorityMutationClass::Rejected)
            return fail("computational_substrate", "authority contract rejection mismatch");

        AuthorityPilotInput aAuthorityInput;
        aAuthorityInput.maComputationalShadow = aShadow;
        aAuthorityInput.maMutation = MutationEvent::clearCell({ nData, 0, 1 });
        aAuthorityInput.mbCleanBaseline = true;

        AuthorityPilotTransition aTransition;
        aTransition.maInput = aAuthorityInput;
        aTransition.maContract = classifyAuthorityMutation(aAuthorityInput.maMutation);
        aTransition.maVerification = makeAuthorityVerification(aTransition.maContract);
        aTransition.meVerdict = AuthorityPilotVerdict::Applicable;

        if (aTransition.isRejected()
            || aTransition.maVerification.meQueueMode != AuthorityVerificationMode::Exact
            || aTransition.meVerdict != AuthorityPilotVerdict::Applicable)
        {
            return fail("computational_substrate", "authority transition schema mismatch");
        }

        InMemoryWorkbookFacade aPilotFacade;
        aPilotFacade.setGrammar(aFacade.getGrammar());
        aPilotFacade.setGeneration(21);
        const auto nPilotSheet = aPilotFacade.addSheet(u"Pilot");
        aPilotFacade.setCell({ nPilotSheet, 0, 0 }, CellValue::number(10.0));
        aPilotFacade.setFormulaCell({ nPilotSheet, 1, 0 }, u"=A1*2", CellValue::number(20.0));
        aPilotFacade.setFormulaCell({ nPilotSheet, 2, 0 }, u"=B1+1", CellValue::number(21.0));

        ComputationalObservationState aPilotObservation;
        aPilotObservation.maFormulaTree = { { nPilotSheet, 1, 0 }, { nPilotSheet, 2, 0 } };
        aPilotObservation.maCellBroadcasters.push_back({
            { nPilotSheet, 0, 0 },
            { { ListenerAnchorKind::FormulaCell, { nPilotSheet, 1, 0 }, 1 } } });
        aPilotObservation.maCellBroadcasters.push_back({
            { nPilotSheet, 1, 0 },
            { { ListenerAnchorKind::FormulaCell, { nPilotSheet, 2, 0 }, 1 } } });

        const auto aPilotShadow = buildComputationalWorkbookShadow(aPilotFacade, aPilotObservation);
        aAuthorityInput.maComputationalShadow = aPilotShadow;
        aAuthorityInput.maGraphShadow = buildDependencyGraphShadow(aPilotShadow, aPilotObservation);
        aAuthorityInput.maIrShadow
            = authoritybuilddetail::buildAuthorityExecutionIrShadow(aPilotShadow, aPilotFacade);
        aAuthorityInput.maMutation = MutationEvent::setScalarValue({ nPilotSheet, 0, 0 });
        aAuthorityInput.moScalarValueAfter = CellValue::number(99.0);

        const auto aAuthorityPlan = buildAuthorityPilotTransition(aAuthorityInput);
        if (aAuthorityPlan.meVerdict != AuthorityPilotVerdict::Applicable)
        {
            if (aAuthorityPlan.maReason == u"mutation_out_of_contract")
                return fail("computational_substrate", "authority verdict out-of-contract");
            if (aAuthorityPlan.maReason == u"dirty_baseline")
                return fail("computational_substrate", "authority verdict dirty baseline");
            if (aAuthorityPlan.maReason == u"opaque_dependency_surface")
                return fail("computational_substrate", "authority verdict opaque dependency");
            if (aAuthorityPlan.maReason == u"missing_scalar_value_after")
                return fail("computational_substrate", "authority verdict missing scalar value");
            return fail("computational_substrate", "authority verdict mismatch");
        }
        if (aAuthorityPlan.maRecalcPlan.maQueue.size() != 2)
            return fail("computational_substrate", "authority queue size mismatch");
        if (aAuthorityPlan.maGraphAfter.getEdgeCount() < 2)
            return fail("computational_substrate", "authority graph edge mismatch");
        if (aAuthorityPlan.maIrAfter.getFormulaCount() != 2)
            return fail("computational_substrate", "authority ir rebuild mismatch");

        const auto* pAuthorityScalar
            = aAuthorityPlan.maComputationalAfter.findCell({ nPilotSheet, 0, 0 });
        const auto* pAuthorityFormula
            = aAuthorityPlan.maComputationalAfter.findCell({ nPilotSheet, 1, 0 });
        if (!pAuthorityScalar || !pAuthorityScalar->maCell.maValue.isNumber()
            || pAuthorityScalar->maCell.maValue.mfNumber != 99.0 || !pAuthorityFormula
            || !pAuthorityFormula->mbInFormulaTree)
        {
            return fail("computational_substrate", "authority projected shadow mismatch");
        }

        const auto aLifecycleAdmitted
            = classifyLifecycleMutation(MutationEvent::setFormula({ nPilotSheet, 3, 0 }, u"=A1+B1"));
        if (!aLifecycleAdmitted.isAdmitted() || !aLifecycleAdmitted.mbRequiresFormulaLifecycleShape)
            return fail("computational_substrate", "lifecycle contract admission mismatch");

        const auto aLifecycleRejected
            = classifyLifecycleMutation(MutationEvent::setScalarValue({ nPilotSheet, 0, 0 }));
        if (aLifecycleRejected.meMutationClass != LifecycleMutationClass::Rejected)
            return fail("computational_substrate", "lifecycle contract rejection mismatch");

        LifecyclePilotInput aLifecycleInput;
        aLifecycleInput.maComputationalShadow = aPilotShadow;
        aLifecycleInput.maGraphShadow = aAuthorityInput.maGraphShadow;
        aLifecycleInput.maIrShadow = aAuthorityInput.maIrShadow;
        aLifecycleInput.maMutation = MutationEvent::clearCell({ nPilotSheet, 1, 0 });
        aLifecycleInput.mbCleanBaseline = true;

        LifecyclePilotTransition aLifecycleTransition;
        aLifecycleTransition.maInput = aLifecycleInput;
        aLifecycleTransition.maContract = classifyLifecycleMutation(aLifecycleInput.maMutation);
        aLifecycleTransition.maVerification = makeLifecycleVerification(aLifecycleTransition.maContract);
        aLifecycleTransition.maSyncActions.push_back({
            LifecycleSyncActionKind::RemoveFormulaCell, { nPilotSheet, 1, 0 }, std::nullopt,
            std::nullopt, true, false });
        aLifecycleTransition.meVerdict = LifecyclePilotVerdict::Applicable;

        if (aLifecycleTransition.isRejected()
            || aLifecycleTransition.maVerification.meComputationalMode
                   != LifecycleVerificationMode::Exact
            || !aLifecycleTransition.maVerification.mbObserveIrOnly
            || aLifecycleTransition.meVerdict != LifecyclePilotVerdict::Applicable
            || aLifecycleTransition.maSyncActions.size() != 1)
        {
            return fail("computational_substrate", "lifecycle transition schema mismatch");
        }

        LifecyclePilotInput aLifecycleInsertInput;
        aLifecycleInsertInput.maComputationalShadow = aPilotShadow;
        aLifecycleInsertInput.maGraphShadow = aAuthorityInput.maGraphShadow;
        aLifecycleInsertInput.maIrShadow = aAuthorityInput.maIrShadow;
        aLifecycleInsertInput.maMutation
            = MutationEvent::setFormula({ nPilotSheet, 3, 0 }, u"=A1+B1");
        aLifecycleInsertInput.mbCleanBaseline = true;

        const auto aLifecycleInsertPlan = buildLifecyclePilotTransition(aLifecycleInsertInput);
        if (aLifecycleInsertPlan.meVerdict != LifecyclePilotVerdict::Applicable
            || aLifecycleInsertPlan.maSyncActions.size() != 1
            || aLifecycleInsertPlan.maSyncActions.front().meKind
                   != LifecycleSyncActionKind::InsertFormulaCell
            || !aLifecycleInsertPlan.maComputationalAfter.findCell({ nPilotSheet, 3, 0 }))
        {
            return fail("computational_substrate", "lifecycle insertion transition mismatch");
        }

        LifecyclePilotInput aLifecycleReplaceInput;
        aLifecycleReplaceInput.maComputationalShadow = aPilotShadow;
        aLifecycleReplaceInput.maGraphShadow = aAuthorityInput.maGraphShadow;
        aLifecycleReplaceInput.maIrShadow = aAuthorityInput.maIrShadow;
        aLifecycleReplaceInput.maMutation
            = MutationEvent::setFormula({ nPilotSheet, 1, 0 }, u"=A1*3");
        aLifecycleReplaceInput.mbCleanBaseline = true;

        const auto aLifecycleReplacePlan = buildLifecyclePilotTransition(aLifecycleReplaceInput);
        if (aLifecycleReplacePlan.meVerdict != LifecyclePilotVerdict::Applicable
            || aLifecycleReplacePlan.maSyncActions.size() != 1
            || aLifecycleReplacePlan.maSyncActions.front().meKind
                   != LifecycleSyncActionKind::ReplaceFormulaCell
            || !aLifecycleReplacePlan.maComputationalAfter.findCell({ nPilotSheet, 1, 0 }))
        {
            return fail("computational_substrate", "lifecycle replacement transition mismatch");
        }

        LifecyclePilotInput aLifecycleRemoveInput;
        aLifecycleRemoveInput.maComputationalShadow = aPilotShadow;
        aLifecycleRemoveInput.maGraphShadow = aAuthorityInput.maGraphShadow;
        aLifecycleRemoveInput.maIrShadow = aAuthorityInput.maIrShadow;
        aLifecycleRemoveInput.maMutation = MutationEvent::clearCell({ nPilotSheet, 1, 0 });
        aLifecycleRemoveInput.mbCleanBaseline = true;

        const auto aLifecycleRemovePlan = buildLifecyclePilotTransition(aLifecycleRemoveInput);
        if (aLifecycleRemovePlan.meVerdict != LifecyclePilotVerdict::Applicable
            || aLifecycleRemovePlan.maSyncActions.size() != 1
            || aLifecycleRemovePlan.maSyncActions.front().meKind
                   != LifecycleSyncActionKind::RemoveFormulaCell
            || aLifecycleRemovePlan.maComputationalAfter.findCell({ nPilotSheet, 1, 0 }))
        {
            return fail("computational_substrate", "lifecycle removal transition mismatch");
        }

        const auto aStructuralAdmitted
            = classifyStructuralMutation(MutationEvent::insertRows(nPilotSheet, 1, 1));
        if (!aStructuralAdmitted.isAdmitted()
            || !aStructuralAdmitted.mbRequiresScalarStructuralSlice)
        {
            return fail("computational_substrate", "structural contract admission mismatch");
        }

        const auto aStructuralDeleteRowAdmitted
            = classifyStructuralMutation(MutationEvent::deleteRows(nPilotSheet, 1, 1));
        if (!aStructuralDeleteRowAdmitted.isAdmitted())
        {
            return fail("computational_substrate", "structural delete-row admission mismatch");
        }

        const auto aStructuralInsertColumnAdmitted
            = classifyStructuralMutation(MutationEvent::insertColumns(nPilotSheet, 1, 1));
        if (!aStructuralInsertColumnAdmitted.isAdmitted())
        {
            return fail("computational_substrate",
                "structural insert-column admission mismatch");
        }

        const auto aStructuralRejected
            = classifyStructuralMutation(MutationEvent::setFormula({ nPilotSheet, 1, 0 }, u"=A1"));
        if (aStructuralRejected.meMutationClass != StructuralMutationClass::Rejected)
        {
            return fail("computational_substrate", "structural contract rejection mismatch");
        }

        const auto aStructuralVerification = makeStructuralVerification(aStructuralAdmitted);
        if (aStructuralVerification.meQueueMode != StructuralVerificationMode::Exact
            || !aStructuralVerification.mbObserveIrOnly)
        {
            return fail("computational_substrate", "structural verification mismatch");
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(31);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(10.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(20.0));
            aBeforeFacade.setFormulaCell(
                { nSheet, 0, 2 }, u"=$A$2*1", CellValue::number(20.0));

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 0, 2 } };
            aBeforeObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 1 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 0, 2 }, 1 } } });
            const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(32);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(10.0));
            aAfterFacade.setCell({ nSheet, 0, 2 }, CellValue::number(20.0));
            aAfterFacade.setFormulaCell(
                { nSheet, 0, 3 }, u"=$A$3*1", CellValue::number(20.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 0, 3 } };
            aAfterObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 2 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 0, 3 }, 1 } } });
            const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertRows(nSheet, 1, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan
                = buildStructuralPilotTransition(aStructuralInput, aAfterFacade, aAfterObservation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::Applicable
                || aStructuralPlan.maSyncActions.size() != 1
                || aStructuralPlan.maSyncActions.front().meKind
                       != StructuralSyncActionKind::InsertRows
                || aStructuralPlan.maComputationalAfter.getFormulaCellCount() != 1
                || aStructuralPlan.maRecalcPlan.maQueue.size() != 1
                || aStructuralPlan.maGraphAfter.getEdgeCount() < 1
                || aStructuralPlan.maReferenceUpdates.size() != 1
                || !aStructuralPlan.maReferenceUpdates.front().moAfterId.has_value()
                || !(aStructuralPlan.maReferenceUpdates.front().moAfterId->maAddress
                     == spreadsheetengine::api::CellAddress { nSheet, 0, 3 }))
            {
                return fail("computational_substrate", "structural insert-row transition mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(41);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setCell({ nSheet, 1, 0 }, CellValue::number(2.0));
            aBeforeFacade.setFormulaCell({ nSheet, 2, 0 }, u"=$B$1+1", CellValue::number(3.0));

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 2, 0 } };
            aBeforeObservation.maCellBroadcasters.push_back({
                { nSheet, 1, 0 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 2, 0 }, 1 } } });
            const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(42);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(2.0));
            aAfterFacade.setFormulaCell({ nSheet, 1, 0 }, u"=$A$1+1", CellValue::number(3.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 1, 0 } };
            aAfterObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 0 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 1, 0 }, 1 } } });
            const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::deleteColumns(nSheet, 0, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan
                = buildStructuralPilotTransition(aStructuralInput, aAfterFacade, aAfterObservation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::Applicable
                || aStructuralPlan.maSyncActions.size() != 1
                || aStructuralPlan.maSyncActions.front().meKind
                       != StructuralSyncActionKind::DeleteColumns
                || aStructuralPlan.maComputationalAfter.getCellCount() != 2
                || aStructuralPlan.maReferenceUpdates.size() != 1
                || !aStructuralPlan.maReferenceUpdates.front().maSummary.mbChanged
                || aStructuralPlan.maReferenceUpdates.front().mbRemovedByStructure)
            {
                return fail("computational_substrate",
                    "structural delete-column transition mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(51);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(10.0));
            aBeforeFacade.setCell({ nSheet, 0, 1 }, CellValue::number(20.0));
            aBeforeFacade.setFormulaCell(
                { nSheet, 0, 2 }, u"=$A$2*1", CellValue::number(20.0));

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 0, 2 } };
            aBeforeObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 1 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 0, 2 }, 1 } } });
            const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(52);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 0, 0 }, CellValue::number(20.0));
            aAfterFacade.setFormulaCell(
                { nSheet, 0, 1 }, u"=$A$1*1", CellValue::number(20.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 0, 1 } };
            aAfterObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 0 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 0, 1 }, 1 } } });
            const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::deleteRows(nSheet, 0, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan
                = buildStructuralPilotTransition(aStructuralInput, aAfterFacade, aAfterObservation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::Applicable
                || aStructuralPlan.maSyncActions.size() != 1
                || aStructuralPlan.maSyncActions.front().meKind
                       != StructuralSyncActionKind::DeleteRows
                || aStructuralPlan.maComputationalAfter.getFormulaCellCount() != 1
                || aStructuralPlan.maRecalcPlan.maQueue.size() != 1
                || aStructuralPlan.maGraphAfter.getEdgeCount() < 1
                || aStructuralPlan.maReferenceUpdates.size() != 1
                || !aStructuralPlan.maReferenceUpdates.front().moAfterId.has_value()
                || !(aStructuralPlan.maReferenceUpdates.front().moAfterId->maAddress
                     == spreadsheetengine::api::CellAddress { nSheet, 0, 1 }))
            {
                return fail("computational_substrate", "structural delete-row transition mismatch");
            }
        }

        {
            InMemoryWorkbookFacade aBeforeFacade;
            aBeforeFacade.setGrammar(aFacade.getGrammar());
            aBeforeFacade.setGeneration(61);
            const auto nSheet = aBeforeFacade.addSheet(u"Pilot");
            aBeforeFacade.setCell({ nSheet, 0, 0 }, CellValue::number(1.0));
            aBeforeFacade.setFormulaCell(
                { nSheet, 1, 0 }, u"=$A$1+1", CellValue::number(2.0));

            ComputationalObservationState aBeforeObservation;
            aBeforeObservation.maFormulaTree = { { nSheet, 1, 0 } };
            aBeforeObservation.maCellBroadcasters.push_back({
                { nSheet, 0, 0 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 1, 0 }, 1 } } });
            const auto aBeforeShadow = buildComputationalWorkbookShadow(aBeforeFacade, aBeforeObservation);
            const auto aBeforeGraph = buildDependencyGraphShadow(aBeforeShadow, aBeforeObservation);
            const auto aBeforeIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aBeforeShadow, aBeforeFacade);

            InMemoryWorkbookFacade aAfterFacade;
            aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
            aAfterFacade.setGeneration(62);
            aAfterFacade.addSheet(u"Pilot");
            aAfterFacade.setCell({ nSheet, 1, 0 }, CellValue::number(1.0));
            aAfterFacade.setFormulaCell(
                { nSheet, 2, 0 }, u"=$B$1+1", CellValue::number(2.0));

            ComputationalObservationState aAfterObservation;
            aAfterObservation.maFormulaTree = { { nSheet, 2, 0 } };
            aAfterObservation.maCellBroadcasters.push_back({
                { nSheet, 1, 0 },
                { { ListenerAnchorKind::FormulaCell, { nSheet, 2, 0 }, 1 } } });
            const auto aAfterShadow = buildComputationalWorkbookShadow(aAfterFacade, aAfterObservation);
            const auto aAfterIr
                = authoritybuilddetail::buildAuthorityExecutionIrShadow(aAfterShadow, aAfterFacade);

            StructuralPilotInput aStructuralInput;
            aStructuralInput.maComputationalShadow = aBeforeShadow;
            aStructuralInput.maGraphShadow = aBeforeGraph;
            aStructuralInput.maIrShadow = aBeforeIr;
            aStructuralInput.maObservedAfterComputationalShadow = aAfterShadow;
            aStructuralInput.maObservedAfterIrShadow = aAfterIr;
            aStructuralInput.maMutation = MutationEvent::insertColumns(nSheet, 0, 1);
            aStructuralInput.mbCleanBaseline = true;

            const auto aStructuralPlan
                = buildStructuralPilotTransition(aStructuralInput, aAfterFacade, aAfterObservation);
            if (aStructuralPlan.meVerdict != StructuralPilotVerdict::Applicable
                || aStructuralPlan.maSyncActions.size() != 1
                || aStructuralPlan.maSyncActions.front().meKind
                       != StructuralSyncActionKind::InsertColumns
                || aStructuralPlan.maComputationalAfter.getFormulaCellCount() != 1
                || aStructuralPlan.maRecalcPlan.maQueue.size() != 1
                || aStructuralPlan.maGraphAfter.getEdgeCount() < 1
                || aStructuralPlan.maReferenceUpdates.size() != 1
                || !aStructuralPlan.maReferenceUpdates.front().moAfterId.has_value()
                || !(aStructuralPlan.maReferenceUpdates.front().moAfterId->maAddress
                     == spreadsheetengine::api::CellAddress { nSheet, 2, 0 }))
            {
                return fail("computational_substrate",
                    "structural insert-column transition mismatch");
            }
        }

    // --- Safe mutation rebuild path ---
    {
        aFacade.setCell({ nData, 0, 0 }, CellValue::number(11.0));
        auto aMutationState = rebuildComputationalShadowAfterMutation(
            aFacade, aObservation, MutationEvent::setScalarValue({ nData, 0, 0 }));
        if (!compareComputationalShadow(aMutationState.maShadow, aFacade, aObservation).mbFullMatch)
            return fail("computational_substrate", "scalar mutation comparison mismatch");
        const auto* pUpdatedScalar = aMutationState.maShadow.findCell({ nData, 0, 0 });
        if (!pUpdatedScalar || !pUpdatedScalar->maCell.maValue.isNumber()
            || pUpdatedScalar->maCell.maValue.mfNumber != 11.0)
        {
            return fail("computational_substrate", "scalar mutation rebuild mismatch");
        }

        aFacade.setFormulaCell({ nData, 1, 0 }, u"=A1*4", CellValue::number(44.0));
        aMutationState = rebuildComputationalShadowAfterMutation(
            aFacade, aObservation, MutationEvent::setFormula({ nData, 1, 0 }, u"=A1*4"));
        if (!compareComputationalShadow(aMutationState.maShadow, aFacade, aObservation).mbFullMatch)
            return fail("computational_substrate", "formula edit comparison mismatch");
        const auto* pEditedFormula = aMutationState.maShadow.findCell({ nData, 1, 0 });
        if (!pEditedFormula || !pEditedFormula->moFormula
            || pEditedFormula->moFormula->maFormulaSource != u"=A1*4")
        {
            return fail("computational_substrate", "formula edit rebuild mismatch");
        }

        aFacade.setFormulaCell({ nData, 2, 0 }, u"=A1+B1", CellValue::number(55.0));
        aObservation.maFormulaTree.push_back({ nData, 2, 0 });
        aMutationState = rebuildComputationalShadowAfterMutation(
            aFacade, aObservation, MutationEvent::setFormula({ nData, 2, 0 }, u"=A1+B1"));
        if (!compareComputationalShadow(aMutationState.maShadow, aFacade, aObservation).mbFullMatch)
            return fail("computational_substrate", "formula insert comparison mismatch");
        if (aMutationState.maShadow.getFormulaCellCount() != 3
            || !aMutationState.maShadow.findCell({ nData, 2, 0 }))
        {
            return fail("computational_substrate", "formula insertion rebuild mismatch");
        }

        aFacade.clearCell({ nData, 0, 1 });
        aMutationState = rebuildComputationalShadowAfterMutation(
            aFacade, aObservation, MutationEvent::clearCell({ nData, 0, 1 }));
        if (!compareComputationalShadow(aMutationState.maShadow, aFacade, aObservation).mbFullMatch)
            return fail("computational_substrate", "clear cell comparison mismatch");
        if (aMutationState.maShadow.findCell({ nData, 0, 1 }))
            return fail("computational_substrate", "clear cell rebuild mismatch");

        const auto aBeforeRange = *aFacade.findNamedRange(u"Metric", std::nullopt);
        if (!aFacade.renameNamedRange(u"Metric", u"MetricRenamed"))
            return fail("computational_substrate", "named range rename setup failed");
        const auto aAfterRange = *aFacade.findNamedRange(u"MetricRenamed", std::nullopt);
        aMutationState = rebuildComputationalShadowAfterMutation(
            aFacade, aObservation, MutationEvent::renameNamedRange(aBeforeRange, aAfterRange));
        if (!compareComputationalShadow(aMutationState.maShadow, aFacade, aObservation).mbFullMatch)
            return fail("computational_substrate", "named range comparison mismatch");
        if (aMutationState.maShadow.maNamedRanges.size() != 1
            || aMutationState.maShadow.maNamedRanges.front().maName != u"MetricRenamed")
        {
            return fail("computational_substrate", "named range rebuild mismatch");
        }
    }

    std::cout << "computational_substrate_tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
