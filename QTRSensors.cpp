#include <stdlib.h>
#include "QTRSensors.h"
#include <Arduino.h>


// Base class constructor
QTRSensors::QTRSensors() :
    calibratedMinimumOn(nullptr),
    calibratedMaximumOn(nullptr),
    calibratedMinimumOff(nullptr),
    calibratedMaximumOff(nullptr),
    _pins(nullptr)
{
    // empty
}


// Base class data member initialization (called by derived class init())
void QTRSensors::init(unsigned char *pins, unsigned char numSensors,
  unsigned char emitterPin)
{
    calibratedMinimumOn = nullptr;
    calibratedMaximumOn = nullptr;
    calibratedMinimumOff = nullptr;
    calibratedMaximumOff = nullptr;

    _lastValue = 0; // assume initially that the line is left.

    if (numSensors > QTR_MAX_SENSORS)
        _numSensors = QTR_MAX_SENSORS;
    else
        _numSensors = numSensors;

    if (_pins == nullptr)
    {
        _pins = (unsigned char*)malloc(sizeof(unsigned char)*_numSensors);
        if (_pins == nullptr)
            return;
    }

    unsigned char i;
    for (i = 0; i < _numSensors; i++)
    {
        _pins[i] = pins[i];
    }

    _emitterPin = emitterPin;
}


// Reads the sensor values into an array. There *MUST* be space
// for as many values as there were sensors specified in the constructor.
// Example usage:
// unsigned int sensor_values[8];
// sensors.read(sensor_values);
// The values returned are a measure of the reflectance in abstract units,
// with higher values corresponding to lower reflectance (e.g. a black
// surface or a void).
void QTRSensors::read(unsigned int *sensor_values, unsigned char readMode)
{
    unsigned char i;

    if (readMode == QTR_EMITTERS_ON || readMode == QTR_EMITTERS_ON_AND_OFF)
        emittersOn();
    else if (readMode == QTR_EMITTERS_OFF)
        emittersOff();

    readPrivate(sensor_values);

    if (readMode != QTR_EMITTERS_MANUAL)
        emittersOff();

    if (readMode == QTR_EMITTERS_ON_AND_OFF)
    {
        unsigned int off_values[QTR_MAX_SENSORS];
        readPrivate(off_values);

        for(i=0;i<_numSensors;i++)
        {
            sensor_values[i] += _maxValue - off_values[i];
        }
    }
}


// Turn the IR LEDs off and on.  These are mainly for use by the read
// method, and calling these functions before or after reading the sensors
// will have no effect on the readings unless the readMode is
// QTR_EMITTERS_MANUAL, but you may wish to use these for testing purposes.
void QTRSensors::emittersOff()
{
    if (_emitterPin == QTR_NO_EMITTER_PIN)
        return;
    pinMode(_emitterPin, OUTPUT);
    digitalWrite(_emitterPin, LOW);
    delayMicroseconds(200);
}

void QTRSensors::emittersOn()
{
    if (_emitterPin == QTR_NO_EMITTER_PIN)
        return;
    pinMode(_emitterPin, OUTPUT);
    digitalWrite(_emitterPin, HIGH);
    delayMicroseconds(200);
}

// Resets the calibration.
void QTRSensors::resetCalibration()
{
    unsigned char i;
    for(i=0;i<_numSensors;i++)
    {
        if(calibratedMinimumOn)
            calibratedMinimumOn[i] = _maxValue;
        if(calibratedMinimumOff)
            calibratedMinimumOff[i] = _maxValue;
        if(calibratedMaximumOn)
            calibratedMaximumOn[i] = 0;
        if(calibratedMaximumOff)
            calibratedMaximumOff[i] = 0;
    }
}

// Reads the sensors 10 times and uses the results for
// calibration.  The sensor values are not returned; instead, the
// maximum and minimum values found over time are stored internally
// and used for the readCalibrated() method.
void QTRSensors::calibrate(unsigned char readMode)
{
    if(readMode == QTR_EMITTERS_ON_AND_OFF || readMode == QTR_EMITTERS_ON)
    {
        calibrateOnOrOff(&calibratedMinimumOn,
                         &calibratedMaximumOn,
                         QTR_EMITTERS_ON);
    }


    if(readMode == QTR_EMITTERS_ON_AND_OFF || readMode == QTR_EMITTERS_OFF)
    {
        calibrateOnOrOff(&calibratedMinimumOff,
                         &calibratedMaximumOff,
                         QTR_EMITTERS_OFF);
    }
}

void QTRSensors::calibrateOnOrOff(unsigned int **calibratedMinimum,
                                        unsigned int **calibratedMaximum,
                                        unsigned char readMode)
{
    int i;
    unsigned int sensor_values[16];
    unsigned int max_sensor_values[16];
    unsigned int min_sensor_values[16];

    // Allocate the arrays if necessary.
    if(*calibratedMaximum == 0)
    {
        *calibratedMaximum = (unsigned int*)malloc(sizeof(unsigned int)*_numSensors);

        // If the malloc failed, don't continue.
        if(*calibratedMaximum == 0)
            return;

        // Initialize the max and min calibrated values to values that
        // will cause the first reading to update them.

        for(i=0;i<_numSensors;i++)
            (*calibratedMaximum)[i] = 0;
    }
    if(*calibratedMinimum == 0)
    {
        *calibratedMinimum = (unsigned int*)malloc(sizeof(unsigned int)*_numSensors);

        // If the malloc failed, don't continue.
        if(*calibratedMinimum == 0)
            return;

        for(i=0;i<_numSensors;i++)
            (*calibratedMinimum)[i] = _maxValue;
    }

    int j;
    for(j=0;j<10;j++)
    {
        read(sensor_values,readMode);
        for(i=0;i<_numSensors;i++)
        {
            // set the max we found THIS time
            if(j == 0 || max_sensor_values[i] < sensor_values[i])
                max_sensor_values[i] = sensor_values[i];

            // set the min we found THIS time
            if(j == 0 || min_sensor_values[i] > sensor_values[i])
                min_sensor_values[i] = sensor_values[i];
        }
    }

    // record the min and max calibration values
    for(i=0;i<_numSensors;i++)
    {
        if(min_sensor_values[i] > (*calibratedMaximum)[i])
            (*calibratedMaximum)[i] = min_sensor_values[i];
        if(max_sensor_values[i] < (*calibratedMinimum)[i])
            (*calibratedMinimum)[i] = max_sensor_values[i];
    }
}


// Returns values calibrated to a value between 0 and 1000, where
// 0 corresponds to the minimum value read by calibrate() and 1000
// corresponds to the maximum value.  Calibration values are
// stored separately for each sensor, so that differences in the
// sensors are accounted for automatically.
void QTRSensors::readCalibrated(unsigned int *sensor_values, unsigned char readMode)
{
    int i;

    // if not calibrated, do nothing
    if(readMode == QTR_EMITTERS_ON_AND_OFF || readMode == QTR_EMITTERS_OFF)
        if(!calibratedMinimumOff || !calibratedMaximumOff)
            return;
    if(readMode == QTR_EMITTERS_ON_AND_OFF || readMode == QTR_EMITTERS_ON)
        if(!calibratedMinimumOn || !calibratedMaximumOn)
            return;

    // read the needed values
    read(sensor_values,readMode);

    for(i=0;i<_numSensors;i++)
    {
        unsigned int calmin,calmax;
        unsigned int denominator;

        // find the correct calibration
        if(readMode == QTR_EMITTERS_ON)
        {
            calmax = calibratedMaximumOn[i];
            calmin = calibratedMinimumOn[i];
        }
        else if(readMode == QTR_EMITTERS_OFF)
        {
            calmax = calibratedMaximumOff[i];
            calmin = calibratedMinimumOff[i];
        }
        else // QTR_EMITTERS_ON_AND_OFF
        {

            if(calibratedMinimumOff[i] < calibratedMinimumOn[i]) // no meaningful signal
                calmin = _maxValue;
            else
                calmin = calibratedMinimumOn[i] + _maxValue - calibratedMinimumOff[i]; // this won't go past _maxValue

            if(calibratedMaximumOff[i] < calibratedMaximumOn[i]) // no meaningful signal
                calmax = _maxValue;
            else
                calmax = calibratedMaximumOn[i] + _maxValue - calibratedMaximumOff[i]; // this won't go past _maxValue
        }

        denominator = calmax - calmin;

        signed int x = 0;
        if(denominator != 0)
            x = (((signed long)sensor_values[i]) - calmin)
                * 1000 / denominator;
        if(x < 0)
            x = 0;
        else if(x > 1000)
            x = 1000;
        sensor_values[i] = x;
    }

}


// Operates the same as read calibrated, but also returns an
// estimated position of the robot with respect to a line. The
// estimate is made using a weighted average of the sensor indices
// multiplied by 1000, so that a return value of 0 indicates that
// the line is directly below sensor 0, a return value of 1000
// indicates that the line is directly below sensor 1, 2000
// indicates that it's below sensor 2000, etc.  Intermediate
// values indicate that the line is between two sensors.  The
// formula is:
//
//    0*value0 + 1000*value1 + 2000*value2 + ...
//   --------------------------------------------
//         value0  +  value1  +  value2 + ...
//
// By default, this function assumes a dark line (high values)
// surrounded by white (low values).  If your line is light on
// black, set the optional second argument white_line to true.  In
// this case, each sensor value will be replaced by (1000-value)
// before the averaging.
int QTRSensors::readLine(unsigned int *sensor_values,
    unsigned char readMode, unsigned char white_line)
{
    unsigned char i, on_line = 0;
    unsigned long avg; // this is for the weighted total, which is long
                       // before division
    unsigned int sum; // this is for the denominator which is <= 64000

    readCalibrated(sensor_values, readMode);

    avg = 0;
    sum = 0;

    for(i=0;i<_numSensors;i++) {
        int value = sensor_values[i];
        if(white_line)
            value = 1000-value;

        // keep track of whether we see the line at all
        if(value > 200) {
            on_line = 1;
        }

        // only average in values that are above a noise threshold
        if(value > 50) {
            avg += (long)(value) * (i * 1000);
            sum += value;
        }
    }

    if(!on_line)
    {
        // If it last read to the left of center, return 0.
        if(_lastValue < (_numSensors-1)*1000/2)
            return 0;

        // If it last read to the right of center, return the max.
        else
            return (_numSensors-1)*1000;

    }

    _lastValue = avg/sum;

    return _lastValue;
}


// Dimmable base class initialization for one bank or combined banks
// (1 emitter control pin) (called by derived class init())
// (overrides QTRSensors::init)
void QTRDimmable::init(unsigned char *pins, unsigned char numSensors,
  unsigned char emitterPin)
{
    QTRSensors::init(pins, numSensors, emitterPin);
    _evenEmitterPin = QTR_NO_EMITTER_PIN;
}

// Dimmable base class initialization for separate banks
// (2 emitter control pins) (called by derived class init())
void QTRDimmable::init(unsigned char *pins, unsigned char numSensors,
  unsigned char oddEmitterPin, unsigned char evenEmitterPin)
{
    QTRSensors::init(pins, numSensors, oddEmitterPin);
    _evenEmitterPin = evenEmitterPin;
}


// Read function for dimmable sensors with support for additional (odd/even)
// readModes. The odd/even readModes only work if the object was initialized
// with two emitter pins.
// (overrides QTRSensors::read)
void QTRDimmable::read(unsigned int *sensor_values, unsigned char readMode)
{
    if (readMode == QTR_EMITTERS_ODD_EVEN || readMode == QTR_EMITTERS_ODD_EVEN_AND_OFF)
    {
        emitterBankSelect(QTR_BANK_ODD);

        // Read the odd-numbered sensors.
        // (readPrivate takes a 0-based array index, so start = 0 to start with
        // the first sensor)
        readPrivate(sensor_values, 2, 0);

        emitterBankSelect(QTR_BANK_EVEN);

        // Read the even-numbered sensors.
        // (readPrivate takes a 0-based array index, so start = 1 to start with
        // the second sensor)
        readPrivate(sensor_values, 2, 1);
    }
    else
    {
        if (readMode == QTR_EMITTERS_ON || readMode == QTR_EMITTERS_ON_AND_OFF)
            emittersOn();
        else if (readMode == QTR_EMITTERS_OFF)
            emittersOff();

        readPrivate(sensor_values);
    }
    if (readMode != QTR_EMITTERS_MANUAL)
        emittersOff();

    if (readMode == QTR_EMITTERS_ON_AND_OFF || readMode == QTR_EMITTERS_ODD_EVEN_AND_OFF)
    {
        unsigned char i;

        unsigned int off_values[QTR_MAX_SENSORS];
        readPrivate(off_values);

        for(i=0;i<_numSensors;i++)
        {
            sensor_values[i] += _maxValue - off_values[i];
        }
    }
}


// Turns off emitters for dimmable sensors with one bank or combined banks.
// (overrides QTRSensors::emittersOff)
void QTRDimmable::emittersOff()
{
    if (_evenEmitterPin != QTR_NO_EMITTER_PIN)
    {
        // Stop driving even emitter ctrl pin
        // and let odd pin control all emitters
        pinMode (_evenEmitterPin, INPUT);
    }
    emittersOff(QTR_BANK_ODD);
}

// Turns off emitters for selected bank on dimmable sensors with separate
// banks. If wait = false, returns immediately without waiting for emitters
// to actually turn off.
void QTRDimmable::emittersOff(unsigned char bank, bool wait)
{
    unsigned char ePin;

    if (bank == QTR_BANK_ODD)
    {
        ePin = _emitterPin;
    }
    else // bank == QTR_BANK_EVEN
    {
        ePin = _evenEmitterPin;
    }

    if (ePin == QTR_NO_EMITTER_PIN)
        return;

    pinMode(ePin, OUTPUT);
    digitalWrite(ePin, LOW);

    if (wait)
    {
        // Wait for emitters to turn off. (minimum 1 ms; add some margin)
        delayMicroseconds(1200);
    }
}

// Turns on emitters for dimmable sensors with one bank or combined banks.
// (overrides QTRSensors::emittersOn)
void QTRDimmable::emittersOn()
{
    if (_evenEmitterPin != QTR_NO_EMITTER_PIN)
    {
        // Stop driving even emitter pin and let odd pin control all emitters.
        pinMode (_evenEmitterPin, INPUT);
    }
    emittersOn(QTR_BANK_ODD);
}

// Turns on emitters for selected bank on dimmable sensors with separate
// banks. If wait = false, returns immediately without waiting for emitters
// to actually turn on.
void QTRDimmable::emittersOn(unsigned char bank, bool wait)
{
    unsigned char ePin;

    if (bank == QTR_BANK_ODD)
    {
        ePin = _emitterPin;
    }
    else // bank == QTR_BANK_EVEN
    {
        ePin = _evenEmitterPin;
    }

    if (ePin == QTR_NO_EMITTER_PIN)
        return;

    pinMode(ePin, OUTPUT);
    digitalWrite(ePin, HIGH);
    unsigned int turnOnStart = micros();

    noInterrupts();
    for (unsigned char i = 0; i < _dimmingLevel; i++)
    {
        delayMicroseconds(1);
        digitalWrite(ePin, LOW);
        delayMicroseconds(1);
        digitalWrite(ePin, HIGH);
    }
    interrupts();

    if (wait)
    {
        // Wait for emitters to turn on: make sure it's been at least 300 us
        // since the emitter pin first went high before returning.
        // (minimum 250 us; add some margin)
        while ((unsigned int)(micros() - turnOnStart) < 300)
        {
            delayMicroseconds(10);
        }
    }
}


// Turns on the selected bank and turns off the other bank with optimized
// timing (no unnecessary waits compared to calling emittersOff and
// emittersOn separately, but still waits for both banks of emitters to be
// in the right states before returning).
void QTRDimmable::emitterBankSelect(unsigned char bank)
{
    unsigned char offBank;

    if (bank == QTR_BANK_ODD)
    {
        offBank = QTR_BANK_EVEN;
    }
    else // bank == QTR_BANK_EVEN
    {
        offBank = QTR_BANK_ODD;
    }

    // Turn off the off-bank; don't wait before proceeding, but record the time.
    emittersOff(offBank, false);
    unsigned int turnOffStart = micros();

    // Turn on the on-bank.
    emittersOn(bank);

    // Finish waiting for the off-bank emitters to turn off: make sure it's been
    // at least 1200 us since the off-bank was turned off before returning.
    // (minimum 1 ms; add some margin)
    while ((unsigned int)(micros() - turnOffStart) < 1200)
    {
        delayMicroseconds(10);
    }
}


// Sets the dimming level (0-31). A dimming level of 0 corresponds
// to full current and brightness, with higher dimming levels meaning lower
// currents; see the product page for your sensor for details.
void QTRDimmable::setDimmingLevel(unsigned char dimmingLevel)
{
    if (dimmingLevel > 31)
    {
        dimmingLevel = 31;
    }
    _dimmingLevel = dimmingLevel;
}


// Derived RC class constructor
QTRSensorsRC::QTRSensorsRC(unsigned char* pins,
  unsigned char numSensors, unsigned int timeout, unsigned char emitterPin)
{
    init(pins, numSensors, timeout, emitterPin);
}


// The array 'pins' contains the Arduino pin number for each sensor.

// 'numSensors' specifies the length of the 'pins' array (i.e. the
// number of QTR-RC sensors you are using).  numSensors must be
// no greater than 16.

// 'timeout' specifies the length of time in microseconds beyond
// which you consider the sensor reading completely black.  That is to say,
// if the pulse length for a pin exceeds 'timeout', pulse timing will stop
// and the reading for that pin will be considered full black.
// It is recommended that you set timeout to be between 1000 and
// 3000 us, depending on things like the height of your sensors and
// ambient lighting.  Using timeout allows you to shorten the
// duration of a sensor-reading cycle while still maintaining
// useful analog measurements of reflectance

// 'emitterPin' is the Arduino pin that controls the IR LEDs on the 8RC
// modules.  If you are using a 1RC (i.e. if there is no emitter pin),
// or if you just want the emitters on all the time and don't want to
// use an I/O pin to control it, use a value of 255 (QTR_NO_EMITTER_PIN).
void QTRSensorsRC::init(unsigned char* pins,
    unsigned char numSensors, unsigned int timeout, unsigned char emitterPin)
{
    QTRSensors::init(pins, numSensors, emitterPin);

    _maxValue = timeout;
}


// Reads the sensor values into an array. There *MUST* be space
// for as many values as there were sensors specified in the constructor.
// Example usage:
// unsigned int sensor_values[8];
// sensors.read(sensor_values);
// ...
// The values returned are in microseconds and range from 0 to
// timeout (as specified in the constructor).
// A 'step' of n means that the first of every n sensors is read, starting
// with 'start' (this is 0-indexed, so start = 0 means start with the first
// sensor). For example, step = 2, start = 1 means read the *even-numbered*
// sensors.
void QTRSensorsRC::readPrivate(unsigned int *sensor_values, unsigned char step, unsigned char start)
{
    unsigned char i;

    if (_pins == 0)
        return;

    for(i = start; i < _numSensors; i += step)
    {
        sensor_values[i] = _maxValue;
        pinMode(_pins[i], OUTPUT);      // make sensor line an output (drives low briefly, but doesn't matter)
        digitalWrite(_pins[i], HIGH);   // drive sensor line high
    }

    delayMicroseconds(10);              // charge lines for 10 us

    for(i = start; i < _numSensors; i += step)
    {
        pinMode(_pins[i], INPUT);       // make sensor line an input
        digitalWrite(_pins[i], LOW);    // important: disable internal pull-up!
    }

    unsigned long startTime = micros();
    while (micros() - startTime < _maxValue)
    {
        unsigned int time = micros() - startTime;
        for (i = start; i < _numSensors; i += step)
        {
            if (digitalRead(_pins[i]) == LOW && time < sensor_values[i])
                sensor_values[i] = time;
        }
    }
}


// Derived Analog class constructor
QTRSensorsAnalog::QTRSensorsAnalog(unsigned char* analogPins,
  unsigned char numSensors, unsigned char numSamplesPerSensor,
  unsigned char emitterPin)
{
    init(analogPins, numSensors, numSamplesPerSensor, emitterPin);
}


// the array 'pins' contains the Arduino analog pin assignment for each
// sensor.  For example, if pins is {0, 1, 7}, sensor 1 is on
// Arduino analog input 0, sensor 2 is on Arduino analog input 1,
// and sensor 3 is on Arduino analog input 7.

// 'numSensors' specifies the length of the 'analogPins' array (i.e. the
// number of QTR-A sensors you are using).  numSensors must be
// no greater than 16.

// 'numSamplesPerSensor' indicates the number of 10-bit analog samples
// to average per channel (i.e. per sensor) for each reading.  The total
// number of analog-to-digital conversions performed will be equal to
// numSensors*numSamplesPerSensor.  Note that it takes about 100 us to
// perform a single analog-to-digital conversion, so:
// if numSamplesPerSensor is 4 and numSensors is 6, it will take
// 4 * 6 * 100 us = ~2.5 ms to perform a full readLine().
// Increasing this parameter increases noise suppression at the cost of
// sample rate.  The recommended value is 4.

// 'emitterPin' is the Arduino pin that controls the IR LEDs on the 8RC
// modules.  If you are using a 1RC (i.e. if there is no emitter pin),
// or if you just want the emitters on all the time and don't want to
// use an I/O pin to control it, use a value of 255 (QTR_NO_EMITTER_PIN).
void QTRSensorsAnalog::init(unsigned char* analogPins,
    unsigned char numSensors, unsigned char numSamplesPerSensor,
    unsigned char emitterPin)
{
    QTRSensors::init(analogPins, numSensors, emitterPin);

    _numSamplesPerSensor = numSamplesPerSensor;
    _maxValue = 1023; // this is the maximum returned by the A/D conversion
}


// Reads the sensor values into an array. There *MUST* be space
// for as many values as there were sensors specified in the constructor.
// Example usage:
// unsigned int sensor_values[8];
// sensors.read(sensor_values);
// The values returned are a measure of the reflectance in terms of a
// 10-bit ADC average with higher values corresponding to lower
// reflectance (e.g. a black surface or a void).
// A 'step' of n means that the first of every n sensors is read, starting
// with 'start' (this is 0-indexed, so start = 0 means start with the first
// sensor). For example, step = 2, start = 1 means read the *even-numbered*
// sensors.
void QTRSensorsAnalog::readPrivate(unsigned int *sensor_values, unsigned char step, unsigned char start)
{
    unsigned char i, j;

    if (_pins == 0)
        return;

    // reset the values
    for(i = start; i < _numSensors; i += step)
        sensor_values[i] = 0;

    for (j = 0; j < _numSamplesPerSensor; j++)
    {
        for (i = start; i < _numSensors; i += step)
        {
            sensor_values[i] += analogRead(_pins[i]);   // add the conversion result
        }
    }

    // get the rounded average of the readings for each sensor
    for (i = start; i < _numSensors; i += step)
        sensor_values[i] = (sensor_values[i] + (_numSamplesPerSensor >> 1)) /
            _numSamplesPerSensor;
}


// Derived Dimmable RC class constructor (one bank or combined banks)
QTRDimmableRC::QTRDimmableRC(unsigned char* pins,
  unsigned char numSensors, unsigned int timeout,
  unsigned char emitterPin)
{
    init(pins, numSensors, timeout, emitterPin);
}

// Derived Dimmable RC class constructor (separate banks)
QTRDimmableRC::QTRDimmableRC(unsigned char* pins,
  unsigned char numSensors, unsigned int timeout,
  unsigned char oddEmitterPin, unsigned char evenEmitterPin)
{
    init(pins, numSensors, timeout, oddEmitterPin, evenEmitterPin);
}


// initialization for dimmable RC sensors with one bank or combined banks
// (1 emitter control pin)
// (overrides QTRSensorsAnalog::init)
void QTRDimmableRC::init(unsigned char* pins,
    unsigned char numSensors, unsigned int timeout, unsigned char emitterPin)
{
    QTRDimmable::init(pins, numSensors, emitterPin);

    _maxValue = timeout;
}

// initialization for dimmable RC sensors with separate banks
// (2 emitter control pins)
void QTRDimmableRC::init(unsigned char* pins,
    unsigned char numSensors, unsigned int timeout,
    unsigned char oddEmitterPin, unsigned char evenEmitterPin)
{
    QTRDimmable::init(pins, numSensors, oddEmitterPin, evenEmitterPin);

    _maxValue = timeout;
}


// Derived Dimmable Analog class constructor (one bank or combined banks)
QTRDimmableAnalog::QTRDimmableAnalog(unsigned char* analogPins,
  unsigned char numSensors, unsigned char numSamplesPerSensor,
  unsigned char emitterPin)
{
    init(analogPins, numSensors, numSamplesPerSensor, emitterPin);
}

// Derived Dimmable Analog class constructor (separate banks)
QTRDimmableAnalog::QTRDimmableAnalog(unsigned char* analogPins,
  unsigned char numSensors, unsigned char numSamplesPerSensor,
  unsigned char oddEmitterPin, unsigned char evenEmitterPin)
{
    init(analogPins, numSensors, numSamplesPerSensor, oddEmitterPin, evenEmitterPin);
}


// initialization for dimmable analog sensors with one bank or combined banks
// (1 emitter control pin)
// (overrides QTRSensorsAnalog::init)
void QTRDimmableAnalog::init(unsigned char* analogPins,
    unsigned char numSensors, unsigned char numSamplesPerSensor,
    unsigned char emitterPin)
{
    QTRDimmable::init(analogPins, numSensors, emitterPin);

    _numSamplesPerSensor = numSamplesPerSensor;
    _maxValue = 1023; // this is the maximum returned by the A/D conversion
}

// initialization for dimmable analog sensors with separate banks
// (2 emitter control pins)
void QTRDimmableAnalog::init(unsigned char* analogPins,
    unsigned char numSensors, unsigned char numSamplesPerSensor,
    unsigned char oddEmitterPin, unsigned char evenEmitterPin)
{
    QTRDimmable::init(analogPins, numSensors, oddEmitterPin, evenEmitterPin);

    _numSamplesPerSensor = numSamplesPerSensor;
    _maxValue = 1023; // this is the maximum returned by the A/D conversion
}


// the destructor frees up allocated memory
QTRSensors::~QTRSensors()
{
    if (_pins)
        free(_pins);
    if(calibratedMaximumOn)
        free(calibratedMaximumOn);
    if(calibratedMaximumOff)
        free(calibratedMaximumOff);
    if(calibratedMinimumOn)
        free(calibratedMinimumOn);
    if(calibratedMinimumOff)
        free(calibratedMinimumOff);
}
