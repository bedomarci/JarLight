#include "Arduino.h"
#include <JC_Button.h>
#include <TaskScheduler.h>
#include "LowPower.h"

#define BTN_ONOFF 2 //8
#define BTN_MODE 4
#define BTN_BRIGHTNESS_SPEED 7
#define BTN_TIMER 12
#define LED_NUM 6
#define TIMER_NUM 6
#define BRIGHTNESS_NUM 5
#define SPEED_NUM 3

#define OFFSET_AMPLITUDE_PERCENTAGE 100
#define OFFSET_SHIFT 4
#define SHOW_TIMER_LENGTH (TASK_SECOND * 2)


enum Mode {
    STATIC,
    SHINING
};

const int ledAddress[]                      = {11, 9, 6, 3, 5, 10};
const long brightnessOptions[BRIGHTNESS_NUM] = {200, 400, 600, 800, 1000};
const long speedOptions[SPEED_NUM]           = {1000, 3500, 10000};
const int timerOption[TIMER_NUM]            = {15, 30, 45, 60, 75, 90};

int   ledEnabled[LED_NUM] = {0, 0, 0, 0, 0, 0};
float ledOffset[LED_NUM]  = {0, 0, 0, 0, 0, 0};

bool    power              = false;
bool    showTimer          = false;
bool feedbackLedState = false;
uint8_t fade               = 100;
uint8_t selectedTimer      = 0;
uint8_t selectedBrightness = 2;
uint8_t selectedSpeed      = 0;
Mode    mode               = STATIC;

void render();

void readOnOff();

void readMode();

void readBrightnessSpeed();

void readTimer();

void shutDown();

void calculateOffset();

void fill(int enabled);
void bootAnimation();
void feedback();

Scheduler scheduler;
Button    btnOnOff(BTN_ONOFF);
Button    btnMode(BTN_MODE);
Button    btnBrightnessSpeed(BTN_BRIGHTNESS_SPEED);
Button    btnTimer(BTN_TIMER);
Task      tRender(TASK_SECOND / 50, TASK_FOREVER, &render, &scheduler);
Task      tTimer(TASK_SECOND, TASK_ONCE, &shutDown, &scheduler);
Task      tShowTimer(TASK_IMMEDIATE, TASK_ONCE, []() {
    showTimer = false;
    render();
}, &scheduler);
Task tFeedback(50, 6, &feedback, &scheduler);

void setup() {
    Serial.begin(9600);
    btnOnOff.begin();
    btnMode.begin();
    btnBrightnessSpeed.begin();
    btnTimer.begin();
    tRender.enable();
    bootAnimation();
}

void bootAnimation() {
    fill(LOW);
    render();
    for (int i        = 0; i < LED_NUM; i++) {
        ledEnabled[i] = 1;
        render();
        delay(50);
    }
    for (int fadeStep = 100; fadeStep >= 0; fadeStep--) {
        fade = fadeStep;
        render();
        delay(2);
    }
    fill(LOW);
    render();
    fade = 100;
    shutDown();
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
        long ledValue = 0;
        if (showTimer) {
            ledValue = (i < selectedTimer) ? brightnessOptions[selectedBrightness] : LOW;
        } else {
            ledValue = ledEnabled[i] * (brightnessOptions[selectedBrightness] + ledOffset[i]);
            if (fade != 100) ledValue = (ledValue * fade) / 100;

        }
        ledValue = constrain(ledValue, 0, 1000);
        ledValue = map(ledValue, 0, 1000, 255, 0);
        analogWrite(ledAddress[i], ledValue);
    }
}

void wakeUp() {
    selectedTimer = 0;
}

void readOnOff() {
    btnOnOff.read();
    if (!btnOnOff.wasReleased()) return;
    power = !power;
    if (power) {
        fill(HIGH);
        tRender.enable();
    } else {
        shutDown();
    }
}

void readMode() {
    btnMode.read();
    if (!btnMode.wasReleased()) return;
    tFeedback.restart();
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
    tFeedback.restart();
    switch (mode) {
        case STATIC:
            selectedBrightness = (selectedBrightness + 1) % BRIGHTNESS_NUM;
            break;
        case SHINING:
            selectedSpeed = (selectedSpeed + 1) % SPEED_NUM;
            break;
    }
    render();
}

void readTimer() {
    btnTimer.read();
    if (!btnTimer.wasReleased()) return;
    tFeedback.restart();
    selectedTimer = (selectedTimer + 1) % (TIMER_NUM + 1);
    showTimer = true;
    tShowTimer.restartDelayed(SHOW_TIMER_LENGTH);
    if (selectedTimer > 0) {
        tTimer.restartDelayed(timerOption[selectedTimer - 1] * TASK_MINUTE);
    } else {
        tTimer.disable();
    }
    render();
}

void calculateOffset() {
    int    shift           = 0;
    double currentRad      = ((double) millis() / speedOptions[selectedSpeed]);
    int    offsetAmplitude = (brightnessOptions[selectedBrightness] * OFFSET_AMPLITUDE_PERCENTAGE) / 100;
    for (int i               = 0; i < LED_NUM; i++) {
        ledOffset[i] = sin(currentRad + shift) * offsetAmplitude;
        shift += OFFSET_SHIFT;
    }
}

void shutDown() {
    power = LOW;
    fill(LOW);
    render();
    tRender.disable();
    attachInterrupt(digitalPinToInterrupt(BTN_ONOFF), wakeUp, LOW);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
    detachInterrupt(digitalPinToInterrupt(BTN_ONOFF));
}

void fill(int enabled) {
    for (int i = 0; i < LED_NUM; i++) {
        ledOffset[i]  = 0;
        ledEnabled[i] = !!enabled;
    }
}

void feedback() {
    if (tFeedback.isFirstIteration()) {
        feedbackLedState = false;
    }
    feedbackLedState = !feedbackLedState;
    digitalWrite(LED_BUILTIN, feedbackLedState);
}