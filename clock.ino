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

#define ST_CP 3 // pins to RTC
#define SH_CP 4
#define DS 5

#define INDICATOR_LED 13 // led that indicates set mode

#define BUTTON_SET 6 // buttons to set time
#define BUTTON_HOUR 7
#define BUTTON_MINUTES 8

#define UPDATE_PERIOD_MS 300 // update time every UPDATE_PERIOD ms
#define CHANGE_TIME_S 3 // change time between 10-based and 8-based numerical systems every CHANGE_TIME s
#define MODES_COUNT 2

#define SMALL_DELAY_MS 100 // delay in set time mode

// every ONE_MODE_CYCLES increase mode
const uint8_t ONE_MODE_CYCLES = (CHANGE_TIME_S * 1000) / UPDATE_PERIOD_MS;

TM1637 screen_HEX(CLK_HEX, DIO_HEX);
TM1637 screen_OCT(CLK_OCT, DIO_OCT);
iarduino_RTC time(RTC_DS3231);

uint8_t current_tick = 0;
uint8_t type = 8;
uint8_t minutes, sec, hours, i = 0;
bool indicator = LOW;

uint8_t mode = 0; // mode of show
// 0 - standart 8/16-based time
// 1 - temperature and 10-based time

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


void temp_to_oct_screen() {
    //TODO
}

/**
 * outputs current time to all displays
 */
void time_output() {
    // set blinking point every second
    screen_HEX.point(!(sec & 1));
    screen_OCT.point(!(sec & 1));
    switch (mode) { // show different info in different modes
        case 1:
            time_to_screen(10, screen_HEX); // output time and temperature to digit screens
            temp_to_oct_screen();
            break;
        case 0:
        default: // use mode 0 as fallback
            time_to_screen(type, screen_OCT); // output time to digit screens
            time_to_screen(16, screen_HEX);
    }

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

void on_set_time() {
    time_output();
    i = 0;
    delay(SMALL_DELAY_MS);
    indicator = LOW;
}

void update_time() {
    time.gettime();
    sec = time.seconds;
    minutes = time.minutes;
    hours = time.Hours;
}

/**
 * sets current time by buttons
 */
void time_set() {
    while (digitalRead(BUTTON_SET)) { // keep going while SET button is still pressed
        digitalWrite(INDICATOR_LED, HIGH);
        delay(SMALL_DELAY_MS);
        update_time();
        time_output();
    }

    for (i = 0; i < 5000 / SMALL_DELAY_MS; ++i) { // timeout about 5 seconds
        delay(SMALL_DELAY_MS);
        indicator = !indicator;
        digitalWrite(INDICATOR_LED, indicator);

        while (digitalRead(BUTTON_HOUR)) {
            time.settime(-1, -1, (time.Hours == 23 ? 0 : time.Hours + 1), -1, -1, -1, -1);
            hours = time.Hours;
            on_set_time();
        }

        while (digitalRead(BUTTON_MINUTES)) {
            time.settime(-1, (time.minutes == 59 ? 0 : time.minutes + 1), -1, -1, -1, -1, -1);
            minutes = time.minutes;
            on_set_time();
        }

        if (digitalRead(BUTTON_SET)) {
            indicator = LOW;
            break;
        }
    }
    indicator = LOW;
    digitalWrite(INDICATOR_LED, indicator);
}

void loop() {
    update_time();

    if (digitalRead(BUTTON_SET)) {
        type = 10;
        mode = 0;
        i++;
        time_output();
        if (i > 10) {
            digitalWrite(INDICATOR_LED, HIGH);
            time_set();
        }
    } else {
        i = 0;
        mode = current_tick / ONE_MODE_CYCLES;
        type = 8;
        time_output();
    }

    // increase tick and check for overflowing
    ++current_tick;
    if (current_tick >= ONE_MODE_CYCLES * MODES_COUNT)
        current_tick = 0;

    delay(UPDATE_PERIOD_MS);
}
