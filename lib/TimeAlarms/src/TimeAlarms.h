//  TimeAlarms.h - Arduino Time alarms header for use with Time library

#ifndef TimeAlarms_h
#define TimeAlarms_h

#include <Arduino.h>
#include "TimeLib.h"
#include <deque>

#define DAY_FLAG_SUNDAY     ((uint8_t)0x01)
#define DAY_FLAG_MONDAY     ((uint8_t)0x02)
#define DAY_FLAG_TUESDAY    ((uint8_t)0x04)
#define DAY_FLAG_WEDNESDAY  ((uint8_t)0x08)
#define DAY_FLAG_THURSDAY   ((uint8_t)0x10)
#define DAY_FLAG_FRIDAY     ((uint8_t)0x20)
#define DAY_FLAG_SATURDAY   ((uint8_t)0x40)

#define DEFAULT_ALARM_DURATION  (1*SECS_PER_HOUR)   //default alarm duration - 1 hour

typedef enum DateTimeUnits:uint8_t {
    dtSecond,
    dtMinute,
    dtHour,
    dtDay,
    dtWeekday
} dtUnits_t;

typedef enum RepeatType:uint8_t {
    DT_REPEAT_NONE,
    DT_REPEAT_INTERVAL,
    DT_REPEAT_DAILY,
    DT_REPEAT_WEEKLY,
    DT_REPEAT_MONTHLY,
    DT_REPEAT_WEEKDAY_OF_MONTH,  // for repeating on a specific weekday of a specific month - e.g. 3rd Thursday of November
    DT_REPEAT_YEARLY
} dtRepeatType_t;

typedef struct AlarmMode {
    bool isEnabled: 1 = true;   // the timer is only actioned if isEnabled is true
    dtRepeatType_t alarmRepeat: 3 = DT_REPEAT_NONE;
    uint8_t weekDayOccurrence : 3 = 0; // for DT_REPEAT_WEEKDAY_OF_MONTH - nth occurrence, weekday showed by dayRepeat bit flag
    uint8_t dayRepeat: 7 = 0;   // ignored when alarmRepeat is one of DT_REPEAT_NONE, DT_REPEAT_INTERVAL, DT_REPEAT_DAILY or DT_REPEAT_YEARLY
    uint8_t monthRepeat : 4 = 0; // for DT_REPEAT_WEEKDAY_OF_MONTH - month targeted

    inline bool isOneTime() const { return dayRepeat == 0 && alarmRepeat == DT_REPEAT_NONE; }
    timeDayOfWeek_t toDayOfWeek() const {
        if (dayRepeat == 0)
            return dowInvalid;
        uint8_t repeat = dayRepeat, n = 1;
        while ((repeat & 0x01) == 0) {
            repeat = repeat >> 1;
            n++;
        }
        return n > 7 ? dowInvalid : (timeDayOfWeek_t) n;
    }
} AlarmMode_t;

typedef enum AlarmState:uint8_t {
    Idle, On, Off
} dtAlarmState_t;

extern const AlarmMode_t modeTriggerOnce;
extern const AlarmMode_t modeRepeatWeekDays;
extern const AlarmMode_t modeRepeatWeekend;
extern const AlarmMode_t modeRepeatMonday;
extern const AlarmMode_t modeRepeatTuesday;
extern const AlarmMode_t modeRepeatWednesday;
extern const AlarmMode_t modeRepeatThursday;
extern const AlarmMode_t modeRepeatFriday;
extern const AlarmMode_t modeRepeatYearly;
extern const AlarmMode_t modeTplRepeatMonthly;
extern const AlarmMode_t modeTplRepeatWeekdayOfMonth;

typedef uint8_t AlarmID_t;  //maximum 255 alerts - somewhat artificial limitation for handling indexes, the actual number of alarms is limited by memory

#define AlarmHMS(_hr_, _min_, _sec_) (_hr_ * SECS_PER_HOUR + _min_ * SECS_PER_MIN + _sec_)

typedef void (*AlarmHandlerPtr)();  // alarm callback function typedef

class AlarmClass {
public:
    AlarmClass(AlarmHandlerPtr handlerOn, AlarmHandlerPtr handlerOff, time_t value, time_t duration, const AlarmMode_t &mode);

    AlarmHandlerPtr const onEventHandler;
    AlarmHandlerPtr const offEventHandler;

    AlarmMode_t Mode;
    time_t nextTrigger;
    uint8_t priority;
    const time_t value;
    const time_t duration;

    void updateNextTrigger();
    dtAlarmState_t nextState(time_t time);
    inline void enable() { Mode.isEnabled = onEventHandler != nullptr; }
    inline void disable() { Mode.isEnabled = false; }
    inline time_t onTime() const { return nextTrigger; }
    inline time_t offTime() const { return nextTrigger + duration; }
    inline dtAlarmState_t getState() const { return state; }

protected:
    dtAlarmState_t state;

};

// class containing the collection of alarms
class TimeAlarmsClass {
private:
    std::deque<AlarmClass *> alarms;
    bool isServicing;
    time_t lastTriggerTime {}; // the time of the last alarm serviced
    AlarmID_t create(time_t value, time_t duration, AlarmHandlerPtr onTickHandler, AlarmHandlerPtr offTickHandler, AlarmMode_t alarmMode);

public:
    TimeAlarmsClass();

    void serviceAlarms();

    // functions to create alarms and timers
    AlarmID_t newAlarmOnce(time_t value, time_t dur, AlarmHandlerPtr onHandler, AlarmHandlerPtr offHandler = nullptr) {
        return create(value, dur, onHandler, offHandler, modeTriggerOnce);
    }

    AlarmID_t newAlarmOnce(const uint8_t H, const uint8_t M, const uint8_t S, AlarmHandlerPtr onHandler, AlarmHandlerPtr offHandler = nullptr) {
        return newAlarmOnce(AlarmHMS(H, M, S), DEFAULT_ALARM_DURATION, onHandler, offHandler);
    }

    // trigger once on a given weekday and time
    AlarmID_t newAlarmOnce(const timeDayOfWeek_t DOW, const uint8_t H, const uint8_t M, const uint8_t S, AlarmHandlerPtr onHandler, AlarmHandlerPtr offHandler = nullptr) {
        time_t value = (DOW - 1) * SECS_PER_DAY + AlarmHMS(H, M, S);
        return newAlarmOnce(value, DEFAULT_ALARM_DURATION, onHandler, offHandler);
    }

    // trigger with given alarm mode - default duration 1hour
    AlarmID_t newAlarmRepeat(time_t value, time_t dur, AlarmMode_t alarmMode, AlarmHandlerPtr onHandler, AlarmHandlerPtr offHandler = nullptr) {
        return create(value, dur, onHandler, offHandler, alarmMode);
    }

    AlarmID_t newAlarmRepeat(const uint8_t H, const uint8_t M, const uint8_t S, AlarmMode_t alarmMode, AlarmHandlerPtr onHandler, AlarmHandlerPtr offHandler = nullptr) {
        return newAlarmRepeat(AlarmHMS(H, M, S), DEFAULT_ALARM_DURATION, alarmMode, onHandler, offHandler);
    }

    AlarmID_t newAlarmRepeat(const uint8_t D, const uint8_t H, const uint8_t M, const uint8_t S, AlarmMode_t alarmMode, AlarmHandlerPtr onHandler, AlarmHandlerPtr offHandler = nullptr) {
        time_t value = (D-1) * SECS_PER_DAY + AlarmHMS(H, M, S);
        return newAlarmRepeat(value, DEFAULT_ALARM_DURATION, alarmMode, onHandler, offHandler);
    }

    // trigger at a regular interval
    AlarmID_t newTimerRepeat(time_t value, time_t dur, AlarmHandlerPtr onHandler, AlarmHandlerPtr offHandler = nullptr) {
        AlarmMode_t modeTimer {.alarmRepeat=DT_REPEAT_INTERVAL, .isEnabled=true, .dayRepeat=0};
        return create(value, dur, onHandler, offHandler, modeTimer);
    }

    AlarmID_t newTimerRepeat(const uint8_t H, const uint8_t M, const uint8_t S, AlarmHandlerPtr onHandler, AlarmHandlerPtr offHandler = nullptr) {
        return newTimerRepeat(AlarmHMS(H, M, S), SECS_PER_MIN, onHandler, offHandler);
    }

    // utility methods
    static uint8_t getDigitsNow(dtUnits_t Units) ;         // returns the current digit value for the given time unit

    void enable(AlarmID_t ID) const;
    void disable(AlarmID_t ID) const;
    time_t getLastAlarmTime() const;
    bool IsServicing() const;
    void updateNextTrigger(AlarmID_t ID, time_t value);
    time_t getNextTrigger(AlarmID_t ID) const;
    AlarmClass* getNextAlarm() const;

    bool free(AlarmID_t ID);
    bool free(AlarmClass* alarm);

    uint8_t count() const;
    bool isAllocated(AlarmID_t ID) const;

protected:
    void deleteAlarm(const std::deque<AlarmClass*>::iterator& it);
};

extern TimeAlarmsClass Alarms;

#endif /* TimeAlarms_h */