#include <Arduino.h>
#include <iarduino_RTC.h>
#include <Wire.h>
#include <DS3231.h>
#include <TM1637_mult.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <HardwareSerial.h>

//#define DEBUG 0 // Поставить 1 для вывода в консоль отладочной информации

#define CLK_HEX 12 // Пины "циферблатов"
#define DIO_HEX 11
#define CLK_OCT 10
#define DIO_OCT 9

#define BRIGHTNESS 1 // яркость циферблатов

#define ST_CP 3 // pins to RTC
#define SH_CP 4
#define DS 5

#define INDICATOR_LED 13 // Индикатор режима настройки

#define BUTTON_SET 6 // Кнопки для настройки времени
#define BUTTON_HOUR 7
#define BUTTON_MINUTES 8

#define TEMPERATURE_PIN A0 // Для датчика(ов?) температуры используя 1-wire

// Закодированные символы
#define DEGREE_SYMBOL 0b01100011
#define C_SYMBOL 0b00111001
#define r_SYMBOL 0b01010000
#define E_SYMBOL 0b01111001
// Структура сивола:
// ┏ 0 ┓
// 5   1
// ┣ 6 ┫
// 4   2
// ┗ 3 ┛

// ----------------------------------  <Настраиваемые константы> ----------------------------------
// Обновлять время на экране каждые .. мс
#define UPDATE_PERIOD_MS 250

// Менять режим отображения каждые .. с
#define CHANGE_TIME_S 5

// Увеличить для вывода дополнительной информации
#define MODES_COUNT 2

// Задержка в режиме настройки, мс
#define SMALL_DELAY_MS 100
// ================================== </Настраиваемые константы> ==================================

// Каждый режим отображается .. раз
const uint8_t ONE_MODE_CYCLES = (CHANGE_TIME_S * 1000) / UPDATE_PERIOD_MS;

TM1637 screen_HEX(CLK_HEX, DIO_HEX);
TM1637 screen_OCT(CLK_OCT, DIO_OCT);
iarduino_RTC time(RTC_DS3231);

OneWire oneWire(TEMPERATURE_PIN);
DallasTemperature thermo_sensors(&oneWire);
DeviceAddress thermometer_address;
bool thermometer_connected = false;

uint8_t current_tick = 0;
uint8_t type = 8;
uint8_t minutes, sec, hours, i = 0;
bool indicator = LOW;

uint8_t mode = 0; // Режим
// 0 - 8/16-ричная СС
// 1 - Температура и 10-ричная СС

void setup() {
    Wire.begin();
    time.begin();

    uint8_t pins_output[] = {SH_CP, ST_CP, DS, INDICATOR_LED}; // Временный массив со всеми выходными пинами в 1 месте
    for (auto &i: pins_output)
        pinMode(i, OUTPUT);
    uint8_t pins_input[] = {BUTTON_SET, BUTTON_HOUR,
                            BUTTON_MINUTES}; // Временный массив со всеми входными пинами в 1 месте
    for (auto &i: pins_input)
        pinMode(i, INPUT);

    screen_HEX.init();
    screen_OCT.init();

    screen_HEX.set(BRIGHTNESS);
    screen_OCT.set(BRIGHTNESS);

    screen_HEX.point(true);
    screen_OCT.point(true);

#if DEBUG
    Serial.begin(9600);
#endif
    thermometer_connected = thermo_sensors.getAddress(thermometer_address, 0); // Получить адресс термометра
    thermo_sensors.setWaitForConversion(false); // Включаем асинхронность, чтоб не блокировался основной поток
    thermo_sensors.requestTemperaturesByAddress(
            thermometer_address); // Сразу запрос температуры, чтоб была получена к моменту вывода
}

/**
 * Выводит текущее время на экран *@param screen* с *@param base*-ричной СС
 */
void time_to_screen(uint8_t base, TM1637 &screen) {
    screen.point(!(sec & 1)); // Мигаем каждую секунду
    screen.display(0, hours / base);
    screen.display(1, hours % base);
    screen.display(2, minutes / base);
    screen.display(3, minutes % base);
}


void temp_to_screen(TM1637 &screen) {
    screen.point(false);
    if (thermometer_connected) {
        thermo_sensors.requestTemperaturesByAddress(thermometer_address);
        // Асинхронно посылаем асинхронный запрос на асинхронное получение асинхронной температуры
        int8_t res = (int8_t) thermo_sensors.getTempC(thermometer_address);
#if DEBUG
        Serial.println(thermo_sensors.getTempC(thermometer_address));
#endif
        if (res == DEVICE_DISCONNECTED_C)
            thermometer_connected = false;
        else {
            screen.display(0, abs(res) / (int8_t) 10);
            screen.display(1, abs(res) % (int8_t) 10);
            screen.display_raw(2, DEGREE_SYMBOL);
            screen.display_raw(3, C_SYMBOL);
        }
    }
    if (!thermometer_connected) {
        screen.display_raw(0, E_SYMBOL);
        screen.display_raw(1, r_SYMBOL);
        screen.display_raw(2, r_SYMBOL);
        screen.display_raw(3, 0);
#if DEBUG
        Serial.print("Searching sensors...");
#endif
        thermometer_connected = thermo_sensors.getAddress(thermometer_address, 0);
#if DEBUG
        Serial.println(thermometer_connected);
#endif
    }
}

/**
 * Выводит текущее состояние на экраны
 */
void time_output() {
    switch (mode) {
        case 1:
            time_to_screen(10, screen_OCT); // Вывести нормальное время и температуру
            temp_to_screen(screen_HEX);
            break;
        case 0:
        default: // Режим 0 по умолчанию
            time_to_screen(type, screen_OCT); // Вывести время
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

    thermo_sensors.begin();
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
 * Устанавливает текущее время при помощи кнопок
 */
void time_set() {
    while (digitalRead(BUTTON_SET)) { // Пока кнопка зажата, продолжаем все по старому
        digitalWrite(INDICATOR_LED, HIGH);
        delay(SMALL_DELAY_MS);
        update_time();
        time_output();
    }

    for (i = 0; i < 5000 / SMALL_DELAY_MS; ++i) { // ~5 секунд
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
#if DEBUG
    Serial.println(sec);
#endif
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

    // Следующий тик (с проверкой на переполнение)
    ++current_tick;
    if (current_tick >= ONE_MODE_CYCLES * MODES_COUNT)
        current_tick = 0;

    delay(UPDATE_PERIOD_MS);
}
