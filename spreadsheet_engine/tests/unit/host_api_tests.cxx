/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cfloat>
#include <cmath>
#include <iostream>
#include <random>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/detail/HostValueAccess.hxx>
#include <spreadsheetengine/runtime/InMemoryHost.hxx>
#include <spreadsheetengine/runtime/RpnRandom.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::CellAddress;
    using spreadsheetengine::api::CellRange;
    using spreadsheetengine::api::CellValue;
    using spreadsheetengine::api::CellValueKind;
    using spreadsheetengine::api::CellValueView;
    using spreadsheetengine::api::DateParts;
    using spreadsheetengine::api::Error;
    using spreadsheetengine::core::host::coerceToNumber;
    using spreadsheetengine::core::host::coerceValueViewElementToNumber;
    using spreadsheetengine::core::host::formatValue;
    using spreadsheetengine::core::host::InMemoryEvaluationHost;
    using spreadsheetengine::core::rpn::planRandArray;
    using spreadsheetengine::core::rpn::planRandbetween;
    using spreadsheetengine::core::rpn::planRandom;
    using spreadsheetengine::core::rpn::RandomOutputFrame;
    using spreadsheetengine::core::host::readValueView;
    using spreadsheetengine::core::host::readValueViewElement;
    using spreadsheetengine::standalone::test::almostEqual;
    using spreadsheetengine::standalone::test::fail;

    InMemoryEvaluationHost aHost;
    aHost.setLocaleTag(u"en-US");
    aHost.setNullDate(DateParts { 1899, 12, 30 });
    aHost.setParsedNumber(u"42.5", 42.5, 11);
    aHost.setFormattedNumber(42.5, 11, u"42.5");

    const auto nSheet0 = aHost.addSheet(u"Sheet1");
    const auto nSheet1 = aHost.addSheet(u"Sheet2");

    if (nSheet0 != 0 || nSheet1 != 1 || aHost.sheetCount() != 2 || !aHost.hasSheet(nSheet0)
        || !aHost.hasSheet(nSheet1) || aHost.hasSheet(5))
    {
        return fail("spreadsheetengine_host_tests", "sheet registration mismatch");
    }

    const auto aSheetName = aHost.getSheetName(nSheet1);
    if (!aSheetName || aSheetName.maValue != u"Sheet2")
        return fail("spreadsheetengine_host_tests", "getSheetName() mismatch");

    const auto aMissingSheetName = aHost.getSheetName(5);
    if (aMissingSheetName || aMissingSheetName.meError != Error::IllegalArgument)
        return fail("spreadsheetengine_host_tests", "missing-sheet handling mismatch");

    if (aHost.getNullDate().mnYear != 1899 || aHost.getNullDate().mnMonth != 12
        || aHost.getNullDate().mnDay != 30 || aHost.getLocaleTag() != u"en-US")
    {
        return fail("spreadsheetengine_host_tests", "runtime environment mismatch");
    }

    aHost.seedRandomGenerator(1337);
    std::mt19937 aExpectedRandom(1337);
    std::uniform_real_distribution<double> aUnitDistribution(0.0, 1.0);
    const auto aRandomSample = aHost.sampleUniformReal(0.0, 1.0);
    if (!aRandomSample || !almostEqual(aRandomSample.maValue, aUnitDistribution(aExpectedRandom)))
    {
        return fail("spreadsheetengine_host_tests", "sampleUniformReal() mismatch");
    }

    const auto aInvalidRandomRange = aHost.sampleUniformReal(3.0, 2.0);
    if (aInvalidRandomRange || aInvalidRandomRange.meError != Error::IllegalArgument)
        return fail("spreadsheetengine_host_tests", "sampleUniformReal() error mismatch");

    aHost.seedRandomGenerator(2026);
    std::mt19937 aExpectedRuntime(2026);

    const auto aScalarRandom = planRandom(aHost, RandomOutputFrame {});
    if (!aScalarRandom || !aScalarRandom.maValue.mbIsScalar
        || !almostEqual(aScalarRandom.maValue.mfScalar, aUnitDistribution(aExpectedRuntime)))
    {
        return fail("spreadsheetengine_host_tests", "planRandom() scalar mismatch");
    }

    const auto aCompatRandom = planRandom(
        aHost, RandomOutputFrame { true, true, spreadsheetengine::api::MatrixDimensions { 1, 1 } });
    if (!aCompatRandom || !aCompatRandom.maValue.mbIsScalar
        || !almostEqual(aCompatRandom.maValue.mfScalar, aUnitDistribution(aExpectedRuntime)))
    {
        return fail("spreadsheetengine_host_tests", "planRandom() compat scalar mismatch");
    }

    const auto aZeroDimMatrixRandom = planRandom(
        aHost, RandomOutputFrame { true, true, spreadsheetengine::api::MatrixDimensions { 0, 0 } });
    if (!aZeroDimMatrixRandom || aZeroDimMatrixRandom.maValue.mbIsScalar
        || aZeroDimMatrixRandom.maValue.maMatrix.maDimensions.mnColumns != 1
        || aZeroDimMatrixRandom.maValue.maMatrix.maDimensions.mnRows != 1
        || !almostEqual(aZeroDimMatrixRandom.maValue.maMatrix.maValues.front().mfNumber,
               aUnitDistribution(aExpectedRuntime)))
    {
        return fail("spreadsheetengine_host_tests", "planRandom() zero-dimension matrix mismatch");
    }

    const auto aMatrixRandom = planRandom(
        aHost, RandomOutputFrame { true, false, spreadsheetengine::api::MatrixDimensions { 2, 2 } });
    const double fExpected00 = aUnitDistribution(aExpectedRuntime);
    const double fExpected01 = aUnitDistribution(aExpectedRuntime);
    const double fExpected10 = aUnitDistribution(aExpectedRuntime);
    const double fExpected11 = aUnitDistribution(aExpectedRuntime);
    if (!aMatrixRandom || aMatrixRandom.maValue.mbIsScalar
        || aMatrixRandom.maValue.maMatrix.maDimensions.mnColumns != 2
        || aMatrixRandom.maValue.maMatrix.maDimensions.mnRows != 2
        || !almostEqual(aMatrixRandom.maValue.maMatrix.maValues[0].mfNumber, fExpected00)
        || !almostEqual(aMatrixRandom.maValue.maMatrix.maValues[1].mfNumber, fExpected10)
        || !almostEqual(aMatrixRandom.maValue.maMatrix.maValues[2].mfNumber, fExpected01)
        || !almostEqual(aMatrixRandom.maValue.maMatrix.maValues[3].mfNumber, fExpected11))
    {
        return fail("spreadsheetengine_host_tests", "planRandom() matrix mismatch");
    }

    std::uniform_real_distribution<double> aBetweenDistribution(
        2.0, std::nextafter(6.0, -DBL_MAX));
    const auto aRandbetween = planRandbetween(aHost,
        RandomOutputFrame { true, false, spreadsheetengine::api::MatrixDimensions { 2, 1 } }, 2.0,
        5.0);
    if (!aRandbetween || aRandbetween.maValue.mbIsScalar
        || !almostEqual(aRandbetween.maValue.maMatrix.maValues[0].mfNumber,
               std::floor(aBetweenDistribution(aExpectedRuntime)))
        || !almostEqual(aRandbetween.maValue.maMatrix.maValues[1].mfNumber,
               std::floor(aBetweenDistribution(aExpectedRuntime))))
    {
        return fail("spreadsheetengine_host_tests", "planRandbetween() mismatch");
    }

    std::uniform_real_distribution<double> aWholeArrayDistribution(
        1.0, std::nextafter(4.0, -DBL_MAX));
    const auto aRandArray = planRandArray(
        aHost, spreadsheetengine::api::MatrixDimensions { 2, 2 }, 1.0, 3.0, true);
    const double fArray00 = std::floor(aWholeArrayDistribution(aExpectedRuntime));
    const double fArray01 = std::floor(aWholeArrayDistribution(aExpectedRuntime));
    const double fArray10 = std::floor(aWholeArrayDistribution(aExpectedRuntime));
    const double fArray11 = std::floor(aWholeArrayDistribution(aExpectedRuntime));
    if (!aRandArray || aRandArray.maValue.mbIsScalar
        || !almostEqual(aRandArray.maValue.maMatrix.maValues[0].mfNumber, fArray00)
        || !almostEqual(aRandArray.maValue.maMatrix.maValues[1].mfNumber, fArray10)
        || !almostEqual(aRandArray.maValue.maMatrix.maValues[2].mfNumber, fArray01)
        || !almostEqual(aRandArray.maValue.maMatrix.maValues[3].mfNumber, fArray11))
    {
        return fail("spreadsheetengine_host_tests", "planRandArray() matrix mismatch");
    }

    if (!aHost.setCellValue({ nSheet0, 0, 0 }, CellValue::number(42.5))
        || !aHost.setCellValue({ nSheet0, 1, 0 }, CellValue::text(u"hello"))
        || !aHost.setCellValue({ nSheet0, 2, 0 }, CellValue::error(Error::NoValue))
        || !aHost.setCellValue({ nSheet0, 3, 0 }, CellValue::boolean(true))
        || !aHost.setCellValue({ nSheet0, 4, 0 }, CellValue::text(u"42.5")))
    {
        return fail("spreadsheetengine_host_tests", "setCellValue() mismatch");
    }

    const auto aNumber = aHost.getCellValue({ nSheet0, 0, 0 });
    if (!aNumber || !aNumber.maValue.isNumber() || !almostEqual(aNumber.maValue.mfNumber, 42.5))
        return fail("spreadsheetengine_host_tests", "numeric cell read mismatch");

    const auto aText = aHost.getCellValue({ nSheet0, 1, 0 });
    if (!aText || !aText.maValue.isText() || aText.maValue.maString != u"hello")
        return fail("spreadsheetengine_host_tests", "text cell read mismatch");

    const auto aError = aHost.getCellValue({ nSheet0, 2, 0 });
    if (!aError || !aError.maValue.isError() || aError.maValue.meError != Error::NoValue)
        return fail("spreadsheetengine_host_tests", "error cell read mismatch");

    const auto aBoolean = aHost.getCellValue({ nSheet0, 3, 0 });
    if (!aBoolean || aBoolean.maValue.meKind != CellValueKind::Boolean
        || !almostEqual(aBoolean.maValue.mfNumber, 1.0))
    {
        return fail("spreadsheetengine_host_tests", "boolean cell read mismatch");
    }

    const auto aEmpty = aHost.getCellValue({ nSheet0, 5, 0 });
    if (!aEmpty || !aEmpty.maValue.isEmpty())
        return fail("spreadsheetengine_host_tests", "empty cell read mismatch");

    const auto aInvalidCell = aHost.getCellValue({ 9, 0, 0 });
    if (aInvalidCell || aInvalidCell.meError != Error::IllegalArgument)
        return fail("spreadsheetengine_host_tests", "invalid-sheet cell handling mismatch");

    const CellRange aRange { CellAddress { nSheet0, 0, 0 }, CellAddress { nSheet0, 3, 0 } };
    const auto aRangeValue = aHost.getRangeValue(aRange, 1, 0);
    if (!aRangeValue || !aRangeValue.maValue.isText() || aRangeValue.maValue.maString != u"hello")
        return fail("spreadsheetengine_host_tests", "range fetch mismatch");

    const auto aInvalidRangeOffset = aHost.getRangeValue(aRange, 5, 0);
    if (aInvalidRangeOffset || aInvalidRangeOffset.meError != Error::IllegalArgument)
        return fail("spreadsheetengine_host_tests", "range offset handling mismatch");

    const auto aResolvedRange = aHost.resolveReference(aRange);
    if (!aResolvedRange || !aResolvedRange.maValue.isNormalized()
        || aResolvedRange.maValue.matrixDimensions().mnColumns != 4
        || aResolvedRange.maValue.matrixDimensions().mnRows != 1
        || aResolvedRange.maValue.addressAt(1, 0) != CellAddress { nSheet0, 1, 0 })
    {
        return fail("spreadsheetengine_host_tests", "reference resolution mismatch");
    }

    const auto aInvalidRange = aHost.resolveReference(
        CellRange { CellAddress { nSheet0, 3, 0 }, CellAddress { nSheet0, 0, 0 } });
    if (aInvalidRange || aInvalidRange.meError != Error::IllegalArgument)
        return fail("spreadsheetengine_host_tests", "invalid reference handling mismatch");

    const auto aScalarResolved = readValueView(
        aHost, CellRange { CellAddress { nSheet0, 0, 0 }, CellAddress { nSheet0, 0, 0 } });
    if (!aScalarResolved || !aScalarResolved.maValue.isScalar()
        || !aScalarResolved.maValue.maValue.isNumber())
    {
        return fail("spreadsheetengine_host_tests", "scalar value view resolution mismatch");
    }

    const auto aScalarView = CellValueView::scalar(CellValue::number(7.0));
    if (!aScalarView.isScalar() || aScalarView.isMatrixReference()
        || !almostEqual(aScalarView.maValue.mfNumber, 7.0))
    {
        return fail("spreadsheetengine_host_tests", "scalar value view mismatch");
    }

    const auto aMatrixView = CellValueView::matrixReference(aResolvedRange.maValue);
    if (!aMatrixView.isMatrixReference() || aMatrixView.isScalar()
        || aMatrixView.maReference.matrixDimensions().mnColumns != 4)
    {
        return fail("spreadsheetengine_host_tests", "matrix value view mismatch");
    }

    const auto aResolvedMatrixView = readValueView(aHost, aRange);
    if (!aResolvedMatrixView || !aResolvedMatrixView.maValue.isMatrixReference())
        return fail("spreadsheetengine_host_tests", "matrix value view resolution mismatch");

    const auto aScalarElement = readValueViewElement(aHost, aScalarResolved.maValue);
    if (!aScalarElement || !aScalarElement.maValue.isNumber()
        || !almostEqual(aScalarElement.maValue.mfNumber, 42.5))
    {
        return fail("spreadsheetengine_host_tests", "scalar element fetch mismatch");
    }

    const auto aMatrixElement = readValueViewElement(aHost, aResolvedMatrixView.maValue, 1, 0);
    if (!aMatrixElement || !aMatrixElement.maValue.isText()
        || aMatrixElement.maValue.maString != u"hello")
    {
        return fail("spreadsheetengine_host_tests", "matrix element fetch mismatch");
    }

    const auto aParsedNumber = aHost.parseNumber(u"42.5");
    if (!aParsedNumber || !almostEqual(aParsedNumber.maValue.mfValue, 42.5)
        || aParsedNumber.maValue.mnFormat != 11
        || aParsedNumber.maValue.meKind
               != spreadsheetengine::api::NumberParseResult::Kind::Number)
    {
        return fail("spreadsheetengine_host_tests", "parseNumber() mismatch");
    }

    const auto aMissingNumber = aHost.parseNumber(u"not-a-number");
    if (aMissingNumber || aMissingNumber.meError != Error::NoValue)
        return fail("spreadsheetengine_host_tests", "parseNumber() error mismatch");

    const auto aFormattedNumber = aHost.formatNumber(42.5, 11);
    if (!aFormattedNumber || aFormattedNumber.maValue != u"42.5")
        return fail("spreadsheetengine_host_tests", "formatNumber() mismatch");

    const auto aMissingFormat = aHost.formatNumber(42.5, 12);
    if (aMissingFormat || aMissingFormat.meError != Error::IllegalArgument)
        return fail("spreadsheetengine_host_tests", "formatNumber() error mismatch");

    const auto aCoercedNumeric = coerceToNumber(aHost, CellValue::text(u"42.5"));
    if (!aCoercedNumeric || !almostEqual(aCoercedNumeric.maValue.mfValue, 42.5))
        return fail("spreadsheetengine_host_tests", "coerceToNumber() text mismatch");

    const auto aCoercedEmpty = coerceToNumber(aHost, CellValue::empty());
    if (aCoercedEmpty || aCoercedEmpty.meError != Error::NoValue)
        return fail("spreadsheetengine_host_tests", "coerceToNumber() empty mismatch");

    const auto aFormattedText = formatValue(aHost, CellValue::number(42.5), 11);
    if (!aFormattedText || aFormattedText.maValue != u"42.5")
        return fail("spreadsheetengine_host_tests", "formatValue() number mismatch");

    const auto aFormattedError = formatValue(aHost, CellValue::error(Error::DivisionByZero));
    if (aFormattedError || aFormattedError.meError != Error::DivisionByZero)
        return fail("spreadsheetengine_host_tests", "formatValue() error mismatch");

    const auto aCoercedMatrixValue = coerceValueViewElementToNumber(aHost, aResolvedMatrixView.maValue);
    if (!aCoercedMatrixValue || !almostEqual(aCoercedMatrixValue.maValue.mfValue, 42.5))
        return fail("spreadsheetengine_host_tests", "coerceValueViewElementToNumber() mismatch");

    const auto aTextNumberView = readValueView(
        aHost, CellRange { CellAddress { nSheet0, 4, 0 }, CellAddress { nSheet0, 4, 0 } });
    if (!aTextNumberView || !aTextNumberView.maValue.isScalar())
        return fail("spreadsheetengine_host_tests", "text-number view mismatch");

    const auto aCoercedMatrixText
        = coerceValueViewElementToNumber(aHost, aTextNumberView.maValue);
    if (!aCoercedMatrixText || !almostEqual(aCoercedMatrixText.maValue.mfValue, 42.5))
        return fail("spreadsheetengine_host_tests", "coerceValueViewElementToNumber() text mismatch");

    std::cout << "spreadsheetengine host api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
