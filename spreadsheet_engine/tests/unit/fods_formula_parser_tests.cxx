/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <iostream>

#include <spreadsheetengine/detail/OdfFormulaParser.hxx>

#include "TestSupport.hxx"

namespace
{

using spreadsheetengine::core::formula::BinaryOperator;
using spreadsheetengine::core::formula::Node;
using spreadsheetengine::core::formula::NodeKind;
using spreadsheetengine::core::formula::UnaryOperator;

const Node& onlyChild(const Node& rNode)
{
    return *rNode.maChildren.front();
}

const Node& leftChild(const Node& rNode)
{
    return *rNode.maChildren[0];
}

const Node& rightChild(const Node& rNode)
{
    return *rNode.maChildren[1];
}

}

int main()
{
    using spreadsheetengine::core::formula::parseFormula;
    using spreadsheetengine::standalone::test::almostEqual;
    using spreadsheetengine::standalone::test::fail;

    {
        const auto aResult = parseFormula(u"of:=ABS(0)");
        if (!aResult || aResult.mpRoot->meKind != NodeKind::FunctionCall
            || aResult.mpRoot->maPrimaryText != u"ABS" || aResult.mpRoot->maChildren.size() != 1
            || onlyChild(*aResult.mpRoot).meKind != NodeKind::NumberLiteral
            || !almostEqual(onlyChild(*aResult.mpRoot).mfNumber, 0.0))
        {
            return fail("spreadsheetengine_fods_parser_tests", "ABS parse mismatch");
        }
    }

    {
        const auto aResult = parseFormula(u"of:=[.A2]=[.B2]");
        if (!aResult || aResult.mpRoot->meKind != NodeKind::BinaryOperation
            || aResult.mpRoot->meBinaryOperator != BinaryOperator::Equal
            || leftChild(*aResult.mpRoot).meKind != NodeKind::CellReference
            || leftChild(*aResult.mpRoot).maPrimaryText != u".A2"
            || rightChild(*aResult.mpRoot).meKind != NodeKind::CellReference
            || rightChild(*aResult.mpRoot).maPrimaryText != u".B2")
        {
            return fail("spreadsheetengine_fods_parser_tests", "cell comparison parse mismatch");
        }
    }

    {
        const auto aResult = parseFormula(u"of:=MATCH([.F8];[.F5:.M5];1)");
        if (!aResult || aResult.mpRoot->meKind != NodeKind::FunctionCall
            || aResult.mpRoot->maPrimaryText != u"MATCH" || aResult.mpRoot->maChildren.size() != 3
            || aResult.mpRoot->maChildren[0]->meKind != NodeKind::CellReference
            || aResult.mpRoot->maChildren[1]->meKind != NodeKind::RangeReference
            || aResult.mpRoot->maChildren[1]->maPrimaryText != u".F5"
            || aResult.mpRoot->maChildren[1]->maSecondaryText != u".M5"
            || aResult.mpRoot->maChildren[2]->meKind != NodeKind::NumberLiteral
            || !almostEqual(aResult.mpRoot->maChildren[2]->mfNumber, 1.0))
        {
            return fail("spreadsheetengine_fods_parser_tests", "MATCH parse mismatch");
        }
    }

    {
        const auto aResult = parseFormula(u"of:=MATCH(\"a\";range1)");
        if (!aResult || aResult.mpRoot->meKind != NodeKind::FunctionCall
            || aResult.mpRoot->maChildren.size() != 2
            || aResult.mpRoot->maChildren[0]->meKind != NodeKind::StringLiteral
            || aResult.mpRoot->maChildren[0]->maPrimaryText != u"a"
            || aResult.mpRoot->maChildren[1]->meKind != NodeKind::NamedReference
            || aResult.mpRoot->maChildren[1]->maPrimaryText != u"range1")
        {
            return fail("spreadsheetengine_fods_parser_tests", "named-range parse mismatch");
        }
    }

    {
        const auto aResult = parseFormula(u"of:=IF([.J2];;2)");
        if (!aResult || aResult.mpRoot->meKind != NodeKind::FunctionCall
            || aResult.mpRoot->maPrimaryText != u"IF" || aResult.mpRoot->maChildren.size() != 3
            || aResult.mpRoot->maChildren[0]->meKind != NodeKind::CellReference
            || aResult.mpRoot->maChildren[1]->meKind != NodeKind::EmptyArgument
            || aResult.mpRoot->maChildren[2]->meKind != NodeKind::NumberLiteral
            || !almostEqual(aResult.mpRoot->maChildren[2]->mfNumber, 2.0))
        {
            return fail("spreadsheetengine_fods_parser_tests", "empty IF argument parse mismatch");
        }
    }

    {
        const auto aResult = parseFormula(u"of:=IF(\"FOO\"=\"FOO\";-[.F17];[.F17])");
        if (!aResult || aResult.mpRoot->meKind != NodeKind::FunctionCall
            || aResult.mpRoot->maChildren.size() != 3
            || aResult.mpRoot->maChildren[0]->meKind != NodeKind::BinaryOperation
            || aResult.mpRoot->maChildren[0]->meBinaryOperator != BinaryOperator::Equal
            || aResult.mpRoot->maChildren[1]->meKind != NodeKind::UnaryOperation
            || aResult.mpRoot->maChildren[1]->meUnaryOperator != UnaryOperator::Minus
            || onlyChild(*aResult.mpRoot->maChildren[1]).meKind != NodeKind::CellReference
            || aResult.mpRoot->maChildren[2]->meKind != NodeKind::CellReference)
        {
            return fail("spreadsheetengine_fods_parser_tests", "nested IF parse mismatch");
        }
    }

    {
        const auto aResult = parseFormula(u"of:=DATEVALUE(\"1954-07-20 16:30:01\")");
        if (!aResult || aResult.mpRoot->meKind != NodeKind::FunctionCall
            || aResult.mpRoot->maPrimaryText != u"DATEVALUE"
            || aResult.mpRoot->maChildren.size() != 1
            || onlyChild(*aResult.mpRoot).meKind != NodeKind::StringLiteral
            || onlyChild(*aResult.mpRoot).maPrimaryText != u"1954-07-20 16:30:01")
        {
            return fail("spreadsheetengine_fods_parser_tests", "DATEVALUE parse mismatch");
        }
    }

    {
        const auto aResult = parseFormula(u"=#N/A");
        if (!aResult || aResult.mpRoot->meKind != NodeKind::ErrorLiteral
            || aResult.mpRoot->maPrimaryText != u"#N/A")
        {
            return fail("spreadsheetengine_fods_parser_tests", "error literal parse mismatch");
        }
    }

    {
        const auto aResult = parseFormula(u"=#DIV/0!");
        if (!aResult || aResult.mpRoot->meKind != NodeKind::ErrorLiteral
            || aResult.mpRoot->maPrimaryText != u"#DIV/0!")
        {
            return fail("spreadsheetengine_fods_parser_tests", "slash error literal parse mismatch");
        }
    }

    {
        const auto aResult = parseFormula(u"TRUE");
        if (!aResult || aResult.mpRoot->meKind != NodeKind::BooleanLiteral
            || !aResult.mpRoot->mbBoolean)
        {
            return fail("spreadsheetengine_fods_parser_tests", "boolean literal parse mismatch");
        }
    }

    {
        const auto aResult = parseFormula(u"of:=ABS(");
        if (aResult)
            return fail("spreadsheetengine_fods_parser_tests", "invalid formula unexpectedly parsed");
    }

    std::cout << "spreadsheetengine FODS formula parser tests passed\n";
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
