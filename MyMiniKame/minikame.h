#ifndef minikame_h
#define minikame_h

#include <Servo.h>
#include "octosnake.h"
#include <Wire.h>
#include "PCA9685.h"

#define SERVO_PULSE_MIN     1   // Minimal pulse length corresponding to 0° in milliseconds
#define SERVO_PULSE_MAX     2   // Maximal pulse length corresponding to 190° in milliseconds
#define SERVO_FREQUENCY    50   // Frequency for Servos

class MiniKame{
public:
    void init();
    void run(float steps, int period);
    void walk(float steps, int period);
    void omniWalk(float steps, int T, bool side, float turn_factor);
    //void backward(float steps, int period);
    void turnL(float steps, int period);
    void turnR(float steps, int period);
    void moonwalkL(float steps, int period);
    void dance(float steps, int period);
    void upDown(float steps, int period);
    void pushUp(float steps, int period);
    void hello();
    void jump();
    void home();
    void zero();
    void frontBack(float steps, int period);

    void setServo(int id, float target);
    void reverseServo(int id);
    float getServo(int id);
    void moveServos(int time, float target[8]);

private:
    Oscillator oscillator[8];
    PCA9685 servoController;
    Servo servo[8];
    int servo_channel[8];
    int trim[8];
    bool reverse[8];
    unsigned long _init_time;
    unsigned long _final_time;
    unsigned long _partial_time;
    float _increment[8];
    float _servo_position[8];

    int angToUsec(float value);
    int angToPWM(float value);
    void execute(float steps, int period[8], int amplitude[8], int offset[8], int phase[8]);
};

#endif
