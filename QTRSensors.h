/// \file QTRSensors.h

#pragma once

#include <stdint.h>

/// \brief Emitter behavior when taking readings.
///
/// Note that emitter control will only work if you specify a valid emitter pin
/// with setEmitterPin(), and the odd/even modes will only work if you are
/// using a second-generation QTR or QTRX sensor with two emitter control pins
/// and you specify both pins with setEmitterPins().
enum class QTRReadMode : uint8_t {
  /// Each reading is made without turning on the infrared (IR) emitters. The
  /// reading represents ambient light levels near the sensor.
  Off,

  /// Each reading is made with the emitters on. The reading is a measure of
  /// reflectance.
  On,

  /// For each sensor, a reading is made in both the on and off states. The
  /// value returned is **on + max &minus; off**, where **on** and **off** are
  /// the reading with the emitters on and off, respectively, and **max** is
  /// the maximum possible sensor reading. This mode can reduce the amount of
  /// interference from uneven ambient lighting.
  OnAndOff,

  /// The odd-numbered sensors are read with the odd-numbered emitters on, then
  /// the even-numbered sensors are read with the even-numbered emitters on.
  /// This mode can reduce interference between adjacent sensors, especially on
  /// QTRX sensor boards. It is only usable with second-generation QTR and QTRX
  /// sensor arrays that have two emitter control pins.
  OddEven,

  /// The odd and even sensors are read separately with the respective emitters
  /// on, then all sensors are read with emitters off and **on + max &minus;
  /// off** is returned. (In other words, this mode combines OddEven and
  /// OnAndOff.)
  OddEvenAndOff,

  /// Calling read() with this mode prevents it from automatically controlling
  /// the emitters: they are left in their existing states, which allows manual
  /// control of the emitters for testing and advanced use. Calibrating and
  /// obtaining calibrated readings are not supported with this mode.
  Manual
};

/// Sensor types.
enum class QTRType : uint8_t {
  Undefined,
  RC,
  Analog
};

/// Emitters selected to turn on or off.
enum class QTREmitters : uint8_t {
  All,
  Odd,
  Even,
  None
};

/// Represents an undefined emitter control pin.
const uint8_t QTRNoEmitterPin = 255;

/// Default timeout for RC sensors (in microseconds).
const uint16_t QTRRCDefaultTimeout = 2500;

/// The maximum number of sensors supported by an instance of this class.
const uint8_t QTRMaxSensors = 31;

/// \brief Represents a QTR sensor array.
///
/// An instance of this class represents a QTR sensor array, consisting of one
/// or more sensors of the same type. This could be either a single QTR sensor
/// board or multiple boards controlled as a group.
///
/// See \ref md_usage for an overview of how this library can be used and some
/// example code.
class QTRSensors
{
  public:

    QTRSensors() = default;

    ~QTRSensors();

    /// \brief Specifies that the sensors are RC.
    ///
    /// Call this function to set up RC-type sensors.
    void setTypeRC();

    /// \brief Specifies that the sensor type is analog.
    ///
    /// Call this function to set up A-type sensors.
    void setTypeAnalog();

    /// \brief Returns the type of the sensors.
    ///
    /// \return The sensor type as a member of the ::QTRType enum.
    ///
    /// See also setTypeRC() and setTypeAnalog().
    QTRType getType() { return _type; }

    /// \brief Sets the sensor pins.
    ///
    /// \param[in] pins A pointer to an array containing the Arduino pins that
    /// the sensors are connected to.
    ///
    /// \param sensorCount The number of sensors, which should match the length
    /// of the pins array.
    ///
    /// Example usage:
    /// ~~~{.cpp}
    /// // Set pins for four RC sensors connected to pins 6, 7, A0, and A1.
    /// // (Most analog pins can also be used as digital pins.)
    /// qtr.setTypeRC();
    /// qtr.setSensorPins((const uint8_t[]){6, 7, A0, A1}, 4);
    /// ~~~
    /// ~~~{.cpp}
    /// // Set pins for four analog sensors connected to pins A2, A3, A4, and A5.
    /// qtr.setTypeAnalog();
    /// qtr.setSensorPins((const uint8_t[]){A2, A3, A4, A5}, 4);
    /// ~~~
    ///
    /// If \link CalibrationData calibration data \endlink has already been
    /// stored, calling this method will force the storage for the calibration
    /// values to be reallocated and reinitialized the next time calibrate() is
    /// called (it sets `calibrationOn.initialized` and
    /// `calibrationOff.initialized` to false).
    void setSensorPins(const uint8_t * pins, uint8_t sensorCount);

    /// \brief Sets the timeout for RC sensors.
    ///
    /// \param timeout The length of time, in microseconds, beyond which you
    /// consider the sensor reading completely black.
    ///
    /// If the pulse length for a pin exceeds \p timeout, pulse timing will
    /// stop and the reading for that pin will be considered full black. It is
    /// recommended that you set \p timeout to be between 1000 and 3000
    /// &micro;s, depending on factors like the height of your sensors and
    /// ambient lighting. This allows you to shorten the duration of a
    /// sensor-reading cycle while maintaining useful measurements of
    /// reflectance. The default timeout is 2500 &micro;s.
    ///
    /// The maximum allowed timeout is 32767.
    /// (This prevents any possibility of an overflow when using
    /// QTRReadMode::OnAndOff or QTRReadMode::OddEvenAndOff).
    ///
    /// The timeout setting only applies to RC sensors.
    void setTimeout(uint16_t timeout);

    /// \brief Returns the timeout for RC sensors.
    ///
    /// \return The RC sensor timeout in microseconds.
    ///
    /// See also setTimeout().
    uint16_t getTimeout() { return _timeout; }

    /// \brief Sets the number of analog readings to average per analog sensor.
    ///
    /// \param samples The number of 10-bit analog samples (analog-to-digital
    /// conversions) to average per sensor each time it is read.
    ///
    /// Increasing \p samples increases noise suppression at the cost of sample
    /// rate. The maximum number of samples per sensor is 64; the default is 4.
    ///
    /// The samples per sensor setting only applies to analog sensors.
    void setSamplesPerSensor(uint8_t samples);

    /// \brief Returns the number of analog readings to average per analog
    /// sensor.
    ///
    /// \return The samples per channel for analog sensors.
    ///
    /// See also setSamplesPerSensor().
    uint16_t getSamplesPerSensor() { return _samplesPerSensor; }

    /// \brief Sets the emitter control pin for the sensors.
    ///
    /// \param emitterPin The Arduino digital pin that controls whether the IR
    /// LEDs are on or off.
    ///
    /// Specifying an emitter pin is optional, and the pin is not present on
    /// some QTR sensor boards. If a valid pin is connected and specified, the
    /// emitters will only be turned on during a reading; otherwise, the IR
    /// emitters will always be on. No emitter pin is specified by default.
    ///
    /// With second-generation QTR or QTRX sensor arrays that have two emitter
    /// control pins, you can control all of the emitters together by
    /// specifying a single emitter pin connected to either the CTRL ODD or
    /// CTRL EVEN pin on the sensor board. For independent control of the odd-
    /// and even-numbered emitters, see setEmitterPins().
    ///
    /// If you call this function after an emitter pin/pins have already been
    /// specified, any existing emitter pins will be released; see also
    /// releaseEmitterPins().
    void setEmitterPin(uint8_t emitterPin);

    /// \brief Sets separate odd and even emitter control pins for the sensors.
    ///
    /// \param oddEmitterPin The Arduino digital pin that controls the
    /// odd-numbered IR LEDs.
    ///
    /// \param evenEmitterPin The Arduino digital pin that controls the
    /// even-numbered IR LEDs.
    ///
    /// This function only works with second-generation QTR or QTRX sensor
    /// arrays that have two emitter control pins. To specify a single emitter
    /// pin for all sensors, see setEmitterPin().
    ///
    /// If you call this function after an emitter pin/pins have already been
    /// specified, any existing emitter pins will be released; see also
    /// releaseEmitterPins().
    void setEmitterPins(uint8_t oddEmitterPin, uint8_t evenEmitterPin);

    /// \brief Releases emitter pin/pins that have been set.
    ///
    /// This function releases any emitter pins that were previously specified,
    /// making them inputs and stopping further control of the emitters through
    /// them.
    ///
    /// See also setEmitterPin() and setEmitterPins().
    void releaseEmitterPins();

    /// \brief Returns the number of emitter control pins in use.
    ///
    /// \return The number of emitter control pins previously specified (1 with
    /// setEmitterPin() or 2 with setEmitterPins()). If no emitter pins have
    /// been specified (the default), or if previously specified pins were
    /// released with releaseEmitterPins(), this function returns 0.
    uint8_t getEmitterPinCount() { return _emitterPinCount; }

    /// \brief Returns the emitter control pin.
    ///
    /// \return The Arduino digital pin number of the emitter control pin
    /// (QTRNoEmitterPin if undefined).
    ///
    /// This function is intended for use when there is a single emitter pin
    /// specified; you can use getOddEmitterPin() and getEvenEmitterPin()
    /// instead when two are specified.
    ///
    /// See also setEmitterPin().
    uint8_t getEmitterPin() { return _oddEmitterPin; }

    /// \brief Returns the odd emitter control pin.
    ///
    /// \return The Arduino digital pin number of the odd emitter control pin
    /// (QTRNoEmitterPin if undefined).
    ///
    /// This function is intended for use when there are separate odd and even
    /// emitter pins specified; you can use getEmitterPin() instead when only
    /// one is specified.
    ///
    /// See also getEvenEmitterPin() and setEmitterPins().
    uint8_t getOddEmitterPin() { return _oddEmitterPin; }

    /// \brief Returns the even emitter control pin.
    ///
    /// \return The Arduino digital pin number of the even emitter control pin
    /// (QTRNoEmitterPin if undefined).
    ///
    /// This function is intended for use when there are separate odd and even
    /// emitter pins specified; you can use getEmitterPin() instead when only
    /// one is specified.
    ///
    /// See also getOddEmitterPin() and setEmitterPins().
    uint8_t getEvenEmitterPin()  { return _evenEmitterPin; }

    /// \brief Specifies that the sensors are dimmable.
    ///
    /// Calling this function is optional when setting up second-generation QTR
    /// or QTRX sensors. By default, the library assumes the sensors are
    /// dimmable.
    ///
    /// For first-generation QTR sensors, see setNonDimmable().
    void setDimmable() { _dimmable = true; }

    /// \brief Specifies that the sensors are non-dimmable.
    ///
    /// Call this function to set up first-generation QTR sensors and allow
    /// them to be read slightly faster (since their emitters can be turned on
    /// and off slightly more quickly than those on dimmable sensors).
    ///
    /// See also setDimmable().
    void setNonDimmable() { _dimmable = false; }

    /// \brief Returns whether the sensors are dimmable.
    ///
    /// \return True if this object is configured to treat the sensors as
    /// dimmable, false otherwise.
    ///
    /// See also setDimmable() and setNonDimmable().
    bool getDimmable() { return _dimmable; }

    /// \brief Sets the dimming level.
    ///
    /// \param dimmingLevel The dimming level (0 to 31). A dimming level of 0
    /// corresponds to full current and brightness, with higher dimming levels
    /// meaning lower currents.
    ///
    /// See your sensor board's product page or documentation for details on
    /// the relationship of the dimming level to the LED current.
    ///
    /// The dimming level will take effect the next time emittersOn() is called
    /// (either from your own program or by one of the library's read methods),
    /// and it will be applied again whenever the emitters are turned on after
    /// that.
    ///
    /// This setting is only used by dimmable sensors, and an emitter control
    /// pin/pins must be connected and defined for dimming to be applied.
    void setDimmingLevel(uint8_t dimmingLevel);

    /// \brief Returns the dimming level.
    ///
    /// \return The dimming level.
    ///
    /// See also setDimmingLevel().
    uint8_t getDimmingLevel() { return _dimmingLevel; }

    /// \brief Turns the IR LEDs off.
    ///
    /// \param emitters Which emitters to turn off, as a member of the
    /// ::QTREmitters enum. The default is QTREmitters::All.
    ///
    /// \param wait If true (the default), this function delays to give the
    /// sensors time to turn off before returning. Otherwise, it returns
    /// immediately.
    ///
    /// This function is mainly for use by the read() method. Since read()
    /// normally turns the emitters on and off automatically for each reading,
    /// calling this function yourself will not affect the readings unless the
    /// read mode is QTRReadMode::Manual, which tells read() to leave the
    /// emitters alone.
    void emittersOff(QTREmitters emitters = QTREmitters::All, bool wait = true);

    /// \brief Turns the IR LEDs on.
    ///
    /// \param emitters Which emitters to turn on, as a member of the
    /// ::QTREmitters enum. The default is QTREmitters::All.
    ///
    /// \param wait If true (the default), this function delays to give the
    /// sensors time to turn on before returning. Otherwise, it returns
    /// immediately.
    ///
    /// If the sensors are dimmable and a dimming level is set, this function
    /// will apply the dimming level after turning the emitters on.
    ///
    /// This function is mainly for use by the read() method. Since read()
    /// normally turns the emitters on and off automatically for each reading,
    /// calling this function yourself will not affect the readings unless the
    /// read mode is QTRReadMode::Manual, which tells read() to leave the
    /// emitters alone.
    void emittersOn(QTREmitters emitters = QTREmitters::All, bool wait = true);

    /// \brief Turns on the selected emitters and turns off the other emitters
    /// with optimized timing.
    ///
    /// \param emitters Which emitters to turn on, as a member of the
    /// ::QTREmitters enum. The other emitters will be turned off.
    ///
    /// This function turns on the selected emitters while it waits for the
    /// other emitters to turn off. For example,
    /// `emittersSelect(QTREmitters::Odd)` turns on the odd-numbered emitters
    /// while turning off the even-numbered emitters. Using this method avoids
    /// unnecessary delays compared to calling emittersOff() and emittersOn()
    /// separately, but it still waits for all emitters to be in the right
    /// states before returning.
    void emittersSelect(QTREmitters emitters);

    /// \brief Reads the sensors for calibration.
    ///
    /// \param mode The emitter behavior during calibration, as a member of the
    /// ::QTRReadMode enum. The default is QTRReadMode::On. Manual emitter
    /// control with QTRReadMode::Manual is not supported.
    ///
    /// This method reads the sensors 10 times and uses the results for
    /// calibration. The sensor values are not returned; instead, the maximum
    /// and minimum values found over time are stored in #calibrationOn and/or
    /// #calibrationOff for use by the readCalibrated() method.
    ///
    /// If the storage for the calibration values has not been initialized,
    /// this function will (re)allocate the arrays and initialize the maximum
    /// and minimum values to 0 and the maximum possible sensor reading,
    /// respectively, so that the very first calibration sensor reading will
    /// update both of them.
    ///
    /// Note that the `minimum` and `maximum` pointers in the CalibrationData
    /// structs will point to arrays of length \p sensorCount, as specified in
    /// setSensorPins(), and they will only be allocated when calibrate() is
    /// called. If you only calibrate with the emitters on, the calibration
    /// arrays that hold the off values will not be allocated (and vice versa).
    ///
    /// See \ref md_usage for more information and example code.
    void calibrate(QTRReadMode mode = QTRReadMode::On);

    /// \brief Resets all calibration that has been done.
    void resetCalibration();

    /// \brief Reads the raw sensor values into an array.
    ///
    /// \param[out] sensorValues A pointer to an array in which to store the
    /// raw sensor readings. There **MUST** be space in the array for as many
    /// values as there were sensors specified in setSensorPins().
    ///
    /// \param mode The emitter behavior during the read, as a member of the
    /// ::QTRReadMode enum. The default is QTRReadMode::On.
    ///
    /// Example usage:
    /// ~~~{.cpp}
    /// uint16_t sensorValues[8];
    /// qtr.read(sensorValues);
    /// ~~~
    ///
    /// The values returned are a measure of the reflectance in abstract units,
    /// with higher values corresponding to lower reflectance (e.g. a black
    /// surface or a void).
    ///
    /// Analog sensors will return a raw value between 0 and 1023 (like
    /// Arduino's `analogRead()` function).
    ///
    /// RC sensors will return a raw value in microseconds between 0 and the
    /// timeout setting configured with setTimeout() (the default timeout is
    /// 2500 &micro;s).
    ///
    /// See \ref md_usage for more information and example code.
    void read(uint16_t * sensorValues, QTRReadMode mode = QTRReadMode::On);

    /// \brief Reads the sensors and provides calibrated values between 0 and
    /// 1000.
    ///
    /// \param[out] sensorValues A pointer to an array in which to store the
    /// calibrated sensor readings.  There **MUST** be space in the array for
    /// as many values as there were sensors specified in setSensorPins().
    ///
    /// \param mode The emitter behavior during the read, as a member of the
    /// ::QTRReadMode enum. The default is QTRReadMode::On. Manual emitter
    /// control with QTRReadMode::Manual is not supported.
    ///
    /// 0 corresponds to the minimum value stored in #calibrationOn or
    /// #calibrationOff, depending on \p mode, and 1000 corresponds to the
    /// maximum value. Calibration values are typically obtained by calling
    /// calibrate(), and they are stored separately for each sensor, so that
    /// differences in the sensors are accounted for automatically.
    ///
    /// See \ref md_usage for more information and example code.
    void readCalibrated(uint16_t * sensorValues, QTRReadMode mode = QTRReadMode::On);

    /// \brief Reads the sensors, provides calibrated values, and returns an
    /// estimated black line position.
    ///
    /// \param[out] sensorValues A pointer to an array in which to store the
    /// calibrated sensor readings.  There **MUST** be space in the array for
    /// as many values as there were sensors specified in setSensorPins().
    ///
    /// \param mode The emitter behavior during the read, as a member of the
    /// ::QTRReadMode enum. The default is QTRReadMode::On. Manual emitter
    /// control with QTRReadMode::Manual is not supported.
    ///
    /// \return An estimate of the position of a black line under the sensors.
    ///
    /// The estimate is made using a weighted average of the sensor indices
    /// multiplied by 1000, so that a return value of 0 indicates that the line
    /// is directly below sensor 0, a return value of 1000 indicates that the
    /// line is directly below sensor 1, 2000 indicates that it's below sensor
    /// 2000, etc. Intermediate values indicate that the line is between two
    /// sensors. The formula is (where \f$v_0\f$ represents the value from the
    /// first sensor):
    ///
    /// \f[
    /// {(0 \times v_0) + (1000 \times v_1) + (2000 \times v_2) + \cdots
    /// \over
    /// v_0 + v_1 + v_2 + \cdots}
    /// \f]
    ///
    /// As long as your sensors aren’t spaced too far apart relative to the
    /// line, this returned value is designed to be monotonic, which makes it
    /// great for use in closed-loop PID control. Additionally, this method
    /// remembers where it last saw the line, so if you ever lose the line to
    /// the left or the right, its line position will continue to indicate the
    /// direction you need to go to reacquire the line. For example, if sensor
    /// 4 is your rightmost sensor and you end up completely off the line to
    /// the left, this function will continue to return 4000.
    ///
    /// This function is intended to detect a black (or dark-colored) line on a
    /// white (or light-colored) background. For a white line, see
    /// readLineWhite().
    ///
    /// See \ref md_usage for more information and example code.
    uint16_t readLineBlack(uint16_t * sensorValues, QTRReadMode mode = QTRReadMode::On)
    {
      return readLinePrivate(sensorValues, mode, false);
    }

    /// \brief Reads the sensors, provides calibrated values, and returns an
    /// estimated white line position.
    ///
    /// \param[out] sensorValues A pointer to an array in which to store the
    /// calibrated sensor readings.  There **MUST** be space in the array for
    /// as many values as there were sensors specified in setSensorPins().
    ///
    /// \param mode The emitter behavior during the read, as a member of the
    /// ::QTRReadMode enum. The default is QTRReadMode::On. Manual emitter
    /// control with QTRReadMode::Manual is not supported.
    ///
    /// \return An estimate of the position of a white line under the sensors.
    ///
    /// This function is intended to detect a white (or light-colored) line on
    /// a black (or dark-colored) background. For a black line, see
    /// readLineBlack().
    ///
    /// See \ref md_usage for more information and example code.
    uint16_t readLineWhite(uint16_t * sensorValues, QTRReadMode mode = QTRReadMode::On)
    {
      return readLinePrivate(sensorValues, mode, true);
    }


    /// \brief Stores sensor calibration data.
    ///
    /// See calibrate() and readCalibrated() for details.
    struct CalibrationData
    {
      /// Whether array pointers have been allocated and initialized.
      bool initialized = false;
      /// Lowest readings seen during calibration.
      uint16_t * minimum = nullptr;
      /// Highest readings seen during calibration.
      uint16_t * maximum = nullptr;
    };

    /// \name Calibration data
    ///
    /// See calibrate() and readCalibrated() for details.
    ///
    /// These variables are made public so that you can use them for your own
    /// calculations and do things like saving the values to EEPROM, performing
    /// sanity checking, etc.
    /// \{

    /// Data from calibrating with emitters on.
    CalibrationData calibrationOn;

    /// Data from calibrating with emitters off.
    CalibrationData calibrationOff;

    /// \}

  private:

    uint16_t emittersOnWithPin(uint8_t pin);

    // Handles the actual calibration, including (re)allocating and
    // initializing the storage for the calibration values if necessary.
    void calibrateOnOrOff(CalibrationData & calibration, QTRReadMode mode);

    void readPrivate(uint16_t * sensorValues, uint8_t start = 0, uint8_t step = 1);

    uint16_t readLinePrivate(uint16_t * sensorValues, QTRReadMode mode, bool invertReadings);

    QTRType _type = QTRType::Undefined;

    uint8_t * _sensorPins = nullptr;
    uint8_t _sensorCount = 0;

    uint16_t _timeout = QTRRCDefaultTimeout; // only used for RC sensors
    uint16_t _maxValue = QTRRCDefaultTimeout; // the maximum value returned by readPrivate()
    uint8_t _samplesPerSensor = 4; // only used for analog sensors

    uint8_t _oddEmitterPin = QTRNoEmitterPin; // also used for single emitter pin
    uint8_t _evenEmitterPin = QTRNoEmitterPin;
    uint8_t _emitterPinCount = 0;

    bool _dimmable = true;
    uint8_t _dimmingLevel = 0;

    uint16_t _lastPosition = 0;
};
