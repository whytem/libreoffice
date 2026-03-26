/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <string_view>

#include <spreadsheetengine/api/Error.hxx>
#include <spreadsheetengine/detail/WorkbookModel.hxx>

namespace spreadsheetengine::core::fods
{

struct IgnoredFeatureSummary
{
    std::size_t mnChartElementCount = 0;
    std::size_t mnDrawElementCount = 0;
    std::size_t mnContentValidationCount = 0;
    std::size_t mnAnnotationCount = 0;
};

struct LoadResult
{
    workbook::Workbook maWorkbook;
    IgnoredFeatureSummary maIgnoredFeatures;
};

[[nodiscard]] api::ValueResult<LoadResult> loadWorkbook(std::string_view rPath);

} // namespace spreadsheetengine::core::fods

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
