/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>
#include <optional>

#include <spreadsheetengine/detail/substrate/ComputationalShadow.hxx>
#include <spreadsheetengine/detail/workbook/FacadeConsumers.hxx>
#include <spreadsheetengine/detail/workbook/InMemoryWorkbookFacade.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacade.hxx>
#include <spreadsheetengine/detail/workbook/WorkbookFacadeTypes.hxx>

#include "TestSupport.hxx"

int main()
{
    using namespace spreadsheetengine::detail::facade;
    using spreadsheetengine::api::CellAddress;
    using spreadsheetengine::api::CellRange;
    using spreadsheetengine::api::CellValue;
    using spreadsheetengine::standalone::test::fail;

    // --- FormulaCellId ---
    {
        FormulaCellId aId1 { { 0, 1, 2 } };
        FormulaCellId aId2 { { 0, 1, 2 } };
        FormulaCellId aId3 { { 0, 1, 3 } };

        if (!(aId1 == aId2))
            return fail("workbook_facade_types", "FormulaCellId equality failed");
        if (aId1 == aId3)
            return fail("workbook_facade_types", "FormulaCellId inequality failed");
    }

    // --- NamedRangeId ---
    {
        NamedRangeId aGlobal { 0, std::nullopt };
        NamedRangeId aLocal { 1, SheetId { 2 } };
        NamedRangeId aInvalid {};

        if (!aGlobal.isValid())
            return fail("workbook_facade_types", "NamedRangeId global should be valid");
        if (!aGlobal.isGlobal())
            return fail("workbook_facade_types", "NamedRangeId global scope failed");
        if (!aLocal.isValid())
            return fail("workbook_facade_types", "NamedRangeId local should be valid");
        if (aLocal.isGlobal())
            return fail("workbook_facade_types", "NamedRangeId local scope failed");
        if (aInvalid.isValid())
            return fail("workbook_facade_types", "NamedRangeId default should be invalid");
    }

    // --- SheetDescriptor ---
    {
        SheetDescriptor aSheet1 { 0, u"Sheet1", false };
        SheetDescriptor aSheet2 { 0, u"Sheet1", false };
        SheetDescriptor aSheet3 { 1, u"Sheet2", true };

        if (!(aSheet1 == aSheet2))
            return fail("workbook_facade_types", "SheetDescriptor equality failed");
        if (aSheet1 == aSheet3)
            return fail("workbook_facade_types", "SheetDescriptor inequality failed");
    }

    // --- CellDescriptor ---
    {
        CellDescriptor aEmpty {};
        if (aEmpty.meKind != CellKind::Empty)
            return fail("workbook_facade_types", "CellDescriptor default should be empty");

        CellDescriptor aScalar { { 0, 0, 0 }, CellKind::Scalar, CellValue::number(42.0), false };
        if (aScalar.meKind != CellKind::Scalar || aScalar.mbHasFormula)
            return fail("workbook_facade_types", "CellDescriptor scalar mismatch");

        CellDescriptor aFormula { { 0, 0, 0 }, CellKind::Formula, CellValue::number(10.0), true };
        if (aFormula.meKind != CellKind::Formula || !aFormula.mbHasFormula)
            return fail("workbook_facade_types", "CellDescriptor formula mismatch");
    }

    // --- FormulaCellDescriptor ---
    {
        FormulaCellDescriptor aDesc {};
        aDesc.maId = FormulaCellId { { 0, 5, 10 } };
        aDesc.maCachedValue = CellValue::number(99.0);
        aDesc.maFormulaSource = u"=A1+B1";
        aDesc.meKind = FormulaCellKind::Ordinary;
        aDesc.mbDirty = true;
        aDesc.mbNeedsRecalc = false;

        if (aDesc.meKind != FormulaCellKind::Ordinary)
            return fail("workbook_facade_types", "FormulaCellDescriptor kind mismatch");
        if (!aDesc.mbDirty)
            return fail("workbook_facade_types", "FormulaCellDescriptor dirty flag mismatch");

        FormulaCellDescriptor aGroupMember {};
        aGroupMember.meKind = FormulaCellKind::SharedGroupMember;
        if (aGroupMember.meKind != FormulaCellKind::SharedGroupMember)
            return fail("workbook_facade_types", "FormulaCellDescriptor shared group mismatch");
    }

    // --- NamedRangeDescriptor ---
    {
        NamedRangeDescriptor aGlobalRange {};
        aGlobalRange.maId = NamedRangeId { 0, std::nullopt };
        aGlobalRange.maName = u"MyRange";
        aGlobalRange.meScope = NamedRangeScope::Global;
        aGlobalRange.maBaseAddress = { 0, 0, 0 };
        aGlobalRange.maTargetExpression = u"$Sheet1.$A$1:$B$10";

        if (aGlobalRange.meScope != NamedRangeScope::Global)
            return fail("workbook_facade_types", "NamedRangeDescriptor global scope mismatch");

        NamedRangeDescriptor aLocalRange {};
        aLocalRange.maId = NamedRangeId { 1, SheetId { 0 } };
        aLocalRange.maName = u"LocalName";
        aLocalRange.meScope = NamedRangeScope::SheetLocal;
        aLocalRange.moScopeSheet = SheetId { 0 };

        if (aLocalRange.meScope != NamedRangeScope::SheetLocal
            || !aLocalRange.moScopeSheet.has_value())
        {
            return fail("workbook_facade_types", "NamedRangeDescriptor local scope mismatch");
        }
    }

    // --- FormulaGroupDescriptor ---
    {
        FormulaGroupDescriptor aGroup { { 0, 0, 0 }, 5, true };
        if (!aGroup.isValid())
            return fail("workbook_facade_types", "FormulaGroupDescriptor should be valid");
        if (aGroup.mnLength != 5)
            return fail("workbook_facade_types", "FormulaGroupDescriptor length mismatch");

        FormulaGroupDescriptor aEmpty {};
        if (aEmpty.isValid())
            return fail("workbook_facade_types", "FormulaGroupDescriptor default should be invalid");
    }

    // --- WorkbookSnapshotInfo ---
    {
        WorkbookSnapshotInfo aInfo { 1, 3, 100 };
        WorkbookSnapshotInfo aInfo2 { 1, 3, 100 };

        if (!(aInfo == aInfo2))
            return fail("workbook_facade_types", "WorkbookSnapshotInfo equality failed");

        WorkbookSnapshotInfo aInfo3 { 2, 3, 100 };
        if (aInfo == aInfo3)
            return fail("workbook_facade_types", "WorkbookSnapshotInfo inequality failed");
    }

    // --- MutationEvent factory helpers ---
    {
        const auto aSetValue = MutationEvent::setScalarValue({ 0, 1, 2 });
        if (aSetValue.meKind != MutationKind::SetScalarValue
            || aSetValue.maAddress.mnColumn != 1 || aSetValue.maAddress.mnRow != 2)
        {
            return fail("workbook_facade_types", "MutationEvent::setScalarValue mismatch");
        }

        const auto aSetFormula = MutationEvent::setFormula({ 0, 0, 0 }, u"=SUM(A1:A10)");
        if (aSetFormula.meKind != MutationKind::SetFormula
            || aSetFormula.maText != u"=SUM(A1:A10)")
        {
            return fail("workbook_facade_types", "MutationEvent::setFormula mismatch");
        }

        const auto aClearCell = MutationEvent::clearCell({ 1, 2, 3 });
        if (aClearCell.meKind != MutationKind::ClearCell)
            return fail("workbook_facade_types", "MutationEvent::clearCell mismatch");

        const auto aClearRange = MutationEvent::clearRange({ { 0, 0, 0 }, { 0, 5, 10 } });
        if (aClearRange.meKind != MutationKind::ClearRange)
            return fail("workbook_facade_types", "MutationEvent::clearRange mismatch");

        const auto aInsertRows = MutationEvent::insertRows(0, 5, 3);
        if (aInsertRows.meKind != MutationKind::InsertRows || aInsertRows.mnCount != 3
            || aInsertRows.maAddress.mnRow != 5)
        {
            return fail("workbook_facade_types", "MutationEvent::insertRows mismatch");
        }

        const auto aDeleteRows = MutationEvent::deleteRows(0, 2, 1);
        if (aDeleteRows.meKind != MutationKind::DeleteRows || aDeleteRows.mnCount != 1)
            return fail("workbook_facade_types", "MutationEvent::deleteRows mismatch");

        const auto aInsertCols = MutationEvent::insertColumns(0, 3, 2);
        if (aInsertCols.meKind != MutationKind::InsertColumns || aInsertCols.mnCount != 2)
            return fail("workbook_facade_types", "MutationEvent::insertColumns mismatch");

        const auto aDeleteCols = MutationEvent::deleteColumns(1, 0, 4);
        if (aDeleteCols.meKind != MutationKind::DeleteColumns || aDeleteCols.mnCount != 4)
            return fail("workbook_facade_types", "MutationEvent::deleteColumns mismatch");

        CellRange aSrc { { 0, 0, 0 }, { 0, 3, 3 } };
        CellRange aDst { { 1, 0, 0 }, { 1, 3, 3 } };
        const auto aMoveRange = MutationEvent::moveRange(aSrc, aDst);
        if (aMoveRange.meKind != MutationKind::MoveRange || aMoveRange.mbCopy)
            return fail("workbook_facade_types", "MutationEvent::moveRange mismatch");

        const auto aCopyRange = MutationEvent::copyRange(aSrc, aDst);
        if (aCopyRange.meKind != MutationKind::CopyRange || !aCopyRange.mbCopy)
            return fail("workbook_facade_types", "MutationEvent::copyRange mismatch");

        const auto aRenameSheet = MutationEvent::renameSheet(0, u"NewName");
        if (aRenameSheet.meKind != MutationKind::RenameSheet
            || aRenameSheet.maText != u"NewName")
        {
            return fail("workbook_facade_types", "MutationEvent::renameSheet mismatch");
        }

        NamedRangeDescriptor aOldNamedRange {};
        aOldNamedRange.maId = NamedRangeId { 7, SheetId { 0 } };
        aOldNamedRange.maName = u"Old";
        aOldNamedRange.meScope = NamedRangeScope::SheetLocal;
        aOldNamedRange.moScopeSheet = SheetId { 0 };
        aOldNamedRange.maBaseAddress = { 0, 1, 2 };
        aOldNamedRange.maTargetExpression = u"$A$1:$B$4";

        NamedRangeDescriptor aNewNamedRange = aOldNamedRange;
        aNewNamedRange.maName = u"New";

        const auto aAddName = MutationEvent::addNamedRange(aNewNamedRange);
        if (aAddName.meKind != MutationKind::AddNamedRange || aAddName.maText != u"New"
            || aAddName.moNamedRangeBefore.has_value() || !aAddName.moNamedRangeAfter
            || aAddName.moNamedRangeAfter->maTargetExpression != u"$A$1:$B$4")
        {
            return fail("workbook_facade_types", "MutationEvent::addNamedRange mismatch");
        }

        const auto aRemoveName = MutationEvent::removeNamedRange(aOldNamedRange);
        if (aRemoveName.meKind != MutationKind::RemoveNamedRange || !aRemoveName.moNamedRangeBefore
            || aRemoveName.moNamedRangeAfter.has_value()
            || aRemoveName.moNamedRangeBefore->maName != u"Old")
        {
            return fail("workbook_facade_types", "MutationEvent::removeNamedRange mismatch");
        }

        const auto aRenameName = MutationEvent::renameNamedRange(aOldNamedRange, aNewNamedRange);
        if (aRenameName.meKind != MutationKind::RenameNamedRange
            || aRenameName.maText != u"New"
            || !aRenameName.moNamedRangeBefore || !aRenameName.moNamedRangeAfter
            || aRenameName.moNamedRangeBefore->maName != u"Old"
            || aRenameName.moNamedRangeAfter->maName != u"New")
        {
            return fail("workbook_facade_types", "MutationEvent::renameNamedRange mismatch");
        }
    }

    // --- MutationEvent equality ---
    {
        const auto aEvent1 = MutationEvent::setScalarValue({ 0, 1, 2 });
        const auto aEvent2 = MutationEvent::setScalarValue({ 0, 1, 2 });
        const auto aEvent3 = MutationEvent::clearCell({ 0, 1, 2 });

        if (!(aEvent1 == aEvent2))
            return fail("workbook_facade_types", "MutationEvent equality failed");
        if (aEvent1 == aEvent3)
            return fail("workbook_facade_types", "MutationEvent inequality failed");
    }

    // =================================================================
    // InMemoryWorkbookFacade contract tests (Phases 2-3)
    // =================================================================

    // --- Build a test workbook ---
    InMemoryWorkbookFacade aFacade;
    aFacade.setGrammar({ spreadsheetengine::api::FormulaLanguage::Odff,
        spreadsheetengine::api::AddressConvention::OdfA1, false });
    aFacade.setGeneration(42);

    const auto nSheet0 = aFacade.addSheet(u"Data");
    const auto nSheet1 = aFacade.addSheet(u"Summary");
    aFacade.addSheet(u"Hidden", true);

    // Scalar cells.
    aFacade.setCell({ nSheet0, 0, 0 }, CellValue::number(100.0));
    aFacade.setCell({ nSheet0, 0, 1 }, CellValue::text(u"hello"));
    aFacade.setCell({ nSheet0, 1, 0 }, CellValue::boolean(true));

    // Formula cells.
    aFacade.setFormulaCell({ nSheet0, 0, 2 }, u"=A1+A2",
        CellValue::number(100.0));
    aFacade.setFormulaCell({ nSheet0, 0, 3 }, u"=SUM(A1:A3)",
        CellValue::number(200.0), FormulaCellKind::Ordinary, true);
    aFacade.setFormulaCell({ nSheet1, 0, 0 }, u"=Data.A1",
        CellValue::number(100.0));

    // Named ranges.
    aFacade.addNamedRange(u"MyRange", std::nullopt, { 0, 0, 0 },
        u"$Data.$A$1:$A$10");
    aFacade.addNamedRange(u"LocalName", SheetId { 0 }, { 0, 0, 0 },
        u"$A$1:$B$5");

    // Formula groups.
    aFacade.addFormulaGroup({ nSheet0, 0, 2 }, 2, true);

    // Use the facade through the abstract interface.
    const WorkbookFacade& rFacade = aFacade;

    // --- Phase 2: Workbook-level queries ---
    {
        if (rFacade.getSheetCount() != 3)
            return fail("facade_read", "sheet count mismatch");

        const auto oSheet0 = rFacade.findSheetId(u"Data");
        if (!oSheet0.has_value() || *oSheet0 != 0)
            return fail("facade_read", "findSheetId Data failed");

        const auto oSheet1 = rFacade.findSheetId(u"Summary");
        if (!oSheet1.has_value() || *oSheet1 != 1)
            return fail("facade_read", "findSheetId Summary failed");

        const auto oMissing = rFacade.findSheetId(u"NoSuchSheet");
        if (oMissing.has_value())
            return fail("facade_read", "findSheetId should return nullopt for missing");

        const auto oDesc = rFacade.getSheetDescriptor(0);
        if (!oDesc || oDesc->maName != u"Data" || oDesc->mbHidden)
            return fail("facade_read", "getSheetDescriptor(0) mismatch");

        const auto oHiddenDesc = rFacade.getSheetDescriptor(2);
        if (!oHiddenDesc || !oHiddenDesc->mbHidden)
            return fail("facade_read", "hidden sheet descriptor mismatch");

        const auto oInvalidDesc = rFacade.getSheetDescriptor(99);
        if (oInvalidDesc.has_value())
            return fail("facade_read", "getSheetDescriptor should return nullopt for invalid id");

        const auto aDescs = rFacade.getSheetDescriptors();
        if (aDescs.size() != 3 || aDescs[0].maName != u"Data"
            || aDescs[1].maName != u"Summary" || aDescs[2].maName != u"Hidden")
        {
            return fail("facade_read", "getSheetDescriptors mismatch");
        }
    }

    // --- Grammar and snapshot ---
    {
        const auto aGrammar = rFacade.getGrammar();
        if (aGrammar.meLanguage != spreadsheetengine::api::FormulaLanguage::Odff)
            return fail("facade_read", "grammar language mismatch");

        const auto aSnapshot = rFacade.getSnapshotInfo();
        if (aSnapshot.mnGeneration != 42 || aSnapshot.mnSheetCount != 3
            || aSnapshot.mnFormulaCellCount != 3)
        {
            return fail("facade_read", "snapshot info mismatch");
        }
    }

    // --- Phase 2: Cell-level queries ---
    {
        if (!rFacade.hasCell({ 0, 0, 0 }))
            return fail("facade_cell", "hasCell should be true for existing cell");
        if (rFacade.hasCell({ 0, 5, 5 }))
            return fail("facade_cell", "hasCell should be false for missing cell");

        // Scalar cell.
        const auto aScalarDesc = rFacade.getCellDescriptor({ 0, 0, 0 });
        if (aScalarDesc.meKind != CellKind::Scalar || aScalarDesc.mbHasFormula
            || !aScalarDesc.maValue.isNumber() || aScalarDesc.maValue.mfNumber != 100.0)
        {
            return fail("facade_cell", "scalar cell descriptor mismatch");
        }

        // Text cell.
        const auto aTextDesc = rFacade.getCellDescriptor({ 0, 0, 1 });
        if (aTextDesc.meKind != CellKind::Scalar || !aTextDesc.maValue.isText()
            || aTextDesc.maValue.maString != u"hello")
        {
            return fail("facade_cell", "text cell descriptor mismatch");
        }

        // Formula cell via getCellDescriptor.
        const auto aFormulaDesc = rFacade.getCellDescriptor({ 0, 0, 2 });
        if (aFormulaDesc.meKind != CellKind::Formula || !aFormulaDesc.mbHasFormula)
            return fail("facade_cell", "formula cell descriptor mismatch");

        // Empty cell.
        const auto aEmptyDesc = rFacade.getCellDescriptor({ 0, 9, 9 });
        if (aEmptyDesc.meKind != CellKind::Empty)
            return fail("facade_cell", "empty cell descriptor mismatch");

        // FormulaCellDescriptor.
        const auto oFcDesc = rFacade.getFormulaCellDescriptor({ 0, 0, 2 });
        if (!oFcDesc || oFcDesc->maFormulaSource != u"=A1+A2"
            || oFcDesc->meKind != FormulaCellKind::Ordinary)
        {
            return fail("facade_cell", "getFormulaCellDescriptor mismatch");
        }

        // Dirty formula cell.
        const auto oDirtyDesc = rFacade.getFormulaCellDescriptor({ 0, 0, 3 });
        if (!oDirtyDesc || !oDirtyDesc->mbDirty)
            return fail("facade_cell", "dirty formula cell mismatch");

        // Non-formula cell returns nullopt.
        const auto oNonFormula = rFacade.getFormulaCellDescriptor({ 0, 0, 0 });
        if (oNonFormula.has_value())
            return fail("facade_cell", "non-formula should return nullopt");
    }

    // --- Phase 2: Formula-cell iteration ---
    {
        sal_Int32 nSheet0FormulaCount = 0;
        rFacade.visitFormulaCells(0,
            [&nSheet0FormulaCount](const FormulaCellDescriptor&) {
                ++nSheet0FormulaCount;
                return true;
            });
        if (nSheet0FormulaCount != 2)
            return fail("facade_iteration", "sheet0 formula cell count mismatch");

        sal_Int32 nTotalFormulaCount = 0;
        rFacade.visitAllFormulaCells(
            [&nTotalFormulaCount](const FormulaCellDescriptor&) {
                ++nTotalFormulaCount;
                return true;
            });
        if (nTotalFormulaCount != 3)
            return fail("facade_iteration", "total formula cell count mismatch");

        // Early stop.
        sal_Int32 nStoppedCount = 0;
        rFacade.visitAllFormulaCells(
            [&nStoppedCount](const FormulaCellDescriptor&) {
                ++nStoppedCount;
                return false; // stop after first
            });
        if (nStoppedCount != 1)
            return fail("facade_iteration", "early stop iteration mismatch");
    }

    // --- Phase 2.5: Whole-cell iteration ---
    {
        sal_Int32 nSheet0CellCount = 0;
        rFacade.visitCells(0, [&nSheet0CellCount](const CellDescriptor&) {
            ++nSheet0CellCount;
            return true;
        });
        if (nSheet0CellCount != 5)
            return fail("facade_iteration", "sheet0 whole-cell count mismatch");

        sal_Int32 nWorkbookCellCount = 0;
        rFacade.visitAllCells([&nWorkbookCellCount](const CellDescriptor&) {
            ++nWorkbookCellCount;
            return true;
        });
        if (nWorkbookCellCount != 6)
            return fail("facade_iteration", "workbook whole-cell count mismatch");
    }

    // --- Phase 3: Named-range queries ---
    {
        if (rFacade.getNamedRangeCount() != 2)
            return fail("facade_named_range", "named range count mismatch");

        const auto oGlobal = rFacade.findNamedRange(u"MyRange");
        if (!oGlobal || oGlobal->meScope != NamedRangeScope::Global
            || oGlobal->maTargetExpression != u"$Data.$A$1:$A$10")
        {
            return fail("facade_named_range", "global named range lookup mismatch");
        }

        const auto oLocal = rFacade.findNamedRange(u"LocalName", SheetId { 0 });
        if (!oLocal || oLocal->meScope != NamedRangeScope::SheetLocal
            || !oLocal->moScopeSheet.has_value() || *oLocal->moScopeSheet != 0)
        {
            return fail("facade_named_range", "local named range lookup mismatch");
        }

        const auto oMissing = rFacade.findNamedRange(u"NoSuchRange");
        if (oMissing.has_value())
            return fail("facade_named_range", "missing named range should return nullopt");

        const auto aAllRanges = rFacade.getNamedRangeDescriptors();
        if (aAllRanges.size() != 2)
            return fail("facade_named_range", "getNamedRangeDescriptors count mismatch");
    }

    // --- Phase 3: Shared-formula/group queries ---
    {
        const auto oGroup = rFacade.getFormulaGroupDescriptor({ 0, 0, 2 });
        if (!oGroup || oGroup->mnLength != 2 || !oGroup->mbShareable
            || oGroup->maAnchor.mnRow != 2)
        {
            return fail("facade_group", "formula group descriptor mismatch");
        }

        // Member at offset 1 should see the same group.
        const auto oGroupMember = rFacade.getFormulaGroupDescriptor({ 0, 0, 3 });
        if (!oGroupMember || oGroupMember->maAnchor.mnRow != 2
            || oGroupMember->mnLength != 2)
        {
            return fail("facade_group", "formula group member mismatch");
        }

        // Non-grouped cell.
        const auto oNoGroup = rFacade.getFormulaGroupDescriptor({ 1, 0, 0 });
        if (oNoGroup.has_value())
            return fail("facade_group", "non-grouped cell should return nullopt");
    }

    // =================================================================
    // Phase 4: First real consumers
    // =================================================================

    // --- Formula-cell enumeration shadow consumer ---
    {
        const auto aEnum = consumers::enumerateFormulaCells(rFacade);
        if (aEnum.mnTotalFormulaCells != 3)
            return fail("facade_consumers", "formula cell enumeration total mismatch");
        if (aEnum.mnOrdinaryCells != 3)
            return fail("facade_consumers", "ordinary cell count mismatch");
        if (aEnum.mnDirtyCells != 1)
            return fail("facade_consumers", "dirty cell count mismatch");
    }

    // --- Shared-formula group summary consumer ---
    {
        const auto aGroupSummary = consumers::summarizeFormulaGroups(rFacade);
        if (aGroupSummary.mnGroupCount != 1)
            return fail("facade_consumers", "group count mismatch");
        if (aGroupSummary.mnTotalGroupLength != 2)
            return fail("facade_consumers", "group total length mismatch");
        if (aGroupSummary.mnShareableGroups != 1)
            return fail("facade_consumers", "shareable group count mismatch");
    }

    // --- Shared-formula regroup classification ---
    {
        InMemoryWorkbookFacade aBeforeFacade;
        aBeforeFacade.setGrammar(aFacade.getGrammar());
        aBeforeFacade.setGeneration(50);
        const auto nRegroupSheet = aBeforeFacade.addSheet(u"Pilot");
        aBeforeFacade.setCell({ nRegroupSheet, 0, 0 }, CellValue::number(1.0));
        aBeforeFacade.setCell({ nRegroupSheet, 0, 1 }, CellValue::number(2.0));
        aBeforeFacade.setCell({ nRegroupSheet, 0, 2 }, CellValue::number(3.0));
        aBeforeFacade.setFormulaCell({ nRegroupSheet, 1, 0 }, u"=A1*3",
            CellValue::number(3.0), FormulaCellKind::Ordinary, true, true);
        aBeforeFacade.setFormulaCell({ nRegroupSheet, 1, 1 }, u"=A2*2",
            CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nRegroupSheet, 1, 2 }, u"=A3*2",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.addFormulaGroup({ nRegroupSheet, 1, 1 }, 2, true);

        InMemoryWorkbookFacade aAfterFacade;
        aAfterFacade.setGrammar(aFacade.getGrammar());
        aAfterFacade.setGeneration(51);
        aAfterFacade.addSheet(u"Pilot");
        aAfterFacade.setCell({ nRegroupSheet, 0, 0 }, CellValue::number(1.0));
        aAfterFacade.setCell({ nRegroupSheet, 0, 1 }, CellValue::number(2.0));
        aAfterFacade.setCell({ nRegroupSheet, 0, 2 }, CellValue::number(3.0));
        aAfterFacade.setFormulaCell({ nRegroupSheet, 1, 0 }, u"=A1*3",
            CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nRegroupSheet, 1, 1 }, u"=A2*3",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nRegroupSheet, 1, 2 }, u"=A3*2",
            CellValue::number(6.0), FormulaCellKind::Ordinary, true, true);
        aAfterFacade.addFormulaGroup({ nRegroupSheet, 1, 0 }, 2, true);

        const auto aClassification = consumers::classifySharedFormulaMutation(
            aBeforeFacade, aAfterFacade,
            MutationEvent::setFormula({ nRegroupSheet, 1, 1 }, u"=A2*3"));
        if (aClassification.maTransition.meKind
                != consumers::SharedFormulaGroupTransitionKind::Rebuild
            || aClassification.meFamily
                   != consumers::SharedFormulaMutationFamily::Regroup
            || !aClassification.mbTouchedAddressSharedBefore
            || !aClassification.mbTouchedAddressSharedAfter)
        {
            return fail("facade_consumers", "shared-group regroup classification mismatch");
        }
    }

    // --- Shared-formula merge classification ---
    {
        InMemoryWorkbookFacade aBeforeFacade;
        aBeforeFacade.setGrammar(aFacade.getGrammar());
        aBeforeFacade.setGeneration(52);
        const auto nMergeSheet = aBeforeFacade.addSheet(u"Pilot");
        aBeforeFacade.setCell({ nMergeSheet, 0, 0 }, CellValue::number(1.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 1 }, CellValue::number(2.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 2 }, CellValue::number(3.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 3 }, CellValue::number(4.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 4 }, CellValue::number(5.0));
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 0 }, u"=A1*2",
            CellValue::number(2.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 1 }, u"=A2*2",
            CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 3 }, u"=A4*2",
            CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 4 }, u"=A5*2",
            CellValue::number(10.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.addFormulaGroup({ nMergeSheet, 1, 0 }, 2, true);
        aBeforeFacade.addFormulaGroup({ nMergeSheet, 1, 3 }, 2, true);

        InMemoryWorkbookFacade aAfterFacade;
        aAfterFacade.setGrammar(aFacade.getGrammar());
        aAfterFacade.setGeneration(53);
        aAfterFacade.addSheet(u"Pilot");
        aAfterFacade.setCell({ nMergeSheet, 0, 0 }, CellValue::number(1.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 1 }, CellValue::number(2.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 2 }, CellValue::number(3.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 3 }, CellValue::number(4.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 4 }, CellValue::number(5.0));
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 0 }, u"=A1*2",
            CellValue::number(2.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 1 }, u"=A2*2",
            CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 2 }, u"=A3*2",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 3 }, u"=A4*2",
            CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 4 }, u"=A5*2",
            CellValue::number(10.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.addFormulaGroup({ nMergeSheet, 1, 0 }, 5, true);

        const auto aClassification = consumers::classifySharedFormulaMutation(
            aBeforeFacade, aAfterFacade,
            MutationEvent::setFormula({ nMergeSheet, 1, 2 }, u"=A3*2"));
        if (aClassification.maTransition.meKind
                != consumers::SharedFormulaGroupTransitionKind::Rebuild
            || aClassification.meFamily
                   != consumers::SharedFormulaMutationFamily::Merge
            || aClassification.mbTouchedAddressSharedBefore
            || !aClassification.mbTouchedAddressSharedAfter
            || aClassification.mnBeforeNeighborhoodGroupCount != 2
            || aClassification.mnAfterNeighborhoodGroupCount != 1)
        {
            return fail("facade_consumers", "shared-group merge classification mismatch");
        }
    }

    // --- Shared-formula one-sided-insert classification ---
    {
        InMemoryWorkbookFacade aBeforeFacade;
        aBeforeFacade.setGrammar(aFacade.getGrammar());
        aBeforeFacade.setGeneration(54);
        const auto nMergeSheet = aBeforeFacade.addSheet(u"Pilot");
        aBeforeFacade.setCell({ nMergeSheet, 0, 0 }, CellValue::number(1.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 1 }, CellValue::number(2.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 2 }, CellValue::number(3.0));
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 0 }, u"=A1*2",
            CellValue::number(2.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 1 }, u"=A2*2",
            CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.addFormulaGroup({ nMergeSheet, 1, 0 }, 2, true);

        InMemoryWorkbookFacade aAfterFacade;
        aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
        aAfterFacade.setGeneration(55);
        aAfterFacade.addSheet(u"Pilot");
        aAfterFacade.setCell({ nMergeSheet, 0, 0 }, CellValue::number(1.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 1 }, CellValue::number(2.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 2 }, CellValue::number(3.0));
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 0 }, u"=A1*2",
            CellValue::number(2.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 1 }, u"=A2*2",
            CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 2 }, u"=A3*2",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.addFormulaGroup({ nMergeSheet, 1, 0 }, 3, true);

        const auto aClassification = consumers::classifySharedFormulaMutation(
            aBeforeFacade, aAfterFacade,
            MutationEvent::setFormula({ nMergeSheet, 1, 2 }, u"=A3*2"));
        if (aClassification.maTransition.meKind
                != consumers::SharedFormulaGroupTransitionKind::Rebuild
            || aClassification.meFamily
                   != consumers::SharedFormulaMutationFamily::OneSidedInsert
            || aClassification.mbTouchedAddressSharedBefore
            || !aClassification.mbTouchedAddressSharedAfter
            || aClassification.mnBeforeNeighborhoodGroupCount != 1
            || aClassification.mnAfterNeighborhoodGroupCount != 1)
        {
            return fail("facade_consumers",
                "shared-group one-sided insert classification mismatch");
        }
    }

    // --- Shared-formula replacement-merge classification ---
    {
        InMemoryWorkbookFacade aBeforeFacade;
        aBeforeFacade.setGrammar(aFacade.getGrammar());
        aBeforeFacade.setGeneration(56);
        const auto nMergeSheet = aBeforeFacade.addSheet(u"Pilot");
        aBeforeFacade.setCell({ nMergeSheet, 0, 0 }, CellValue::number(1.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 1 }, CellValue::number(2.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 2 }, CellValue::number(3.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 3 }, CellValue::number(4.0));
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 0 }, u"=A1*3",
            CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 1 }, u"=A2*3",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 2 }, u"=A3*2",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 3 }, u"=A4*2",
            CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.addFormulaGroup({ nMergeSheet, 1, 0 }, 2, true);
        aBeforeFacade.addFormulaGroup({ nMergeSheet, 1, 2 }, 2, true);

        InMemoryWorkbookFacade aAfterFacade;
        aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
        aAfterFacade.setGeneration(57);
        aAfterFacade.addSheet(u"Pilot");
        aAfterFacade.setCell({ nMergeSheet, 0, 0 }, CellValue::number(1.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 1 }, CellValue::number(2.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 2 }, CellValue::number(3.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 3 }, CellValue::number(4.0));
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 0 }, u"=A1*3",
            CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 1 }, u"=A2*3",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 2 }, u"=A3*3",
            CellValue::number(9.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 3 }, u"=A4*2",
            CellValue::number(8.0), FormulaCellKind::Ordinary, true, true);
        aAfterFacade.addFormulaGroup({ nMergeSheet, 1, 0 }, 3, true);

        const auto aClassification = consumers::classifySharedFormulaMutation(
            aBeforeFacade, aAfterFacade,
            MutationEvent::setFormula({ nMergeSheet, 1, 2 }, u"=A3*3"));
        if (aClassification.maTransition.meKind
                != consumers::SharedFormulaGroupTransitionKind::Rebuild
            || aClassification.meFamily
                   != consumers::SharedFormulaMutationFamily::ReplacementMerge
            || !aClassification.mbTouchedAddressSharedBefore
            || !aClassification.mbTouchedAddressSharedAfter
            || aClassification.mnBeforeNeighborhoodGroupCount != 2
            || aClassification.mnAfterNeighborhoodGroupCount != 1)
        {
            return fail("facade_consumers",
                "shared-group replacement-merge classification mismatch");
        }
    }

    // --- Shared-formula multi-group-collapse classification ---
    {
        InMemoryWorkbookFacade aBeforeFacade;
        aBeforeFacade.setGrammar(aFacade.getGrammar());
        aBeforeFacade.setGeneration(58);
        const auto nMergeSheet = aBeforeFacade.addSheet(u"Pilot");
        aBeforeFacade.setCell({ nMergeSheet, 0, 0 }, CellValue::number(1.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 1 }, CellValue::number(2.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 2 }, CellValue::number(3.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 3 }, CellValue::number(4.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 4 }, CellValue::number(5.0));
        aBeforeFacade.setCell({ nMergeSheet, 0, 5 }, CellValue::number(6.0));
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 0 }, u"=A1*3",
            CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 1 }, u"=A2*3",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 2 }, u"=A3*2",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 3 }, u"=A4*2",
            CellValue::number(8.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 4 }, u"=A5*3",
            CellValue::number(15.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nMergeSheet, 1, 5 }, u"=A6*3",
            CellValue::number(18.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.addFormulaGroup({ nMergeSheet, 1, 0 }, 2, true);
        aBeforeFacade.addFormulaGroup({ nMergeSheet, 1, 2 }, 2, true);
        aBeforeFacade.addFormulaGroup({ nMergeSheet, 1, 4 }, 2, true);

        InMemoryWorkbookFacade aAfterFacade;
        aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
        aAfterFacade.setGeneration(59);
        aAfterFacade.addSheet(u"Pilot");
        aAfterFacade.setCell({ nMergeSheet, 0, 0 }, CellValue::number(1.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 1 }, CellValue::number(2.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 2 }, CellValue::number(3.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 3 }, CellValue::number(4.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 4 }, CellValue::number(5.0));
        aAfterFacade.setCell({ nMergeSheet, 0, 5 }, CellValue::number(6.0));
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 0 }, u"=A1*3",
            CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 1 }, u"=A2*3",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 2 }, u"=A3*3",
            CellValue::number(9.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 3 }, u"=A4*3",
            CellValue::number(12.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 4 }, u"=A5*3",
            CellValue::number(15.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nMergeSheet, 1, 5 }, u"=A6*3",
            CellValue::number(18.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.addFormulaGroup({ nMergeSheet, 1, 0 }, 6, true);

        const auto aClassification = consumers::classifySharedFormulaMutation(
            aBeforeFacade, aAfterFacade,
            MutationEvent::setFormula({ nMergeSheet, 1, 2 }, u"=A3*3"));
        if (aClassification.maTransition.meKind
                != consumers::SharedFormulaGroupTransitionKind::Rebuild
            || aClassification.meFamily
                   != consumers::SharedFormulaMutationFamily::MultiGroupCollapse
            || !aClassification.mbTouchedAddressSharedBefore
            || !aClassification.mbTouchedAddressSharedAfter
            || aClassification.mnBeforeNeighborhoodGroupCount != 3
            || aClassification.mnAfterNeighborhoodGroupCount != 1)
        {
            return fail("facade_consumers",
                "shared-group multi-group collapse classification mismatch");
        }
    }

    // --- Shared-formula named-range-combined preserve classification ---
    {
        InMemoryWorkbookFacade aBeforeFacade;
        aBeforeFacade.setGrammar(aFacade.getGrammar());
        aBeforeFacade.setGeneration(60);
        const auto nNamedRangeSheet = aBeforeFacade.addSheet(u"Data");
        aBeforeFacade.setCell({ nNamedRangeSheet, 0, 0 }, CellValue::number(1.0));
        aBeforeFacade.setCell({ nNamedRangeSheet, 0, 1 }, CellValue::number(2.0));
        aBeforeFacade.setCell({ nNamedRangeSheet, 0, 2 }, CellValue::number(3.0));
        aBeforeFacade.addNamedRange(
            u"Metrics", std::nullopt, { nNamedRangeSheet, 0, 0 }, u"$Data.$A$1:$A$2");
        aBeforeFacade.setFormulaCell({ nNamedRangeSheet, 1, 0 }, u"=COUNTA(Metrics)+A1",
            CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nNamedRangeSheet, 1, 1 }, u"=COUNTA(Metrics)+A2",
            CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nNamedRangeSheet, 1, 2 }, u"=COUNTA(Metrics)+A3",
            CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.addFormulaGroup({ nNamedRangeSheet, 1, 0 }, 3, true);
        aBeforeFacade.setFormulaCell({ nNamedRangeSheet, 2, 0 }, u"=COUNTA(Metrics)",
            CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

        InMemoryWorkbookFacade aAfterFacade;
        aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
        aAfterFacade.setGeneration(61);
        aAfterFacade.addSheet(u"Data");
        aAfterFacade.setCell({ nNamedRangeSheet, 0, 0 }, CellValue::number(1.0));
        aAfterFacade.setCell({ nNamedRangeSheet, 0, 1 }, CellValue::number(2.0));
        aAfterFacade.setCell({ nNamedRangeSheet, 0, 2 }, CellValue::number(3.0));
        aAfterFacade.addNamedRange(
            u"Metrics", std::nullopt, { nNamedRangeSheet, 0, 0 }, u"$Data.$A$1:$A$2");
        aAfterFacade.setFormulaCell({ nNamedRangeSheet, 1, 0 }, u"=COUNTA(Metrics)+A1",
            CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nNamedRangeSheet, 1, 1 }, u"=COUNTA(Metrics)+A2",
            CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nNamedRangeSheet, 1, 2 }, u"=COUNTA(Metrics)+A3",
            CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.addFormulaGroup({ nNamedRangeSheet, 1, 0 }, 3, true);
        aAfterFacade.setFormulaCell({ nNamedRangeSheet, 2, 0 }, u"=COUNTA(Metrics)",
            CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

        const auto aClassification = consumers::classifySharedFormulaMutation(
            aBeforeFacade, aAfterFacade,
            MutationEvent::setFormula({ nNamedRangeSheet, 1, 1 }, u"=COUNTA(Metrics)+A2"));
        const auto aNamedRangeBoundary
            = consumers::classifySharedFormulaNamedRangeMutationBoundary(
                aBeforeFacade, aAfterFacade,
                MutationEvent::setFormula(
                    { nNamedRangeSheet, 1, 1 }, u"=COUNTA(Metrics)+A2"));
        if (aClassification.meFamily != consumers::SharedFormulaMutationFamily::SameTextPreserve
            || aNamedRangeBoundary.meBoundary
                   != consumers::SharedFormulaNamedRangeMutationBoundary::
                       GlobalSingleAreaSameSheet
            || aNamedRangeBoundary.mnNamedRangeCount != 1
            || !aNamedRangeBoundary.mbDescriptorsStable
            || !aNamedRangeBoundary.mbAllConsumersStayOnSheet)
        {
            return fail("facade_consumers",
                "shared-group named-range preserve classification mismatch");
        }
    }

    // --- Shared-formula named-range-combined off-sheet single-consumer classification ---
    {
        InMemoryWorkbookFacade aBeforeFacade;
        aBeforeFacade.setGrammar(aFacade.getGrammar());
        aBeforeFacade.setGeneration(62);
        const auto nDataSheet = aBeforeFacade.addSheet(u"Data");
        const auto nSummarySheet = aBeforeFacade.addSheet(u"Summary");
        aBeforeFacade.setCell({ nDataSheet, 0, 0 }, CellValue::number(1.0));
        aBeforeFacade.setCell({ nDataSheet, 0, 1 }, CellValue::number(2.0));
        aBeforeFacade.setCell({ nDataSheet, 0, 2 }, CellValue::number(3.0));
        aBeforeFacade.addNamedRange(
            u"Metrics", std::nullopt, { nDataSheet, 0, 0 }, u"$Data.$A$1:$A$2");
        aBeforeFacade.setFormulaCell({ nDataSheet, 1, 0 }, u"=COUNTA(Metrics)+A1",
            CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nDataSheet, 1, 1 }, u"=COUNTA(Metrics)+A2",
            CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nDataSheet, 1, 2 }, u"=COUNTA(Metrics)+A3",
            CellValue::number(5.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.addFormulaGroup({ nDataSheet, 1, 0 }, 3, true);
        aBeforeFacade.setFormulaCell({ nSummarySheet, 0, 0 }, u"=COUNTA(Metrics)",
            CellValue::number(2.0), FormulaCellKind::Ordinary, true, true);

        InMemoryWorkbookFacade aAfterFacade = aBeforeFacade;
        aAfterFacade.setGeneration(63);

        const auto aNamedRangeBoundary
            = consumers::classifySharedFormulaNamedRangeMutationBoundary(
                aBeforeFacade, aAfterFacade,
                MutationEvent::setFormula(
                    { nDataSheet, 1, 1 }, u"=COUNTA(Metrics)+A2"));
        if (aNamedRangeBoundary.meBoundary
                != consumers::SharedFormulaNamedRangeMutationBoundary::
                       GlobalSingleAreaSingleConsumerSheet
            || aNamedRangeBoundary.mnNamedRangeCount != 1
            || !aNamedRangeBoundary.mbDescriptorsStable
            || aNamedRangeBoundary.mbAllConsumersStayOnSheet)
        {
            return fail("facade_consumers",
                "shared-group named-range off-sheet single-consumer boundary mismatch");
        }
    }

    // --- Direct off-sheet shared-formula same-text-preserve classification ---
    {
        InMemoryWorkbookFacade aBeforeFacade;
        aBeforeFacade.setGrammar(aFacade.getGrammar());
        aBeforeFacade.setGeneration(64);
        const auto nDataSheet = aBeforeFacade.addSheet(u"Data");
        const auto nSummarySheet = aBeforeFacade.addSheet(u"Summary");
        aBeforeFacade.setCell({ nDataSheet, 0, 0 }, CellValue::number(1.0));
        aBeforeFacade.setCell({ nDataSheet, 0, 1 }, CellValue::number(2.0));
        aBeforeFacade.setCell({ nDataSheet, 0, 2 }, CellValue::number(3.0));
        aBeforeFacade.setFormulaCell({ nDataSheet, 1, 0 }, u"=A1*2",
            CellValue::number(2.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nDataSheet, 1, 1 }, u"=A2*2",
            CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nDataSheet, 1, 2 }, u"=A3*2",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.addFormulaGroup({ nDataSheet, 1, 0 }, 3, true);
        aBeforeFacade.setFormulaCell({ nSummarySheet, 2, 0 }, u"=Data.B1+Data.B2+Data.B3",
            CellValue::number(12.0), FormulaCellKind::Ordinary, true, true);

        InMemoryWorkbookFacade aAfterFacade = aBeforeFacade;
        aAfterFacade.setGeneration(65);

        const auto aClassification = consumers::classifySharedFormulaMutation(
            aBeforeFacade, aAfterFacade,
            MutationEvent::setFormula({ nDataSheet, 1, 1 }, u"=A2*2"));
        const auto aNamedRangeBoundary
            = consumers::classifySharedFormulaNamedRangeMutationBoundary(
                aBeforeFacade, aAfterFacade,
                MutationEvent::setFormula({ nDataSheet, 1, 1 }, u"=A2*2"));
        if (aClassification.meFamily != consumers::SharedFormulaMutationFamily::SameTextPreserve
            || aNamedRangeBoundary.meBoundary
                   != consumers::SharedFormulaNamedRangeMutationBoundary::None
            || aClassification.maTransition.meKind
                   != consumers::SharedFormulaGroupTransitionKind::Preserve
            || !aClassification.mbTouchedAddressSharedBefore
            || !aClassification.mbTouchedAddressSharedAfter)
        {
            return fail("facade_consumers",
                "shared-group off-sheet same-text-preserve classification mismatch");
        }
    }

    // --- Direct off-sheet shared-formula regroup classification ---
    {
        InMemoryWorkbookFacade aBeforeFacade;
        aBeforeFacade.setGrammar(aFacade.getGrammar());
        aBeforeFacade.setGeneration(66);
        const auto nDataSheet = aBeforeFacade.addSheet(u"Data");
        const auto nSummarySheet = aBeforeFacade.addSheet(u"Summary");
        aBeforeFacade.setCell({ nDataSheet, 0, 0 }, CellValue::number(1.0));
        aBeforeFacade.setCell({ nDataSheet, 0, 1 }, CellValue::number(2.0));
        aBeforeFacade.setCell({ nDataSheet, 0, 2 }, CellValue::number(3.0));
        aBeforeFacade.setFormulaCell({ nDataSheet, 1, 0 }, u"=A1*3",
            CellValue::number(3.0), FormulaCellKind::Ordinary, true, true);
        aBeforeFacade.setFormulaCell({ nDataSheet, 1, 1 }, u"=A2*2",
            CellValue::number(4.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.setFormulaCell({ nDataSheet, 1, 2 }, u"=A3*2",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aBeforeFacade.addFormulaGroup({ nDataSheet, 1, 1 }, 2, true);
        aBeforeFacade.setFormulaCell({ nSummarySheet, 2, 0 }, u"=Data.B1+Data.B2+Data.B3",
            CellValue::number(13.0), FormulaCellKind::Ordinary, true, true);

        InMemoryWorkbookFacade aAfterFacade;
        aAfterFacade.setGrammar(aBeforeFacade.getGrammar());
        aAfterFacade.setGeneration(67);
        aAfterFacade.addSheet(u"Data");
        aAfterFacade.addSheet(u"Summary");
        aAfterFacade.setCell({ nDataSheet, 0, 0 }, CellValue::number(1.0));
        aAfterFacade.setCell({ nDataSheet, 0, 1 }, CellValue::number(2.0));
        aAfterFacade.setCell({ nDataSheet, 0, 2 }, CellValue::number(3.0));
        aAfterFacade.setFormulaCell({ nDataSheet, 1, 0 }, u"=A1*3",
            CellValue::number(3.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nDataSheet, 1, 1 }, u"=A2*3",
            CellValue::number(6.0), FormulaCellKind::SharedGroupMember, true, true);
        aAfterFacade.setFormulaCell({ nDataSheet, 1, 2 }, u"=A3*2",
            CellValue::number(6.0), FormulaCellKind::Ordinary, true, true);
        aAfterFacade.addFormulaGroup({ nDataSheet, 1, 0 }, 2, true);
        aAfterFacade.setFormulaCell({ nSummarySheet, 2, 0 }, u"=Data.B1+Data.B2+Data.B3",
            CellValue::number(15.0), FormulaCellKind::Ordinary, true, true);

        const auto aClassification = consumers::classifySharedFormulaMutation(
            aBeforeFacade, aAfterFacade,
            MutationEvent::setFormula({ nDataSheet, 1, 1 }, u"=A2*3"));
        const auto aNamedRangeBoundary
            = consumers::classifySharedFormulaNamedRangeMutationBoundary(
                aBeforeFacade, aAfterFacade,
                MutationEvent::setFormula({ nDataSheet, 1, 1 }, u"=A2*3"));
        if (aClassification.meFamily != consumers::SharedFormulaMutationFamily::Regroup
            || aNamedRangeBoundary.meBoundary
                   != consumers::SharedFormulaNamedRangeMutationBoundary::None
            || aClassification.maTransition.meKind
                   != consumers::SharedFormulaGroupTransitionKind::Rebuild
            || !aClassification.mbTouchedAddressSharedBefore
            || !aClassification.mbTouchedAddressSharedAfter)
        {
            return fail("facade_consumers",
                "shared-group off-sheet regroup classification mismatch");
        }
    }

    // --- Named-range inventory consumer ---
    {
        const auto aInventory = consumers::inventoryNamedRanges(rFacade);
        if (aInventory.mnGlobalCount != 1)
            return fail("facade_consumers", "global named range count mismatch");
        if (aInventory.mnSheetLocalCount != 1)
            return fail("facade_consumers", "sheet-local named range count mismatch");
    }

    // --- Snapshot comparison ---
    {
        const auto aSnapshot1 = rFacade.getSnapshotInfo();
        const auto aSnapshot2 = rFacade.getSnapshotInfo();
        const auto aComparison = consumers::compareSnapshots(aSnapshot1, aSnapshot2);
        if (!aComparison.mbFullMatch)
            return fail("facade_consumers", "snapshot self-comparison should match");

        WorkbookSnapshotInfo aDifferent { 99, 1, 0 };
        const auto aDiffComparison = consumers::compareSnapshots(aSnapshot1, aDifferent);
        if (aDiffComparison.mbFullMatch)
            return fail("facade_consumers", "different snapshots should not match");
    }

    // --- Formula corpus collection ---
    {
        const auto aCorpus = consumers::collectFormulaCorpus(rFacade);
        if (aCorpus.size() != 3)
            return fail("facade_consumers", "full corpus size mismatch");

        // Limited collection.
        const auto aLimited = consumers::collectFormulaCorpus(rFacade, 2);
        if (aLimited.size() != 2)
            return fail("facade_consumers", "limited corpus size mismatch");

        // Verify the corpus contains formula source text.
        bool bFoundSum = false;
        for (const auto& rEntry : aCorpus)
        {
            if (rEntry.second == u"=SUM(A1:A3)")
                bFoundSum = true;
        }
        if (!bFoundSum)
            return fail("facade_consumers", "corpus should contain SUM formula");
    }

    // --- Phase 1 substrate schema smoke ---
    {
        using spreadsheetengine::detail::substrate::ComputationalSheetShadow;
        using spreadsheetengine::detail::substrate::ComputationalWorkbookShadow;
        using spreadsheetengine::detail::substrate::ShadowFormulaGroupId;

        ComputationalWorkbookShadow aShadow;
        aShadow.maSnapshot = rFacade.getSnapshotInfo();
        aShadow.maGrammar = rFacade.getGrammar();

        ComputationalSheetShadow aSheetShadow;
        aSheetShadow.maSheet = *rFacade.getSheetDescriptor(0);
        aSheetShadow.maCells.push_back({
            { { 0, 0, 2 } },
            rFacade.getCellDescriptor({ 0, 0, 2 }),
            rFacade.getFormulaCellDescriptor({ 0, 0, 2 }),
            ShadowFormulaGroupId { { 0, 0, 2 }, 2 },
            true,
            false });
        aShadow.maSheets.push_back(std::move(aSheetShadow));

        if (aShadow.getCellCount() != 1 || aShadow.getFormulaCellCount() != 1)
            return fail("substrate_schema", "shadow workbook counts mismatch");
        if (!aShadow.findCell({ 0, 0, 2 }))
            return fail("substrate_schema", "shadow cell lookup mismatch");
    }

    // =================================================================
    // Phase 5: Mutation event modeling and invalidation inputs
    // =================================================================

    // Verify mutation events carry the right data for invalidation modeling.
    {
        // Row insert: should capture sheet, starting row, and count.
        const auto aInsert = MutationEvent::insertRows(0, 5, 10);
        if (aInsert.meKind != MutationKind::InsertRows
            || aInsert.mnSheet != 0 || aInsert.maAddress.mnRow != 5
            || aInsert.mnCount != 10)
        {
            return fail("facade_mutation", "insertRows invalidation input mismatch");
        }

        // Row delete: should capture affected row and count.
        const auto aDelete = MutationEvent::deleteRows(1, 3, 2);
        if (aDelete.mnSheet != 1 || aDelete.maAddress.mnRow != 3
            || aDelete.mnCount != 2)
        {
            return fail("facade_mutation", "deleteRows invalidation input mismatch");
        }

        // Column insert.
        const auto aColInsert = MutationEvent::insertColumns(0, 2, 3);
        if (aColInsert.maAddress.mnColumn != 2 || aColInsert.mnCount != 3)
            return fail("facade_mutation", "insertColumns invalidation input mismatch");

        // Move range: source and destination should differ.
        CellRange aSrc { { 0, 0, 0 }, { 0, 5, 5 } };
        CellRange aDst { { 1, 10, 10 }, { 1, 15, 15 } };
        const auto aMove = MutationEvent::moveRange(aSrc, aDst);
        if (aMove.maRange.maStart.mnSheet != 0
            || aMove.maDestination.maStart.mnSheet != 1 || aMove.mbCopy)
        {
            return fail("facade_mutation", "moveRange invalidation input mismatch");
        }

        // Copy range: should be a copy.
        const auto aCopy = MutationEvent::copyRange(aSrc, aDst);
        if (!aCopy.mbCopy)
            return fail("facade_mutation", "copyRange should flag mbCopy");

        // Sheet rename: captures sheet id and new name.
        const auto aRename = MutationEvent::renameSheet(2, u"Renamed");
        if (aRename.mnSheet != 2 || aRename.maText != u"Renamed")
            return fail("facade_mutation", "renameSheet invalidation input mismatch");

        // Named range mutations carry before/after descriptors for
        // dependency and invalidation consumers.
        NamedRangeDescriptor aBefore {};
        aBefore.maId = NamedRangeId { 3, std::nullopt };
        aBefore.maName = u"OldRange";
        aBefore.meScope = NamedRangeScope::Global;
        aBefore.maBaseAddress = { 0, 0, 0 };
        aBefore.maTargetExpression = u"$Data.$A$1:$A$10";

        NamedRangeDescriptor aAfter = aBefore;
        aAfter.maName = u"NewRange";

        const auto aAdd = MutationEvent::addNamedRange(aAfter);
        const auto aRemove = MutationEvent::removeNamedRange(aBefore);
        const auto aRenameNR = MutationEvent::renameNamedRange(aBefore, aAfter);

        if (aAdd.maText != u"NewRange"
            || !aAdd.moNamedRangeAfter
            || aRemove.maText != u"OldRange"
            || !aRemove.moNamedRangeBefore
            || aRenameNR.maText != u"NewRange"
            || !aRenameNR.moNamedRangeBefore
            || !aRenameNR.moNamedRangeAfter
            || aRenameNR.moNamedRangeBefore->maName != u"OldRange"
            || aRenameNR.moNamedRangeAfter->maName != u"NewRange")
        {
            return fail("facade_mutation", "named range mutation descriptor mismatch");
        }
    }

    // Verify that mutation events form a stable vocabulary suitable for
    // invalidation pattern matching.
    {
        // All 14 mutation kinds should be representable.
        std::vector<MutationKind> aAllKinds = {
            MutationKind::SetScalarValue,
            MutationKind::SetFormula,
            MutationKind::ClearCell,
            MutationKind::ClearRange,
            MutationKind::InsertRows,
            MutationKind::DeleteRows,
            MutationKind::InsertColumns,
            MutationKind::DeleteColumns,
            MutationKind::MoveRange,
            MutationKind::CopyRange,
            MutationKind::RenameSheet,
            MutationKind::AddNamedRange,
            MutationKind::RemoveNamedRange,
            MutationKind::RenameNamedRange,
        };
        if (aAllKinds.size() != 14)
            return fail("facade_mutation", "mutation kind vocabulary size mismatch");

        // Each kind should have a distinct numeric value.
        for (std::size_t i = 0; i < aAllKinds.size(); ++i)
        {
            for (std::size_t j = i + 1; j < aAllKinds.size(); ++j)
            {
                if (aAllKinds[i] == aAllKinds[j])
                    return fail("facade_mutation", "mutation kinds should be distinct");
            }
        }
    }

    std::cout << "workbook_facade_tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
