#ifndef MQ4_H
#define MQ4_H

#include <Arduino.h>

class MQ4 {
private:
    int _pin;
    float _voltage;
    float _ppm;

public:
    MQ4(int pin) {
        _pin = pin;
        pinMode(_pin, INPUT);
    }

    float getPPM() {
        _voltage = analogRead(_pin) * (3.3 / 1023.0);
        _ppm = _voltage * 100; // Basic conversion, adjust based on sensor calibration
        return _ppm;
    }
};

#endif 