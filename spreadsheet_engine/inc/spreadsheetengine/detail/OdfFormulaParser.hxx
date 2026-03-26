/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <vector>

#include <sal/types.h>

#include <spreadsheetengine/api/String.hxx>

namespace spreadsheetengine::core::formula
{

enum class NodeKind : sal_uInt8
{
    NumberLiteral,
    StringLiteral,
    BooleanLiteral,
    ErrorLiteral,
    EmptyArgument,
    CellReference,
    RangeReference,
    NamedReference,
    UnaryOperation,
    BinaryOperation,
    FunctionCall
};

enum class UnaryOperator : sal_uInt8
{
    Plus,
    Minus
};

enum class BinaryOperator : sal_uInt8
{
    Add,
    Subtract,
    Multiply,
    Divide,
    Power,
    Concat,
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual
};

struct Node
{
    NodeKind meKind = NodeKind::NumberLiteral;
    api::String maPrimaryText;
    api::String maSecondaryText;
    double mfNumber = 0.0;
    bool mbBoolean = false;
    UnaryOperator meUnaryOperator = UnaryOperator::Plus;
    BinaryOperator meBinaryOperator = BinaryOperator::Add;
    std::vector<std::unique_ptr<Node>> maChildren;
};

struct ParseError
{
    api::String maMessage;
    std::size_t mnOffset = 0;
};

struct ParseResult
{
    std::unique_ptr<Node> mpRoot;
    ParseError maError;
    bool mbOk = false;

    constexpr explicit operator bool() const { return mbOk; }
};

[[nodiscard]] ParseResult parseFormula(api::StringView rFormula);

} // namespace spreadsheetengine::core::formula

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
