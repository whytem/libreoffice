/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/api/Host.hxx>
#include <spreadsheetengine/detail/HostValueAccess.hxx>
#include <spreadsheetengine/runtime/InMemoryHost.hxx>

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
