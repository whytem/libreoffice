/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

namespace spreadsheetengine::api::matrixframe
{

enum class StackKind : unsigned char
{
    Other,
    Unknown,
    DoubleRef,
    Matrix,
    JumpMatrix
};

enum class ParamKind : unsigned char
{
    Other,
    Value,
    Array,
    Reference,
    ReferenceOrRefArray,
    ReferenceOrForceArray,
    ForceArray
};

[[nodiscard]] inline bool shouldConvertJumpConditionToMatrix(
    StackKind eStackType, StackKind ePreviousStackType)
{
    if (eStackType == StackKind::Unknown || eStackType == StackKind::Matrix)
        return false;

    return eStackType == StackKind::DoubleRef || ePreviousStackType == StackKind::JumpMatrix;
}

[[nodiscard]] inline bool shouldTrackValueParameterDimensions(ParamKind eType)
{
    return eType == ParamKind::Value;
}

[[nodiscard]] inline bool shouldConvertDoubleRefParameter(ParamKind eType, bool bInArrayContext)
{
    return eType != ParamKind::Reference && eType != ParamKind::ReferenceOrRefArray
           && eType != ParamKind::ReferenceOrForceArray
           && (eType != ParamKind::Value || bInArrayContext);
}

[[nodiscard]] inline bool shouldConvertExternalDoubleRefParameter(ParamKind eType)
{
    return eType == ParamKind::Value || eType == ParamKind::Array;
}

[[nodiscard]] inline bool allowsReferenceListParameter(ParamKind eType)
{
    return eType == ParamKind::Reference || eType == ParamKind::ReferenceOrRefArray
           || eType == ParamKind::ReferenceOrForceArray || eType == ParamKind::ForceArray;
}

} // namespace spreadsheetengine::api::matrixframe

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
