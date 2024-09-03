// Copyright (c) 2024 by Dan Luca. All rights reserved.
//

#ifndef TEEN_LIGHTFX_FXSCHEDULE_H
#define TEEN_LIGHTFX_FXSCHEDULE_H

#include <deque>
#include "Arduino.h"

extern time_t wakeupTimeOn;
extern time_t wakeupTimeOff;
extern time_t schoolDayBedTime;
extern time_t weekendBedTime;
extern time_t vacationBedTime;


void setupAlarmSchedule();
void clearAlarmSchedule();
void alarm_loop();

void wakeupOn();
void wakeupOff();
void sleepOn();
void sleepOff();
void quiet();
void adjustCurrentEffect(time_t time);

typedef void (*AlarmHandlerPtr)();  // alarm callback function typedef

typedef enum DayType:uint8_t {
    School, DayOff, Vacation, NotHome
} dtDayType;

enum AlarmType {
    WAKEUP, BEDTIME, ALARM_OFF
};

const char* alarmTypeToString(AlarmType alType);
uint countFutureAlarms(AlarmType alType, time_t refTime);
DayType getDayType(time_t time = 0);

struct Interval {
    uint16_t start; //inclusive
    uint16_t end;   //exclusive
    dtDayType type;
};

struct AlarmData {
    const time_t value;
    const AlarmType type;
    AlarmHandlerPtr const onEventHandler;
};

//weekends are implicit, no need to specify - see https://www.wayzataschools.org/calendar
const Interval schoolSchedule[] PROGMEM = {
        {.start=0x0101, .end=0x0102, .type=DayOff},     //new year
        {.start=0x0102, .end=0x0114, .type=School},
        {.start=0x0114, .end=0x0115, .type=DayOff},     //Martin Luther King
        {.start=0x0115, .end=0x0118, .type=School},
        {.start=0x0118, .end=0x0119, .type=DayOff},
        {.start=0x0119, .end=0x020E, .type=School},
        {.start=0x020E, .end=0x0212, .type=DayOff},     //President Day weekend
        {.start=0x0212, .end=0x030E, .type=School},
        {.start=0x030E, .end=0x030F, .type=DayOff},     //Flex learning day
        {.start=0x030F, .end=0x031C, .type=School},
        {.start=0x031C, .end=0x031D, .type=DayOff},
        {.start=0x031D, .end=0x0407, .type=Vacation},   //spring break + one extra day 04/08
        {.start=0x0407, .end=0x051A, .type=School},
        {.start=0x051A, .end=0x051B, .type=DayOff},     //Memorial day
        {.start=0x051B, .end=0x0606, .type=School},
        {.start=0x0606, .end=0x0903, .type=Vacation},   //summer break
        {.start=0x0903, .end=0x0917, .type=School},
        {.start=0x0917, .end=0x0918, .type=DayOff},     //Flex learning day
        {.start=0x0918, .end=0x0A10, .type=School},
        {.start=0x0A10, .end=0x0A13, .type=DayOff},     //October educators day off
        {.start=0x0A13, .end=0x0B08, .type=School},
        {.start=0x0B08, .end=0x0B09, .type=DayOff},     //Flex learning day
        {.start=0x0B09, .end=0x0B1B, .type=School},
        {.start=0x0B1B, .end=0x0B1E, .type=DayOff},     //Thanksgiving weekend
        {.start=0x0B1E, .end=0x0C17, .type=School},
        {.start=0x0C17, .end=0x0C20, .type=Vacation},   //Christmas break
};

extern std::deque<AlarmData*> scheduledAlarms;

#endif //TEEN_LIGHTFX_FXSCHEDULE_H
