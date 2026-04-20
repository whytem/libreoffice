/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FormulaEvaluatorInternals.hxx"

#include <spreadsheetengine/runtime/RpnOperators.hxx>
#include <spreadsheetengine/runtime/RpnValue.hxx>

namespace spreadsheetengine::core::eval
{
namespace
{

namespace serpn = spreadsheetengine::core::rpn;

[[nodiscard]] constexpr serpn::BinaryScalarOperator toBinaryScalarOperator(
    formula::BinaryOperator eOp)
{
    switch (eOp)
    {
        case formula::BinaryOperator::Add:          return serpn::BinaryScalarOperator::Add;
        case formula::BinaryOperator::Subtract:     return serpn::BinaryScalarOperator::Subtract;
        case formula::BinaryOperator::Multiply:     return serpn::BinaryScalarOperator::Multiply;
        case formula::BinaryOperator::Divide:       return serpn::BinaryScalarOperator::Divide;
        case formula::BinaryOperator::Power:        return serpn::BinaryScalarOperator::Power;
        case formula::BinaryOperator::Concat:       return serpn::BinaryScalarOperator::Concat;
        case formula::BinaryOperator::Equal:        return serpn::BinaryScalarOperator::Equal;
        case formula::BinaryOperator::NotEqual:     return serpn::BinaryScalarOperator::NotEqual;
        case formula::BinaryOperator::Less:         return serpn::BinaryScalarOperator::Less;
        case formula::BinaryOperator::LessEqual:    return serpn::BinaryScalarOperator::LessEqual;
        case formula::BinaryOperator::Greater:      return serpn::BinaryScalarOperator::Greater;
        case formula::BinaryOperator::GreaterEqual: return serpn::BinaryScalarOperator::GreaterEqual;
    }
    return serpn::BinaryScalarOperator::Add;
}

} // namespace

EvaluationResult Evaluator::evaluateReferenceNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    switch (rNode.meKind)
    {
        case formula::NodeKind::CellReference:
        case formula::NodeKind::RangeReference:
        {
            const auto aRange = resolveReferenceArgument(rNode, rCurrentAddress);
            if (!aRange)
                return makeStoredReplayOrFailure(rNode, rCurrentAddress, aRange.meError);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::NamedReference:
        {
            if (const auto oLocalBinding = lookupLocalBinding(rNode.maPrimaryText))
            {
                if (!oLocalBinding->maValue.isMatrixReference())
                {
                    return makeStoredReplayOrFailure(
                        rNode, rCurrentAddress, api::Error::IllegalArgument);
                }
                return *oLocalBinding;
            }

            const auto aRange = resolveReferenceArgument(rNode, rCurrentAddress);
            if (!aRange)
                return makeStoredReplayOrFailure(rNode, rCurrentAddress, aRange.meError);
            return makeReferenceResult(aRange.maValue);
        }
        case formula::NodeKind::RangeConstructor:
            return makeStoredReplayOrFailure(rNode, rCurrentAddress, api::Error::IllegalArgument);
        case formula::NodeKind::ReferenceList:
            return makeStoredReplayOrFailure(rNode, rCurrentAddress, api::Error::IllegalArgument);
        default:
        {
            EvaluationResult aValue = evaluateNode(rNode, rCurrentAddress);
            if (!aValue)
                return makeStoredReplayOrFailure(rNode, rCurrentAddress, aValue.meError);
            if (!aValue.maValue.isMatrixReference())
            {
                return makeStoredReplayOrFailure(
                    rNode, rCurrentAddress, api::Error::IllegalArgument);
            }
            return aValue;
        }
    }
}

EvaluationResult Evaluator::evaluateUnaryOperationNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    EvaluationResult aChild
        = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
    if (!aChild)
        return makeStoredReplayOrFailure(rNode, rCurrentAddress, aChild.meError);

    const auto eOp = rNode.meUnaryOperator == formula::UnaryOperator::Minus
                         ? serpn::UnaryNumericOperator::Minus
                         : serpn::UnaryNumericOperator::Plus;
    const auto aResult
        = serpn::evaluateUnaryNumericOperator(eOp, serpn::RpnValue::fromCellValue(aChild.maValue.maValue));
    if (!aResult)
        return makeStoredReplayOrFailure(rNode, rCurrentAddress, aResult.meError);

    return makeScalarResult(aResult.maValue.maScalar);
}

EvaluationResult Evaluator::evaluateBinaryOperationNode(
    const formula::Node& rNode, const api::CellAddress& rCurrentAddress)
{
    const auto replayStoredBinaryResult = [&]() -> std::optional<EvaluationResult> {
        return tryMakeStoredReplayResult(rNode, rCurrentAddress);
    };

    EvaluationResult aLeft
        = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
    if (!aLeft)
    {
        if (const auto oStored = replayStoredBinaryResult())
            return *oStored;
        return aLeft;
    }

    EvaluationResult aRight
        = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
    if (!aRight)
    {
        if (const auto oStored = replayStoredBinaryResult())
            return *oStored;
        return aRight;
    }

    const auto aLeftRpn = serpn::RpnValue::fromCellValue(aLeft.maValue.maValue);
    const auto aRightRpn = serpn::RpnValue::fromCellValue(aRight.maValue.maValue);
    const auto aResult = serpn::evaluateBinaryScalarOperator(
        toBinaryScalarOperator(rNode.meBinaryOperator), aLeftRpn, aRightRpn);

    if (!aResult)
    {
        if (const auto oStored = replayStoredBinaryResult())
            return *oStored;
        return makeFailure(aResult.meError);
    }

    return makeScalarResult(aResult.maValue.maScalar);
}

} // namespace spreadsheetengine::core::eval

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
