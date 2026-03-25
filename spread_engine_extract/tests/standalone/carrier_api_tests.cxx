/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/api/FormulaResult.hxx>
#include <spreadsheetengine/api/ReferenceData.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::CellAddress;
    using spreadsheetengine::api::Error;
    using spreadsheetengine::api::formulavalue::CarrierType;
    using spreadsheetengine::api::formulavalue::ValueType;
    using spreadsheetengine::api::refdata::ComplexRefData;
    using spreadsheetengine::api::refdata::SheetLimits;
    using spreadsheetengine::api::refdata::SingleRefData;
    using spreadsheetengine::standalone::test::fail;

    const auto aInvalid = spreadsheetengine::api::formulavalue::makeInvalidResult();
    const auto aValue = spreadsheetengine::api::formulavalue::makeValueResult(12.5);
    const auto aString = spreadsheetengine::api::formulavalue::makeStringResult(u"abc", true);
    const auto aError = spreadsheetengine::api::formulavalue::makeErrorResult(Error::DivisionByZero);
    if (aInvalid.meType != ValueType::Invalid || aValue.meType != ValueType::Value
        || aString.meType != ValueType::String || !aString.mbMultiLine
        || aError.meType != ValueType::Error || aError.meError != Error::DivisionByZero
        || !spreadsheetengine::api::formulavalue::isValueCarrierType(CarrierType::Double, false)
        || !spreadsheetengine::api::formulavalue::isValueCarrierType(CarrierType::Unknown, false)
        || !spreadsheetengine::api::formulavalue::isValueCarrierType(CarrierType::EmptyCell, true)
        || spreadsheetengine::api::formulavalue::isValueCarrierType(CarrierType::String, false)
        || !spreadsheetengine::api::formulavalue::isValueCarrierTypeNoError(CarrierType::EmptyCell)
        || spreadsheetengine::api::formulavalue::isValueCarrierTypeNoError(CarrierType::Error)
        || !spreadsheetengine::api::formulavalue::isStringCarrierType(CarrierType::HybridCell))
    {
        return fail("spreadsheetengine_carrier_tests", "formula result carrier mismatch");
    }

    const SheetLimits aLimits { 1023, 65535, 1023 };
    const CellAddress aPos { 4, 10, 20 };
    SingleRefData aRef1 { 5, -2, 3, { true, false, true, false, false, false, false, true } };
    SingleRefData aRef2 { -1, 4, 2, { true, false, true, false, false, false, false, true } };

    const auto aAbs1 = spreadsheetengine::api::refdata::toAbsoluteAddress(aRef1, aLimits, aPos);
    if (aAbs1.mnColumn != 15 || aAbs1.mnRow != 18 || aAbs1.mnSheet != 3)
        return fail("spreadsheetengine_carrier_tests", "single ref absolute address mismatch");

    spreadsheetengine::api::refdata::putInOrder(aRef1, aRef2, aPos);
    const auto aOrdered1 = spreadsheetengine::api::refdata::toAbsoluteAddress(aRef1, aLimits, aPos);
    const auto aOrdered2 = spreadsheetengine::api::refdata::toAbsoluteAddress(aRef2, aLimits, aPos);
    if (aOrdered1.mnColumn > aOrdered2.mnColumn || aOrdered1.mnRow > aOrdered2.mnRow)
        return fail("spreadsheetengine_carrier_tests", "single ref ordering mismatch");

    ComplexRefData aRange { aRef1, aRef2, false };
    spreadsheetengine::api::refdata::putInOrder(aRange, aPos);
    const auto aAbsRange = spreadsheetengine::api::refdata::toAbsoluteRange(aRange, aLimits, aPos);
    if (aAbsRange.maStart.mnSheet != 2 || aAbsRange.maEnd.mnSheet != 3
        || aAbsRange.maStart.mnColumn > aAbsRange.maEnd.mnColumn
        || aAbsRange.maStart.mnRow > aAbsRange.maEnd.mnRow)
        return fail("spreadsheetengine_carrier_tests", "range normalization mismatch");

    SingleRefData aExtend { 30, 10, 8, { false, false, false, false, false, false, true, true } };
    const auto aExtended
        = spreadsheetengine::api::refdata::extendReferenceRange(aRange, aExtend, aLimits, aPos);
    const auto aExtendedAbs
        = spreadsheetengine::api::refdata::toAbsoluteRange(aExtended, aLimits, aPos);
    if (aExtendedAbs.maStart.mnSheet != 2 || aExtendedAbs.maStart.mnColumn != 9
        || aExtendedAbs.maStart.mnRow != 10 || aExtendedAbs.maEnd.mnSheet != 8
        || aExtendedAbs.maEnd.mnColumn != 30 || aExtendedAbs.maEnd.mnRow != 24)
    {
        return fail("spreadsheetengine_carrier_tests", "range extension mismatch");
    }

    ComplexRefData aWholeColumn {
        { 0, 0, 0, { false, false, false, false, false, false, false, false } },
        { 0, aLimits.mnMaxRow, 0, { false, false, false, false, false, false, false, false } },
        false
    };
    if (!spreadsheetengine::api::refdata::isEntireColumn(aWholeColumn, aLimits))
        return fail("spreadsheetengine_carrier_tests", "entire-column detection mismatch");

    std::cout << "spreadsheetengine carrier api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
