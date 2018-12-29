#include <Arduino.h>
#include <iarduino_RTC.h>
#include <Wire.h>
#include <DS3231.h>
#include <TM1637_mult.h>

#define CLK_HEX 12 // Пины "циферблатов"
#define DIO_HEX 11
#define CLK_OCT 10
#define DIO_OCT 9

#define BRIGHTNESS 1 // brightness of clocks

#define ST_CP 3
#define SH_CP 4 // ???
#define DS 5

#define INDICATOR_LED 13 // led that indicates set mode

#define BUTTON_SET 6 // buttons to set time
#define BUTTON_HOUR 7
#define BUTTON_MINUTES 8

TM1637 screen_HEX(CLK_HEX, DIO_HEX);
TM1637 screen_OCT(CLK_OCT, DIO_OCT);
iarduino_RTC time(RTC_DS3231);

uint8_t type = 8;
uint8_t minutes, sec, hours, i = 0, a = 0;
bool indicator = LOW;

void setup() {
    Wire.begin();
    time.begin();

    pinMode(SH_CP, OUTPUT);
    pinMode(ST_CP, OUTPUT);
    pinMode(DS, OUTPUT);
    pinMode(INDICATOR_LED, OUTPUT);
    pinMode(BUTTON_SET, INPUT);
    pinMode(BUTTON_HOUR, INPUT);
    pinMode(BUTTON_MINUTES, INPUT);

    screen_HEX.init();
    screen_HEX.set(BRIGHTNESS);

    screen_OCT.init();
    screen_OCT.set(BRIGHTNESS);

    screen_HEX.point(true);
    screen_OCT.point(true);
}

/**
 * outputs current time to *@param screen* with *@param base* numeric base
 */
void time_to_screen(uint8_t base, TM1637 &screen) {
    screen.display(0, hours / base);
    screen.display(1, hours % base);
    screen.display(2, minutes / base);
    screen.display(3, minutes % base);
}

/**
 * outputs current time to all displays
 */
void time_output() {
    // set blinking point every second
    screen_HEX.point(!(sec & 1));
    screen_OCT.point(!(sec & 1));

    // output time to digit screens
    time_to_screen(type, screen_OCT);
    time_to_screen(16, screen_HEX);

    // выводим двоичные часы/минуты/секунды
    // Инициализируем начало приема данных
    digitalWrite(ST_CP, LOW);

    // Последовательная передача данных на пин DS
    shiftOut(DS, SH_CP, MSBFIRST, sec);
    shiftOut(DS, SH_CP, MSBFIRST, minutes);
    shiftOut(DS, SH_CP, MSBFIRST, hours);

    // Инициализируем окончание передачи данных.
    // Регистры подадут напряжение на указанные выходы
    digitalWrite(ST_CP, HIGH);
}

/**
 * sets current time by buttons
 */
void time_set() {
    i = 0;

    while (digitalRead(BUTTON_SET))
        digitalWrite(INDICATOR_LED, HIGH);

    while (i < 100) {
        delay(100);
        indicator = !indicator;
        digitalWrite(INDICATOR_LED, indicator);

        if (digitalRead(BUTTON_HOUR)) {
            delay(50);
            time.settime(-1, -1, (time.Hours == 23 ? 0 : time.Hours + 1), -1, -1, -1, -1);
            hours = time.Hours;
            time_output();
            i = 0;
            indicator = LOW;
        }

        if (digitalRead(BUTTON_MINUTES)) {
            delay(50);
            time.settime(-1, (time.minutes == 59 ? 0 : time.minutes + 1), -1, -1, -1, -1, -1);
            minutes = time.minutes;
            time_output();
            i = 0;
            indicator = LOW;
        }

        i++;

        if (digitalRead(BUTTON_SET)) {
            i = 100;
            indicator = LOW;
        }
    }
    indicator = LOW;
    digitalWrite(INDICATOR_LED, indicator);
}

void loop() {
    time.gettime();
    sec = time.seconds;
    minutes = time.minutes;
    hours = time.Hours;

    if (digitalRead(BUTTON_SET)) {
        type = 10;
        time_output();
        i++;
        if (i > 10) {
            digitalWrite(INDICATOR_LED, HIGH);
            time_set();
        }
    } else {
        time_output();
        i = 0;
        type = 8;
    }

    delay(300);
}