# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,spreadsheetengine))

$(eval $(call gb_Library_set_include,spreadsheetengine,\
    $$(INCLUDE) \
    -I$(SRCDIR)/sc/inc \
    -I$(SRCDIR)/spreadsheet_engine/inc \
))

$(eval $(call gb_Library_add_defs,spreadsheetengine,\
    -DSPREADSHEETENGINE_DLLIMPLEMENTATION \
))

$(eval $(call gb_Library_use_sdk_api,spreadsheetengine))

$(eval $(call gb_Library_use_externals,spreadsheetengine,\
    boost_headers \
    icu_headers \
    icui18n \
    icuuc \
))

$(eval $(call gb_Library_use_libraries,spreadsheetengine,\
    comphelper \
    for \
    sal \
    tl \
    utl \
))

$(eval $(call gb_Library_add_exception_objects,spreadsheetengine,\
    spreadsheet_engine/source/compat/formula/FormulaGrammar \
    spreadsheet_engine/source/core/MathBitwise \
    spreadsheet_engine/source/core/CalcConfig \
    spreadsheet_engine/source/core/CompiledFormulaInflation \
    spreadsheet_engine/source/core/CompilerSupport \
    spreadsheet_engine/source/core/ConversionRuntime \
    spreadsheet_engine/source/core/DateTimeParse \
    spreadsheet_engine/source/core/DateTimeParts \
    spreadsheet_engine/source/core/DateTimeWeek \
    spreadsheet_engine/source/core/DateTimeWorkday \
    spreadsheet_engine/source/core/FinancialRuntime \
    spreadsheet_engine/source/core/ForceCalculation \
    spreadsheet_engine/source/core/FormulaEvaluator \
    spreadsheet_engine/source/core/FormulaEvaluatorAggregate \
    spreadsheet_engine/source/core/FormulaEvaluatorConversion \
    spreadsheet_engine/source/core/FormulaEvaluatorDateTime \
    spreadsheet_engine/source/core/FormulaEvaluatorFinancial \
    spreadsheet_engine/source/core/FormulaEvaluatorInformation \
    spreadsheet_engine/source/core/FormulaEvaluatorLogical \
    spreadsheet_engine/source/core/FormulaEvaluatorLookup \
    spreadsheet_engine/source/core/FormulaEvaluatorMath \
    spreadsheet_engine/source/core/FormulaEvaluatorOperators \
    spreadsheet_engine/source/core/FormulaEvaluatorSpreadsheet \
    spreadsheet_engine/source/core/FormulaEvaluatorSpecialForms \
    spreadsheet_engine/source/core/FormulaEvaluatorStatistical \
    spreadsheet_engine/source/core/FormulaEvaluatorText \
    spreadsheet_engine/source/core/LookupRuntime \
    spreadsheet_engine/source/core/OdfFormulaParser \
    spreadsheet_engine/source/core/MathAggregate \
    spreadsheet_engine/source/core/MathFinancial \
    spreadsheet_engine/source/core/MathFunctionRuntime \
    spreadsheet_engine/source/core/MathRounding \
    spreadsheet_engine/source/core/MathScalar \
    spreadsheet_engine/source/core/MathStatistical \
    spreadsheet_engine/source/core/MathTranscendental \
    spreadsheet_engine/source/core/MatrixOperators \
    spreadsheet_engine/source/core/NumeralConversion \
    spreadsheet_engine/source/core/QueryRuntime \
    spreadsheet_engine/source/core/LibraryProbe \
    spreadsheet_engine/source/core/TextCase \
    spreadsheet_engine/source/core/TextFunctionRuntime \
    spreadsheet_engine/source/core/TextRuntimeSupport \
    spreadsheet_engine/source/core/TextScalar \
    spreadsheet_engine/source/core/TextWidth \
))

# vim: set noet sw=4 ts=4:
