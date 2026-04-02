/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/bootstrapfixture.hxx>

#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/sheet/addin/XAnalysis.hpp>
#include <com/sun/star/uno/XInterface.hpp>
#include <com/sun/star/util/Date.hpp>

#include <com/sun/star/uno/Reference.hxx>

#include <comphelper/genericpropertyset.hxx>
#include <comphelper/propertysetinfo.hxx>
#include <comphelper/processfactory.hxx>
#include <spreadsheetengine/compat/libreoffice/FinancialAddInExecution.hxx>

namespace
{
css::uno::Reference<css::beans::XPropertySet> makeAnalysisOptions()
{
    static const comphelper::PropertyMapEntry aEntries[] = {
        { u"NullDate"_ustr, 0, cppu::UnoType<css::util::Date>::get(), 0, 0 },
    };
    auto* pInfo = new comphelper::PropertySetInfo(
        std::span<const comphelper::PropertyMapEntry>(aEntries, std::size(aEntries)));
    auto xOptions = comphelper::GenericPropertySet_CreateInstance(pInfo);
    xOptions->setPropertyValue(
        u"NullDate"_ustr, css::uno::Any(css::util::Date { 30, 12, 1899 }));
    return xOptions;
}

css::uno::Any makeHolidayMatrix(std::initializer_list<sal_Int32> aDays)
{
    css::uno::Sequence<css::uno::Sequence<css::uno::Any>> aMatrix(1);
    aMatrix.getArray()[0].realloc(aDays.size());
    sal_Int32 nIndex = 0;
    for (sal_Int32 nDay : aDays)
        aMatrix.getArray()[0].getArray()[nIndex++] = css::uno::Any(nDay);
    return css::uno::Any(aMatrix);
}

class Test : public test::BootstrapFixture
{
public:
    virtual void setUp() override;

protected:
    css::uno::Reference<css::sheet::addin::XAnalysis> mxAnalysis;
};

void Test::setUp()
{
    test::BootstrapFixture::setUp();
    auto xFactory(comphelper::getProcessServiceFactory());
    mxAnalysis.set(xFactory->createInstance(u"com.sun.star.sheet.addin.Analysis"_ustr),
                   css::uno::UNO_QUERY_THROW);
}

CPPUNIT_TEST_FIXTURE(Test, test_getDec2Hex)
{
    // Test that 'Places' argument accepts different numeric types
    CPPUNIT_ASSERT_EQUAL(u"000000006E"_ustr,
                         mxAnalysis->getDec2Hex({}, 110, css::uno::Any(sal_Int8(10))));
    CPPUNIT_ASSERT_EQUAL(u"000000006E"_ustr,
                         mxAnalysis->getDec2Hex({}, 110, css::uno::Any(sal_Int16(10))));
    CPPUNIT_ASSERT_EQUAL(u"000000006E"_ustr,
                         mxAnalysis->getDec2Hex({}, 110, css::uno::Any(sal_uInt16(10))));
    CPPUNIT_ASSERT_EQUAL(u"000000006E"_ustr,
                         mxAnalysis->getDec2Hex({}, 110, css::uno::Any(sal_Int32(10))));
    CPPUNIT_ASSERT_EQUAL(u"000000006E"_ustr,
                         mxAnalysis->getDec2Hex({}, 110, css::uno::Any(sal_uInt32(10))));
    CPPUNIT_ASSERT_EQUAL(u"000000006E"_ustr,
                         mxAnalysis->getDec2Hex({}, 110, css::uno::Any(sal_Int64(10))));
    CPPUNIT_ASSERT_EQUAL(u"000000006E"_ustr,
                         mxAnalysis->getDec2Hex({}, 110, css::uno::Any(sal_uInt64(10))));
    CPPUNIT_ASSERT_EQUAL(u"000000006E"_ustr,
                         mxAnalysis->getDec2Hex({}, 110, css::uno::Any(double(10))));
    CPPUNIT_ASSERT_EQUAL(u"000000006E"_ustr,
                         mxAnalysis->getDec2Hex({}, 110, css::uno::Any(float(10))));
}

CPPUNIT_TEST_FIXTURE(Test, test_sharedFinancialRuntimeDelegates)
{
    const auto xOptions = makeAnalysisOptions();
    const sal_Int32 nIssue = 40909;
    const sal_Int32 nFirstInterest = 41091;
    const sal_Int32 nSettlement = 41320;
    const sal_Int32 nDurationSettlement = 36892;
    const sal_Int32 nDurationMaturity = 38718;
    const sal_Int32 nYieldmatSettlement = 36206;
    const sal_Int32 nYieldmatMaturity = 36263;
    const sal_Int32 nYieldmatIssue = 36110;

    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0509453369140622, mxAnalysis->getEffect(0.05, 4), 1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.05, mxAnalysis->getNominal(0.0509453369140622, 4), 1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.02, mxAnalysis->getDollarfr(1.125, 16), 1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.125, mxAnalysis->getDollarde(1.02, 16), 1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        365.958904109589, mxAnalysis->getAccrint(
                               xOptions, nIssue, nFirstInterest, nSettlement, 0.065,
                               css::uno::Any(5000.0), 2, css::uno::Any(sal_Int32(3))),
        1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        4.20161802829783,
        mxAnalysis->getDuration(
            xOptions, nDurationSettlement, nDurationMaturity, 0.08, 0.09, 2,
            css::uno::Any(sal_Int32(3))),
        1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        0.061,
        mxAnalysis->getYieldmat(
            xOptions, nYieldmatSettlement, nYieldmatMaturity, nYieldmatIssue, 0.061,
            99.984498875557, css::uno::Any(sal_Int32(0))),
        1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-600.875855808337,
                                 mxAnalysis->getCumprinc(0.055 / 12.0, 24, 5000, 4, 6, 1),
                                 1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-57.5412415342252,
                                 mxAnalysis->getCumipmt(0.055 / 12.0, 24, 5000, 4, 6, 1),
                                 1e-12);

    css::uno::Sequence<css::uno::Sequence<double>> aSchedule(1);
    aSchedule.getArray()[0].realloc(2);
    aSchedule.getArray()[0].getArray()[0] = 0.1;
    aSchedule.getArray()[0].getArray()[1] = 0.2;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(132.0, mxAnalysis->getFvschedule(100.0, aSchedule), 1e-12);

    css::uno::Sequence<css::uno::Sequence<double>> aValues(1);
    aValues.getArray()[0].realloc(2);
    aValues.getArray()[0].getArray()[0] = 100.0;
    aValues.getArray()[0].getArray()[1] = 200.0;
    css::uno::Sequence<css::uno::Sequence<sal_Int32>> aDates(1);
    aDates.getArray()[0].realloc(2);
    aDates.getArray()[0].getArray()[0] = 1;
    aDates.getArray()[0].getArray()[1] = 366;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(281.818181818182, mxAnalysis->getXnpv(0.1, aValues, aDates),
                                 1e-12);

    css::uno::Sequence<css::uno::Sequence<double>> aIrrValues(1);
    aIrrValues.getArray()[0].realloc(2);
    aIrrValues.getArray()[0].getArray()[0] = -100.0;
    aIrrValues.getArray()[0].getArray()[1] = 110.0;
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.1,
                                 mxAnalysis->getXirr({}, aIrrValues, aDates, css::uno::Any(0.1)),
                                 1e-12);
}

CPPUNIT_TEST_FIXTURE(Test, testDirectFinancialAddInAdapter)
{
    namespace sefinanceexec = spreadsheetengine::compat::libreoffice::financialaddinexecution;

    const spreadsheetengine::api::DateParts aNullDate{ 1899, 12, 30 };
    const sal_Int32 nIssue = 40909;
    const sal_Int32 nSettlement = 41320;
    const sal_Int32 nDurationSettlement = 36892;
    const sal_Int32 nDurationMaturity = 38718;
    const sal_Int32 nPriceSettlement = 36206;
    const sal_Int32 nPriceMaturity = 39401;
    const sal_Int32 nYieldmatSettlement = 36206;
    const sal_Int32 nYieldmatMaturity = 36263;
    const sal_Int32 nYieldmatIssue = 36110;
    const sal_Int32 nOddlyieldSettlement = 36270;
    const sal_Int32 nOddlyieldMaturity = 36326;
    const sal_Int32 nOddlyieldLastInterest = 36083;
    const sal_Int32 nCouponSettlement = 36916;
    const sal_Int32 nCouponMaturity = 37210;

    const auto aEffect = sefinanceexec::DirectFinancialAddInAdapter::evaluateEffect(0.05, 4);
    CPPUNIT_ASSERT(aEffect);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0509453369140622, aEffect.maValue, 1e-12);

    const auto aNominal
        = sefinanceexec::DirectFinancialAddInAdapter::evaluateNominal(0.0509453369140622, 4);
    CPPUNIT_ASSERT(aNominal);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.05, aNominal.maValue, 1e-12);

    const auto aDollarFr
        = sefinanceexec::DirectFinancialAddInAdapter::evaluateDollarFraction(1.125, 16);
    CPPUNIT_ASSERT(aDollarFr);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.02, aDollarFr.maValue, 1e-12);

    const auto aDollarDe
        = sefinanceexec::DirectFinancialAddInAdapter::evaluateDollarDecimal(1.02, 16);
    CPPUNIT_ASSERT(aDollarDe);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.125, aDollarDe.maValue, 1e-12);

    const auto aCumprinc = sefinanceexec::DirectFinancialAddInAdapter::evaluateCumulativePrincipal(
        0.055 / 12.0, 24, 5000, 4, 6, true);
    CPPUNIT_ASSERT(aCumprinc);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-600.875855808337, aCumprinc.maValue, 1e-12);

    const auto aCumipmt = sefinanceexec::DirectFinancialAddInAdapter::evaluateCumulativeInterest(
        0.055 / 12.0, 24, 5000, 4, 6, true);
    CPPUNIT_ASSERT(aCumipmt);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(-57.5412415342252, aCumipmt.maValue, 1e-12);

    sefinanceexec::DirectFinancialAddInAdapter aBasisThreeAdapter(aNullDate, 3);
    const auto aAccrint
        = aBasisThreeAdapter.evaluateAccrint(nIssue, nSettlement, 0.065, 5000.0, 2);
    CPPUNIT_ASSERT(aAccrint);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(365.958904109589, aAccrint.maValue, 1e-12);

    const auto aAccrintm
        = aBasisThreeAdapter.evaluateAccrintm(nIssue, nSettlement, 0.065, 5000.0);
    CPPUNIT_ASSERT(aAccrintm);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(365.958904109589, aAccrintm.maValue, 1e-12);

    const auto aDuration = aBasisThreeAdapter.evaluateDuration(
        nDurationSettlement, nDurationMaturity, 0.08, 0.09, 2);
    CPPUNIT_ASSERT(aDuration);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.20161802829783, aDuration.maValue, 1e-12);

    sefinanceexec::DirectFinancialAddInAdapter aBasisZeroAdapter(aNullDate, 0);
    const auto aPrice = aBasisZeroAdapter.evaluatePrice(
        nPriceSettlement, nPriceMaturity, 0.0575, 0.065, 100.0, 2);
    CPPUNIT_ASSERT(aPrice);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(95.0428743993921, aPrice.maValue, 1e-12);

    const auto aYieldmat = aBasisZeroAdapter.evaluateYieldmat(
        nYieldmatSettlement, nYieldmatMaturity, nYieldmatIssue, 0.061, 99.984498875557);
    CPPUNIT_ASSERT(aYieldmat);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.061, aYieldmat.maValue, 1e-12);

    const auto aOddlyield = aBasisZeroAdapter.evaluateOddlyield(
        nOddlyieldSettlement, nOddlyieldMaturity, nOddlyieldLastInterest, 0.0375, 99.875, 100.0,
        2);
    CPPUNIT_ASSERT(aOddlyield);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0448731663302424, aOddlyield.maValue, 1e-12);

    const auto aCoupnum
        = aBasisThreeAdapter.evaluateCoupnum(nCouponSettlement, nCouponMaturity, 2);
    CPPUNIT_ASSERT(aCoupnum);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.0, aCoupnum.maValue, 1e-12);

    sefinanceexec::DirectFinancialAddInAdapter aNullDateAdapter(aNullDate);
    const auto aTbillEq = aNullDateAdapter.evaluateTbillEq(36250, 36260, 0.0914);
    CPPUNIT_ASSERT(aTbillEq);
    CPPUNIT_ASSERT(aTbillEq.maValue > 0.0);
}

CPPUNIT_TEST_FIXTURE(Test, test_sharedCalendarRuntimeDelegates)
{
    const auto xOptions = makeAnalysisOptions();
    const auto aHolidays = makeHolidayMatrix({ 41945, 41946, 41947 }); // 2014-11-02..04

    CPPUNIT_ASSERT_EQUAL(sal_Int32(41954), // 2014-11-11
                         mxAnalysis->getWorkday(xOptions, 41944, 5, aHolidays));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(18),
                         mxAnalysis->getNetworkdays(xOptions, 41944, 41973, aHolidays));
    CPPUNIT_ASSERT_DOUBLES_EQUAL(
        1.0, mxAnalysis->getYearfrac(xOptions, 41640, 42005, css::uno::Any(sal_Int32(0))), 1e-12);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1),
                         mxAnalysis->getWeeknum(xOptions, 42370, 1)); // 2016-01-01
    CPPUNIT_ASSERT_EQUAL(sal_Int32(37011),
                         mxAnalysis->getEdate(xOptions, 36981, 1)); // 2001-03-31 -> 2001-04-30
    CPPUNIT_ASSERT_EQUAL(sal_Int32(42063),
                         mxAnalysis->getEomonth(xOptions, 42015, 1)); // 2015-01-11 -> 2015-02-28
    CPPUNIT_ASSERT_THROW(mxAnalysis->getTbilleq(xOptions, 36250, 36678, 0.0914),
                         css::lang::IllegalArgumentException); // >360 days
}
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
