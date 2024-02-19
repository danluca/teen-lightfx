// Copyright (c) 2024 by Dan Luca. All rights reserved.
//

#ifndef TEEN_LIGHTFX_FXSCHEDULE_H
#define TEEN_LIGHTFX_FXSCHEDULE_H

#include <deque>
#include "Arduino.h"

void setupAlarmSchedule();
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
        {.start=0x0101, .end=0x0103, .type=DayOff},     //new year
        {.start=0x0103, .end=0x010F, .type=School},
        {.start=0x010F, .end=0x0110, .type=DayOff},     //Martin Luther King
        {.start=0x0110, .end=0x011A, .type=School},
        {.start=0x011A, .end=0x011B, .type=DayOff},
        {.start=0x011B, .end=0x020F, .type=School},
        {.start=0x020F, .end=0x0214, .type=DayOff},     //President Day weekend
        {.start=0x0214, .end=0x0308, .type=School},
        {.start=0x0308, .end=0x0309, .type=DayOff},     //Flex learning day
        {.start=0x0309, .end=0x031D, .type=School},
        {.start=0x031D, .end=0x031E, .type=DayOff},
        {.start=0x031E, .end=0x0409, .type=Vacation},   //spring break + one extra day 04/08
        {.start=0x0409, .end=0x051B, .type=School},
        {.start=0x051B, .end=0x051C, .type=DayOff},     //Memorial day
        {.start=0x051C, .end=0x0607, .type=School},
        {.start=0x0607, .end=0x0903, .type=Vacation},   //summer break
};

extern std::deque<AlarmData*> scheduledAlarms;

#endif //TEEN_LIGHTFX_FXSCHEDULE_H
