/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <array>
#include <memory>
#include <vector>

#include <spreadsheetengine/api/MatrixFrame.hxx>
#include <spreadsheetengine/detail/ExecutionContext.hxx>
#include <spreadsheetengine/runtime/RpnControlFlow.hxx>
#include <spreadsheetengine/runtime/RpnCriteria.hxx>
#include <spreadsheetengine/runtime/RpnDatabase.hxx>
#include <spreadsheetengine/runtime/RpnMatrix.hxx>
#include <spreadsheetengine/runtime/RpnOperators.hxx>
#include <spreadsheetengine/runtime/RpnReference.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>
#include <spreadsheetengine/runtime/ScalarCoercion.hxx>

#include "TestSupport.hxx"

namespace
{

struct ReleaseToken
{
    bool mbReleased = false;
};

struct CacheEntry
{
    int mnValue = -1;

    bool operator==(const CacheEntry& rOther) const { return mnValue == rOther.mnValue; }
};

struct CachedToken
{
    int mnRefCount = 0;
    bool mbReleased = false;
    bool mbRetained = false;
};

struct PoolEntry
{
    int mnValue = 0;
};

} // namespace

int main()
{
    using spreadsheetengine::api::CellValue;
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::ResolvedReference;
    using spreadsheetengine::api::matrixframe::allowsReferenceListParameter;
    using spreadsheetengine::api::matrixframe::ParamKind;
    using spreadsheetengine::api::matrixframe::shouldConvertDoubleRefParameter;
    using spreadsheetengine::api::matrixframe::
        shouldConvertExternalDoubleRefParameter;
    using spreadsheetengine::api::matrixframe::shouldConvertJumpConditionToMatrix;
    using spreadsheetengine::api::matrixframe::shouldTrackValueParameterDimensions;
    using spreadsheetengine::api::matrixframe::StackKind;
    using spreadsheetengine::core::coercion::coerceToBoolean;
    using spreadsheetengine::core::coercion::coerceToNumber;
    using spreadsheetengine::core::coercion::coerceToString;
    using spreadsheetengine::core::coercion::normalizeNonNegativeLengthArgument;
    using spreadsheetengine::core::coercion::normalizeOneBasedStringPositionArgument;
    using spreadsheetengine::core::coercion::normalizeStringPositionArgument;
    using spreadsheetengine::core::rpn::RpnCoercionReadiness;
    using spreadsheetengine::core::rpn::RpnValue;
    using spreadsheetengine::core::rpn::BinaryScalarOperator;
    using spreadsheetengine::core::rpn::UnaryNumericOperator;
    using spreadsheetengine::core::rpn::classifyBinaryScalarOperatorReadiness;
    using spreadsheetengine::core::rpn::classifyUnaryScalarOperatorReadiness;
    using spreadsheetengine::core::rpn::evaluateBinaryScalarOperator;
    using spreadsheetengine::core::rpn::evaluateUnaryNumericOperator;
    using spreadsheetengine::standalone::test::fail;

    const auto aNumericText = coerceToNumber(CellValue::text(u"12.5"));
    const auto aInvalidNumericText = coerceToNumber(CellValue::text(u"abc"));
    const auto aBooleanTrue = coerceToBoolean(CellValue::text(u"TRUE"));
    const auto aBooleanNumeric = coerceToBoolean(CellValue::number(2.0));
    const auto aStringNumber = coerceToString(CellValue::number(12.5));
    const auto aStringBoolean = coerceToString(CellValue::boolean(true));
    const auto aPosition = normalizeStringPositionArgument(2.9);
    const auto aZeroLength = normalizeNonNegativeLengthArgument(0.9);
    const auto aOneBasedPosition = normalizeOneBasedStringPositionArgument(1.9);
    const auto aInvalidPosition = normalizeStringPositionArgument(-1.0);
    const auto aInvalidOneBased = normalizeOneBasedStringPositionArgument(0.0);

    if (!aNumericText || aNumericText.maValue != 12.5 || aInvalidNumericText
        || aInvalidNumericText.meError != Error::IllegalArgument || !aBooleanTrue
        || !aBooleanTrue.maValue || !aBooleanNumeric || !aBooleanNumeric.maValue || !aStringNumber
        || aStringNumber.maValue != u"12.5" || !aStringBoolean
        || aStringBoolean.maValue != u"TRUE" || !aPosition || aPosition.maValue != 2
        || !aZeroLength || aZeroLength.maValue != 0 || !aOneBasedPosition
        || aOneBasedPosition.maValue != 1 || aInvalidPosition
        || aInvalidPosition.meError != Error::IllegalArgument || aInvalidOneBased
        || aInvalidOneBased.meError != Error::IllegalArgument)
    {
        return fail("spreadsheetengine_execution_tests", "scalar coercion helper mismatch");
    }

    const auto aRpnNumericText = spreadsheetengine::core::rpn::coerceToNumber(RpnValue::text(u"12.5"));
    const auto aRpnBooleanString
        = spreadsheetengine::core::rpn::coerceToString(RpnValue::boolean(true));
    const auto aRpnError = spreadsheetengine::core::rpn::coerceToNumber(
        RpnValue::error(Error::DivisionByZero));
    const auto aReferenceReadiness = classifyUnaryScalarOperatorReadiness(
        RpnValue::reference(ResolvedReference { { { 0, 1, 2 }, { 0, 1, 2 } } }));
    const auto aDeferredBoolean = spreadsheetengine::core::rpn::coerceToBoolean(
        RpnValue::reference(ResolvedReference { { { 0, 2, 3 }, { 0, 2, 3 } } }));
    const auto aMatrixReadiness = classifyBinaryScalarOperatorReadiness(
        RpnValue::matrix({ 2, 3 }), RpnValue::number(9.0));
    const auto aMixedReferenceReadiness = classifyBinaryScalarOperatorReadiness(
        RpnValue::number(1.0),
        RpnValue::reference(ResolvedReference { { { 0, 4, 5 }, { 0, 4, 5 } } }));

    if (!aRpnNumericText || aRpnNumericText.maValue != 12.5 || !aRpnBooleanString
        || aRpnBooleanString.maValue != u"TRUE" || aRpnError
        || aRpnError.meError != Error::DivisionByZero
        || aReferenceReadiness != RpnCoercionReadiness::NeedsReferenceResolution
        || aDeferredBoolean
        || aDeferredBoolean.meReadiness != RpnCoercionReadiness::NeedsReferenceResolution
        || aMatrixReadiness != RpnCoercionReadiness::NeedsMatrixMaterialization
        || aMixedReferenceReadiness != RpnCoercionReadiness::NeedsReferenceResolution)
    {
        return fail("spreadsheetengine_execution_tests", "rpn value coercion contract mismatch");
    }

    const auto aUnaryMinus = evaluateUnaryNumericOperator(
        UnaryNumericOperator::Minus, RpnValue::number(3.5));
    const auto aBinaryAdd = evaluateBinaryScalarOperator(
        BinaryScalarOperator::Add, RpnValue::number(1.5), RpnValue::text(u"2.5"));
    const auto aConcat = evaluateBinaryScalarOperator(
        BinaryScalarOperator::Concat, RpnValue::text(u"ab"), RpnValue::boolean(true));
    const auto aStringLess = evaluateBinaryScalarOperator(
        BinaryScalarOperator::Less, RpnValue::text(u"apple"), RpnValue::text(u"banana"));
    const auto aNumericEqual = evaluateBinaryScalarOperator(
        BinaryScalarOperator::Equal, RpnValue::number(4.0), RpnValue::text(u"4"));
    const auto aDivisionByZero = evaluateBinaryScalarOperator(
        BinaryScalarOperator::Divide, RpnValue::number(1.0), RpnValue::number(0.0));
    const auto aDeferredBinary = evaluateBinaryScalarOperator(
        BinaryScalarOperator::Multiply,
        RpnValue::reference(ResolvedReference { { { 0, 6, 7 }, { 0, 6, 7 } } }),
        RpnValue::number(5.0));
    const auto aDeferredPower = evaluateBinaryScalarOperator(
        BinaryScalarOperator::Power, RpnValue::matrix({ 2, 2 }), RpnValue::number(2.0));

    if (!aUnaryMinus || aUnaryMinus.maValue.maScalar.mfNumber != -3.5 || !aBinaryAdd
        || aBinaryAdd.maValue.maScalar.mfNumber != 4.0 || !aConcat
        || aConcat.maValue.maScalar.maString != u"abTRUE" || !aStringLess
        || !aStringLess.maValue.maScalar.mfNumber || !aNumericEqual
        || !aNumericEqual.maValue.maScalar.mfNumber || aDivisionByZero
        || aDivisionByZero.meError != Error::DivisionByZero || aDeferredBinary
        || aDeferredBinary.meReadiness != RpnCoercionReadiness::NeedsReferenceResolution
        || aDeferredPower
        || aDeferredPower.meReadiness != RpnCoercionReadiness::NeedsMatrixMaterialization)
    {
        return fail("spreadsheetengine_execution_tests", "rpn operator contract mismatch");
    }

    {
        using spreadsheetengine::core::rpn::BranchDirective;
        using spreadsheetengine::core::rpn::LetScope;
        using spreadsheetengine::core::rpn::planChooseBranch;
        using spreadsheetengine::core::rpn::planIfBranch;
        using spreadsheetengine::core::rpn::planIfErrorBranch;
        using spreadsheetengine::core::rpn::planIfsBranch;
        using spreadsheetengine::core::rpn::planSwitchBranch;
        using spreadsheetengine::core::rpn::RpnValue;

        const auto aIfTrue = planIfBranch(RpnValue::boolean(true), std::size_t { 0 }, std::size_t { 1 });
        const auto aIfFalse = planIfBranch(RpnValue::boolean(false), std::size_t { 0 }, std::size_t { 1 });
        const auto aIfBareTrue = planIfBranch(RpnValue::number(1.0), std::nullopt, std::nullopt);
        const auto aIfBareFalse = planIfBranch(RpnValue::number(0.0), std::nullopt, std::nullopt);
        const auto aIfDeferredMatrix = planIfBranch(
            RpnValue::matrix({ 2, 2 }), std::size_t { 0 }, std::size_t { 1 });
        const auto aIfDeferredRef = planIfBranch(
            RpnValue::reference(ResolvedReference { { { 0, 3, 4 }, { 0, 3, 4 } } }),
            std::size_t { 0 }, std::size_t { 1 });

        if (!aIfTrue || aIfTrue.maValue.meDirective != BranchDirective::TakeSlot
            || aIfTrue.maValue.mnSlot != 0
            || !aIfFalse || aIfFalse.maValue.meDirective != BranchDirective::TakeSlot
            || aIfFalse.maValue.mnSlot != 1
            || !aIfBareTrue
            || aIfBareTrue.maValue.meDirective != BranchDirective::ReturnSyntheticBoolean
            || !aIfBareTrue.maValue.mbSyntheticBool
            || !aIfBareFalse
            || aIfBareFalse.maValue.meDirective != BranchDirective::ReturnSyntheticBoolean
            || aIfBareFalse.maValue.mbSyntheticBool
            || aIfDeferredMatrix
            || aIfDeferredMatrix.meReadiness != RpnCoercionReadiness::NeedsMatrixMaterialization
            || aIfDeferredRef
            || aIfDeferredRef.meReadiness != RpnCoercionReadiness::NeedsReferenceResolution)
        {
            return fail("spreadsheetengine_execution_tests", "planIfBranch contract mismatch");
        }

        const auto aChoose2of3 = planChooseBranch(RpnValue::number(2.0), 3);
        const auto aChooseOutOfRange = planChooseBranch(RpnValue::number(5.0), 3);

        if (!aChoose2of3 || aChoose2of3.maValue.meDirective != BranchDirective::TakeSlot
            || aChoose2of3.maValue.mnSlot != 2
            || !aChooseOutOfRange
            || aChooseOutOfRange.maValue.meDirective != BranchDirective::PropagateError
            || aChooseOutOfRange.maValue.meError != Error::IllegalArgument)
        {
            return fail("spreadsheetengine_execution_tests", "planChooseBranch contract mismatch");
        }

        const auto aIfsMatch = planIfsBranch(RpnValue::boolean(true), std::size_t { 2 },
                                              static_cast<std::int16_t>(3));
        const auto aIfsSkip = planIfsBranch(RpnValue::boolean(false), std::size_t { 2 },
                                             static_cast<std::int16_t>(5));
        const auto aIfsNA = planIfsBranch(RpnValue::boolean(false), std::size_t { 2 },
                                           static_cast<std::int16_t>(1));

        if (!aIfsMatch || aIfsMatch.maValue.meDirective != BranchDirective::TakeSlot
            || aIfsMatch.maValue.mnSlot != 2
            || !aIfsSkip || aIfsSkip.maValue.meDirective != BranchDirective::TakeSlot
            || aIfsSkip.maValue.mnSlot != 3
            || !aIfsNA || aIfsNA.maValue.meDirective != BranchDirective::ReturnNotAvailable)
        {
            return fail("spreadsheetengine_execution_tests", "planIfsBranch contract mismatch");
        }

        const std::array<RpnValue, 3> aCaseLabels = {
            RpnValue::number(1.0),
            RpnValue::number(2.0),
            RpnValue::number(3.0)
        };
        const std::span<const RpnValue> aCaseSpan(aCaseLabels.data(), aCaseLabels.size());
        const auto aSwitchHit = planSwitchBranch(RpnValue::number(2.0), aCaseSpan, std::nullopt);
        const auto aSwitchMiss = planSwitchBranch(RpnValue::number(7.0), aCaseSpan, std::nullopt);
        const auto aSwitchDefault = planSwitchBranch(RpnValue::number(7.0), aCaseSpan,
                                                      std::size_t { 99 });

        if (!aSwitchHit || aSwitchHit.maValue.meDirective != BranchDirective::TakeSlot
            || aSwitchHit.maValue.mnSlot != 1
            || !aSwitchMiss
            || aSwitchMiss.maValue.meDirective != BranchDirective::PropagateError
            || aSwitchMiss.maValue.meError != Error::NotAvailable
            || !aSwitchDefault
            || aSwitchDefault.maValue.meDirective != BranchDirective::TakeSlot
            || aSwitchDefault.maValue.mnSlot != 99)
        {
            return fail("spreadsheetengine_execution_tests", "planSwitchBranch contract mismatch");
        }

        const auto aIfErrorKeep = planIfErrorBranch(Error::None, false, 0);
        const auto aIfErrorReplace = planIfErrorBranch(Error::DivisionByZero, false, 5);
        const auto aIfNAOnlyKeep = planIfErrorBranch(Error::DivisionByZero, true, 5);
        const auto aIfNAOnlyReplace = planIfErrorBranch(Error::NotAvailable, true, 5);

        if (aIfErrorKeep.meDirective != BranchDirective::KeepPrimaryValue
            || aIfErrorReplace.meDirective != BranchDirective::EvaluateAlternate
            || aIfErrorReplace.mnSlot != 5
            || aIfNAOnlyKeep.meDirective != BranchDirective::KeepPrimaryValue
            || aIfNAOnlyReplace.meDirective != BranchDirective::EvaluateAlternate)
        {
            return fail("spreadsheetengine_execution_tests", "planIfErrorBranch contract mismatch");
        }

        LetScope aScope;
        aScope.bind(spreadsheetengine::api::String(u"x"), RpnValue::number(10.0));
        aScope.bind(spreadsheetengine::api::String(u"y"), RpnValue::text(u"hello"));
        const auto oX = aScope.lookup(spreadsheetengine::api::StringView(u"x"));
        const auto oY = aScope.lookup(spreadsheetengine::api::StringView(u"y"));
        const auto oMissing = aScope.lookup(spreadsheetengine::api::StringView(u"z"));

        if (!oX || oX->maScalar.mfNumber != 10.0
            || !oY || oY->maScalar.maString != u"hello"
            || oMissing.has_value())
        {
            return fail("spreadsheetengine_execution_tests", "LetScope contract mismatch");
        }
    }

    {
        using spreadsheetengine::core::rpn::AxisOrdinalKind;
        using spreadsheetengine::core::rpn::coerceToPositiveIndex;
        using spreadsheetengine::core::rpn::coerceToSignedOffset;
        using spreadsheetengine::core::rpn::IndexProjectionParameters;
        using spreadsheetengine::core::rpn::OffsetParameters;
        using spreadsheetengine::core::rpn::planAreaCount;
        using spreadsheetengine::core::rpn::planAxisOrdinal;
        using spreadsheetengine::core::rpn::planOffset;
        using spreadsheetengine::core::rpn::planSpanCount;
        using spreadsheetengine::core::rpn::projectIndexMatrix;
        using spreadsheetengine::core::rpn::projectIndexReference;
        using spreadsheetengine::core::rpn::RpnValue;
        using spreadsheetengine::core::rpn::SpanCountKind;

        const auto aSingleC2R3
            = RpnValue::reference(ResolvedReference { { { 0, 1, 2 }, { 0, 1, 2 } } });
        const auto aRangeB2C5
            = RpnValue::reference(ResolvedReference { { { 0, 1, 1 }, { 0, 2, 4 } } });
        const auto aMatrix3x2 = RpnValue::matrix({ 3, 2 });

        const auto aColumn = planAxisOrdinal(aSingleC2R3, AxisOrdinalKind::Column);
        const auto aRow = planAxisOrdinal(aSingleC2R3, AxisOrdinalKind::Row);
        const auto aSheet = planAxisOrdinal(aSingleC2R3, AxisOrdinalKind::Sheet);
        const auto aColumnDefer = planAxisOrdinal(RpnValue::number(5.0), AxisOrdinalKind::Column);

        if (!aColumn || aColumn.maValue != 2.0 || !aRow || aRow.maValue != 3.0
            || !aSheet || aSheet.maValue != 1.0
            || aColumnDefer
            || aColumnDefer.meReadiness != RpnCoercionReadiness::NeedsReferenceResolution)
        {
            return fail("spreadsheetengine_execution_tests", "planAxisOrdinal contract mismatch");
        }

        const auto aColumnsRef = planSpanCount(aRangeB2C5, SpanCountKind::Columns);
        const auto aRowsRef = planSpanCount(aRangeB2C5, SpanCountKind::Rows);
        const auto aSheetsRef = planSpanCount(aRangeB2C5, SpanCountKind::Sheets);
        const auto aColumnsMatrix = planSpanCount(aMatrix3x2, SpanCountKind::Columns);
        const auto aRowsMatrix = planSpanCount(aMatrix3x2, SpanCountKind::Rows);

        if (!aColumnsRef || aColumnsRef.maValue != 2.0
            || !aRowsRef || aRowsRef.maValue != 4.0
            || !aSheetsRef || aSheetsRef.maValue != 1.0
            || !aColumnsMatrix || aColumnsMatrix.maValue != 3.0
            || !aRowsMatrix || aRowsMatrix.maValue != 2.0)
        {
            return fail("spreadsheetengine_execution_tests", "planSpanCount contract mismatch");
        }

        const auto aThreeAreas = planAreaCount(3);
        const auto aZeroAreas = planAreaCount(0);
        if (!aThreeAreas || aThreeAreas.maValue != 3.0
            || aZeroAreas || aZeroAreas.meError != Error::IllegalArgument)
        {
            return fail("spreadsheetengine_execution_tests", "planAreaCount contract mismatch");
        }

        OffsetParameters aOffsetShift;
        aOffsetShift.mnRowOffset = 1;
        aOffsetShift.mnColumnOffset = 1;
        aOffsetShift.mnMaxColumn = 1023;
        aOffsetShift.mnMaxRow = 1048575;
        const auto aOffsetResult = planOffset(aSingleC2R3, aOffsetShift);
        if (!aOffsetResult
            || aOffsetResult.maValue.maStart.mnColumn != 2
            || aOffsetResult.maValue.maStart.mnRow != 3
            || aOffsetResult.maValue.maEnd.mnColumn != 2
            || aOffsetResult.maValue.maEnd.mnRow != 3)
        {
            return fail("spreadsheetengine_execution_tests", "planOffset contract mismatch");
        }

        const auto aIdxScalar = coerceToPositiveIndex(RpnValue::number(3.0));
        const auto aIdxBad = coerceToPositiveIndex(RpnValue::number(0.0));
        const auto aIdxRef = coerceToPositiveIndex(aSingleC2R3);
        const auto aOffSigned = coerceToSignedOffset(RpnValue::number(-2.0));

        if (!aIdxScalar || aIdxScalar.maValue != 3
            || aIdxBad || aIdxBad.meError != Error::IllegalArgument
            || aIdxRef
            || aIdxRef.meReadiness != RpnCoercionReadiness::NeedsReferenceResolution
            || !aOffSigned || aOffSigned.maValue != -2)
        {
            return fail("spreadsheetengine_execution_tests", "coerce index/offset contract mismatch");
        }

        IndexProjectionParameters aProj;
        aProj.mnRowIndex = 2;
        aProj.mnColumnIndex = 1;
        aProj.mbColumnArgumentMissing = false;
        aProj.mnParamCount = 3;
        aProj.mnAreaIndex = 1;
        aProj.mnAreaCount = 1;
        const auto aRefProj = projectIndexReference(aRangeB2C5, aProj);
        const auto aMatProj = projectIndexMatrix(aMatrix3x2, aProj);
        const auto aRefProjDefer = projectIndexReference(aMatrix3x2, aProj);

        if (!aRefProj || !aMatProj
            || aRefProjDefer
            || aRefProjDefer.meReadiness != RpnCoercionReadiness::NeedsReferenceResolution)
        {
            return fail("spreadsheetengine_execution_tests", "Index projection contract mismatch");
        }
    }

    {
        using spreadsheetengine::core::rpn::AggregationBridge;
        using spreadsheetengine::core::rpn::applyFieldSelector;
        using spreadsheetengine::core::rpn::bridgeAggregation;
        using spreadsheetengine::core::rpn::buildCriteriaPredicate;
        using spreadsheetengine::core::rpn::countEmptyCells;
        using spreadsheetengine::core::rpn::DatabaseAggregation;
        using spreadsheetengine::core::rpn::DatabaseQueryDescriptor;
        using spreadsheetengine::core::rpn::RpnValue;

        // Minimal parsers to exercise the criteria-predicate contract.
        const auto pParseNumberText
            = +[](spreadsheetengine::api::StringView) -> std::optional<spreadsheetengine::api::NumberParseResult> {
            return std::nullopt;
        };
        const auto pParseAsciiDouble
            = +[](spreadsheetengine::api::StringView) -> std::optional<double> {
            return std::nullopt;
        };

        const auto aNumericPred = buildCriteriaPredicate(
            RpnValue::number(42.0), pParseNumberText, pParseAsciiDouble);
        const auto aTextPred = buildCriteriaPredicate(
            RpnValue::text(u"apple"), pParseNumberText, pParseAsciiDouble);
        const auto aRefPredDefer = buildCriteriaPredicate(
            RpnValue::reference(ResolvedReference { { { 0, 1, 2 }, { 0, 1, 2 } } }),
            pParseNumberText, pParseAsciiDouble);

        if (!aNumericPred
            || !aTextPred
            || aRefPredDefer
            || aRefPredDefer.meReadiness != RpnCoercionReadiness::NeedsReferenceResolution)
        {
            return fail(
                "spreadsheetengine_execution_tests", "buildCriteriaPredicate contract mismatch");
        }

        // countEmptyCells: mix of empty, number, empty-text, text.
        const std::vector<spreadsheetengine::api::CellValue> aValues = {
            spreadsheetengine::api::CellValue::empty(),
            spreadsheetengine::api::CellValue::number(1.0),
            spreadsheetengine::api::CellValue::text(u""),
            spreadsheetengine::api::CellValue::text(u"x")
        };
        const auto aEmptyCount = countEmptyCells(aValues);
        if (!aEmptyCount || aEmptyCount.maValue != 2.0)
        {
            return fail(
                "spreadsheetengine_execution_tests", "countEmptyCells contract mismatch");
        }

        // applyFieldSelector branches.
        DatabaseQueryDescriptor aDesc;
        const auto aEmptySelector = applyFieldSelector(RpnValue::empty(), aDesc);
        if (!aEmptySelector || !aDesc.mbFieldMissing)
        {
            return fail(
                "spreadsheetengine_execution_tests", "applyFieldSelector(empty) mismatch");
        }

        DatabaseQueryDescriptor aDescIndex;
        const auto aNumberSelector = applyFieldSelector(RpnValue::number(3.0), aDescIndex);
        if (!aNumberSelector || !aDescIndex.moFieldByIndex
            || *aDescIndex.moFieldByIndex != 3)
        {
            return fail(
                "spreadsheetengine_execution_tests", "applyFieldSelector(number) mismatch");
        }

        DatabaseQueryDescriptor aDescName;
        const auto aTextSelector = applyFieldSelector(RpnValue::text(u"Amount"), aDescName);
        if (!aTextSelector || !aDescName.moFieldByName
            || *aDescName.moFieldByName != u"Amount")
        {
            return fail(
                "spreadsheetengine_execution_tests", "applyFieldSelector(text) mismatch");
        }

        DatabaseQueryDescriptor aDescRef;
        const auto aRefSelector = applyFieldSelector(
            RpnValue::reference(ResolvedReference { { { 0, 0, 0 }, { 0, 0, 0 } } }),
            aDescRef);
        if (aRefSelector
            || aRefSelector.meReadiness != RpnCoercionReadiness::NeedsReferenceResolution)
        {
            return fail(
                "spreadsheetengine_execution_tests", "applyFieldSelector(ref) should defer");
        }

        // bridgeAggregation routing.
        const auto aSumBridge = bridgeAggregation(DatabaseAggregation::Sum);
        const auto aAvgBridge = bridgeAggregation(DatabaseAggregation::Average);
        const auto aStdDevBridge = bridgeAggregation(DatabaseAggregation::StandardDeviation);
        const auto aStdDevPBridge
            = bridgeAggregation(DatabaseAggregation::StandardDeviationPopulation);
        const auto aProductBridge = bridgeAggregation(DatabaseAggregation::Product);
        const auto aGetBridge = bridgeAggregation(DatabaseAggregation::Get);

        if (!aSumBridge.moKind
            || *aSumBridge.moKind != spreadsheetengine::core::query::CriteriaAggregateKind::Sum
            || !aAvgBridge.moKind
            || *aAvgBridge.moKind
                != spreadsheetengine::core::query::CriteriaAggregateKind::Average
            || aStdDevBridge.moKind
            || !aStdDevBridge.mbRequiresVarianceAggregation
            || aStdDevBridge.mbPopulation
            || !aStdDevPBridge.mbRequiresVarianceAggregation
            || !aStdDevPBridge.mbPopulation
            || !aProductBridge.mbRequiresProductAggregation
            || !aGetBridge.mbRequiresGetAggregation)
        {
            return fail(
                "spreadsheetengine_execution_tests", "bridgeAggregation contract mismatch");
        }
    }

    {
        using spreadsheetengine::core::rpn::MatrixOperand;
        using spreadsheetengine::core::rpn::MatrixProvenance;
        using spreadsheetengine::core::rpn::planBroadcastScalarOverMatrix;
        using spreadsheetengine::core::rpn::planDeterminant;
        using spreadsheetengine::core::rpn::planElementwiseBinary;
        using spreadsheetengine::core::rpn::planIdentityMatrix;
        using spreadsheetengine::core::rpn::planSequenceMatrix;
        using spreadsheetengine::core::rpn::planSumReductionPair;
        using spreadsheetengine::core::rpn::planTranspose;
        using spreadsheetengine::core::rpn::RpnValue;
        using spreadsheetengine::core::rpn::SumReductionKind;

        // Identity matrix planning.
        const auto aId3 = planIdentityMatrix(3);
        if (!aId3 || aId3.maValue.maDimensions.mnColumns != 3
            || aId3.maValue.maDimensions.mnRows != 3
            || aId3.maValue.maValues[0].mfNumber != 1.0
            || aId3.maValue.maValues[4].mfNumber != 1.0
            || aId3.maValue.maValues[8].mfNumber != 1.0
            || aId3.maValue.maValues[1].mfNumber != 0.0)
        {
            return fail(
                "spreadsheetengine_execution_tests", "planIdentityMatrix contract mismatch");
        }
        const auto aIdZero = planIdentityMatrix(0);
        if (aIdZero || aIdZero.meError != Error::IllegalArgument)
        {
            return fail(
                "spreadsheetengine_execution_tests", "planIdentityMatrix(0) must fail");
        }

        // Sequence matrix: 2x3 starting at 10, step 5:
        //   10 15 20
        //   25 30 35
        const auto aSeq = planSequenceMatrix(2, 3, 10.0, 5.0);
        if (!aSeq || aSeq.maValue.maDimensions.mnRows != 2
            || aSeq.maValue.maDimensions.mnColumns != 3
            || aSeq.maValue.maValues[0].mfNumber != 10.0
            || aSeq.maValue.maValues[2].mfNumber != 20.0
            || aSeq.maValue.maValues[5].mfNumber != 35.0)
        {
            return fail(
                "spreadsheetengine_execution_tests", "planSequenceMatrix contract mismatch");
        }

        // Transpose: 1 2 / 3 4 -> 1 3 / 2 4.
        MatrixOperand a22;
        a22.maDimensions = { 2, 2 };
        a22.maValues = { spreadsheetengine::api::CellValue::number(1.0),
                          spreadsheetengine::api::CellValue::number(2.0),
                          spreadsheetengine::api::CellValue::number(3.0),
                          spreadsheetengine::api::CellValue::number(4.0) };
        a22.meProvenance = MatrixProvenance::InlineLiteral;
        const auto aT = planTranspose(a22);
        if (!aT || aT.maValue.maValues[0].mfNumber != 1.0
            || aT.maValue.maValues[1].mfNumber != 3.0
            || aT.maValue.maValues[2].mfNumber != 2.0
            || aT.maValue.maValues[3].mfNumber != 4.0)
        {
            return fail(
                "spreadsheetengine_execution_tests", "planTranspose contract mismatch");
        }

        // Determinant of 1 2 / 3 4 = -2.
        const auto aDet = planDeterminant(a22);
        if (!aDet || aDet.maValue != -2.0)
        {
            return fail(
                "spreadsheetengine_execution_tests", "planDeterminant contract mismatch");
        }

        // Broadcast scalar over matrix: 2 * a22.
        const auto aScaled = planBroadcastScalarOverMatrix(
            spreadsheetengine::core::rpn::BinaryScalarOperator::Multiply,
            RpnValue::number(2.0), a22);
        if (!aScaled
            || aScaled.maValue.maValues[0].mfNumber != 2.0
            || aScaled.maValue.maValues[3].mfNumber != 8.0)
        {
            return fail(
                "spreadsheetengine_execution_tests", "planBroadcastScalarOverMatrix mismatch");
        }

        // Elementwise: a22 + a22.
        const auto aSum = planElementwiseBinary(
            spreadsheetengine::core::rpn::BinaryScalarOperator::Add, a22, a22);
        if (!aSum
            || aSum.maValue.maValues[0].mfNumber != 2.0
            || aSum.maValue.maValues[3].mfNumber != 8.0)
        {
            return fail(
                "spreadsheetengine_execution_tests", "planElementwiseBinary contract mismatch");
        }

        // Sum reduction: SumProduct(a22, a22) = 1 + 4 + 9 + 16 = 30.
        const auto aSp = planSumReductionPair(SumReductionKind::SumProduct, a22, a22);
        if (!aSp || aSp.maValue != 30.0)
        {
            return fail(
                "spreadsheetengine_execution_tests", "planSumReductionPair contract mismatch");
        }
    }

    if (!shouldConvertJumpConditionToMatrix(StackKind::DoubleRef, StackKind::Other)
        || !shouldConvertJumpConditionToMatrix(StackKind::Other, StackKind::JumpMatrix)
        || shouldConvertJumpConditionToMatrix(StackKind::Other, StackKind::Other)
        || shouldConvertJumpConditionToMatrix(StackKind::Matrix, StackKind::JumpMatrix)
        || !shouldTrackValueParameterDimensions(ParamKind::Value)
        || shouldTrackValueParameterDimensions(ParamKind::Array)
        || shouldConvertDoubleRefParameter(ParamKind::Reference, true)
        || shouldConvertDoubleRefParameter(ParamKind::ReferenceOrRefArray, true)
        || shouldConvertDoubleRefParameter(ParamKind::Value, false)
        || !shouldConvertDoubleRefParameter(ParamKind::Value, true)
        || !shouldConvertDoubleRefParameter(ParamKind::Array, false)
        || !shouldConvertExternalDoubleRefParameter(ParamKind::Value)
        || !shouldConvertExternalDoubleRefParameter(ParamKind::Array)
        || shouldConvertExternalDoubleRefParameter(ParamKind::Reference)
        || !allowsReferenceListParameter(ParamKind::ForceArray)
        || !allowsReferenceListParameter(ParamKind::ReferenceOrForceArray)
        || allowsReferenceListParameter(ParamKind::Value))
    {
        return fail("spreadsheetengine_execution_tests", "matrix frame planning mismatch");
    }

    std::size_t nReleasedCount = 0;
    std::vector<ReleaseToken*> aTokens(4, nullptr);
    ReleaseToken aToken1;
    ReleaseToken aToken2;
    aTokens[1] = &aToken1;
    aTokens[3] = &aToken2;
    std::size_t nTokenCachePos = 3;

    spreadsheetengine::core::execution::resetTokenCache(
        aTokens, nTokenCachePos, [&](ReleaseToken* pToken) {
            pToken->mbReleased = true;
            ++nReleasedCount;
        });

    if (!aToken1.mbReleased || !aToken2.mbReleased || nReleasedCount != 2 || nTokenCachePos != 0
        || aTokens[1] != nullptr || aTokens[3] != nullptr)
    {
        return fail("spreadsheetengine_execution_tests", "token cache reset mismatch");
    }

    std::vector<CachedToken*> aCachedTokens(3, nullptr);
    CachedToken aCachedToken1 { 2, false, false };
    CachedToken aCachedToken2 { 1, false, false };
    aCachedTokens[0] = &aCachedToken1;
    aCachedTokens[1] = &aCachedToken2;
    if (spreadsheetengine::core::execution::findReusableCachedToken(
            aCachedTokens, [](CachedToken* pToken) { return pToken->mnRefCount == 1; })
        != &aCachedToken2)
    {
        return fail("spreadsheetengine_execution_tests", "reusable token lookup mismatch");
    }

    std::size_t nReplacementPos = 1;
    CachedToken aReplacement { 1, false, false };
    spreadsheetengine::core::execution::replaceCachedToken(
        aCachedTokens, nReplacementPos, &aReplacement,
        [](CachedToken* pToken) { pToken->mbReleased = true; },
        [](CachedToken* pToken) { pToken->mbRetained = true; });
    if (!aCachedToken2.mbReleased || !aReplacement.mbRetained || aCachedTokens[1] != &aReplacement
        || nReplacementPos != 2)
    {
        return fail("spreadsheetengine_execution_tests", "token cache replacement mismatch");
    }

    const int nDoc1 = 1;
    const int nDoc2 = 2;
    const int nFormatter1 = 11;
    const int nFormatter2 = 12;
    const auto aNoRebindPlan = spreadsheetengine::core::execution::planContextRebind(
        &nDoc1, &nDoc1, &nFormatter1, &nFormatter1);
    const auto aDocRebindPlan = spreadsheetengine::core::execution::planContextRebind(
        &nDoc1, &nDoc2, &nFormatter1, &nFormatter1);
    const auto aFormatterRebindPlan = spreadsheetengine::core::execution::planContextRebind(
        &nDoc1, &nDoc1, &nFormatter1, &nFormatter2);
    if (aNoRebindPlan.mbDocChanged || aNoRebindPlan.mbFormatterChanged
        || aNoRebindPlan.mbResetLookupCache || aNoRebindPlan.mbResetRecentCaches
        || !aDocRebindPlan.mbDocChanged || !aDocRebindPlan.mbResetLookupCache
        || aDocRebindPlan.mbFormatterChanged || aDocRebindPlan.mbResetRecentCaches
        || !aFormatterRebindPlan.mbFormatterChanged
        || !aFormatterRebindPlan.mbResetRecentCaches || aFormatterRebindPlan.mbDocChanged
        || aFormatterRebindPlan.mbResetLookupCache)
    {
        return fail("spreadsheetengine_execution_tests", "context rebind plan mismatch");
    }

    if (spreadsheetengine::core::execution::composeHighLowCacheKey(0x01234567u, 0x89ABu)
        != 0x01234567000089ABULL)
    {
        return fail("spreadsheetengine_execution_tests", "cache key composition mismatch");
    }

    const auto aThreadedReusePlan
        = spreadsheetengine::core::execution::planThreadedPoolSlot(2, 4, 1);
    const auto aThreadedCreatePlan
        = spreadsheetengine::core::execution::planThreadedPoolSlot(2, 4, 3);
    if (!aThreadedReusePlan.mbValid || aThreadedReusePlan.mbCreateNew
        || aThreadedReusePlan.mnSlotIndex != 1 || !aThreadedCreatePlan.mbValid
        || !aThreadedCreatePlan.mbCreateNew || aThreadedCreatePlan.mnSlotIndex != 3
        || spreadsheetengine::core::execution::planThreadedPoolSlot(2, 4, 4).mbValid
        || !spreadsheetengine::core::execution::isValidThreadedPoolIndex(4, 3)
        || spreadsheetengine::core::execution::isValidThreadedPoolIndex(4, 4))
    {
        return fail("spreadsheetengine_execution_tests", "threaded pool plan mismatch");
    }

    const auto aAcquirePlan
        = spreadsheetengine::core::execution::planNonThreadedPoolAcquire(2, 1);
    if (!aAcquirePlan.mbValid || aAcquirePlan.mnSlotIndex != 1
        || aAcquirePlan.mnNextFreeAfterAcquire != 2 || aAcquirePlan.mbCreateNew)
    {
        return fail("spreadsheetengine_execution_tests", "pool acquire plan mismatch");
    }

    const auto aGrowAcquirePlan
        = spreadsheetengine::core::execution::planNonThreadedPoolAcquire(2, 2);
    if (!aGrowAcquirePlan.mbValid || !aGrowAcquirePlan.mbCreateNew
        || aGrowAcquirePlan.mnSlotIndex != 2 || aGrowAcquirePlan.mnNextFreeAfterAcquire != 3)
    {
        return fail("spreadsheetengine_execution_tests", "pool grow-acquire plan mismatch");
    }

    if (!spreadsheetengine::core::execution::hasActiveNonThreadedPoolContext(3, 2)
        || spreadsheetengine::core::execution::hasActiveNonThreadedPoolContext(3, 0)
        || spreadsheetengine::core::execution::activeNonThreadedPoolContextIndex(3, 2) != 1)
    {
        return fail("spreadsheetengine_execution_tests", "pool active-context mismatch");
    }

    const auto aReleasePlan
        = spreadsheetengine::core::execution::planNonThreadedPoolRelease(3, 2);
    if (!aReleasePlan.mbValid || aReleasePlan.mnReleasedIndex != 1
        || aReleasePlan.mnNextFreeAfterRelease != 1
        || spreadsheetengine::core::execution::planNonThreadedPoolRelease(3, 0).mbValid)
    {
        return fail("spreadsheetengine_execution_tests", "pool release plan mismatch");
    }

    int nVisitedSum = 0;
    std::vector<std::unique_ptr<PoolEntry>> aPool;
    aPool.emplace_back(std::make_unique<PoolEntry>(PoolEntry { 2 }));
    aPool.emplace_back(nullptr);
    aPool.emplace_back(std::make_unique<PoolEntry>(PoolEntry { 5 }));
    spreadsheetengine::core::execution::forEachLivePoolContext(
        aPool, [&nVisitedSum](PoolEntry& rEntry) { nVisitedSum += rEntry.mnValue; });
    if (nVisitedSum != 7)
        return fail("spreadsheetengine_execution_tests", "pool iteration mismatch");

    std::array<CacheEntry, 4> aCache { CacheEntry { 3 }, CacheEntry { 5 }, CacheEntry { 8 },
                                       CacheEntry { 13 } };
    const auto aFoundCacheEntry = spreadsheetengine::core::execution::findRecentCacheEntry(
        aCache, [](const CacheEntry& rEntry) { return rEntry.mnValue == 8; });
    if (aFoundCacheEntry == aCache.end() || aFoundCacheEntry->mnValue != 8)
        return fail("spreadsheetengine_execution_tests", "recent cache lookup mismatch");

    spreadsheetengine::core::execution::pushRecentCacheEntry(aCache, CacheEntry { 21 });
    if (aCache[0].mnValue != 21 || aCache[1].mnValue != 3 || aCache[2].mnValue != 5
        || aCache[3].mnValue != 8)
    {
        return fail("spreadsheetengine_execution_tests", "recent cache promotion mismatch");
    }

    int nRecentCacheBuildCount = 0;
    auto& rInsertedCacheEntry = spreadsheetengine::core::execution::getOrInsertRecentCacheEntry(
        aCache, [](const CacheEntry& rEntry) { return rEntry.mnValue == 34; },
        [&nRecentCacheBuildCount]() {
            ++nRecentCacheBuildCount;
            return CacheEntry { 34 };
        });
    if (nRecentCacheBuildCount != 1 || rInsertedCacheEntry.mnValue != 34 || aCache[0].mnValue != 34)
    {
        return fail("spreadsheetengine_execution_tests", "recent cache insert mismatch");
    }

    auto& rHitCacheEntry = spreadsheetengine::core::execution::getOrInsertRecentCacheEntry(
        aCache, [](const CacheEntry& rEntry) { return rEntry.mnValue == 34; },
        [&nRecentCacheBuildCount]() {
            ++nRecentCacheBuildCount;
            return CacheEntry { 55 };
        });
    if (nRecentCacheBuildCount != 1 || rHitCacheEntry.mnValue != 34 || &rHitCacheEntry != &aCache[0])
    {
        return fail("spreadsheetengine_execution_tests", "recent cache hit mismatch");
    }

    spreadsheetengine::core::execution::resetRecentCache(aCache);
    if (aCache[0].mnValue != -1 || aCache[3].mnValue != -1)
        return fail("spreadsheetengine_execution_tests", "recent cache reset mismatch");

    std::vector<unsigned char> aConditions { 1, 2, 3 };
    std::vector<int> aDelayedState { 7, 9 };
    ReleaseToken aToken3;
    aTokens[0] = &aToken3;
    nTokenCachePos = 2;
    spreadsheetengine::core::execution::cleanupScratchState(
        aConditions, aDelayedState, aTokens, nTokenCachePos, [&](ReleaseToken* pToken) {
            pToken->mbReleased = true;
        });

    if (!aConditions.empty() || !aDelayedState.empty() || nTokenCachePos != 0
        || aTokens[0] != nullptr || !aToken3.mbReleased)
    {
        return fail("spreadsheetengine_execution_tests", "scratch cleanup mismatch");
    }

    const int nDocA = 1;
    const int nDocB = 2;
    auto xLookupCache = std::make_unique<int>(5);
    auto xLanguageData = std::make_unique<int>(6);
    auto xAuxFormatKeyMap = std::make_unique<int>(7);
    int nFormatter = 11;
    int nFormatData = 12;
    int nNatNum = 13;
    int* pFormatter = &nFormatter;
    int* pFormatData = &nFormatData;
    int* pNatNum = &nNatNum;

    if (spreadsheetengine::core::execution::clearDocBoundStateIfMatches(
            &nDocB, &nDocA, xLookupCache, xLanguageData, xAuxFormatKeyMap, pFormatter, pFormatData,
            pNatNum))
    {
        return fail("spreadsheetengine_execution_tests", "doc-bound state should not clear");
    }

    if (!spreadsheetengine::core::execution::clearDocBoundStateIfMatches(
            &nDocA, &nDocA, xLookupCache, xLanguageData, xAuxFormatKeyMap, pFormatter, pFormatData,
            pNatNum))
    {
        return fail("spreadsheetengine_execution_tests", "doc-bound state clear mismatch");
    }

    if (xLookupCache || xLanguageData || xAuxFormatKeyMap || pFormatter || pFormatData || pNatNum)
        return fail("spreadsheetengine_execution_tests", "doc-bound state not cleared");

    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
