#include <Arduino.h>
#include <Time.h>
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <EEPROM.h>
#include <SPU.h>
#include "Calendar.h"

AlarmCalendar::AlarmCalendar(){
  Serial.printf("\n Alarm calendar");
  alarmAmount = 0;
  nextId = 0;
  initAlarms();
  showAlarms();
}

int AlarmCalendar::addWeeklyAlarm(int d, int h, int min, int finishH, int finishM, int event, int eventId){
  // devuelve la pos del arreglo = id de la alarma
  // parametros ingresados por el usuario

  int id = 0;
  while (id < MAX_ALARMS and alarms[id].used){ //se fija si hay espacio libre
    id++;
  } 
  if (id < MAX_ALARMS){ // hay espacio para una nueva alarma
    // guardo los datos de la nueva alarma
    alarms[id].day = d;
    alarms[id].hour = h;
    alarms[id].minutes = min;
    alarms[id].finishHour = finishH;
    alarms[id].finishMinutes = finishM;
    alarms[id].used = 1; // la asigno como ocupada
    alarms[id].type = WEEKLY_ALARM;
    alarms[id].event = event;
    alarms[id].eventId = eventId;
    alarms[id].id = nextId++;
    Serial.printf("\n Weekly alarm added for %s between %d:%d and %d:%d\n Id: %d",
      days(d), h, min, finishH, finishM, alarms[id].eventId);
    storeAlarms();
    alarmAmount++;
    return alarms[id].id;
  }else{
    Serial.printf("\n Doesn't have enough memory to create a new alarm.");
    Serial.printf("\n Please delete any alarm.");
    return -1;
  }   
}

int AlarmCalendar::addAlarmOnce(int d, int month, int year, int h, int min, int finishH, int finishM, int event, int eventId){
  // devuelve la pos del arreglo = id de la alarma
  // parametros ingresados por el usuario

  int id = 0;
  while (id < MAX_ALARMS and alarms[id].used){ //se fija si hay espacio libre
    id++;
  }
  if (id < MAX_ALARMS){ // hay espacio para una nueva alarma
    // guardo los datos de la nueva alarma
    alarms[id].day = d;
    alarms[id].hour = h;
    alarms[id].minutes = min;
    alarms[id].finishHour = finishH;
    alarms[id].finishMinutes = finishM;
    alarms[id].month = month;
    alarms[id].year = year % 100;
    alarms[id].used = 1; // la asigno como ocupada
    alarms[id].event = event;
    alarms[id].eventId = eventId;
    alarms[id].type = ALARM_ONCE;
    alarms[id].id = nextId++;
    Serial.printf("\n Alarm added for %d/%d/%d between %d:%d and %d:%d\n Id: %d",
      d, month, year, h, min, finishH, finishM, alarms[id].eventId);
    // deberia agregar un evento
    storeAlarms();
    alarmAmount++;
    return alarms[id].id;
  }else{
    Serial.printf("\n No dispone de espacio para crear una nueva Alarma");
    Serial.printf("\n Por favor elimine una alarma.");
    return -1;
  }   
}

void AlarmCalendar::removeAlarm(int id){
  for(int i = 0; i < MAX_ALARMS; i++){
    if(alarms[i].id == id){
      alarms[i].used = 0;
      alarmAmount--;
      storeAlarms();
      Serial.printf("\n Alarm %d delete", id);
      return;
    }
  }  
}

void AlarmCalendar::removeAlarmsWithEventAndId(int event, int eventId){
  for(int i = 0; i < MAX_ALARMS; i++){
    if(alarms[i].used && (alarms[i].event == event) && (alarms[i].eventId == eventId)){
      alarms[i].used = 0;
      alarmAmount--;
      Serial.printf("\n Alarm %d delete", alarms[i].id);
    }
  }
  storeAlarms();
}

alarm* AlarmCalendar::getAlarms(){
  alarm* a = (alarm*)malloc(sizeof(alarm)*MAX_ALARMS);
  int pos= 0;
  for(int i = 0; i <MAX_ALARMS; i++){
    if(alarms[i].used){
      a[pos]= alarms[i];
      pos++;
    }
  }
  return a;
}

int AlarmCalendar::alarmNumber(){
  return alarmAmount;
}

void AlarmCalendar::showAlarms(){ // id - Monday Start at: hh:mm Finish at: hh:mm
  for (int i = 0; i < MAX_ALARMS; i++){
    if(alarms[i].used){
      switch(alarms[i].type){
        case ALARM_ONCE:
          Serial.printf("\n Alarm %d - %d/%d/%d Start at: %d:%d Finish at: %d:%d Event: %d",
            alarms[i].id, alarms[i].day, alarms[i].month, alarms[i].year, alarms[i].hour, alarms[i].minutes,
            alarms[i].finishHour, alarms[i].finishMinutes, alarms[i].event);
          break;
        case WEEKLY_ALARM:
          Serial.printf("\n Weekly %d - %s Start at: %d:%d Finish at: %d:%d Event: %d",
              alarms[i].id, days(alarms[i].day), alarms[i].hour, alarms[i].minutes,
              alarms[i].finishHour, alarms[i].finishMinutes, alarms[i].event);
          break;
      }
    }
  }
}

char* AlarmCalendar::days(int d){
  switch(d){  
    case 1:  
      return "Lunes";  
      break;  
    case 2:  
      return "Martes";  
      break; 
    case 3:
      return "Miercoles";
      break;
    case 4:
      return "Jueves";
      break;
    case 5:
      return "Viernes";
      break;
    case 6:
      return "Sabado";
      break;
    case 7:
      return "Domingo";
      break;    
  }  
}  

void AlarmCalendar::initAlarms(){
  for(int i = 0; i < MAX_ALARMS; i++){
    alarms[i].used = 0;
    alarms[i].started = false;
  }
  readAlarms();
}

void AlarmCalendar::checkAlarms(){
  time_t t = now();
  alarm* aux;
  for(int i = 0; i < MAX_ALARMS; i++){
    if(alarms[i].used){
      aux = &alarms[i];
      checkAlarm(aux, i, t);
    }
  }
}

void AlarmCalendar::checkAlarm(alarm* aux, int id, time_t t){
  int h = hour(t);
  int m = minute(t);
  int wd  = weekday(t);
  if(wd == 1) wd = 7;
  else wd--;
  int d = day(t);
  int mth = month(t);
  int yr = year(t) % 100; // Ultimos dos digitos del año
  Serial.printf("\n Checking alarm with time %d:%d %d/%d/%d day %d", h, m, d, mth, yr, wd);
  switch(aux->type){
    case ALARM_ONCE:
      if(!aux->started){
        if((aux->hour == h) && (aux->minutes == m) && (aux->day == d) && (aux->month == mth)){
          aux->started = true;
          Serial.printf("\n Alarm %d started", id);
          eventOn(aux->event, aux->eventId);
        }
      } else if((aux->finishHour == h) && (aux->finishMinutes == m) && (aux->day == d) && (aux->month == mth)) {
        aux->started = false;
        eventOff(aux->event, aux->eventId);
        Serial.printf("\n Alarm %d ended", id);
        removeAlarm(aux->id);
      }
      break;
    case WEEKLY_ALARM:
      if(!aux->started){
        if((aux->hour == h) && (aux->minutes == m) && (aux->day == wd)){
          aux->started = true;
          Serial.printf("\n Alarm %d started", id);
          eventOn(aux->event, aux->eventId);
        }
      } else if((aux->finishHour == h) && (aux->finishMinutes == m) && (aux->day == wd)) {
        aux->started = false;
        eventOff(aux->event, aux->eventId);
        Serial.printf("\n Alarm %d ended", id);
      }
      break;
  }
}

void AlarmCalendar::storeAlarms(){
  int position = ALARMS_DIR;
  for(int i = 0; i < MAX_ALARMS; i++){
    alarm aux = alarms[i];
    if(aux.used == 1){
      EEPROM.write(position++, (byte)aux.type); // Información que se almacena para todas las alarmas
      EEPROM.write(position++, (byte)aux.day);
      EEPROM.write(position++, (byte)aux.hour);
      EEPROM.write(position++, (byte)aux.minutes);
      EEPROM.write(position++, (byte)aux.finishHour);
      EEPROM.write(position++, (byte)aux.finishMinutes);
      EEPROM.write(position++, (byte)aux.event);
      EEPROM.write(position++, (byte)aux.eventId);
      EEPROM.write(position++, (byte)aux.id);
      switch(aux.type){ // Información que depende del tipo
        case ALARM_ONCE:
          EEPROM.write(position++, (byte)(aux.year)); // Guarda ultimos dos digitos del año
          EEPROM.write(position++, (byte)(aux.month));
          break;
      } 
    }
  }
  EEPROM.write(position, (byte)0);
  EEPROM.commit();
}

void AlarmCalendar::readAlarms(){
  int alarmIndex = 0;
  alarm aux;
  int position = ALARMS_DIR;
  // El primer byte es el tipo de alarma
  aux.type = EEPROM.read(position++);
  while(aux.type != DELIMITER){ // Lee hasta que encuentra el delimitador
    aux.day = EEPROM.read(position++);
    aux.hour = EEPROM.read(position++);
    aux.minutes = EEPROM.read(position++);
    aux.finishHour = EEPROM.read(position++);
    aux.finishMinutes = EEPROM.read(position++);
    aux.event = EEPROM.read(position++);
    aux.eventId = EEPROM.read(position++);
    aux.used = 1;
    aux.started = false;
    aux.id = EEPROM.read(position++);
    if(aux.id >= nextId)
      nextId = aux.id+1;
    switch(aux.type){
      case ALARM_ONCE:
        aux.year = EEPROM.read(position++);
        aux.month = EEPROM.read(position++);
        break;
    }
    alarms[alarmIndex++] = aux;
    alarmAmount++;
    aux.type = EEPROM.read(position++);
  }
}

void AlarmCalendar::eventOn(int code, int id){
  for(int i = 0; i < MAX_CALENDAR_EVENTS; i++){
    if(events[i].code == code){
      events[i].on(id);
      return;
    }
  }
  Serial.printf("\n Undefined event: %d", code);
}

void AlarmCalendar::eventOff(int code, int id){
  for(int i = 0; i < MAX_CALENDAR_EVENTS; i++){
    if(events[i].code == code){
      events[i].off(id);
      return;
    }
  }
  Serial.printf("\n Undefined event: %d", code);
}

void AlarmCalendar::addEvent(int code, void (*on)(int), void (*off)(int)){
  int pos = 0;
  for(int i = 0; i < MAX_CALENDAR_EVENTS; i++){
    if(events[i].code == EVENT_NOT_SET){
      pos = i;
    } else if(events[i].code == code) return; // Si el codigo ya existe no hace nada
  }
  alarmEvent e;
  e.code = code;
  e.on = on;
  e.off = off;
  events[pos] = e;
}
