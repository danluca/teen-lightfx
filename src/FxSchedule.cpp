// Copyright (c) 2024 by Dan Luca. All rights reserved.
//

#include "FxSchedule.h"
#include "FastLED.h"
#include "timeutil.h"
#include "log.h"

#define DEFAULT_WAKEUP_TIME_ON  (7*SECS_PER_HOUR)                           //7:00am wakeup
#define DEFAULT_WAKEUP_TIME_OFF (DEFAULT_WAKEUP_TIME_ON + 30*SECS_PER_MIN)  //7:30am wakeup effect turn off
#define DEFAULT_SCHOOLDAY_BEDTIME   (20*SECS_PER_HOUR)                      //8:00pm bedtime school day
#define DEFAULT_WEEKEND_BEDTIME     (DEFAULT_SCHOOLDAY_BEDTIME + SECS_PER_HOUR) //9:00pm bedtime weekend, day off
#define DEFAULT_VACATION_BEDTIME    (DEFAULT_WEEKEND_BEDTIME + SECS_PER_HOUR)   //10:00pm bedtime vacation
#define DEFAULT_SLEEP_IN            (2*SECS_PER_HOUR)

time_t wakeupTimeOn = DEFAULT_WAKEUP_TIME_ON;
time_t wakeupTimeOff = DEFAULT_WAKEUP_TIME_OFF;
time_t schoolDayBedTime = DEFAULT_SCHOOLDAY_BEDTIME;
time_t weekendBedTime = DEFAULT_WEEKEND_BEDTIME;
time_t vacationBedTime = DEFAULT_VACATION_BEDTIME;
uint16_t currentDay = 0;

std::deque<AlarmData*> scheduledAlarms;

dtDayType getTomorrowDayType(time_t time) {
    uint16_t tomorrow = encodeMonthDay(time + SECS_PER_DAY);
    for (auto &i : schoolSchedule) {
        if (tomorrow >= i.start && tomorrow < i.end)
            return i.type;
    }
    return NotHome;
}

time_t getBedTime(time_t startDay) {
    time_t bedTime = startDay;
    switch (getTomorrowDayType(startDay)) {
        case School: bedTime+=schoolDayBedTime; break;
        case DayOff: bedTime+=weekendBedTime; break;
        case NotHome:
        case Vacation: bedTime+=vacationBedTime; break;
    }
    return bedTime;
}

void scheduleSchoolDay(time_t time) {
    time_t startDay = previousMidnight(time);

    uint8_t curAlarmCount = scheduledAlarms.size();
    //wake-up if not passed it
    if ((time-startDay) < wakeupTimeOn) {
        time_t upOn = startDay + wakeupTimeOn;
        time_t upOff = startDay + wakeupTimeOff;
        scheduledAlarms.push_back(new AlarmData{.value=upOn, .type=WAKEUP, .onEventHandler=wakeupOn});
        scheduledAlarms.push_back(new AlarmData{.value=upOff, .type=ALARM_OFF, .onEventHandler=wakeupOff});
    }
    //sleep
    time_t bedTime = getBedTime(startDay);
    scheduledAlarms.push_back(new AlarmData {.value=bedTime, .type=BEDTIME, .onEventHandler=sleepOn});
    Log.infoln("Scheduled %d alarms for SchoolDay %y", scheduledAlarms.size() - curAlarmCount, time);
}

void scheduleDayOff(time_t time) {
    time_t startDay = previousMidnight(time);

    uint8_t curAlarmCount = scheduledAlarms.size();
    //wake-up if not passed it
    if ((time-startDay) < wakeupTimeOn + DEFAULT_SLEEP_IN) {
        time_t upOn = startDay + wakeupTimeOn + DEFAULT_SLEEP_IN;
        time_t upOff = startDay + wakeupTimeOff + DEFAULT_SLEEP_IN;
        scheduledAlarms.push_back(new AlarmData{.value=upOn, .type=WAKEUP, .onEventHandler=wakeupOn});
        scheduledAlarms.push_back(new AlarmData{.value=upOff, .type=ALARM_OFF, .onEventHandler=wakeupOff});
    }
    //sleep
    time_t bedTime = getBedTime(startDay);
    scheduledAlarms.push_back(new AlarmData {.value=bedTime, .type=BEDTIME, .onEventHandler=sleepOn});
    Log.infoln("Scheduled %d alarms for Day Off %y", scheduledAlarms.size() - curAlarmCount, time);
}

void scheduleVacation(time_t time) {
    time_t startDay = previousMidnight(time);

    uint8_t curAlarmCount = scheduledAlarms.size();
    //wake-up if not passed it
    if ((time-startDay) < wakeupTimeOn + DEFAULT_SLEEP_IN) {
        time_t upOn = startDay + wakeupTimeOn + DEFAULT_SLEEP_IN;
        time_t upOff = startDay + wakeupTimeOff + DEFAULT_SLEEP_IN;
        scheduledAlarms.push_back(new AlarmData{.value=upOn, .type=WAKEUP, .onEventHandler=wakeupOn});
        scheduledAlarms.push_back(new AlarmData{.value=upOff, .type=ALARM_OFF, .onEventHandler=wakeupOff});
    }
    //sleep
    time_t bedTime = getBedTime(startDay);
    scheduledAlarms.push_back(new AlarmData {.value=bedTime, .type=BEDTIME, .onEventHandler=sleepOn});
    Log.infoln("Scheduled %d alarms for Vacation Day %y", scheduledAlarms.size() - curAlarmCount, time);
}

void scheduleNotHome(time_t time) {
    //no alarms for not being home
    scheduledAlarms.clear();
    //ensure we have a scheduled alarm for quiet time
    time_t startDay = previousMidnight(time);
    time_t upOff = startDay + wakeupTimeOff + DEFAULT_SLEEP_IN;
    scheduledAlarms.push_back(new AlarmData {.value=upOff, .type=ALARM_OFF, .onEventHandler=sleepOff});
    Log.infoln("Scheduled 1 alarms for Not Home Day %y", time);
}

void logAlarms() {
    for (const auto &al : scheduledAlarms)
        Log.infoln("Alarm %X type %d scheduled for %y; handler %X", (long)al, al->type, al->value, (long)al->onEventHandler);
}

/**
 * Setup the default sleep/wake-up schedule for the school year
 * TODO: how to save and read from JSON?
 */
void setupAlarmSchedule() {
    //alarms for today
    time_t time = now();
    currentDay = day(time);
    auto dow = static_cast<timeDayOfWeek_t>(weekday(time));
    uint16_t today = encodeMonthDay(time);
    uint16_t tmrw = encodeMonthDay(time + SECS_PER_DAY);
    const Interval *interval = nullptr;
    const Interval *weekend = nullptr;

    if (dow == dowSaturday || dow == dowSunday) {
        weekend = new Interval {.start=today, .end=tmrw, .type=DayOff};
    }
    for (auto &i : schoolSchedule) {
        if (today >= i.start && today < i.end) {
            if (i.type == School && weekend != nullptr)
                interval = weekend;
            else
                interval = &i;
            break;
        }
    }
    if (interval != nullptr) {
        switch (interval->type) {
            case School: scheduleSchoolDay(time); break;
            case DayOff: scheduleDayOff(time); break;
            case Vacation: scheduleVacation(time); break;
            case NotHome: scheduleNotHome(time); break;
        }
    }
    adjustCurrentEffect(time);
    delete weekend;
    logAlarms();
}

void alarm_loop() {
    EVERY_N_SECONDS(60) {
        if (currentDay != day())
            setupAlarmSchedule();
        time_t time = now();
        for (auto it = scheduledAlarms.begin(); it != scheduledAlarms.end();) {
            auto al = *it;
            if (al->value <= time) {
                Log.infoln("Alarm %X type %d triggered at %y for scheduled time %y; handler %X", (long)al, al->type, time, al->value, (long)al->onEventHandler);
                al->onEventHandler();
                it = scheduledAlarms.erase(it);
                delete al;
            } else
                ++it;
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
