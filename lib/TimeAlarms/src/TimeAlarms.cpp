/*
  TimeAlarms.cpp - Arduino Time alarms for use with Time library
  Copyright (c) 2024-present Dan Luca
  Prior Work Copyright (c) 2008-2011 Michael Margolis.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
 */

#include "TimeAlarms.h"

const AlarmMode_t modeTriggerOnce;
const AlarmMode_t modeRepeatWeekDays {.isEnabled = true, .dayRepeat = DAY_FLAG_MONDAY + DAY_FLAG_TUESDAY + DAY_FLAG_WEDNESDAY + DAY_FLAG_THURSDAY + DAY_FLAG_FRIDAY, .alarmRepeat = DT_REPEAT_WEEKLY};
const AlarmMode_t modeRepeatWeekend {.isEnabled = true, .dayRepeat = DAY_FLAG_SATURDAY + DAY_FLAG_SUNDAY, .alarmRepeat = DT_REPEAT_WEEKLY};
const AlarmMode_t modeRepeatMonday {.isEnabled = true, .dayRepeat = DAY_FLAG_MONDAY, .alarmRepeat = DT_REPEAT_WEEKLY};
const AlarmMode_t modeRepeatTuesday {.isEnabled = true, .dayRepeat = DAY_FLAG_TUESDAY, .alarmRepeat = DT_REPEAT_WEEKLY};
const AlarmMode_t modeRepeatWednesday {.isEnabled = true, .dayRepeat = DAY_FLAG_WEDNESDAY, .alarmRepeat = DT_REPEAT_WEEKLY};
const AlarmMode_t modeRepeatThursday {.isEnabled = true, .dayRepeat = DAY_FLAG_THURSDAY, .alarmRepeat = DT_REPEAT_WEEKLY};
const AlarmMode_t modeRepeatFriday {.isEnabled = true, .dayRepeat = DAY_FLAG_FRIDAY, .alarmRepeat = DT_REPEAT_WEEKLY};
const AlarmMode_t modeRepeatYearly {.isEnabled = true, .alarmRepeat = DT_REPEAT_YEARLY};
const AlarmMode_t modeTplRepeatMonthly {.isEnabled = true, .alarmRepeat = DT_REPEAT_MONTHLY};
const AlarmMode_t modeTplRepeatWeekdayOfMonth {.isEnabled = true, .alarmRepeat = DT_REPEAT_WEEKDAY_OF_MONTH};

/**
 * Advances the given time to the beginning of the first day of the next month that matches the days mask provided
 * @param refTime reference time for current month, day
 * @param D the day of next month to advance to, between 1 - 31
 * @param daysMask the days mask to match
 * @return beginning of next month on first weekday that matches the mask, midnight
 */
time_t advanceNextMonth(const time_t refTime, const uint8_t D, const uint8_t daysMask = 0) {
    time_t startDay = previousMidnight(refTime);
    int8_t curMonth = month(refTime);  //values from 1 to 12, January is 1
    int8_t curDay = day(refTime);      //values from 1 to 31
    time_t nextTime = startDay;
    //jumpstart the next time date towards the end of this month
    if (curDay < 28)
        nextTime += SECS_PER_DAY * (28-curDay);
    while (month(nextTime) == curMonth)
        nextTime += SECS_PER_DAY;
    nextTime += (D - 1) * SECS_PER_DAY;
    if (daysMask > 0) {
        auto mask = (int8_t)(DAY_FLAG_SUNDAY << (weekday(nextTime) - 1));
        while ((daysMask & mask) == 0) {
            nextTime += SECS_PER_DAY;
            mask = (int8_t)(DAY_FLAG_SUNDAY << (weekday(nextTime) - 1));
        }
    }
    return nextTime;
}

/**
 * Advances the given time to next year on given month and day, midnight
 * <p>If a day mask is provided, it advances to the first day that matches the days mask provided - potentially changing the day & month intended</p>
 * @param refTime reference time for current year, month, day
 * @param M the month to advance to
 * @param D the day of the month to advance to
 * @param daysMask optional day mask to advance the time until this mask matches
 * @return time next year on month and day specified, midnight. If a dayMask is specified time is advanced as needed for a first match
 */
time_t advanceNextYear(const time_t refTime, const uint8_t M, const uint8_t D, const uint8_t daysMask = 0) {
    time_t startMonth = previousMidnight(refTime) - day(refTime) * SECS_PER_DAY;
    time_t nextTime = startMonth;
    int curMonth = month(refTime); //values from 1 to 12, January is 1
    int curYear = year(refTime);
    if (curMonth < 12)
        nextTime += 28 * (12-curMonth) * SECS_PER_DAY;  //jumpstart the next time to December, approximately
    while (year(nextTime) == curYear)
        nextTime += SECS_PER_DAY;
    //advance close to the month specified
    nextTime += (M - 1) * 28 * SECS_PER_DAY;
    while (month(nextTime) != M)
        nextTime += SECS_PER_DAY;
    nextTime += (D - 1) * SECS_PER_DAY;   //advance to the day specified
    //advance to the next day that matches the dayMask
    if (daysMask > 0) {
        int dow = weekday(nextTime) - 1;
        auto mask = (int8_t)(DAY_FLAG_SUNDAY << dow);
        while ((daysMask & mask) == 0) {
            nextTime += SECS_PER_DAY;
            dow = weekday(nextTime) - 1;
            mask = (int8_t)(DAY_FLAG_SUNDAY << dow);
        }
    }
    return nextTime;
}

/**
 * Advances the given time to the nth day of the week of a month in the next year
 * <p>E.g. Advance to 3rd Thursday of November, next year</p>
 * @param refTime reference time for current year, month
 * @param M the month to advance to (next year)
 * @param nthOccurrence which occurrence of the dayOfWeek to advance to
 * @param dayOfWeek the day of the week
 * @return time in next year, corresponding to nth day of week in the given month, midnight
 */
time_t advanceNextYearNthWeekdayOfMonth(const time_t refTime, const uint8_t M, const uint8_t nthOccurrence, const timeDayOfWeek_t dayOfWeek) {
    time_t startDay = previousMidnight(refTime);
    time_t nextTime = startDay;
    int curMonth = month(refTime); //values from 1 to 12, January is 1
    int curYear = year(refTime);

    nextTime += 28 * (12 + M - curMonth - 1) * SECS_PER_DAY;  //jumpstart close to needed month next year
    //ensure we are next year
    while (year(nextTime) == curYear)
        nextTime += SECS_PER_DAY;
    //ensure we are in the month desired
    while (month(nextTime) != M)
        nextTime += SECS_PER_DAY;
    auto dow = static_cast<timeDayOfWeek_t>(weekday(nextTime));
    uint8_t dowCount = dow == dayOfWeek ? 1 : 0;
    // keep adding a day and check if it's the desired weekday until we have found the nth occurrence
    while(dowCount < nthOccurrence){
        nextTime += SECS_PER_DAY;
        dow = static_cast<timeDayOfWeek_t>(weekday(nextTime));
        if(dow == dayOfWeek) {
            dowCount++;
        }
    }
    return nextTime;
}

/**
 * Creates a time_t value from individual time elements
 * @param yr the year - full 4 digits or 2 digits since 2000
 * @param mnt the month, January is 1
 * @param dy the day of the month, first day is 1
 * @param hr the hour
 * @param min the minute
 * @param sec the second
 * @return time_t value corresponding to the elements provided
 * @see setTime(int hr,int min,int sec,int dy, int mnth, int yr)
 */
time_t buildTime(const uint16_t yr, const uint8_t mnt, const uint8_t dy, const uint8_t hr, const uint8_t min, const uint8_t sec) {
    uint8_t tmYear = yr > 99 ? yr - 1970 : yr + 30;
    tmElements_t tmTime {.Year = tmYear, .Month = mnt, .Day = dy, .Hour = hr, .Minute = min, .Second = sec};
    return makeTime(tmTime);
}

//**************************************************************
// AlarmClass

AlarmClass::AlarmClass(AlarmHandlerPtr handlerOn, AlarmHandlerPtr handlerOff, time_t value, time_t duration, const AlarmMode_t &mode): onEventHandler(handlerOn), offEventHandler(handlerOff),
                                                                                                                                       state(Idle), Mode(mode), nextTrigger(0), priority(0), value(value), duration(duration > 0 ? duration : 0) {
    //check data integrity - alarm repeat setting takes precedence
    Mode.isEnabled = Mode.isEnabled && onEventHandler != nullptr;
    switch (Mode.alarmRepeat) {
        case DT_REPEAT_NONE:
        case DT_REPEAT_INTERVAL:
        case DT_REPEAT_YEARLY:
            Mode.dayRepeat = 0x00;
            break;
        case DT_REPEAT_DAILY:
            Mode.dayRepeat = 0x7F;
            break;
        default:
            //if no day repeat has been set, default to working week
            if (Mode.dayRepeat == 0)
                Mode.dayRepeat = modeRepeatWeekDays.dayRepeat;
            break;
    }
    //update next trigger time
    updateNextTrigger();
}

void AlarmClass::updateNextTrigger() {
    if (Mode.isEnabled) {
        time_t time = now();
        // if we have a next trigger that hasn't triggered yet, don't change it
        if (nextTrigger >= time)
            return;
        // either we don't have a next trigger time (first time through this) or the alarm has triggered and needs updated
        const auto daysPerWeek = (uint8_t)DAYS_PER_WEEK;
        time_t startDay = previousMidnight(time);
        int8_t dowNow = weekday(time) - 1;  //values from 0 to 6, Sunday is 0
        switch (Mode.alarmRepeat) {
            case DT_REPEAT_INTERVAL:
            case DT_REPEAT_NONE:
                nextTrigger = time + value;
                break;
            case DT_REPEAT_DAILY: {
                nextTrigger = value + startDay;
                break;
            }
            case DT_REPEAT_WEEKLY: {
                for (int8_t i = 1; i <= daysPerWeek; i++) {
                    int8_t dow = (dowNow + i) % daysPerWeek;    //start checking with next day
                    uint8_t mask = DAY_FLAG_SUNDAY << dow;
                    if (Mode.dayRepeat & mask) {
                        nextTrigger = value + startDay + SECS_PER_DAY * i;
                        break;
                    }
                }
                break;
            }
            case DT_REPEAT_MONTHLY: {
                if (Mode.dayRepeat == 0 || dowNow == daysPerWeek-1)
                    nextTrigger = advanceNextMonth(time, 0) + value;
                else {
                    //start iterating from tomorrow - for today we already triggerred the alarm
                    time_t nextTime = startDay + SECS_PER_DAY + elapsedSecsToday(value);
                    for (int8_t i = 0; i < daysPerWeek; i++) {
                        int8_t dow = weekday(nextTime)-1;
                        uint8_t mask = DAY_FLAG_SUNDAY << dow;
                        if (Mode.dayRepeat & mask) {
                            nextTrigger = nextTime;
                            break;
                        }
                        nextTime += SECS_PER_DAY;
                    }
                }
                break;
            }
            case DT_REPEAT_YEARLY: {
                nextTrigger = advanceNextYear(startDay, Mode.monthRepeat, elapsedDays(value), Mode.dayRepeat) +
                        elapsedSecsToday(value);
                break;
            }
            case DT_REPEAT_WEEKDAY_OF_MONTH: {
                nextTrigger = advanceNextYearNthWeekdayOfMonth(startDay, Mode.monthRepeat, Mode.weekDayOccurrence, Mode.toDayOfWeek()) +
                              elapsedSecsToday(value);

                break;
            }
            default:
                break;
        }
    }
}

dtAlarmState_t AlarmClass::nextState(time_t time) {
    if (!Mode.isEnabled)
        return state = Idle;
    switch (state) {
        case Idle:
            if (time >= onTime()) {
                state = On;
                if (onEventHandler != nullptr)
                    (*onEventHandler)();
            }
            break;
        case On:
            if (time >= offTime()) {
                state = Off;
                if (offEventHandler != nullptr)
                    (*offEventHandler)();
            }
            break;
        case Off:
            state = Idle;
            updateNextTrigger();
            break;
    }
    return state;
}

//**************************************************************
// TimeAlarmsClass

TimeAlarmsClass::TimeAlarmsClass() {
    isServicing = false;
}

void TimeAlarmsClass::enable(AlarmID_t ID) const {
    if (isAllocated(ID)) {
        alarms[ID]->enable();
        alarms[ID]->updateNextTrigger();
    }
}

void TimeAlarmsClass::disable(AlarmID_t ID) const {
    if (isAllocated(ID))
        alarms[ID]->disable();
}

// write the given value to the given alarm
void TimeAlarmsClass::updateNextTrigger(AlarmID_t ID, time_t value) {
    if (isAllocated(ID)) {
        alarms[ID]->nextTrigger = value;  //note: if this is in the past, the enable() will move it to the next cycle if it is repeatable
        enable(ID);  // update trigger time
    }
}

// return the value for the given alarm ID
time_t TimeAlarmsClass::getNextTrigger(AlarmID_t ID) const {
    return isAllocated(ID) ? alarms[ID]->nextTrigger : 0;
}

void TimeAlarmsClass::deleteAlarm(const std::deque<AlarmClass*>::iterator& it) {
    alarms.erase(it);
    delete *it;
}

bool TimeAlarmsClass::free(AlarmID_t ID) {
    if (isAllocated(ID)) {
        deleteAlarm(alarms.begin() + ID);
        return true;
    }
    return false;
}

bool TimeAlarmsClass::free(AlarmClass *alarm) {
    for (auto it = alarms.begin(); it != alarms.end(); it++) {
        auto al = *it;
        if (al == alarm) {
            deleteAlarm(it);
            return true;
        }
    }
    return false;
}

// returns the number of allocated timers
uint8_t TimeAlarmsClass::count() const {
    return alarms.size();
}

// returns true if this id is allocated
bool TimeAlarmsClass::isAllocated(AlarmID_t ID) const {
    return (ID < count());
}

/**
 * @return the time of last triggered alarm (if alarm type was one-time, the alarm object may not exist anymore)
 */
time_t TimeAlarmsClass::getLastAlarmTime() const {
    return lastTriggerTime;
}

/**
 *
 * @param Units units to retrieve digits for
 * @return the current digits of the time unit provided
 */
uint8_t TimeAlarmsClass::getDigitsNow(dtUnits_t Units) {
    time_t time = now();
    switch (Units) {
        case dtSecond: return numberOfSeconds(time);
        case dtMinute: return numberOfMinutes(time);
        case dtHour: return numberOfHours(time);
        case dtDay: return day(time);
        case dtWeekday: return dayOfWeek(time);
    }

    return 255;  // This should never happen
}

//returns isServicing
bool TimeAlarmsClass::IsServicing() const {
    return isServicing;
}

/**
 * Handle any alarms past due
 */
void TimeAlarmsClass::serviceAlarms() {
    if (!isServicing) {
        isServicing = true;
        time_t time = now();
        for (auto it = alarms.begin(); it != alarms.end(); it++) {
            auto al = *it;
            if (al->nextState(time) == Off) {
                if (al->Mode.isOneTime())
                    deleteAlarm(it);            // we're done with this alarm, dispose it
                else
                    al->nextState(time);        // it updates the next trigger time
            }
        }
        isServicing = false;
    }
}

/**
 *
 * @return the next scheduled alarm, or null if none
 */
AlarmClass* TimeAlarmsClass::getNextAlarm() const {
    time_t time = now();
    time_t nextTrigger = 0;
    AlarmClass *alPtr = nullptr;
    for (auto al : alarms) {
        if (!al->Mode.isEnabled || al->nextTrigger < time)
            continue;
        if (nextTrigger == 0 || al->nextTrigger < nextTrigger) {
            nextTrigger = al->nextTrigger;
            alPtr = al;
        }
    }

    return alPtr;
}

/**
 * Create an alarm and return its index value
 */
AlarmID_t TimeAlarmsClass::create(time_t value, time_t duration, AlarmHandlerPtr onTickHandler, AlarmHandlerPtr offTickHandler, AlarmMode_t alarmMode) {
    auto *al = new AlarmClass(onTickHandler, offTickHandler, value, duration, alarmMode);
    alarms.push_back(al);
    return alarms.size() - 1;
}

// make one instance for the user to use
TimeAlarmsClass Alarms = TimeAlarmsClass();