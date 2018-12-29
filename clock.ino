#include <Arduino.h>
#include <TM163.h>
#include <TM1637.h>
#include <iarduino_RTC.h>
#include <Wire.h>
#include <DS3231.h>

#define CLK 12
#define DIO 11
#define CLK2 10
#define DIO2 9
#define brightness 1
#define SH_CP 4
#define ST_CP 3
#define DS 5

const uint8_t SET = 6;
const uint8_t HOUR = 7;
const uint8_t MIN = 8;

TM1637 tm1637HEX(CLK, DIO);
TM163 tm1637OCT(CLK2, DIO2);
iarduino_RTC time(RTC_DS3231);

char *t;
uint8_t type = 8;
uint8_t min, sec, hour, i = 0, a = 0;
byte timeDisp[4];
bool flag = LOW;

//time_out(hour,min,sec);

void setup() {

    Wire.begin();
    time.begin();

    pinMode(SH_CP, OUTPUT);
    pinMode(ST_CP, OUTPUT);
    pinMode(DS, OUTPUT);
    pinMode(13, OUTPUT);
    pinMode(SET, INPUT);
    pinMode(HOUR, INPUT);
    pinMode(MIN, INPUT);

    tm1637HEX.init();
    tm1637HEX.set(brightness);

    tm1637OCT.init();
    tm1637OCT.set(brightness);

    tm1637HEX.point(true);
    tm1637OCT.point(true);

}

void time_out() {

    tm1637HEX.point(!(sec & 1));
    tm1637OCT.point(!(sec & 1));
//flag=!( sec & 1);
//digitalWrite(13,flag);

    if (type == 10) {
        timeDisp[0] = hour / 10;
        timeDisp[1] = hour % 10;
        timeDisp[2] = min / 10;
        timeDisp[3] = min % 10;
//  timeDisp[2] = sec / 10;
//  timeDisp[3] = sec % 10;
        for (int i = 0; i < 4; i++) { tm1637OCT.display(i, timeDisp[i]); }
    } else {
        timeDisp[0] = hour / 8;
        timeDisp[1] = hour % 8;
        timeDisp[2] = min / 8;
        timeDisp[3] = min % 8;
        for (int i = 0; i < 4; i++) { tm1637OCT.display(i, timeDisp[i]); }
    }

    timeDisp[0] = hour / 16;
    timeDisp[1] = hour % 16;
    timeDisp[2] = min / 16;
    timeDisp[3] = min % 16;
    for (int i = 0; i < 4; i++) { tm1637HEX.display(i, timeDisp[i]); }

    // выводим двоичные часы/минуты/секунды
    // Инициализируем начало приема данных
    digitalWrite(ST_CP, LOW);

    // Последовательная передача данных на пин DS
    shiftOut(DS, SH_CP, MSBFIRST, sec);
    shiftOut(DS, SH_CP, MSBFIRST, min);
    shiftOut(DS, SH_CP, MSBFIRST, hour);
    // Инициализируем окончание передачи данных.
    // Регистры подадут напряжение на указанные выходы
    digitalWrite(ST_CP, HIGH);

}

void time_set() {

    i = 0;

    while (digitalRead(SET)) {
        digitalWrite(13, HIGH);
    }

    while (i < 100) {
        delay(100);
        flag = !flag;
        digitalWrite(13, flag);
        if (digitalRead(HOUR)) {
            delay(50);
            time.settime(-1, -1, (time.Hours == 23 ? 0 : time.Hours + 1), -1, -1, -1, -1);
            hour = time.Hours;
            time_out();
            i = 0;
            flag = LOW;
        }

        if (digitalRead(MIN)) {
            delay(50);
            time.settime(-1, (time.minutes == 59 ? 0 : time.minutes + 1), -1, -1, -1, -1, -1);
            min = time.minutes;
            time_out();
            i = 0;
            flag = LOW;
        }

        i++;

        if (digitalRead(SET)) {
            i = 100;
            flag = LOW;
        }
    }
    flag = LOW;
    digitalWrite(13, flag);
}

void loop() {

    time.gettime();
    sec = time.seconds;
    min = time.minutes;
    hour = time.Hours;

    if (digitalRead(SET)) {
        type = 10;
        time_out();
        i++;
        if (i > 10) {
            digitalWrite(13, HIGH);
            time_set();
        }
    } else {
        time_out();
        i = 0;
        type = 8;
    }

    delay(300);
}