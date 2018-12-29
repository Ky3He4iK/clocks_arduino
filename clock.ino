#include <Arduino.h>
#include <iarduino_RTC.h>
#include <Wire.h>
#include <DS3231.h>
#include <TM1637_mult.h>

#define CLK_HEX 12 // Пины "циферблатов"
#define DIO_HEX 11
#define CLK_OCT 10
#define DIO_OCT 9

#define brightness 1 // brightness of clocks

#define ST_CP 3
#define SH_CP 4 // ???
#define DS 5

const uint8_t SET = 6;
const uint8_t HOUR = 7;
const uint8_t MIN = 8;

TM1637 screen_HEX(CLK_HEX, DIO_HEX);
TM1637 screen_OCT(CLK_OCT, DIO_OCT);
iarduino_RTC time(RTC_DS3231);

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

    screen_HEX.init();
    screen_HEX.set(brightness);

    screen_OCT.init();
    screen_OCT.set(brightness);

    screen_HEX.point(true);
    screen_OCT.point(true);
}

void time_out() {
    screen_HEX.point(!(sec & 1));
    screen_OCT.point(!(sec & 1));

    if (type == 10) {
        timeDisp[0] = hour / 10;
        timeDisp[1] = hour % 10;
        timeDisp[2] = min / 10;
        timeDisp[3] = min % 10;
        for (int i = 0; i < 4; i++) {
            screen_OCT.display(i, timeDisp[i]);
        }
    } else {
        timeDisp[0] = hour / 8;
        timeDisp[1] = hour % 8;
        timeDisp[2] = min / 8;
        timeDisp[3] = min % 8;
        for (int i = 0; i < 4; i++)
            screen_OCT.display(i, timeDisp[i]);
    }

    timeDisp[0] = hour / 16;
    timeDisp[1] = hour % 16;
    timeDisp[2] = min / 16;
    timeDisp[3] = min % 16;
    for (int i = 0; i < 4; i++)
        screen_HEX.display(i, timeDisp[i]);

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

    while (digitalRead(SET))
        digitalWrite(13, HIGH);

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