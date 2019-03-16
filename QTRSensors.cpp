#include "QTRSensors.h"
#include <Arduino.h>

void QTRSensors::setTypeRC()
{
  _type = QTRType::RC;
  _maxValue = _timeout;
}

void QTRSensors::setTypeAnalog()
{
  _type = QTRType::Analog;
  _maxValue = 1023; // Arduino analogRead() returns a 10-bit value by default
}

void QTRSensors::setSensorPins(const uint8_t * pins, uint8_t sensorCount)
{
  if (sensorCount > QTRMaxSensors) { sensorCount = QTRMaxSensors; }

  // (Re)allocate and initialize the array if necessary.
  uint8_t * oldSensorPins = _sensorPins;
  _sensorPins = (uint8_t *)realloc(_sensorPins, sizeof(uint8_t) * sensorCount);
  if (_sensorPins == nullptr)
  {
    // Memory allocation failed; don't continue.
    free(oldSensorPins); // deallocate any memory used by old array
    return;
  }

  for (uint8_t i = 0; i < sensorCount; i++)
  {
    _sensorPins[i] = pins[i];
  }

  _sensorCount = sensorCount;

  // Any previous calibration values are no longer valid, and the calibration
  // arrays might need to be reallocated if the sensor count was changed.
  calibrationOn.initialized = false;
  calibrationOff.initialized = false;
}

void QTRSensors::setTimeout(uint16_t timeout)
{
  if (timeout > 32767) { timeout = 32767; }
  _timeout = timeout;
  if (_type == QTRType::RC) { _maxValue = timeout; }
}

void QTRSensors::setSamplesPerSensor(uint8_t samples)
{
  if (samples > 64) { samples = 64; }
  _samplesPerSensor = samples;
}

void QTRSensors::setEmitterPin(uint8_t emitterPin)
{
  releaseEmitterPins();

  _oddEmitterPin = emitterPin;
  pinMode(_oddEmitterPin, OUTPUT);

  _emitterPinCount = 1;
}

void QTRSensors::setEmitterPins(uint8_t oddEmitterPin, uint8_t evenEmitterPin)
{
  releaseEmitterPins();

  _oddEmitterPin = oddEmitterPin;
  _evenEmitterPin = evenEmitterPin;
  pinMode(_oddEmitterPin, OUTPUT);
  pinMode(_evenEmitterPin, OUTPUT);

  _emitterPinCount = 2;
}

void QTRSensors::releaseEmitterPins()
{
  if (_oddEmitterPin != QTRNoEmitterPin)
  {
    pinMode(_oddEmitterPin, INPUT);
    _oddEmitterPin = QTRNoEmitterPin;
  }

  if (_evenEmitterPin != QTRNoEmitterPin)
  {
    pinMode(_evenEmitterPin, INPUT);
    _evenEmitterPin = QTRNoEmitterPin;
  }

  _emitterPinCount = 0;
}

void QTRSensors::setDimmingLevel(uint8_t dimmingLevel)
{
  if (dimmingLevel > 31) { dimmingLevel = 31; }
  _dimmingLevel = dimmingLevel;
}

// emitters defaults to QTREmitters::All; wait defaults to true
void QTRSensors::emittersOff(QTREmitters emitters, bool wait)
{
  bool pinChanged = false;

  // Use odd emitter pin in these cases:
  // - 1 emitter pin, emitters = all
  // - 2 emitter pins, emitters = all
  // - 2 emitter pins, emitters = odd
  if (emitters == QTREmitters::All ||
      (_emitterPinCount == 2 && emitters == QTREmitters::Odd))
  {
    // Check if pin is defined and only turn off if not already off
    if ((_oddEmitterPin != QTRNoEmitterPin) &&
        (digitalRead(_oddEmitterPin) == HIGH))
    {
      digitalWrite(_oddEmitterPin, LOW);
      pinChanged = true;
    }
  }

  // Use even emitter pin in these cases:
  // - 2 emitter pins, emitters = all
  // - 2 emitter pins, emitters = even
  if (_emitterPinCount == 2 &&
      (emitters == QTREmitters::All || emitters == QTREmitters::Even))
  {
    // Check if pin is defined and only turn off if not already off
    if ((_evenEmitterPin != QTRNoEmitterPin) &&
        (digitalRead(_evenEmitterPin) == HIGH))
    {
      digitalWrite(_evenEmitterPin, LOW);
      pinChanged = true;
    }
  }

  if (wait && pinChanged)
  {
    if (_dimmable)
    {
      // driver min is 1 ms
      delayMicroseconds(1200);
    }
    else
    {
      delayMicroseconds(200);
    }
  }
}

void QTRSensors::emittersOn(QTREmitters emitters, bool wait)
{
  bool pinChanged = false;
  uint16_t emittersOnStart;

  // Use odd emitter pin in these cases:
  // - 1 emitter pin, emitters = all
  // - 2 emitter pins, emitters = all
  // - 2 emitter pins, emitters = odd
  if (emitters == QTREmitters::All ||
      (_emitterPinCount == 2 && emitters == QTREmitters::Odd))
  {
    // Check if pin is defined, and only turn on non-dimmable sensors if not
    // already on, but always turn dimmable sensors off and back on because
    // we might be changing the dimming level (emittersOnWithPin() should take
    // care of this)
    if ((_oddEmitterPin != QTRNoEmitterPin) &&
        ( _dimmable || (digitalRead(_oddEmitterPin) == LOW)))
    {
      emittersOnStart = emittersOnWithPin(_oddEmitterPin);
      pinChanged = true;
    }
  }

  // Use even emitter pin in these cases:
  // - 2 emitter pins, emitters = all
  // - 2 emitter pins, emitters = even
  if (_emitterPinCount == 2 &&
      (emitters == QTREmitters::All || emitters == QTREmitters::Even))
  {
    // Check if pin is defined, and only turn on non-dimmable sensors if not
    // already on, but always turn dimmable sensors off and back on because
    // we might be changing the dimming level (emittersOnWithPin() should take
    // care of this)
    if ((_evenEmitterPin != QTRNoEmitterPin) &&
        (_dimmable || (digitalRead(_evenEmitterPin) == LOW)))
    {
      emittersOnStart = emittersOnWithPin(_evenEmitterPin);
      pinChanged = true;
    }
  }

  if (wait && pinChanged)
  {
    if (_dimmable)
    {
      // Make sure it's been at least 300 us since the emitter pin was first set
      // high before returning. (Driver min is 250 us.) Some time might have
      // already passed while we set the dimming level.
      while ((uint16_t)(micros() - emittersOnStart) < 300)
      {
        delayMicroseconds(10);
      }
    }
    else
    {
      delayMicroseconds(200);
    }
  }
}

// assumes pin is valid (not QTRNoEmitterPin)
// returns time when pin was first set high (used by emittersSelect())
uint16_t QTRSensors::emittersOnWithPin(uint8_t pin)
{
  if (_dimmable && (digitalRead(pin) == HIGH))
  {
    // We are turning on dimmable emitters that are already on. To avoid messing
    // up the dimming level, we have to turn the emitters off and back on. This
    // means the turn-off delay will happen even if wait = false was passed to
    // emittersOn(). (Driver min is 1 ms.)
    digitalWrite(pin, LOW);
    delayMicroseconds(1200);
  }

  digitalWrite(pin, HIGH);
  uint16_t emittersOnStart = micros();

  if (_dimmable && (_dimmingLevel > 0))
  {
    noInterrupts();

    for (uint8_t i = 0; i < _dimmingLevel; i++)
    {
      delayMicroseconds(1);
      digitalWrite(pin, LOW);
      delayMicroseconds(1);
      digitalWrite(pin, HIGH);
    }

    interrupts();
  }

  return emittersOnStart;
}

void QTRSensors::emittersSelect(QTREmitters emitters)
{
  QTREmitters offEmitters;

  switch (emitters)
  {
    case QTREmitters::Odd:
      offEmitters = QTREmitters::Even;
      break;

    case QTREmitters::Even:
      offEmitters = QTREmitters::Odd;
      break;

    case QTREmitters::All:
      emittersOn();
      return;

    case QTREmitters::None:
      emittersOff();
      return;

    default: // invalid
      return;
  }

  // Turn off the off-emitters; don't wait before proceeding, but record the time.
  emittersOff(offEmitters, false);
  uint16_t turnOffStart = micros();

  // Turn on the on-emitters and wait.
  emittersOn(emitters);

  if (_dimmable)
  {
    // Finish waiting for the off-emitters emitters to turn off: make sure it's been
    // at least 1200 us since the off-emitters was turned off before returning.
    // (Driver min is 1 ms.) Some time has already passed while we waited for
    // the on-emitters to turn on.
    while ((uint16_t)(micros() - turnOffStart) < 1200)
    {
      delayMicroseconds(10);
    }
  }
}

void QTRSensors::resetCalibration()
{
  for (uint8_t i = 0; i < _sensorCount; i++)
  {
    if (calibrationOn.maximum)   { calibrationOn.maximum[i] = 0; }
    if (calibrationOff.maximum)  { calibrationOff.maximum[i] = 0; }
    if (calibrationOn.minimum)   { calibrationOn.minimum[i] = _maxValue; }
    if (calibrationOff.minimum)  { calibrationOff.minimum[i] = _maxValue; }
  }
}

void QTRSensors::calibrate(QTRReadMode mode)
{
  // manual emitter control is not supported
  if (mode == QTRReadMode::Manual) { return; }

  if (mode == QTRReadMode::On ||
      mode == QTRReadMode::OnAndOff)
  {
    calibrateOnOrOff(calibrationOn, QTRReadMode::On);
  }
  else if (mode == QTRReadMode::OddEven ||
           mode == QTRReadMode::OddEvenAndOff)
  {
    calibrateOnOrOff(calibrationOn, QTRReadMode::OddEven);
  }

  if (mode == QTRReadMode::OnAndOff ||
      mode == QTRReadMode::OddEvenAndOff ||
      mode == QTRReadMode::Off)
  {
    calibrateOnOrOff(calibrationOff, QTRReadMode::Off);
  }
}

void QTRSensors::calibrateOnOrOff(CalibrationData & calibration, QTRReadMode mode)
{
  uint16_t sensorValues[QTRMaxSensors];
  uint16_t maxSensorValues[QTRMaxSensors];
  uint16_t minSensorValues[QTRMaxSensors];

  // (Re)allocate and initialize the arrays if necessary.
  if (!calibration.initialized)
  {
    uint16_t * oldMaximum = calibration.maximum;
    calibration.maximum = (uint16_t *)realloc(calibration.maximum,
                                              sizeof(uint16_t) * _sensorCount);
    if (calibration.maximum == nullptr)
    {
      // Memory allocation failed; don't continue.
      free(oldMaximum); // deallocate any memory used by old array
      return;
    }

    uint16_t * oldMinimum = calibration.minimum;
    calibration.minimum = (uint16_t *)realloc(calibration.minimum,
                                              sizeof(uint16_t) * _sensorCount);
    if (calibration.minimum == nullptr)
    {
      // Memory allocation failed; don't continue.
      free(oldMinimum); // deallocate any memory used by old array
      return;
    }

    // Initialize the max and min calibrated values to values that
    // will cause the first reading to update them.
    for (uint8_t i = 0; i < _sensorCount; i++)
    {
      calibration.maximum[i] = 0;
      calibration.minimum[i] = _maxValue;
    }

    calibration.initialized = true;
  }

  for (uint8_t j = 0; j < 10; j++)
  {
    read(sensorValues, mode);

    for (uint8_t i = 0; i < _sensorCount; i++)
    {
      // set the max we found THIS time
      if ((j == 0) || (sensorValues[i] > maxSensorValues[i]))
      {
        maxSensorValues[i] = sensorValues[i];
      }

      // set the min we found THIS time
      if ((j == 0) || (sensorValues[i] < minSensorValues[i]))
      {
        minSensorValues[i] = sensorValues[i];
      }
    }
  }

  // record the min and max calibration values
  for (uint8_t i = 0; i < _sensorCount; i++)
  {
    // Update maximum only if the min of 10 readings was still higher than it
    // (we got 10 readings in a row higher than the existing maximum).
    if (minSensorValues[i] > calibration.maximum[i])
    {
      calibration.maximum[i] = minSensorValues[i];
    }

    // Update minimum only if the max of 10 readings was still lower than it
    // (we got 10 readings in a row lower than the existing minimum).
    if (maxSensorValues[i] < calibration.minimum[i])
    {
      calibration.minimum[i] = maxSensorValues[i];
    }
  }
}

void QTRSensors::read(uint16_t * sensorValues, QTRReadMode mode)
{
  switch (mode)
  {
    case QTRReadMode::Off:
      emittersOff();
      // fall through
    case QTRReadMode::Manual:
      readPrivate(sensorValues);
      return;

    case QTRReadMode::On:
    case QTRReadMode::OnAndOff:
      emittersOn();
      readPrivate(sensorValues);
      emittersOff();
      break;

    case QTRReadMode::OddEven:
    case QTRReadMode::OddEvenAndOff:
      // Turn on odd emitters and read the odd-numbered sensors.
      // (readPrivate takes a 0-based array index, so start = 0 to start with
      // the first sensor)
      emittersSelect(QTREmitters::Odd);
      readPrivate(sensorValues, 0, 2);

      // Turn on even emitters and read the even-numbered sensors.
      // (readPrivate takes a 0-based array index, so start = 1 to start with
      // the second sensor)
      emittersSelect(QTREmitters::Even);
      readPrivate(sensorValues, 1, 2);

      emittersOff();
      break;

    default: // invalid - do nothing
      return;
  }

  if (mode == QTRReadMode::OnAndOff ||
      mode == QTRReadMode::OddEvenAndOff)
  {
    // Take a second set of readings and return the values (on + max - off).

    uint16_t offValues[QTRMaxSensors];
    readPrivate(offValues);

    for (uint8_t i = 0; i < _sensorCount; i++)
    {
      sensorValues[i] += _maxValue - offValues[i];
      if (sensorValues[i] > _maxValue)
      {
        // This usually doesn't happen, because the sensor reading should
        // go up when the emitters are turned off.
        sensorValues[i] = _maxValue;
      }
    }
  }
}

void QTRSensors::readCalibrated(uint16_t * sensorValues, QTRReadMode mode)
{
  // manual emitter control is not supported
  if (mode == QTRReadMode::Manual) { return; }

  // if not calibrated, do nothing

  if (mode == QTRReadMode::On ||
      mode == QTRReadMode::OnAndOff ||
      mode == QTRReadMode::OddEvenAndOff)
  {
    if (!calibrationOn.initialized)
    {
      return;
    }
  }

  if (mode == QTRReadMode::Off ||
      mode == QTRReadMode::OnAndOff ||
      mode == QTRReadMode::OddEvenAndOff)
  {
    if (!calibrationOff.initialized)
    {
      return;
    }
  }

  // read the needed values
  read(sensorValues, mode);

  for (uint8_t i = 0; i < _sensorCount; i++)
  {
    uint16_t calmin, calmax;

    // find the correct calibration
    if (mode == QTRReadMode::On ||
        mode == QTRReadMode::OddEven)
    {
      calmax = calibrationOn.maximum[i];
      calmin = calibrationOn.minimum[i];
    }
    else if (mode == QTRReadMode::Off)
    {
      calmax = calibrationOff.maximum[i];
      calmin = calibrationOff.minimum[i];
    }
    else // QTRReadMode::OnAndOff, QTRReadMode::OddEvenAndOff
    {
      if (calibrationOff.minimum[i] < calibrationOn.minimum[i])
      {
        // no meaningful signal
        calmin = _maxValue;
      }
      else
      {
        // this won't go past _maxValue
        calmin = calibrationOn.minimum[i] + _maxValue - calibrationOff.minimum[i];
      }

      if (calibrationOff.maximum[i] < calibrationOn.maximum[i])
      {
        // no meaningful signal
        calmax = _maxValue;
      }
      else
      {
        // this won't go past _maxValue
        calmax = calibrationOn.maximum[i] + _maxValue - calibrationOff.maximum[i];
      }
    }

    uint16_t denominator = calmax - calmin;
    int16_t value = 0;

    if (denominator != 0)
    {
      value = (((int32_t)sensorValues[i]) - calmin) * 1000 / denominator;
    }

    if (value < 0) { value = 0; }
    else if (value > 1000) { value = 1000; }

    sensorValues[i] = value;
  }
}

// Reads the first of every [step] sensors, starting with [start] (0-indexed, so
// start = 0 means start with the first sensor).
// For example, step = 2, start = 1 means read the *even-numbered* sensors.
// start defaults to 0, step defaults to 1
void QTRSensors::readPrivate(uint16_t * sensorValues, uint8_t start, uint8_t step)
{
  if (_sensorPins == nullptr) { return; }

  switch (_type)
  {
    case QTRType::RC:
      for (uint8_t i = start; i < _sensorCount; i += step)
      {
        sensorValues[i] = _maxValue;
        // make sensor line an output (drives low briefly, but doesn't matter)
        pinMode(_sensorPins[i], OUTPUT);
        // drive sensor line high
        digitalWrite(_sensorPins[i], HIGH);
      }

      delayMicroseconds(10); // charge lines for 10 us

      {
        // disable interrupts so we can switch all the pins as close to the same
        // time as possible
        noInterrupts();

        // record start time before the first sensor is switched to input
        // (similarly, time is checked before the first sensor is read in the
        // loop below)
        uint32_t startTime = micros();
        uint16_t time = 0;

        for (uint8_t i = start; i < _sensorCount; i += step)
        {
          // make sensor line an input (should also ensure pull-up is disabled)
          pinMode(_sensorPins[i], INPUT);
        }

        interrupts(); // re-enable

        while (time < _maxValue)
        {
          // disable interrupts so we can read all the pins as close to the same
          // time as possible
          noInterrupts();

          time = micros() - startTime;
          for (uint8_t i = start; i < _sensorCount; i += step)
          {
            if ((digitalRead(_sensorPins[i]) == LOW) && (time < sensorValues[i]))
            {
              // record the first time the line reads low
              sensorValues[i] = time;
            }
          }

          interrupts(); // re-enable
        }
      }
      return;

    case QTRType::Analog:
      // reset the values
      for (uint8_t i = start; i < _sensorCount; i += step)
      {
        sensorValues[i] = 0;
      }

      for (uint8_t j = 0; j < _samplesPerSensor; j++)
      {
        for (uint8_t i = start; i < _sensorCount; i += step)
        {
          // add the conversion result
          sensorValues[i] += analogRead(_sensorPins[i]);
        }
      }

      // get the rounded average of the readings for each sensor
      for (uint8_t i = start; i < _sensorCount; i += step)
      {
        sensorValues[i] = (sensorValues[i] + (_samplesPerSensor >> 1)) /
          _samplesPerSensor;
      }
      return;

    default: // QTRType::Undefined or invalid - do nothing
      return;
  }
}

uint16_t QTRSensors::readLinePrivate(uint16_t * sensorValues, QTRReadMode mode,
                         bool invertReadings)
{
  bool onLine = false;
  uint32_t avg = 0; // this is for the weighted total
  uint16_t sum = 0; // this is for the denominator, which is <= 64000

  // manual emitter control is not supported
  if (mode == QTRReadMode::Manual) { return 0; }

  readCalibrated(sensorValues, mode);

  for (uint8_t i = 0; i < _sensorCount; i++)
  {
    uint16_t value = sensorValues[i];
    if (invertReadings) { value = 1000 - value; }

    // keep track of whether we see the line at all
    if (value > 200) { onLine = true; }

    // only average in values that are above a noise threshold
    if (value > 50)
    {
      avg += (uint32_t)value * (i * 1000);
      sum += value;
    }
  }

  if (!onLine)
  {
    // If it last read to the left of center, return 0.
    if (_lastPosition < (_sensorCount - 1) * 1000 / 2)
    {
      return 0;
    }
    // If it last read to the right of center, return the max.
    else
    {
      return (_sensorCount - 1) * 1000;
    }
  }

  _lastPosition = avg / sum;
  return _lastPosition;
}

// the destructor frees up allocated memory
QTRSensors::~QTRSensors()
{
  releaseEmitterPins();

  if (_sensorPins)            { free(_sensorPins); }
  if (calibrationOn.maximum)  { free(calibrationOn.maximum); }
  if (calibrationOff.maximum) { free(calibrationOff.maximum); }
  if (calibrationOn.minimum)  { free(calibrationOn.minimum); }
  if (calibrationOff.minimum) { free(calibrationOff.minimum); }
}
