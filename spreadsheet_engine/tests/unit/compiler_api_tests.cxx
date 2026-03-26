/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/api/Compiler.hxx>
#include <spreadsheetengine/api/Grammar.hxx>
#include <spreadsheetengine/api/StringReference.hxx>
#include <spreadsheetengine/compat/formula/FormulaGrammar.hxx>
#include <spreadsheetengine/detail/CompilerSupport.hxx>

#include "TestSupport.hxx"

int main()
{
    using spreadsheetengine::api::CompilerCharFlags;
    using spreadsheetengine::standalone::test::fail;

    const auto& rOdfTable = spreadsheetengine::core::compiler::getCharTable(
        spreadsheetengine::api::AddressConvention::OdfA1);
    if (!spreadsheetengine::api::hasAnyCompilerCharFlag(
            rOdfTable['['], CompilerCharFlags::OdfLBracket)
        || !spreadsheetengine::api::hasAnyCompilerCharFlag(
            rOdfTable['!'], CompilerCharFlags::OdfLabelOp))
    {
        return fail("spreadsheetengine_compiler_tests", "ODF char table mismatch");
    }

    const auto& rXlTable = spreadsheetengine::core::compiler::getCharTable(
        spreadsheetengine::api::AddressConvention::XlOox);
    if (!spreadsheetengine::api::hasAnyCompilerCharFlag(
            rXlTable['['], CompilerCharFlags::CharIdent)
        || !spreadsheetengine::api::hasAnyCompilerCharFlag(
            rXlTable[']'], CompilerCharFlags::Ident))
    {
        return fail("spreadsheetengine_compiler_tests", "OOXML char table mismatch");
    }

    const auto aApiGrammar
        = spreadsheetengine::compat::formula::FormulaGrammar::mapAPItoGrammar(true, false);
    if (aApiGrammar.meLanguage != spreadsheetengine::api::FormulaLanguage::Api
        || aApiGrammar.meAddressConvention != spreadsheetengine::api::AddressConvention::OooA1
        || !aApiGrammar.mbEnglish)
    {
        return fail("spreadsheetengine_compiler_tests", "API grammar mapping mismatch");
    }

    const auto aMergedGrammar = spreadsheetengine::compat::formula::FormulaGrammar::mergeToGrammar(
        aApiGrammar, spreadsheetengine::api::AddressConvention::XlOox);
    if (!spreadsheetengine::compat::formula::FormulaGrammar::isExcelSyntax(aMergedGrammar)
        || !spreadsheetengine::compat::formula::FormulaGrammar::isRefConventionOOXML(
               aMergedGrammar))
    {
        return fail("spreadsheetengine_compiler_tests", "grammar merge mismatch");
    }

    if (!spreadsheetengine::compat::formula::FormulaGrammar::isSupported(
            { spreadsheetengine::api::FormulaLanguage::XlEnglish,
                spreadsheetengine::api::AddressConvention::XlA1, true })
        || spreadsheetengine::compat::formula::FormulaGrammar::isSupported(
            { spreadsheetengine::api::FormulaLanguage::Native,
                spreadsheetengine::api::AddressConvention::XlOox, false }))
    {
        return fail("spreadsheetengine_compiler_tests", "grammar support mismatch");
    }

    const auto aIndirectHybrid
        = spreadsheetengine::api::stringreference::resolveIndirectAddressSyntaxPolicy(
            spreadsheetengine::api::AddressConvention::Unknown,
            spreadsheetengine::api::AddressConvention::XlA1, true, false);
    const auto aIndirectConfigured
        = spreadsheetengine::api::stringreference::resolveIndirectAddressSyntaxPolicy(
            spreadsheetengine::api::AddressConvention::OdfA1,
            spreadsheetengine::api::AddressConvention::XlA1, false, false);
    const auto aIndirectR1C1
        = spreadsheetengine::api::stringreference::resolveIndirectAddressSyntaxPolicy(
            spreadsheetengine::api::AddressConvention::OooA1,
            spreadsheetengine::api::AddressConvention::XlA1, false, true);
    const auto aAddressConv = spreadsheetengine::api::stringreference::resolveAddressFunctionConvention(
        spreadsheetengine::api::AddressConvention::XlR1C1,
        spreadsheetengine::api::AddressConvention::OooA1, false);
    const auto aAddressDefault
        = spreadsheetengine::api::stringreference::resolveAddressFunctionConvention(
            spreadsheetengine::api::AddressConvention::Unknown,
            spreadsheetengine::api::AddressConvention::OdfA1, false);
    if (aIndirectHybrid.mePrimary != spreadsheetengine::api::AddressConvention::OooA1
        || aIndirectHybrid.moFallback != spreadsheetengine::api::AddressConvention::XlA1
        || aIndirectConfigured.mePrimary != spreadsheetengine::api::AddressConvention::OdfA1
        || aIndirectConfigured.moFallback.has_value()
        || aIndirectR1C1.mePrimary != spreadsheetengine::api::AddressConvention::XlR1C1
        || aIndirectR1C1.moFallback.has_value()
        || aAddressConv != spreadsheetengine::api::AddressConvention::XlA1
        || aAddressDefault != spreadsheetengine::api::AddressConvention::OooA1)
    {
        return fail("spreadsheetengine_compiler_tests", "string reference policy mismatch");
    }

    std::cout << "spreadsheetengine compiler api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
