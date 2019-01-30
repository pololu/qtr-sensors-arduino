#include "QTRSensors.h"
#include <Arduino.h>

void QTRSensors::setTypeRC()
{
  _type = Type::RC;
  _maxValue = _timeout;
}

void QTRSensors::setTypeAnalog()
{
  _type = Type::Analog;
  _maxValue = 1023; // Arduino analogRead() returns a 10-bit value by default
}

void QTRSensors::setSensorPins(uint8_t *pins, uint8_t sensorCount)
{
  if (sensorCount > MaxSensors)
  {
    sensorCount = MaxSensors;
  }

  if (_sensorPins == nullptr)
  {
    _sensorPins = (uint8_t *)malloc(sizeof(uint8_t) * sensorCount);
    if (_sensorPins == nullptr) { return; } // memory allocation failed
  }

  for (uint8_t i = 0; i < sensorCount; i++)
  {
    _sensorPins[i] = pins[i];
  }

  _sensorCount = sensorCount;
}

void QTRSensors::setTimeout(uint16_t timeout)
{
  _timeout = timeout;
  if (_type == Type::RC) { _maxValue = timeout; }
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
  if (_oddEmitterPin != NoEmitterPin)
  {
    pinMode(_oddEmitterPin, INPUT);
    _oddEmitterPin = NoEmitterPin;
  }

  if (_evenEmitterPin != NoEmitterPin)
  {
    pinMode(_evenEmitterPin, INPUT);
    _evenEmitterPin = NoEmitterPin;
  }

  _emitterPinCount = 0;
}

// Sets the dimming level (0-31). A dimming level of 0 corresponds
// to full current and brightness, with higher dimming levels meaning lower
// currents; see the product page for your sensor for details. The dimming level
// does not take effect until the next time emittersOn() is called.
void QTRSensors::setDimmingLevel(uint8_t dimmingLevel)
{
  if (dimmingLevel > 31)
  {
    dimmingLevel = 31;
  }
  _dimmingLevel = dimmingLevel;
}


// Turn the IR LEDs off and on.  These are mainly for use by the read
// method, and calling these functions before or after reading the sensors
// will have no effect on the readings unless the mode is
// ReadMode::Manual, but you may wish to use these for testing purposes.
//
// emitters defaults to Emitters::All; wait defaults to true
void QTRSensors::emittersOff(Emitters emitters, bool wait)
{
  bool pinChanged = false;

  // Use odd emitter pin in these cases:
  // - 1 emitter pin, emitters = all
  // - 2 emitter pins, emitters = all
  // - 2 emitter pins, emitters = odd
  if (emitters == Emitters::All || (_emitterPinCount == 2 && emitters == Emitters::Odd))
  {
    // Check if pin is defined and only turn off if not already off
    if ((_oddEmitterPin != NoEmitterPin) &&
        (digitalRead(_oddEmitterPin) == HIGH))
    {
      digitalWrite(_oddEmitterPin, LOW);
      pinChanged = true;
    }
  }

  // Use even emitter pin in these cases:
  // - 2 emitter pins, emitters = all
  // - 2 emitter pins, emitters = even
  if (_emitterPinCount == 2 && (emitters == Emitters::All || emitters == Emitters::Even))
  {
    // Check if pin is defined and only turn off if not already off
    if ((_evenEmitterPin != NoEmitterPin) &&
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

void QTRSensors::emittersOn(Emitters emitters, bool wait)
{
  bool pinChanged = false;
  uint16_t emittersOnStart;

  // Use odd emitter pin in these cases:
  // - 1 emitter pin, emitters = all
  // - 2 emitter pins, emitters = all
  // - 2 emitter pins, emitters = odd
  if (emitters == Emitters::All || (_emitterPinCount == 2 && emitters == Emitters::Odd))
  {
    // Check if pin is defined, and only turn on non-dimmable sensors if not
    // already on, but always turn dimmable sensors off and back on because
    // we might be changing the dimming level (emittersOnWithPin() should take
    // care of this)
    if ((_oddEmitterPin != NoEmitterPin) &&
        ( _dimmable || (digitalRead(_oddEmitterPin) == LOW)))
    {
      emittersOnStart = emittersOnWithPin(_oddEmitterPin);
      pinChanged = true;
    }
  }

  // Use even emitter pin in these cases:
  // - 2 emitter pins, emitters = all
  // - 2 emitter pins, emitters = even
  if (_emitterPinCount == 2 && (emitters == Emitters::All || emitters == Emitters::Even))
  {
    // Check if pin is defined, and only turn on non-dimmable sensors if not
    // already on, but always turn dimmable sensors off and back on because
    // we might be changing the dimming level (emittersOnWithPin() should take
    // care of this)
    if ((_evenEmitterPin != NoEmitterPin) &&
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

// assumes pin is valid (not NoEmitterPin)
// returns time when pin was first set high
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

// Turns on the selected emitters and turns off the other emitters with optimized
// timing (no unnecessary waits compared to calling emittersOff and
// emittersOn separately, but still waits for both emitterss of emitters to be
// in the right states before returning).
void QTRSensors::EmittersSelect(Emitters emitters)
{
  Emitters offEmitters;

  switch (emitters)
  {
    case Emitters::Odd:
      offEmitters = Emitters::Even;
      break;

    case Emitters::Even:
      offEmitters = Emitters::Odd;
      break;

    case Emitters::All:
      emittersOn();
      return;

    case Emitters::None:
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


// Resets the calibration.
void QTRSensors::resetCalibration()
{
  for (uint8_t i = 0; i < _sensorCount; i++)
  {
    if (calibratedMinimumOn)   { calibratedMinimumOn[i] = _maxValue; }
    if (calibratedMinimumOff)  { calibratedMinimumOff[i] = _maxValue; }
    if (calibratedMaximumOn)   { calibratedMaximumOn[i] = 0; }
    if (calibratedMaximumOff)  { calibratedMaximumOff[i] = 0; }
  }
}


// Reads the sensors 10 times and uses the results for
// calibration.  The sensor values are not returned; instead, the
// maximum and minimum values found over time are stored internally
// and used for the readCalibrated() method.
void QTRSensors::calibrate(ReadMode mode)
{
  // manual emitter control is not supported
  if (mode == ReadMode::Manual) { return; }

  if (mode == ReadMode::On ||
      mode == ReadMode::OnAndOff)
  {
    calibrateOnOrOff(&calibratedMinimumOn, &calibratedMaximumOn,
                     ReadMode::On);
  }
  else if (mode == ReadMode::OddEven ||
           mode == ReadMode::OddEvenAndOff)
  {
    calibrateOnOrOff(&calibratedMinimumOn, &calibratedMaximumOn,
                     ReadMode::OddEven);
  }

  if (mode == ReadMode::OnAndOff ||
      mode == ReadMode::OddEvenAndOff ||
      mode == ReadMode::Off)
  {
    calibrateOnOrOff(&calibratedMinimumOff, &calibratedMaximumOff,
                     ReadMode::Off);
  }
}


void QTRSensors::calibrateOnOrOff(uint16_t **calibratedMinimum,
                                  uint16_t **calibratedMaximum,
                                  ReadMode mode)
{
  uint16_t sensorValues[MaxSensors];
  uint16_t maxSensorValues[MaxSensors];
  uint16_t minSensorValues[MaxSensors];

  // Allocate the arrays if necessary.
  if (*calibratedMaximum == nullptr)
  {
    *calibratedMaximum = (uint16_t*)malloc(sizeof(uint16_t) * _sensorCount);

    // If the malloc failed, don't continue.
    if (*calibratedMaximum == nullptr) { return; }

    // Initialize the max and min calibrated values to values that
    // will cause the first reading to update them.
    for (uint8_t i = 0; i < _sensorCount; i++)
    {
      (*calibratedMaximum)[i] = 0;
    }
  }
  if (*calibratedMinimum == nullptr)
  {
    *calibratedMinimum = (uint16_t*)malloc(sizeof(uint16_t) * _sensorCount);

    // If the malloc failed, don't continue.
    if (*calibratedMinimum == nullptr) { return; }

    for (uint8_t i = 0; i < _sensorCount; i++)
    {
      (*calibratedMinimum)[i] = _maxValue;
    }
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
    // Update calibratedMaximum only if the min of 10 readings was still higher
    // than it (we got 10 readings in a row higher than the existing
    // calibratedMaximum).
    if (minSensorValues[i] > (*calibratedMaximum)[i])
    {
      (*calibratedMaximum)[i] = minSensorValues[i];
    }

    // Update calibratedMinimum only if the max of 10 readings was still lower
    // than it (we got 10 readings in a row lower than the existing
    // calibratedMinimum).
    if (maxSensorValues[i] < (*calibratedMinimum)[i])
    {
      (*calibratedMinimum)[i] = maxSensorValues[i];
    }
  }
}

// Reads the sensor values into an array. There *MUST* be space
// for as many values as there were sensors specified in the constructor.
// Example usage:
// uint16_t sensorValues[8];
// sensors.read(sensorValues);
// The values returned are a measure of the reflectance in abstract units,
// with higher values corresponding to lower reflectance (e.g. a black
// surface or a void).
void QTRSensors::read(uint16_t *sensorValues, ReadMode mode)
{
  switch (mode)
  {
    case ReadMode::Off:
      emittersOff();
      // fall through

    case ReadMode::Manual:
      readPrivate(sensorValues);
      return;

    case ReadMode::On:
    case ReadMode::OnAndOff:
      emittersOn();
      readPrivate(sensorValues);
      emittersOff();
      break;

    case ReadMode::OddEven:
    case ReadMode::OddEvenAndOff:
      // Turn on odd emitters and read the odd-numbered sensors.
      // (readPrivate takes a 0-based array index, so start = 0 to start with
      // the first sensor)
      EmittersSelect(Emitters::Odd);
      readPrivate(sensorValues, 0, 2);

      // Turn on even emitters and read the even-numbered sensors.
      // (readPrivate takes a 0-based array index, so start = 1 to start with
      // the second sensor)
      EmittersSelect(Emitters::Even);
      readPrivate(sensorValues, 1, 2);

      emittersOff();
      break;

    default: // invalid - do nothing
      return;
  }

  if (mode == ReadMode::OnAndOff ||
      mode == ReadMode::OddEvenAndOff)
  {
    // Take a second set of readings and return the values (on + max - off).

    uint16_t offValues[MaxSensors];
    readPrivate(offValues);

    for (uint8_t i = 0; i < _sensorCount; i++)
    {
      sensorValues[i] += _maxValue - offValues[i];
    }
  }
}


// Returns values calibrated to a value between 0 and 1000, where
// 0 corresponds to the minimum value read by calibrate() and 1000
// corresponds to the maximum value.  Calibration values are
// stored separately for each sensor, so that differences in the
// sensors are accounted for automatically.
void QTRSensors::readCalibrated(uint16_t *sensorValues, ReadMode mode)
{
  // manual emitter control is not supported
  if (mode == ReadMode::Manual) { return; }

  // if not calibrated, do nothing

  if (mode == ReadMode::On ||
      mode == ReadMode::OnAndOff ||
      mode == ReadMode::OddEvenAndOff)
  {
    if ((calibratedMinimumOn == nullptr) || (calibratedMaximumOn == nullptr))
    {
      return;
    }
  }

  if (mode == ReadMode::Off ||
      mode == ReadMode::OnAndOff ||
      mode == ReadMode::OddEvenAndOff)
  {
    if ((calibratedMinimumOff == nullptr) || (calibratedMaximumOff == nullptr))
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
    if (mode == ReadMode::On ||
        mode == ReadMode::OddEven)
    {
      calmax = calibratedMaximumOn[i];
      calmin = calibratedMinimumOn[i];
    }
    else if (mode == ReadMode::Off)
    {
      calmax = calibratedMaximumOff[i];
      calmin = calibratedMinimumOff[i];
    }
    else // ReadMode::OnAndOff, ReadMode::OddEvenAndOff
    {
      if (calibratedMinimumOff[i] < calibratedMinimumOn[i])
      {
        // no meaningful signal
        calmin = _maxValue;
      }
      else
      {
        // this won't go past _maxValue
        calmin = calibratedMinimumOn[i] + _maxValue - calibratedMinimumOff[i];
      }

      if (calibratedMaximumOff[i] < calibratedMaximumOn[i])
      {
        // no meaningful signal
        calmax = _maxValue;
      }
      else
      {
        // this won't go past _maxValue
        calmax = calibratedMaximumOn[i] + _maxValue - calibratedMaximumOff[i];
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

// Reads the sensor values into an array. There *MUST* be space
// for as many values as there were sensors specified in the constructor.
// Example usage:
// uint16_t sensorValues[8];
// ...
// sensors.read(sensorValues);
// The values returned are in microseconds and range from 0 to
// timeout (as specified in the constructor).
// The values returned are a measure of the reflectance in terms of a
// 10-bit ADC average with higher values corresponding to lower
// reflectance (e.g. a black surface or a void).
// A 'step' of n means that the first of every n sensors is read, starting
// with 'start' (this is 0-indexed, so start = 0 means start with the first
// sensor). For example, step = 2, start = 1 means read the *even-numbered*
// sensors.

// emitters defaults to Emitters::All
void QTRSensors::readPrivate(uint16_t *sensorValues, uint8_t start, uint8_t step)
{
  if (_sensorPins == nullptr) { return; }

  switch (_type)
  {
    case Type::RC:
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

    case Type::Analog:
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

    default: // QTR_TYPE_UNDEFINED or invalid - do nothing
      return;
  }
}

int QTRSensors::readLinePrivate(uint16_t *sensorValues, ReadMode mode,
                         bool invertReadings)
{
  bool onLine = false;
  uint32_t avg = 0; // this is for the weighted total
  uint16_t sum = 0; // this is for the denominator, which is <= 64000

  // manual emitter control is not supported
  if (mode == ReadMode::Manual) { return; }

  readCalibrated(sensorValues, mode);

  avg = 0;
  sum = 0;

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

  if (_sensorPins)          { free(_sensorPins); }
  if (calibratedMaximumOn)  { free(calibratedMaximumOn); }
  if (calibratedMaximumOff) { free(calibratedMaximumOff); }
  if (calibratedMinimumOn)  { free(calibratedMinimumOn); }
  if (calibratedMinimumOff) { free(calibratedMinimumOff); }
}
