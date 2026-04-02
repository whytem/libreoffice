/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <formula/paramclass.hxx>
#include <formula/token.hxx>

#include <spreadsheetengine/api/MatrixFrame.hxx>

namespace spreadsheetengine::compat::libreoffice::matrixframeexecution
{

[[nodiscard]] inline spreadsheetengine::api::matrixframe::StackKind toApiStackKind(
    formula::StackVar eStackType)
{
    switch (eStackType)
    {
        case formula::svUnknown:
            return spreadsheetengine::api::matrixframe::StackKind::Unknown;
        case formula::svDoubleRef:
            return spreadsheetengine::api::matrixframe::StackKind::DoubleRef;
        case formula::svMatrix:
            return spreadsheetengine::api::matrixframe::StackKind::Matrix;
        case formula::svJumpMatrix:
            return spreadsheetengine::api::matrixframe::StackKind::JumpMatrix;
        default:
            return spreadsheetengine::api::matrixframe::StackKind::Other;
    }
}

[[nodiscard]] inline spreadsheetengine::api::matrixframe::ParamKind toApiParamKind(
    formula::ParamClass eType)
{
    switch (eType)
    {
        case formula::ParamClass::Value:
            return spreadsheetengine::api::matrixframe::ParamKind::Value;
        case formula::ParamClass::Array:
            return spreadsheetengine::api::matrixframe::ParamKind::Array;
        case formula::ParamClass::Reference:
            return spreadsheetengine::api::matrixframe::ParamKind::Reference;
        case formula::ParamClass::ReferenceOrRefArray:
            return spreadsheetengine::api::matrixframe::ParamKind::ReferenceOrRefArray;
        case formula::ParamClass::ReferenceOrForceArray:
            return spreadsheetengine::api::matrixframe::ParamKind::ReferenceOrForceArray;
        case formula::ParamClass::ForceArray:
            return spreadsheetengine::api::matrixframe::ParamKind::ForceArray;
        default:
            return spreadsheetengine::api::matrixframe::ParamKind::Other;
    }
}

[[nodiscard]] inline bool shouldConvertJumpConditionToMatrix(
    formula::StackVar eStackType, formula::StackVar ePreviousStackType)
{
    return spreadsheetengine::api::matrixframe::shouldConvertJumpConditionToMatrix(
        toApiStackKind(eStackType), toApiStackKind(ePreviousStackType));
}

[[nodiscard]] inline bool shouldTrackValueParameterDimensions(formula::ParamClass eType)
{
    return spreadsheetengine::api::matrixframe::shouldTrackValueParameterDimensions(
        toApiParamKind(eType));
}

[[nodiscard]] inline bool shouldConvertDoubleRefParameter(
    formula::ParamClass eType, bool bInArrayContext)
{
    return spreadsheetengine::api::matrixframe::shouldConvertDoubleRefParameter(
        toApiParamKind(eType), bInArrayContext);
}

[[nodiscard]] inline bool shouldConvertExternalDoubleRefParameter(formula::ParamClass eType)
{
    return spreadsheetengine::api::matrixframe::shouldConvertExternalDoubleRefParameter(
        toApiParamKind(eType));
}

[[nodiscard]] inline bool allowsReferenceListParameter(formula::ParamClass eType)
{
    return spreadsheetengine::api::matrixframe::allowsReferenceListParameter(toApiParamKind(eType));
}

} // namespace spreadsheetengine::compat::libreoffice::matrixframeexecution

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
