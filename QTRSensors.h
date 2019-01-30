/// @file QTRSensors.h

/*
  QTRSensors - library for using Pololu QTR reflectance sensors and reflectance
    sensor arrays
*/

#pragma once

#include <stdint.h>

class QTRSensors
{
  public:

    enum class ReadMode : uint8_t {
      Off,
      On,
      OnAndOff,
      OddEven,
      OddEvenAndOff,
      Manual
    };

    enum class Type : uint8_t {
      Undefined,
      RC,
      Analog
    };

    enum class Emitters : uint8_t {
      All,
      Odd,
      Even,
      None
    };

    const uint8_t NoEmitterPin = 255;

    const uint16_t RCDefaultTimeout = 2500;

    const uint8_t MaxSensors = 31;

    QTRSensors() = default;

    ~QTRSensors();

    void setTypeRC();
    void setTypeAnalog();
    Type getType() { return _type; }

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

    void setDimmable() { _dimmable = true; }
    void setNonDimmable() { _dimmable = false; }
    bool isDimmable() { return _dimmable; }

    // Sets and gets the dimming level (0-31). A dimming level of 0 corresponds
    // to full current and brightness, with higher dimming levels meaning lower
    // currents; see the product page for your sensor for details.
    void setDimmingLevel(uint8_t dimmingLevel);
    uint8_t getDimmingLevel() { return _dimmingLevel; }

    // Turn the IR LEDs off and on.  These are mainly for use by the read
    // method, and calling these functions before or after reading the sensors
    // will have no effect on the readings unless the readMode is
    // QTR_EMITTERS_MANUAL, but you may wish to use these for testing purposes.
    void emittersOff(Emitters emitters = Emitters::All, bool wait = true);
    void emittersOn(Emitters emitters = Emitters::All, bool wait = true);

    // Turns on the selected emitters and turns off the other emitters with optimized
    // timing (no unnecessary waits compared to calling emittersOff and
    // emittersOn separately, but still waits for both emitterss of emitters to be
    // in the right states before returning).
    void EmittersSelect(Emitters emitters);

    // Reads the sensors for calibration.  The sensor values are
    // not returned; instead, the maximum and minimum values found
    // over time are stored internally and used for the
    // readCalibrated() method.
    void calibrate(ReadMode mode = ReadMode::On);

    // Resets all calibration that has been done.
    void resetCalibration();

    /// @brief Reads the raw sensor values into an array.
    ///
    /// There **MUST** be space in the @p sensorValues array for as many values
    /// as there were sensors specified in the constructor.
    ///
    /// Example usage:
    /// ~~~{.cpp}
    /// unsigned int sensorValues[8];
    /// qtr.read(sensorValues);
    /// ~~~
    ///
    /// The values returned are a measure of the reflectance in abstract units,
    /// with higher values corresponding to lower reflectance (e.g. a black
    /// surface or a void).
    ///
    /// Analog-type sensors will return a raw value between 0 and 1023 (like
    /// Arduino's `analogRead()` function).
    ///
    /// RC-type sensors will return a raw value between 0 and the \p timeout
    /// setting configured with `setTimeout()` (which defaults to 2500).
    ///
    /// The functions that read values from the sensors all take an argument @p
    /// readMode, which specifies the kind of read that will be performed.
    /// Several options are defined @ref ReadModes
    ///
    /// * #QTR_EMITTERS_OFF specifies that the reading should be made without
    /// turning on the infrared (IR) emitters, in which case the reading
    /// represents ambient light levels near the sensor.
    ///
    void read(uint16_t *sensorValues, ReadMode mode = ReadMode::On);

    /// @brief Reads the sensors and provides calibrated values between 0 and
    /// 1000.
    ///
    /// 0 corresponds to the minimum value read by `calibrate()` and 1000
    /// corresponds to the maximum value.  Calibration values are
    /// stored separately for each sensor, so that differences in the
    /// sensors are accounted for automatically.
    void readCalibrated(uint16_t *sensorValues, ReadMode mode = ReadMode::On);

    /// @brief Reads the sensors, provides calibrated values, and returns an
    /// estimated line position (black line on white background).
    ///
    /// The estimate is made using a weighted average of the sensor indices
    /// multiplied by 1000, so that a return value of 0 indicates that
    /// the line is directly below sensor 0, a return value of 1000
    /// indicates that the line is directly below sensor 1, 2000
    /// indicates that it's below sensor 2000, etc.  Intermediate
    /// values indicate that the line is between two sensors.  The
    /// formula is:
    ///
    ///    0*value0 + 1000*value1 + 2000*value2 + ...
    ///   --------------------------------------------
    ///         value0  +  value1  +  value2 + ...
    uint16_t readLineBlack(uint16_t *sensorValues, ReadMode mode = ReadMode::On)
    {
      readLinePrivate(sensorValues, mode, false);
    }

    /// @brief Reads the sensors, provides calibrated values, and returns an
    /// estimated line position (white line on black background).
    uint16_t readLineWhite(uint16_t *sensorValues, ReadMode mode = ReadMode::On)
    {
      readLinePrivate(sensorValues, mode, true);
    }

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
    uint16_t *calibratedMinimumOn = nullptr;
    uint16_t *calibratedMaximumOn = nullptr;
    uint16_t *calibratedMinimumOff = nullptr;
    uint16_t *calibratedMaximumOff = nullptr;

  private:

    uint16_t emittersOnWithPin(uint8_t pin);

    // Handles the actual calibration. calibratedMinimum and
    // calibratedMaximum are pointers to the requested calibration
    // arrays, which will be allocated if necessary.
    void calibrateOnOrOff(uint16_t **calibratedMinimum,
                          uint16_t **calibratedMaximum,
                          ReadMode mode);

    void QTRSensors::readPrivate(uint16_t *sensorValues, uint8_t start = 0, uint8_t step = 1);


    int readLinePrivate(uint16_t *sensorValues, ReadMode mode, bool invertReadings);

    Type _type = Type::Undefined;

    uint8_t *_sensorPins = nullptr;
    uint8_t _sensorCount = 0;

    uint16_t _timeout = RCDefaultTimeout;
    uint16_t _maxValue = RCDefaultTimeout; // the maximum value returned by readPrivate()
    uint8_t _samplesPerSensor = 4; // only used for analog sensors

    uint8_t _oddEmitterPin = NoEmitterPin; // also used for single emitter pin
    uint8_t _evenEmitterPin = NoEmitterPin;
    uint8_t _emitterPinCount = 0;

    bool _dimmable = true;
    uint8_t _dimmingLevel = 0;

    uint16_t _lastPosition = 0;
};