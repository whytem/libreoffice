/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "FormulaEvaluatorInternals.hxx"

#include <algorithm>
#include <array>

namespace spreadsheetengine::core::eval
{
namespace
{

template <std::size_t N>
[[nodiscard]] bool matchesFunctionRegistry(
    api::StringView rFunctionName, const std::array<api::StringView, N>& rRegistry)
{
    return std::find(rRegistry.begin(), rRegistry.end(), rFunctionName) != rRegistry.end();
}

} // namespace

std::optional<EvaluationResult> Evaluator::tryEvaluateDateTimeFamily(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    static constexpr std::array kDateTimeFunctions{
        api::StringView(u"BASISODATETIME"),
        api::StringView(u"VALUE"),
        api::StringView(u"DATEVALUE"),
        api::StringView(u"TIMEVALUE"),
        api::StringView(u"TIME"),
        api::StringView(u"DATE"),
        api::StringView(u"DATEDIF"),
        api::StringView(u"DAYSINMONTH"),
        api::StringView(u"DAYSINYEAR"),
        api::StringView(u"ISLEAPYEAR"),
        api::StringView(u"ISOWEEKNUM"),
        api::StringView(u"YEAR"),
        api::StringView(u"MONTH"),
        api::StringView(u"DAY"),
        api::StringView(u"HOUR"),
        api::StringView(u"MINUTE"),
        api::StringView(u"SECOND"),
        api::StringView(u"EDATE"),
        api::StringView(u"EOMONTH"),
        api::StringView(u"WEEKDAY"),
        api::StringView(u"WEEKNUM"),
        api::StringView(u"DAYS360"),
        api::StringView(u"EASTERSUNDAY"),
        api::StringView(u"YEARS"),
        api::StringView(u"WEEKS"),
        api::StringView(u"WEEKSINYEAR"),
        api::StringView(u"WORKDAY.INTL"),
        api::StringView(u"NETWORKDAYS.INTL"),
    };
    if (!matchesFunctionRegistry(rFunctionName, kDateTimeFunctions))
        return std::nullopt;
    return evaluateDateTimeFamilyBody(rFunctionName, rNode, rCurrentAddress);
}

EvaluationResult Evaluator::evaluateDateTimeFamilyBody(
    api::StringView rFunctionName, const formula::Node& rNode,
    const api::CellAddress& rCurrentAddress)
{
    const api::StringView aFunctionName = rFunctionName;
    FunctionEvalContext aContext { *this, rNode, rCurrentAddress };

if (aFunctionName == u"BASISODATETIME")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        double fSerialValue = 0.0;
        if (aArgument.maValue.maValue.isText())
        {
            const auto oParsed = sedatetime::parseStandaloneNumberText(aArgument.maValue.maValue.maString);
            if (!oParsed)
                return makeFailure(api::Error::IllegalArgument);
            fSerialValue = oParsed->mfValue;
        }
        else
        {
            const auto aNumber = coerceToNumber(aArgument.maValue.maValue);
            if (!aNumber)
                return makeFailure(aNumber.meError);
            fSerialValue = aNumber.maValue;
        }

        return makeScalarResult(api::CellValue::text(formatBasisDateTime(fSerialValue)));
    }

    if (aFunctionName == u"VALUE" || aFunctionName == u"DATEVALUE" || aFunctionName == u"TIMEVALUE")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto aText = coerceToString(aArgument.maValue.maValue);
        if (!aText)
            return makeFailure(aText.meError);

        const auto oParsed = sedatetime::parseStandaloneNumberText(aText.maValue);
        if (!oParsed)
            return makeFailure(api::Error::IllegalArgument);

        if (aFunctionName == u"VALUE")
            return makeScalarResult(api::CellValue::number(oParsed->mfValue));

        if (aFunctionName == u"DATEVALUE")
        {
            if (oParsed->meKind != api::NumberParseResult::Kind::Date
                && oParsed->meKind != api::NumberParseResult::Kind::DateTime)
            {
                return makeFailure(api::Error::IllegalArgument);
            }

            return makeScalarResult(api::CellValue::number(
                fp::approxFloor(oParsed->mfValue)));
        }

        if (oParsed->meKind != api::NumberParseResult::Kind::Time
            && oParsed->meKind != api::NumberParseResult::Kind::DateTime)
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        return makeScalarResult(api::CellValue::number(
            sedatetime::normalizeTimeFraction(oParsed->mfValue)));
    }

    if (aFunctionName == u"TIME")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aHour
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aHour)
            return aHour;
        EvaluationResult aMinute
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aMinute)
            return aMinute;
        EvaluationResult aSecond
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aSecond)
            return aSecond;

        const auto aHourNumber = coerceToNumber(aHour.maValue.maValue);
        if (!aHourNumber)
            return makeFailure(aHourNumber.meError);
        const auto aMinuteNumber = coerceToNumber(aMinute.maValue.maValue);
        if (!aMinuteNumber)
            return makeFailure(aMinuteNumber.meError);
        const auto aSecondNumber = coerceToNumber(aSecond.maValue.maValue);
        if (!aSecondNumber)
            return makeFailure(aSecondNumber.meError);

        const auto aTimeSerial = api::calendar::makeTimeSerial(
            aHourNumber.maValue, aMinuteNumber.maValue, aSecondNumber.maValue);
        if (!aTimeSerial)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(aTimeSerial.maValue));
    }

    if (aFunctionName == u"DATE")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aYear
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aYear)
            return aYear;
        EvaluationResult aMonth
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aMonth)
            return aMonth;
        EvaluationResult aDay
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aDay)
            return aDay;

        if (aYear.maValue.maValue.isEmpty() || aMonth.maValue.maValue.isEmpty()
            || aDay.maValue.maValue.isEmpty())
        {
            return makeFailure(api::Error::IllegalArgument);
        }

        const auto aYearNumber = coerceToNumber(aYear.maValue.maValue);
        if (!aYearNumber)
            return makeFailure(aYearNumber.meError);
        const auto aMonthNumber = coerceToNumber(aMonth.maValue.maValue);
        if (!aMonthNumber)
            return makeFailure(aMonthNumber.meError);
        const auto aDayNumber = coerceToNumber(aDay.maValue.maValue);
        if (!aDayNumber)
            return makeFailure(aDayNumber.meError);

        const std::int16_t nYear = static_cast<std::int16_t>(std::trunc(aYearNumber.maValue));
        const std::int16_t nMonth = static_cast<std::int16_t>(std::trunc(aMonthNumber.maValue));
        const std::int16_t nDay = static_cast<std::int16_t>(std::trunc(aDayNumber.maValue));
        if (nYear < 0)
            return makeFailure(api::Error::IllegalArgument);

        const auto aDateSerial
            = api::calendar::makeDateSerial(
                sedatetime::defaultNullDate(), nYear, nMonth, nDay, false);
        if (!aDateSerial)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(aDateSerial.maValue));
    }

    if (aFunctionName == u"DATEDIF")
    {
        if (rNode.maChildren.size() != 3)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aStart
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aStart)
            return aStart;
        EvaluationResult aEnd
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aEnd)
            return aEnd;
        EvaluationResult aInterval
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[2], rCurrentAddress));
        if (!aInterval)
            return aInterval;

        const auto oStartDate = sedatetime::coerceToDateSerial(aStart.maValue.maValue);
        const auto oEndDate = sedatetime::coerceToDateSerial(aEnd.maValue.maValue);
        if (!oStartDate || !oEndDate)
            return makeFailure(api::Error::IllegalArgument);

        const auto aIntervalText = coerceToString(aInterval.maValue.maValue);
        if (!aIntervalText)
            return makeFailure(aIntervalText.meError);

        const auto aDateDif
            = api::calendar::dateDif(sedatetime::defaultNullDate(), *oStartDate, *oEndDate,
                aIntervalText.maValue);
        if (!aDateDif)
            return makeFailure(aDateDif.meError);

        return makeScalarResult(api::CellValue::number(aDateDif.maValue));
    }

    if (aFunctionName == u"DAYSINMONTH" || aFunctionName == u"DAYSINYEAR"
        || aFunctionName == u"ISLEAPYEAR" || aFunctionName == u"ISOWEEKNUM")
    {
        if (rNode.maChildren.size() != 1)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aArgument
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aArgument)
            return aArgument;

        const auto oDateSerial = sedatetime::coerceToDateSerial(aArgument.maValue.maValue);
        if (!oDateSerial)
            return makeFailure(api::Error::IllegalArgument);

        const api::DateParts aNullDate = sedatetime::defaultNullDate();
        const std::int16_t nYear = static_cast<std::int16_t>(
            spreadsheetengine::core::datetime::extractYear(aNullDate, *oDateSerial));
        const std::int16_t nMonth = static_cast<std::int16_t>(
            spreadsheetengine::core::datetime::extractMonth(aNullDate, *oDateSerial));

        if (aFunctionName == u"DAYSINMONTH")
        {
            return makeScalarResult(api::CellValue::number(
                static_cast<double>(spreadsheetengine::core::detail::date::getDaysInMonth(
                    static_cast<std::uint16_t>(nMonth), nYear))));
        }

        const bool bLeapYear = spreadsheetengine::core::detail::date::isLeapYear(nYear);
        if (aFunctionName == u"DAYSINYEAR")
            return makeScalarResult(api::CellValue::number(bLeapYear ? 366.0 : 365.0));

        if (aFunctionName == u"ISOWEEKNUM")
        {
            return makeScalarResult(api::CellValue::number(
                static_cast<double>(spreadsheetengine::api::calendar::isoWeekOfYear(
                    aNullDate, *oDateSerial))));
        }

        return makeScalarResult(api::CellValue::boolean(bLeapYear));
    }

    const auto coerceStandaloneDateTimeNumber = [&](const api::CellValue& rValue)
        -> api::ValueResult<double> {
        switch (rValue.meKind)
        {
            case api::CellValueKind::Empty:
                return api::ValueResult<double>::success(0.0);
            case api::CellValueKind::Number:
            case api::CellValueKind::Boolean:
                return api::ValueResult<double>::success(rValue.mfNumber);
            case api::CellValueKind::Text:
            {
                if (const auto oParsed = sedatetime::parseStandaloneNumberText(rValue.maString))
                    return api::ValueResult<double>::success(oParsed->mfValue);
                if (const auto oStored = sedatetime::parseStoredDateValue(rValue.maString))
                    return api::ValueResult<double>::success(*oStored);
                return api::ValueResult<double>::failure(api::Error::IllegalArgument);
            }
            case api::CellValueKind::Error:
                return api::ValueResult<double>::failure(rValue.meError);
        }

        return api::ValueResult<double>::failure(api::Error::IllegalArgument);
    };

    if (aFunctionName == u"YEAR" || aFunctionName == u"MONTH" || aFunctionName == u"DAY")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aDateValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aDateValue)
            return makeScalarResult(api::CellValue::error(aDateValue.meError));
        const auto aDateNumber = coerceStandaloneDateTimeNumber(aDateValue.maValue);
        if (!aDateNumber)
            return makeScalarResult(api::CellValue::error(aDateNumber.meError));
        const api::DateSerial nDateSerial
            = static_cast<api::DateSerial>(fp::approxFloor(aDateNumber.maValue));

        const api::DateParts aNullDate = sedatetime::defaultNullDate();
        if (aFunctionName == u"YEAR")
        {
            return makeScalarResult(api::CellValue::number(
                spreadsheetengine::core::datetime::extractYear(aNullDate, nDateSerial)));
        }
        if (aFunctionName == u"MONTH")
        {
            return makeScalarResult(api::CellValue::number(
                spreadsheetengine::core::datetime::extractMonth(aNullDate, nDateSerial)));
        }

        const auto oDay = spreadsheetengine::core::datetime::extractDay(aNullDate, nDateSerial);
        if (!oDay)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        return makeScalarResult(api::CellValue::number(*oDay));
    }

    if (aFunctionName == u"HOUR" || aFunctionName == u"MINUTE" || aFunctionName == u"SECOND")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aTimeValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aTimeValue)
            return makeScalarResult(api::CellValue::error(aTimeValue.meError));

        const auto aNumber = coerceStandaloneDateTimeNumber(aTimeValue.maValue);
        if (!aNumber)
            return makeScalarResult(api::CellValue::error(aNumber.meError));

        double fComponent = 0.0;
        if (aFunctionName == u"HOUR")
            fComponent = sedatetime::extractHour(aNumber.maValue);
        else if (aFunctionName == u"MINUTE")
            fComponent = sedatetime::extractMinute(aNumber.maValue);
        else
            fComponent = sedatetime::extractSecond(aNumber.maValue);

        return makeScalarResult(api::CellValue::number(fComponent));
    }

    if (aFunctionName == u"EDATE" || aFunctionName == u"EOMONTH")
    {
        if (rNode.maChildren.size() != 2)
            return makeFailure(api::Error::IllegalArgument);

        EvaluationResult aStart
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[0], rCurrentAddress));
        if (!aStart)
            return aStart;
        EvaluationResult aMonths
            = ensureScalarValue(*this, evaluateNode(*rNode.maChildren[1], rCurrentAddress));
        if (!aMonths)
            return aMonths;

        const auto oDateSerial = sedatetime::coerceToDateSerial(aStart.maValue.maValue);
        if (!oDateSerial)
            return makeFailure(api::Error::IllegalArgument);

        const auto aMonthNumber = coerceToNumber(aMonths.maValue.maValue);
        if (!aMonthNumber || !std::isfinite(aMonthNumber.maValue))
            return makeFailure(api::Error::IllegalArgument);

        const std::int32_t nMonthOffset = static_cast<std::int32_t>(std::trunc(aMonthNumber.maValue));
        const auto oShifted = sedatetime::shiftMonthSerial(
            *oDateSerial, nMonthOffset, aFunctionName == u"EOMONTH");
        if (!oShifted)
            return makeFailure(api::Error::IllegalArgument);

        return makeScalarResult(api::CellValue::number(*oShifted));
    }

    if (aFunctionName == u"WEEKDAY")
    {
        if (rNode.maChildren.size() < 1 || rNode.maChildren.size() > 2)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aDateValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aDateValue)
            return makeScalarResult(api::CellValue::error(aDateValue.meError));
        const auto oDateSerial = sedatetime::coerceToDateSerial(aDateValue.maValue);
        if (!oDateSerial)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        std::int16_t nMode = 1;
        if (rNode.maChildren.size() == 2)
        {
            if (rNode.maChildren[1]->meKind == formula::NodeKind::EmptyArgument)
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

            const auto aModeValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[1]);
            if (!aModeValue)
                return makeScalarResult(api::CellValue::error(aModeValue.meError));
            if (aModeValue.maValue.isEmpty())
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

            const auto aModeNumber = coerceToNumber(aModeValue.maValue);
            if (!aModeNumber)
                return makeScalarResult(api::CellValue::error(aModeNumber.meError));
            const auto oWholeMode = toWholeNumber(aModeNumber.maValue);
            if (!oWholeMode)
                return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
            nMode = static_cast<std::int16_t>(*oWholeMode);
        }

        const auto aWeekday
            = api::calendar::dayOfWeek(sedatetime::defaultNullDate(), *oDateSerial, nMode);
        if (!aWeekday)
            return makeScalarResult(api::CellValue::error(aWeekday.meError));

        return makeScalarResult(api::CellValue::number(static_cast<double>(aWeekday.maValue)));
    }

    if (aFunctionName == u"WEEKNUM")
    {
        if (rNode.maChildren.size() < 1 || rNode.maChildren.size() > 2)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aDateValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aDateValue)
            return makeScalarResult(api::CellValue::error(aDateValue.meError));
        const auto oDateSerial = sedatetime::coerceToDateSerial(aDateValue.maValue);
        if (!oDateSerial)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        std::int16_t nMode = 1;
        if (rNode.maChildren.size() == 2
            && rNode.maChildren[1]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aModeValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[1]);
            if (!aModeValue)
                return makeScalarResult(api::CellValue::error(aModeValue.meError));
            if (!aModeValue.maValue.isEmpty())
            {
                const auto aModeNumber = coerceToNumber(aModeValue.maValue);
                if (!aModeNumber)
                    return makeScalarResult(api::CellValue::error(aModeNumber.meError));
                const auto oWholeMode = toWholeNumber(aModeNumber.maValue);
                if (!oWholeMode)
                    return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
                nMode = static_cast<std::int16_t>(*oWholeMode);
            }
        }

        const auto aWeek
            = api::calendar::weekOfYear(sedatetime::defaultNullDate(), *oDateSerial, nMode);
        if (!aWeek)
            return makeScalarResult(api::CellValue::error(aWeek.meError));

        return makeScalarResult(api::CellValue::number(static_cast<double>(aWeek.maValue)));
    }

    if (aFunctionName == u"DAYS360")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aStartValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aStartValue)
            return makeScalarResult(api::CellValue::error(aStartValue.meError));
        const auto aEndValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[1]);
        if (!aEndValue)
            return makeScalarResult(api::CellValue::error(aEndValue.meError));

        const auto oStartDate = sedatetime::coerceToDateSerial(aStartValue.maValue);
        const auto oEndDate = sedatetime::coerceToDateSerial(aEndValue.maValue);
        if (!oStartDate || !oEndDate)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        bool bEuropeanMethod = false;
        if (rNode.maChildren.size() == 3
            && rNode.maChildren[2]->meKind != formula::NodeKind::EmptyArgument)
        {
            const auto aMethodValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[2]);
            if (!aMethodValue)
                return makeScalarResult(api::CellValue::error(aMethodValue.meError));
            if (!aMethodValue.maValue.isEmpty())
            {
                const auto aMethod = coerceToBoolean(aMethodValue.maValue);
                if (!aMethod)
                    return makeScalarResult(api::CellValue::error(aMethod.meError));
                bEuropeanMethod = aMethod.maValue;
            }
        }

        return makeScalarResult(api::CellValue::number(api::calendar::diffDate360(
            sedatetime::defaultNullDate(), *oStartDate, *oEndDate, bEuropeanMethod)));
    }

    if (aFunctionName == u"EASTERSUNDAY")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aYearValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aYearValue)
            return makeScalarResult(api::CellValue::error(aYearValue.meError));
        if (aYearValue.maValue.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aYearNumber = coerceToNumber(aYearValue.maValue);
        if (!aYearNumber)
            return makeScalarResult(api::CellValue::error(aYearNumber.meError));

        const auto oWholeYear = toWholeNumber(aYearNumber.maValue);
        if (!oWholeYear || *oWholeYear < std::numeric_limits<std::int16_t>::min()
            || *oWholeYear > std::numeric_limits<std::int16_t>::max())
        {
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));
        }

        std::int16_t nYear = static_cast<std::int16_t>(*oWholeYear);
        if (nYear >= 0 && nYear < 100)
        {
            constexpr std::int16_t nTwoDigitYearStart = 1930;
            if (nYear < (nTwoDigitYearStart % 100))
                nYear = static_cast<std::int16_t>(nYear + (((nTwoDigitYearStart / 100) + 1) * 100));
            else
                nYear = static_cast<std::int16_t>(nYear + ((nTwoDigitYearStart / 100) * 100));
        }

        const auto aEaster = api::calendar::easterSundaySerial(sedatetime::defaultNullDate(), nYear);
        if (!aEaster)
            return makeScalarResult(api::CellValue::error(aEaster.meError));

        return makeScalarResult(api::CellValue::number(aEaster.maValue));
    }

    if (aFunctionName == u"YEARS")
    {
        if (rNode.maChildren.size() != 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aStartValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[0]);
        if (!aStartValue)
            return makeScalarResult(api::CellValue::error(aStartValue.meError));
        const auto aEndValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[1]);
        if (!aEndValue)
            return makeScalarResult(api::CellValue::error(aEndValue.meError));
        const auto aModeValue = aContext.evaluateScalarArgumentValue(*rNode.maChildren[2]);
        if (!aModeValue)
            return makeScalarResult(api::CellValue::error(aModeValue.meError));
        if (aModeValue.maValue.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto oStartDate = sedatetime::coerceToDateSerial(aStartValue.maValue);
        const auto oEndDate = sedatetime::coerceToDateSerial(aEndValue.maValue);
        if (!oStartDate || !oEndDate)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aModeNumber = coerceToNumber(aModeValue.maValue);
        if (!aModeNumber)
            return makeScalarResult(api::CellValue::error(aModeNumber.meError));
        const auto oWholeMode = toWholeNumber(aModeNumber.maValue);
        if (!oWholeMode || (*oWholeMode != 0 && *oWholeMode != 1))
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const std::int32_t nNullDate = sedate::toAbsoluteDays(sedatetime::defaultNullDate());
        const api::DateParts aStartDate = sedate::fromAbsoluteDays(nNullDate + *oStartDate);
        const api::DateParts aEndDate = sedate::fromAbsoluteDays(nNullDate + *oEndDate);

        std::int32_t nYears = static_cast<std::int32_t>(aEndDate.mnYear) - aStartDate.mnYear;
        if (*oWholeMode == 0)
        {
            std::int32_t nMonths = static_cast<std::int32_t>(aEndDate.mnMonth) - aStartDate.mnMonth
                                + nYears * 12;
            if (*oStartDate < *oEndDate)
            {
                if (aStartDate.mnDay > aEndDate.mnDay)
                    --nMonths;
            }
            else if (*oStartDate > *oEndDate)
            {
                if (aStartDate.mnDay < aEndDate.mnDay)
                    ++nMonths;
            }
            nYears = nMonths / 12;
        }

        return makeScalarResult(api::CellValue::number(static_cast<double>(nYears)));
    }

    if (aFunctionName == u"WEEKS")
    {
        if (rNode.maChildren.size() != 3)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto evaluateWeeksArgument = [&](const formula::Node& rArgument) -> EvaluationResult {
            EvaluationResult aArgument = evaluateNode(rArgument, rCurrentAddress);
            if (!aArgument)
                return makeScalarResult(api::CellValue::error(aArgument.meError));
            if (aArgument.maValue.isScalar())
                return aArgument;
            if (!aArgument.maValue.maReference.isSingleCell())
                return makeScalarResult(api::CellValue::error(api::Error::NoValue));

            aArgument = materializeReferenceValue(aArgument.maValue.maReference, 0, 0);
            if (!aArgument)
                return makeScalarResult(api::CellValue::error(aArgument.meError));
            return aArgument;
        };

        EvaluationResult aStart = evaluateWeeksArgument(*rNode.maChildren[0]);
        if (!aStart)
            return aStart;
        EvaluationResult aEnd = evaluateWeeksArgument(*rNode.maChildren[1]);
        if (!aEnd)
            return aEnd;
        EvaluationResult aMode = evaluateWeeksArgument(*rNode.maChildren[2]);
        if (!aMode)
            return aMode;

        const auto oStartDate = sedatetime::coerceToDateSerial(aStart.maValue.maValue);
        const auto oEndDate = sedatetime::coerceToDateSerial(aEnd.maValue.maValue);
        if (!oStartDate || !oEndDate || aMode.maValue.maValue.isEmpty())
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto aModeNumber = coerceToNumber(aMode.maValue.maValue);
        if (!aModeNumber)
            return makeScalarResult(api::CellValue::error(aModeNumber.meError));

        const auto oWholeMode = toWholeNumber(aModeNumber.maValue);
        if (!oWholeMode)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const auto oWeeks = sedatetime::computeWeeksDifference(
            *oStartDate, *oEndDate, static_cast<std::int16_t>(*oWholeMode));
        if (!oWeeks)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        return makeScalarResult(api::CellValue::number(*oWeeks));
    }

    if (aFunctionName == u"WEEKSINYEAR")
    {
        if (rNode.maChildren.size() != 1)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        EvaluationResult aDateArgument = evaluateNode(*rNode.maChildren[0], rCurrentAddress);
        if (!aDateArgument)
            return makeScalarResult(api::CellValue::error(aDateArgument.meError));
        if (!aDateArgument.maValue.isScalar())
        {
            if (!aDateArgument.maValue.maReference.isSingleCell())
                return makeScalarResult(api::CellValue::error(api::Error::NoValue));

            aDateArgument = materializeReferenceValue(aDateArgument.maValue.maReference, 0, 0);
            if (!aDateArgument)
                return makeScalarResult(api::CellValue::error(aDateArgument.meError));
        }

        const auto oDateSerial = sedatetime::coerceToDateSerial(aDateArgument.maValue.maValue);
        if (!oDateSerial)
            return makeScalarResult(api::CellValue::error(api::Error::IllegalArgument));

        const api::DateParts aDate = sedate::fromAbsoluteDays(
            sedate::toAbsoluteDays(sedatetime::defaultNullDate()) + *oDateSerial);
        const std::int32_t nJan1WeekDay
            = (sedate::toAbsoluteDays({ aDate.mnYear, 1, 1 }) - 1) % 7;
        const double fWeeksInYear = nJan1WeekDay == 3
                                        ? 53.0
                                        : (nJan1WeekDay == 2 && sedate::isLeapYear(aDate.mnYear)
                                               ? 53.0
                                               : 52.0);
        return makeScalarResult(api::CellValue::number(fWeeksInYear));
    }

    if (aFunctionName == u"WORKDAY.INTL")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aStartNumber = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aDaysNumber = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aStartNumber)
            return makeFailure(aStartNumber.meError);
        if (!aDaysNumber)
            return makeFailure(aDaysNumber.meError);

        const auto oStartDate = toWholeNumber(std::trunc(aStartNumber.maValue));
        const auto oDays = toWholeNumber(std::trunc(aDaysNumber.maValue));
        if (!oStartDate || !oDays)
            return makeFailure(api::Error::IllegalArgument);

        const auto aWeekendMask = aContext.evaluateWeekendMaskArgument(
            rNode.maChildren.size() >= 3 ? rNode.maChildren[2].get() : nullptr, true);
        if (!aWeekendMask)
            return makeFailure(aWeekendMask.meError);

        const auto aHolidays = aContext.collectHolidaySerials(
            rNode.maChildren.size() >= 4 ? rNode.maChildren[3].get() : nullptr);
        if (!aHolidays)
            return makeFailure(aHolidays.meError);

        return makeScalarResult(api::CellValue::number(advanceWorkdayFods(
            static_cast<api::DateSerial>(*oStartDate), static_cast<api::DateSerial>(*oDays),
            aHolidays.maValue, aWeekendMask.maValue)));
    }

    if (aFunctionName == u"NETWORKDAYS.INTL")
    {
        if (rNode.maChildren.size() < 2 || rNode.maChildren.size() > 4)
            return makeFailure(api::Error::IllegalArgument);

        const auto aStartNumber = aContext.evaluateNumericArgument(*rNode.maChildren[0], std::nullopt);
        const auto aEndNumber = aContext.evaluateNumericArgument(*rNode.maChildren[1], std::nullopt);
        if (!aStartNumber)
            return makeFailure(aStartNumber.meError);
        if (!aEndNumber)
            return makeFailure(aEndNumber.meError);

        const auto oStartDate = toWholeNumber(std::trunc(aStartNumber.maValue));
        const auto oEndDate = toWholeNumber(std::trunc(aEndNumber.maValue));
        if (!oStartDate || !oEndDate)
            return makeFailure(api::Error::IllegalArgument);

        const auto aWeekendMask = aContext.evaluateWeekendMaskArgument(
            rNode.maChildren.size() >= 3 ? rNode.maChildren[2].get() : nullptr, false);
        if (!aWeekendMask)
            return makeFailure(aWeekendMask.meError);

        const auto aHolidays = aContext.collectHolidaySerials(
            rNode.maChildren.size() >= 4 ? rNode.maChildren[3].get() : nullptr);
        if (!aHolidays)
            return makeFailure(aHolidays.meError);

        return makeScalarResult(api::CellValue::number(countWorkdaysFods(
            static_cast<api::DateSerial>(*oStartDate), static_cast<api::DateSerial>(*oEndDate),
            aHolidays.maValue, aWeekendMask.maValue)));
    }

        return makeFailure(api::Error::IllegalArgument);
}

} // namespace spreadsheetengine::core::eval
