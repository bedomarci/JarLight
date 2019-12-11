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

enum Mode {
    STATIC,
    SHINING
};

const int ledAddresses[LED_NUM] = {1, 2, 3, 4, 5, 6};
const int brightnessOptions[BRIGHTNESS_NUM] = {20, 40, 60, 70, 80};
const int speedOptions[SPEED_NUM] = {10, 20, 30};
const int timerOption[TIMER_NUM] = {15, 30, 45, 60, 75, 90};


bool power = false;
bool timer = false;
uint8_t selectedTimer = 0;
Mode mode = STATIC;

void render();

void readOnOff();

void readMode();

void readBrightnessSpeed();

void readTimer();

void shutDown();

Scheduler scheduler;
Button btnOnOff(BTN_ONOFF);
Button btnMode(BTN_MODE);
Button btnBrightnessSpeed(BTN_BRIGHTNESS_SPEED);
Button btnTimer(BTN_TIMER);
Task tRender(TASK_SECOND / 50, TASK_FOREVER, &render, &scheduler);
Task tTimer(TASK_SECOND, TASK_ONCE, &shutDown, &scheduler);


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


}

void readBrightnessSpeed() {
    btnBrightnessSpeed.read();
    if (!btnBrightnessSpeed.wasReleased()) return;

}

void readTimer() {
    btnTimer.read();
    if (!btnTimer.wasReleased()) return;
    tTimer.restartDelayed(timerOption[selectedTimer]);
    selectedTimer = selectedTimer + 1 % TIMER_NUM;
}


void shutDown() {
    attachInterrupt(digitalPinToInterrupt(BTN_ONOFF), wakeUp, LOW);
    LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
}