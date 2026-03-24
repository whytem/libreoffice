/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/api/Compiler.hxx>
#include <spreadsheetengine/api/Grammar.hxx>
#include <spreadsheetengine/core/CompilerSupport.hxx>

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

    std::cout << "spreadsheetengine compiler api tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
