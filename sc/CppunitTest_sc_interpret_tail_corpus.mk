# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,sc_interpret_tail_corpus))

$(eval $(call gb_CppunitTest_use_common_precompiled_header,sc_interpret_tail_corpus))

$(eval $(call gb_CppunitTest_add_defs,sc_interpret_tail_corpus,\
    -DSPREADSHEETENGINE_TEST_ROOT=\"$(SRCDIR)/spreadsheet_engine\" \
))

$(eval $(call gb_CppunitTest_add_exception_objects,sc_interpret_tail_corpus, \
    spreadsheet_engine/source/core/FodsLoader \
    sc/qa/unit/interpret_tail_corpus \
))

$(eval $(call gb_CppunitTest_use_library_objects,sc_interpret_tail_corpus, \
    sc \
    scqahelper \
))

$(eval $(call gb_CppunitTest_use_externals,sc_interpret_tail_corpus, \
	boost_headers \
    $(call gb_Helper_optional,OPENCL, \
        clew) \
    icu_headers \
    icui18n \
    icuuc \
    libxml2 \
    mdds_headers \
    orcus \
    orcus-parser \
    md4c \
))

$(eval $(call gb_CppunitTest_use_libraries,sc_interpret_tail_corpus, \
    $(call gb_Helper_optional,AVMEDIA,avmedia) \
    basegfx \
    chart2api \
    comphelper \
    cppu \
    cppuhelper \
    dbtools \
    drawinglayer \
    drawinglayercore \
    docmodel \
    editeng \
    for \
    forui \
    fwk \
    i18nlangtag \
    i18nutil \
    $(call gb_Helper_optional,OPENCL, \
        opencl) \
    sal \
    salhelper \
    sax \
    sfx \
    sb \
    sot \
    spreadsheetengine \
    subsequenttest \
    svl \
    svt \
    svx \
    svxcore \
	test \
    textconversiondlgs \
    tk \
    tl \
    ucbhelper \
	unotest \
    utl \
    $(call gb_Helper_optional,SCRIPTING, \
        vbahelper) \
    vcl \
    xo \
))

$(eval $(call gb_CppunitTest_set_include,sc_interpret_tail_corpus,\
    -I$(SRCDIR)/spreadsheet_engine/inc \
    -I$(SRCDIR)/sc/source/ui/inc \
    -I$(SRCDIR)/sc/source/core/inc \
    -I$(SRCDIR)/sc/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_CppunitTest_use_api,sc_interpret_tail_corpus,\
    offapi \
    udkapi \
    oovbaapi \
))

$(eval $(call gb_CppunitTest_use_ure,sc_interpret_tail_corpus))

$(eval $(call gb_CppunitTest_use_vcl,sc_interpret_tail_corpus))

$(eval $(call gb_CppunitTest_use_custom_headers,sc_interpret_tail_corpus,\
	officecfg/registry \
))

$(eval $(call gb_CppunitTest_use_components,sc_interpret_tail_corpus,\
    configmgr/source/configmgr \
    framework/util/fwk \
    i18npool/source/search/i18nsearch \
    i18npool/util/i18npool \
    sax/source/expatwrap/expwrap \
    scaddins/source/analysis/analysis \
    scaddins/source/datefunc/date \
    scaddins/source/pricing/pricing \
    sfx2/util/sfx \
    ucb/source/core/ucb1 \
    ucb/source/ucp/file/ucpfile1 \
    unoxml/source/service/unoxml \
    uui/util/uui \
    vcl/vcl.common \
))

$(eval $(call gb_CppunitTest_use_configuration,sc_interpret_tail_corpus))

$(eval $(call gb_CppunitTest_add_arguments,sc_interpret_tail_corpus, \
    -env:arg-env=$(gb_Helper_LIBRARY_PATH_VAR)"$$$${$(gb_Helper_LIBRARY_PATH_VAR)+=$$$$$(gb_Helper_LIBRARY_PATH_VAR)}" \
))

# vim: set noet sw=4 ts=4:
