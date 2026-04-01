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

#include <com/sun/star/uno/Reference.hxx>

#include <comphelper/processfactory.hxx>

namespace
{
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
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.0509453369140622, mxAnalysis->getEffect(0.05, 4), 1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(0.05, mxAnalysis->getNominal(0.0509453369140622, 4), 1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.02, mxAnalysis->getDollarfr(1.125, 16), 1e-12);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(1.125, mxAnalysis->getDollarde(1.02, 16), 1e-12);
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
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
