/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <spreadsheetengine/api/Math.hxx>
#include <spreadsheetengine/api/Numeral.hxx>
#include <spreadsheetengine/api/Text.hxx>

int main()
{
    if (spreadsheetengine::api::math::abs(-5.0) != 5.0)
        return 1;

    if (spreadsheetengine::api::text::countCodePoints(u"Libre") != 5)
        return 1;

    const auto aRoman = spreadsheetengine::api::numeral::fromRoman(u"XIV");
    if (!aRoman || aRoman.maValue != 14)
        return 1;

    return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
