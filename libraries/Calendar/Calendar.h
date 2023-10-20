#ifndef CALENDAR_H
#define CALENDAR_H

#define MAX_ALARMS 30 // Maxima cantidad de alarmas en el calendario
#define MAX_CALENDAR_EVENTS 5
#define ALARM_CHECK_TIME 5 // Tiempo cada cuanto se checkea si debe ejecutarse un evento
#define EVENT_NOT_SET -1

// Distinción entre tipos de alarmas
#define ALARM_ONCE 1
#define WEEKLY_ALARM 2

typedef struct {
  int id;
  int day;
  int hour;
  int minutes;
  int finishHour;
  int finishMinutes;
  int month;
  int year;
  int event;
  bool started;
  int used; // para saber si ya se utilizo esta alarma o esta libre
  int type;
  int eventId; // Id para identificar distintas alarmas de un mismo evento
} alarm;

typedef struct e {
  e(): code(EVENT_NOT_SET){} // para saber si esta en uso o libre
  int code;
  void (*on)(int);
  void (*off)(int);
} alarmEvent;

class AlarmCalendar {
  public:
    AlarmCalendar();
    int addWeeklyAlarm(int day, int hour,
                       int minutes,int finishHour,
                       int finishMinutes, int event, int eventId = EVENT_NOT_SET);
    void addEvent(int code, void (*on)(int), void (*off)(int));
    void removeAlarm(int id);
    void removeAlarmsWithEventAndId(int event, int eventId);
    int addAlarmOnce(int day, int month, int year, int hour, int minutes, int finishHour, int finishMinutes, int event, int eventId = EVENT_NOT_SET);
    void showAlarms();
    void checkAlarms();
    alarm* getAlarms();
    int alarmNumber();
    char* days(int d);
  private:
    alarm alarms[MAX_ALARMS];
    alarmEvent events[MAX_CALENDAR_EVENTS];
    int alarmAmount;
    int nextId;
    void initAlarms();
    void eventOn(int code, int eventId);
    void eventOff(int code, int eventId);
    void storeAlarms();
    void readAlarms();
    void checkAlarm(alarm* aux, int id, time_t t);
};

#endif
