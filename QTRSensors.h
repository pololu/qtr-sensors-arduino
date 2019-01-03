/*
  QTRSensors - library for using Pololu QTR reflectance sensors and reflectance
    sensor arrays.  The object used is determined by the type of the sensor
    (analog or RC; dimmable or non-dimmable).  Then simply specify in the
    constructor which Arduino I/O pins are connected to a QTR sensor, and the
    read() method will obtain reflectance measurements for those sensors.
    Smaller sensor values correspond to higher reflectance (e.g.  white) while
    larger sensor values correspond to lower reflectance (e.g.  black or a
    void).

    * QTRSensorsRC should be used for original (non-dimmable) QTR-xRC sensors.
    * QTRSensorsAnalog should be used for original (non-dimmable) QTR-xA sensors.
    * QTRDimmableRC should be used for dimmable QTR-xD-xRC and QTRX-xD-xRC sensors.
    * QTRDimmableAnalog should be used for dimmable QTR-xD-xA and QTRX-xD-xA sensors.
*/

#pragma once

#include <stdint.h>

#define QTR_EMITTERS_OFF              0
#define QTR_EMITTERS_ON               1
#define QTR_EMITTERS_ON_AND_OFF       2
#define QTR_EMITTERS_ODD_EVEN         3
#define QTR_EMITTERS_ODD_EVEN_AND_OFF 4
#define QTR_EMITTERS_MANUAL           5

#define QTR_NO_EMITTER_PIN  255

#define QTR_TYPE_UNDEFINED  0
#define QTR_TYPE_RC         1
#define QTR_TYPE_ANALOG     2

#define QTR_RC_DEFAULT_TIMEOUT 2500

#define QTR_BANK_ALL  0
#define QTR_BANK_ODD  1
#define QTR_BANK_EVEN 2
#define QTR_BANK_NONE 3

#define QTR_MAX_SENSORS 31

class QTRSensors
{
  public:

    QTRSensors();

    ~QTRSensors();

    void setType(uint8_t type);
    uint8_t getType() { return _type; }

    void setSensorPins(uint8_t *pins, uint8_t sensorCount);

    void setTimeout(uint16_t timeout);
    uint16_t getTimeout() { return _timeout; }

    void setSamplesPerSensor(uint8_t samples) { _samplesPerSensor = samples; }
    uint16_t getSamplesPerSensor() { return _samplesPerSensor; }

    void setEmitterPin(uint8_t emitterPin);
    void setEmitterPins(uint8_t oddEmitterPin, uint8_t evenEmitterPin);
    void releaseEmitterPins();

    uint8_t getEmitterPinCount() { return _emitterPinCount; }
    uint8_t getEmitterPin() { return _oddEmitterPin; }
    uint8_t getOddEmitterPin() { return _oddEmitterPin; }
    uint8_t getEvenEmitterPin()  { return _evenEmitterPin; }

    void setDimmable(bool dimmable) { _dimmable = dimmable; }
    bool getDimmable() { return _dimmable; }

    // Sets and gets the dimming level (0-31). A dimming level of 0 corresponds
    // to full current and brightness, with higher dimming levels meaning lower
    // currents; see the product page for your sensor for details.
    void setDimmingLevel(uint8_t dimmingLevel);
    uint8_t getDimmingLevel() { return _dimmingLevel; }


    // Turn the IR LEDs off and on.  These are mainly for use by the read
    // method, and calling these functions before or after reading the sensors
    // will have no effect on the readings unless the readMode is
    // QTR_EMITTERS_MANUAL, but you may wish to use these for testing purposes.
    void emittersOff(uint8_t bank = QTR_BANK_ALL, bool wait = true);
    void emittersOn(uint8_t bank = QTR_BANK_ALL, bool wait = true);

    // Turns on the selected bank and turns off the other bank with optimized
    // timing (no unnecessary waits compared to calling emittersOff and
    // emittersOn separately, but still waits for both banks of emitters to be
    // in the right states before returning).
    void emitterBankSelect(uint8_t bank);


    // Reads the sensors for calibration.  The sensor values are
    // not returned; instead, the maximum and minimum values found
    // over time are stored internally and used for the
    // readCalibrated() method.
    void calibrate(uint8_t readMode = QTR_EMITTERS_ON);

    // Resets all calibration that has been done.
    void resetCalibration();


    // Reads the sensor values into an array. There *MUST* be space
    // for as many values as there were sensors specified in the constructor.
    // Example usage:
    // uint16_t sensorValues[8];
    // sensors.read(sensorValues);
    // The values returned are a measure of the reflectance in abstract units,
    // with higher values corresponding to lower reflectance (e.g. a black
    // surface or a void).
    // If measureOffAndOn is true, measures the values with the
    // emitters on AND off and returns on - (timeout - off).  If this
    // value is less than zero, it returns zero.
    // This method will call the appropriate derived class's readPrivate(),
    // which is defined as a virtual function in the base class and
    // overridden by each derived class's own implementation.
    void read(uint16_t *sensorValues, uint8_t readMode = QTR_EMITTERS_ON);

    // Returns values calibrated to a value between 0 and 1000, where
    // 0 corresponds to the minimum value read by calibrate() and 1000
    // corresponds to the maximum value.  Calibration values are
    // stored separately for each sensor, so that differences in the
    // sensors are accounted for automatically.
    void readCalibrated(uint16_t *sensorValues, uint8_t readMode = QTR_EMITTERS_ON);

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
    // black, set the optional second argument whiteLine to true.  In
    // this case, each sensor value will be replaced by (1000-value)
    // before the averaging.
    int readLine(uint16_t *sensorValues, uint8_t readMode = QTR_EMITTERS_ON, uint8_t whiteLine = 0);

    // Calibrated minimum and maximum values. These start at 1000 and
    // 0, respectively, so that the very first sensor reading will
    // update both of them.
    //
    // The pointers are unallocated until calibrate() is called, and
    // then allocated to exactly the size required.  Depending on the
    // readMode argument to calibrate, only the On or Off values may
    // be allocated, as required.
    //
    // These variables are made public so that you can use them for
    // your own calculations and do things like saving the values to
    // EEPROM, performing sanity checking, etc.
    uint16_t *calibratedMinimumOn;
    uint16_t *calibratedMaximumOn;
    uint16_t *calibratedMinimumOff;
    uint16_t *calibratedMaximumOff;

  private:
    uint16_t emittersOnWithPin(uint8_t pin);

    // Handles the actual calibration. calibratedMinimum and
    // calibratedMaximum are pointers to the requested calibration
    // arrays, which will be allocated if necessary.
    void calibrateOnOrOff(uint16_t **calibratedMinimum,
                          uint16_t **calibratedMaximum,
                          uint8_t readMode);

    void readPrivate(uint16_t *sensorValues, uint8_t bank = QTR_BANK_ALL);

    uint8_t _type;

    uint8_t *_sensorPins;
    uint8_t _sensorCount;

    uint16_t _timeout;
    uint16_t _maxValue; // the maximum value returned by readPrivate()
    uint8_t _samplesPerSensor; // only used for analog sensors

    uint8_t _oddEmitterPin; // also used for single emitter pin
    uint8_t _evenEmitterPin;
    uint8_t _emitterPinCount;

    bool _dimmable;
    uint8_t _dimmingLevel;

    uint16_t _lastPosition;
};