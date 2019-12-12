#include "Arduino.h"
#include <JC_Button.h>
#include <TaskScheduler.h>
#include "LowPower.h"

#define BTN_ONOFF 1
#define BTN_MODE 1
#define BTN_BRIGHTNESS_SPEED 1
#define BTN_TIMER 1
#define LED_NUM 6
#define TIMER_NUM 6
#define BRIGHTNESS_NUM 5
#define SPEED_NUM 3

#define OFFSET_AMPLITUDE 100
#define OFFSET_SHIFT 1.1
#define SHOW_TIMER_LENGTH (TASK_SECOND * 2)


enum Mode {
    STATIC,
    SHINING
};

const int ledAddress[] = {1, 2, 3, 4, 5, 6};
const int brightnessOptions[BRIGHTNESS_NUM] = {200, 400, 600, 700, 800};
const int speedOptions[SPEED_NUM] = {1000, 1500, 2000};
const int timerOption[TIMER_NUM] = {15, 30, 45, 60, 75, 90};

int ledEnabled[LED_NUM] = {0, 0, 0, 0, 0, 0};
float ledOffset[LED_NUM] = {0, 0, 0, 0, 0, 0};



bool power = false;
bool timer = false;
bool showTimer = false;
uint8_t selectedTimer = 0;
uint8_t selectedBrightness = 0;
uint8_t selectedSpeed = 0;
Mode mode = STATIC;

void render();

void readOnOff();

void readMode();

void readBrightnessSpeed();

void readTimer();

void shutDown();

void calculateOffset();

void fill(int enabled);

Scheduler scheduler;
Button btnOnOff(BTN_ONOFF);
Button btnMode(BTN_MODE);
Button btnBrightnessSpeed(BTN_BRIGHTNESS_SPEED);
Button btnTimer(BTN_TIMER);
Task tRender(TASK_SECOND / 50, TASK_FOREVER, &render, &scheduler);
Task tTimer(TASK_SECOND, TASK_ONCE, &shutDown, &scheduler);
Task tShowTimer(TASK_IMMEDIATE, TASK_ONCE, [=]() { showTimer = false; }, &scheduler);


void setup() {
    btnOnOff.begin();
    btnMode.begin();
    btnBrightnessSpeed.begin();
    btnTimer.begin();
    tRender.enable();
}

void loop() {
    readOnOff();
    readMode();
    readBrightnessSpeed();
    readTimer();
    scheduler.execute();
}

void render() {
    if (mode == SHINING)
        calculateOffset();
    for (int i = 0; i < LED_NUM; i++) {
        int ledValue = 0;
        if (showTimer) {
            ledValue = (i <= selectedTimer) ? brightnessOptions[selectedBrightness] : LOW;
        } else {
            ledValue = ledEnabled[i] * (brightnessOptions[selectedBrightness] + ledOffset[i]);
        }
        ledValue = map(ledValue, 0, 1000, 0, 255);
        analogWrite(ledAddress[i], ledValue);
    }
}

void wakeUp() {
    detachInterrupt(digitalPinToInterrupt(BTN_ONOFF));
    timer = false;
}

void readOnOff() {
    btnOnOff.read();
    if (!btnOnOff.wasReleased()) return;
    power = !power;
    if (!power) {
        shutDown();
    }
}

void readMode() {
    btnMode.read();
    if (!btnMode.wasReleased()) return;
    mode = (mode == STATIC) ? SHINING : STATIC;

    switch (mode) {
        case STATIC:
            tRender.disable();
            fill(HIGH);
            render();
            break;
        case SHINING:
            tRender.enable();
            break;
    }
}

void readBrightnessSpeed() {
    btnBrightnessSpeed.read();
    if (!btnBrightnessSpeed.wasReleased()) return;
    switch (mode) {
        case STATIC:
            selectedBrightness = (selectedBrightness + 1) % BRIGHTNESS_NUM;
            break;
        case SHINING:
            selectedSpeed = (selectedSpeed + 1) % SPEED_NUM;
            break;
    }
}

void readTimer() {
    btnTimer.read();
    if (!btnTimer.wasReleased()) return;
    showTimer = true;
    tShowTimer.restartDelayed(SHOW_TIMER_LENGTH);
    tTimer.restartDelayed(timerOption[selectedTimer]);
    selectedTimer = (selectedTimer + 1) % TIMER_NUM;
    timer = true;
}

void calculateOffset() {
    int shift = 0;
    float currentRad = millis() / speedOptions[selectedSpeed];
    for (int i = 0; i < LED_NUM; i++) {
        ledOffset[i] = sin(currentRad + shift) * OFFSET_AMPLITUDE;
        shift += OFFSET_SHIFT;
    }
}

void shutDown() {
    fill(LOW);
    attachInterrupt(digitalPinToInterrupt(BTN_ONOFF), wakeUp, LOW);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    timer = false;
}

void fill(int enabled) {
    for (int i = 0; i < LED_NUM; i++) {
        ledOffset[i] = 0;
        ledEnabled[i] = !!enabled;
    }
}