#ifndef MQ8_H
#define MQ8_H

#include <Arduino.h>

class MQ8 {
private:
    int _pin;
    float _voltage;
    float _ppm;

public:
    MQ8(int pin) {
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