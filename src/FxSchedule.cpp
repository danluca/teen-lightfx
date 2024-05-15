// Copyright (c) 2024 by Dan Luca. All rights reserved.
//

#include "FxSchedule.h"
#include "FastLED.h"
#include "global.h"
#include "timeutil.h"
#include "util.h"
#include "log.h"

#define DEFAULT_WAKEUP_TIME_ON  (7*SECS_PER_HOUR)                           //7:00am wakeup
#define DEFAULT_WAKEUP_TIME_OFF (DEFAULT_WAKEUP_TIME_ON + 30*SECS_PER_MIN)  //7:30am wakeup effect turn off
#define DEFAULT_SCHOOLDAY_BEDTIME   (20*SECS_PER_HOUR + 30*SECS_PER_MIN)    //8:30pm bedtime school day
#define DEFAULT_WEEKEND_BEDTIME     (DEFAULT_SCHOOLDAY_BEDTIME + SECS_PER_HOUR) //9:30pm bedtime weekend, day off
#define DEFAULT_VACATION_BEDTIME    (DEFAULT_WEEKEND_BEDTIME + 45*SECS_PER_MIN)   //10:15pm bedtime vacation
#define DEFAULT_SLEEP_IN            (2*SECS_PER_HOUR)

const char strWakeup[] PROGMEM = "Wake-Up";
const char strBedtime[] PROGMEM = "Bed-time";
const char strAlarmOff[] PROGMEM = "Alarm Off";

time_t wakeupTimeOn = DEFAULT_WAKEUP_TIME_ON;
time_t wakeupTimeOff = DEFAULT_WAKEUP_TIME_OFF;
time_t schoolDayBedTime = DEFAULT_SCHOOLDAY_BEDTIME;
time_t weekendBedTime = DEFAULT_WEEKEND_BEDTIME;
time_t vacationBedTime = DEFAULT_VACATION_BEDTIME;
uint16_t currentDay = 0;

std::deque<AlarmData*> scheduledAlarms;

time_t getBedTime(const time_t startDay, DayType nextDayType) {
    time_t bedTime = startDay;
    switch (nextDayType) {
        case School: bedTime+=schoolDayBedTime; break;
        case DayOff: bedTime+=weekendBedTime; break;
        case Vacation: bedTime+=vacationBedTime; break;
        case NotHome: bedTime = 0; break;
    }
    return bedTime;
}

time_t getWakeupOn(const time_t startDay, DayType dayType) {
    time_t wakeupOn = startDay;
    switch (dayType) {
        case School: wakeupOn += wakeupTimeOn; break;
        case DayOff:
        case Vacation: wakeupOn += wakeupTimeOn + DEFAULT_SLEEP_IN; break;
        case NotHome: wakeupOn = 0; break;      //no wakeup when not at home
    }
    return wakeupOn;
}

time_t getAlarmOff(const time_t startDay, DayType dayType) {
    time_t alarmOff = startDay;
    switch (dayType) {
        case NotHome:
        case School: alarmOff += wakeupTimeOff; break;
        case DayOff:
        case Vacation: alarmOff += wakeupTimeOff + DEFAULT_SLEEP_IN; break;
    }
    return alarmOff;
}

/**
 * Counts scheduled alarms of a particular type that haven't yet triggered (are in the future with respect to given time reference)
 * @param alType alarm type
 * @param refTime time reference
 * @return how many scheduled alarms fit the criteria above
 */
uint countFutureAlarms(const AlarmType alType, const time_t refTime) {
    uint count = 0;
    for (const auto &al : scheduledAlarms) {
        if (al->value > refTime && al->type == alType)
            count++;
    }
    return count;
}

/**
 * Counts the alarms of a type that have been scheduled on the same day as the reference time
 * @param alType alarm type
 * @param refTime time reference
 * @return how many alarms are scheduled for same day as the reference time
 */
uint countTodayAlarms(const AlarmType alType, const time_t refTime) {
    time_t startDay = previousMidnight(refTime);
    time_t startNextDay = startDay+SECS_PER_DAY;
    uint count = 0;
    for (const auto &al : scheduledAlarms) {
        if (al->type == alType && al->value >= startDay && al->value < startNextDay)
            count++;
    }
    return count;
}

DayType getDayType(const time_t time) {
    time_t refTime = time == 0 ? now() : time;
    auto dow = static_cast<timeDayOfWeek_t>(weekday(refTime));
    uint16_t today = encodeMonthDay(refTime);
    bool isWeekend = dow == dowSaturday || dow == dowSunday;
    DayType dayType = NotHome;
    for (auto &i: schoolSchedule) {
        if (today >= i.start && today < i.end) {
            dayType = (i.type == School && isWeekend) ? DayOff : i.type;
            break;
        }
    }
    return dayType;
}

const char *alarmTypeToString(AlarmType alType) {
    switch (alType) {
        case WAKEUP: return strWakeup;
        case BEDTIME: return strBedtime;
        case ALARM_OFF: return strAlarmOff;
        default: return strNR;
    }
}

void scheduleDay(const time_t time) {
    time_t startDay = previousMidnight(time);
    time_t startNextDay = startDay + SECS_PER_DAY;
    DayType todayType = getDayType(time);
    DayType tomorrowType = getDayType(startNextDay);

    size_t curAlarmCount = scheduledAlarms.size();
    //wakeup
    if (countFutureAlarms(WAKEUP, time) < 1) {
        //wake-up today if not passed it, tomorrow if we did
        time_t wakeupTimeToday = getWakeupOn(startDay, todayType);
        time_t wakeupTimeTomorrow = getWakeupOn(startNextDay, tomorrowType);
        if (time < wakeupTimeToday)
            scheduledAlarms.push_back(new AlarmData{.value=wakeupTimeToday, .type=WAKEUP, .onEventHandler=wakeupOn});
        else if (wakeupTimeTomorrow > 0)
            scheduledAlarms.push_back(new AlarmData{.value=wakeupTimeTomorrow, .type=WAKEUP, .onEventHandler=wakeupOn});
    }

    // alarm off
    if (countFutureAlarms(ALARM_OFF, time) < 1) {
        //alarm off today if not passed it, tomorrow if we did
        time_t alarmOffToday = getAlarmOff(startDay, todayType);
        time_t alarmOffTomorrow = getAlarmOff(startNextDay, tomorrowType);
        if (time < alarmOffToday)
            scheduledAlarms.push_back(new AlarmData{.value=alarmOffToday, .type=ALARM_OFF, .onEventHandler=wakeupOff});
        else if (alarmOffTomorrow > 0)
            scheduledAlarms.push_back(new AlarmData{.value=alarmOffTomorrow, .type=ALARM_OFF, .onEventHandler=wakeupOff});
    }

    //sleep
    if (countFutureAlarms(BEDTIME, time) < 1) {
        time_t bedTime = getBedTime(startDay, tomorrowType);
        if (time < bedTime)
            scheduledAlarms.push_back(new AlarmData{.value=bedTime, .type=BEDTIME, .onEventHandler=sleepOn});
        else {
            bedTime = getBedTime(startNextDay, getDayType(startNextDay+SECS_PER_DAY));
            if (bedTime > 0)
                scheduledAlarms.push_back(new AlarmData{.value=bedTime, .type=BEDTIME, .onEventHandler=sleepOn});
        }
    }

    Log.infoln(F("Scheduled %d alarms for Day %y (today type %d, tomorrow type %d)"), scheduledAlarms.size() - curAlarmCount, time, todayType, tomorrowType);
}

/**
 * Logs the alarms to the console - info level
 */
void logAlarms() {
    for (const auto &al : scheduledAlarms)
        Log.infoln(F("Alarm %X type %d scheduled for %y; handler %X"), (long)al, al->type, al->value, (long)al->onEventHandler);
}

/**
 * Setup the default sleep/wake-up schedule for the school year
 */
void setupAlarmSchedule() {
    if (!isSysStatus(SYS_STATUS_WIFI)) {
        Log.warningln(F("Cannot setup alarms without WiFi, likely time is not set"));
        return;
    }
    //alarms for today
    time_t time = now();
    currentDay = day(time);

    if (firstRun) {
        //it is the first run, create alarms for today and execute the previous one relative to current time
        Log.infoln(F("First run - setup alarm schedule and determine current effect for time %y"), time);
        scheduleDay(previousMidnight(time));
        logAlarms();
        adjustCurrentEffect(time);
        //remove all the alarms, we'll create the new ones based on the current time
        for (auto it = scheduledAlarms.begin(); it != scheduledAlarms.end();) {
            auto al = *it;
            it = scheduledAlarms.erase(it);
            delete al;
        }
        firstRun = false;
    }
    scheduleDay(time);
    logAlarms();
}

void alarm_loop() {
    EVERY_N_SECONDS(60) {
        time_t time = now();
        for (auto it = scheduledAlarms.begin(); it != scheduledAlarms.end();) {
            auto al = *it;
            if (al->value <= time) {
                Log.infoln(F("Alarm %X type %d triggered at %y for scheduled time %y; handler %X"), (long)al, al->type, time, al->value, (long)al->onEventHandler);
                al->onEventHandler();
                it = scheduledAlarms.erase(it);
                delete al;
            } else
                ++it;
        }
        //if no more alarms or a new day - attempt to schedule next alarms
        if (scheduledAlarms.empty() || currentDay != day(time)) {
            Log.infoln(F("Alarms queue empty or a new day - schedule more"));
            setupAlarmSchedule();
        } else {
            Log.infoln(F("Alarms remaining:"));
            logAlarms();
        }

//        for (size_t x = 0; x < scheduledAlarms.size(); x++) {
//            AlarmData *al = scheduledAlarms.front();
//            scheduledAlarms.pop_front();
//            if (al->value <= time) {
//                Log.infoln("Alarm %X type %d triggered at %y for scheduled time %y; handler %X", (long)al, al->type, time, al->value, (long)al->onEventHandler);
//                al->onEventHandler();
//                delete al;
//            } else
//                scheduledAlarms.push_back(al);
//        }
    }
}
