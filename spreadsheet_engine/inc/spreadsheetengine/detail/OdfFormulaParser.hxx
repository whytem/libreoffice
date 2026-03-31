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

#include <spreadsheetengine/api/Types.hxx>

#include <spreadsheetengine/api/String.hxx>
#include <spreadsheetengine/spreadsheetenginedllapi.h>

namespace spreadsheetengine::core::formula
{

enum class NodeKind : std::uint8_t
{
    NumberLiteral,
    StringLiteral,
    BooleanLiteral,
    ErrorLiteral,
    EmptyArgument,
    CellReference,
    RangeReference,
    NamedReference,
    RangeConstructor,
    ReferenceList,
    ArrayConstant,
    UnaryOperation,
    BinaryOperation,
    FunctionCall
};

enum class UnaryOperator : std::uint8_t
{
    Plus,
    Minus
};

enum class BinaryOperator : std::uint8_t
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
    sal_Int32 mnArrayRows = 0;
    sal_Int32 mnArrayColumns = 0;
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

[[nodiscard]] SPREADSHEETENGINE_DLLPUBLIC ParseResult parseFormula(api::StringView rFormula);

} // namespace spreadsheetengine::core::formula

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
