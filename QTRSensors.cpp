//#include <stdlib.h>
#include "QTRSensors.h"
#include <Arduino.h>

// Base class constructor
QTRSensors::QTRSensors() :
  _type(QTR_TYPE_UNDEFINED),
  _sensorPins(nullptr),
  _sensorCount(0),
  _timeout(QTR_RC_DEFAULT_TIMEOUT),
  _maxValue(QTR_RC_DEFAULT_TIMEOUT),
  _samplesPerSensor(4),
  _emitterPinCount(0),
  _oddEmitterPin(QTR_NO_EMITTER_PIN),
  _evenEmitterPin(QTR_NO_EMITTER_PIN),
  _dimmable(true), // TODO decide if this should default to false
  _dimmingLevel(0),
  _lastPosition(0),
  calibratedMinimumOn(nullptr),
  calibratedMaximumOn(nullptr),
  calibratedMinimumOff(nullptr),
  calibratedMaximumOff(nullptr)
{
    // empty
}


void QTRSensors::setType(uint8_t type)
{
  switch (type)
  {
    case QTR_TYPE_RC:
      _type = QTR_TYPE_RC;
      _maxValue = _timeout;
      break;

    case QTR_TYPE_ANALOG:
      _type = QTR_TYPE_ANALOG;
      _maxValue = 1023; // Arduino analogRead() returns a 10-bit value by default
      break;

    default:
      _type = QTR_TYPE_UNDEFINED;
  }
}

void QTRSensors::setSensorPins(uint8_t *pins, uint8_t sensorCount)
{
  if (sensorCount > QTR_MAX_SENSORS)
  {
    sensorCount = QTR_MAX_SENSORS;
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
  if (_type == QTR_TYPE_RC) { _maxValue = timeout; }
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
  if (_oddEmitterPin != QTR_NO_EMITTER_PIN)
  {
    pinMode(_oddEmitterPin, INPUT);
    _oddEmitterPin = QTR_NO_EMITTER_PIN;
  }

  if (_evenEmitterPin != QTR_NO_EMITTER_PIN)
  {
    pinMode(_evenEmitterPin, INPUT);
    _evenEmitterPin = QTR_NO_EMITTER_PIN;
  }
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
// will have no effect on the readings unless the readMode is
// QTR_EMITTERS_MANUAL, but you may wish to use these for testing purposes.
//
// bank defaults to QTR_BANK_ALL; wait defaults to true
void QTRSensors::emittersOff(uint8_t bank, bool wait)
{
  bool pinChanged = false;

  // Use odd emitter pin in these cases:
  // - 1 emitter pin, bank = all
  // - 2 emitter pins, bank = all
  // - 2 emitter pins, bank = odd
  if (bank == QTR_BANK_ALL || (_emitterPinCount == 2 && bank == QTR_BANK_ODD))
  {
    if (_oddEmitterPin != QTR_NO_EMITTER_PIN)
    {
      digitalWrite(_oddEmitterPin, LOW);
      pinChanged = true;
    }
  }

  // Use even emitter pin in these cases:
  // - 2 emitter pins, bank = all
  // - 2 emitter pins, bank = even
  if (_emitterPinCount == 2 && (bank == QTR_BANK_ALL || bank == QTR_BANK_EVEN))
  {
    if (_evenEmitterPin != QTR_NO_EMITTER_PIN)
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

void QTRSensors::emittersOn(uint8_t bank, bool wait)
{
  bool pinChanged = false;
  uint16_t emittersOnStart;

  // Use odd emitter pin in these cases:
  // - 1 emitter pin, bank = all
  // - 2 emitter pins, bank = all
  // - 2 emitter pins, bank = odd
  if (bank == QTR_BANK_ALL || (_emitterPinCount == 2 && bank == QTR_BANK_ODD))
  {
    if (_oddEmitterPin != QTR_NO_EMITTER_PIN)
    {
      emittersOnStart = emittersOnWithPin(_oddEmitterPin);
      pinChanged = true;
    }
  }

  // Use even emitter pin in these cases:
  // - 2 emitter pins, bank = all
  // - 2 emitter pins, bank = even
  if (_emitterPinCount == 2 && (bank == QTR_BANK_ALL || bank == QTR_BANK_EVEN))
  {
    if (_evenEmitterPin != QTR_NO_EMITTER_PIN)
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

// assumes pin is valid (not QTR_NO_EMITTER_PIN)
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

// Turns on the selected bank and turns off the other bank with optimized
// timing (no unnecessary waits compared to calling emittersOff and
// emittersOn separately, but still waits for both banks of emitters to be
// in the right states before returning).
void QTRSensors::emitterBankSelect(uint8_t bank)
{
  uint8_t offBank;

  switch (bank)
  {
    case QTR_BANK_ODD:
      offBank = QTR_BANK_EVEN;
      break;

    case QTR_BANK_EVEN:
      offBank = QTR_BANK_ODD;
      break;

    case QTR_BANK_ALL:
      emittersOn();
      return;

    case QTR_BANK_NONE:
      emittersOff();
      return;

    default: // invalid
      return;
  }

  // Turn off the off-bank; don't wait before proceeding, but record the time.
  emittersOff(offBank, false);
  uint16_t turnOffStart = micros();

  // Turn on the on-bank and wait.
  emittersOn(bank);

  if (_dimmable)
  {
    // Finish waiting for the off-bank emitters to turn off: make sure it's been
    // at least 1200 us since the off-bank was turned off before returning.
    // (Driver min is 1 ms.) Some time has already passed while we waited for
    // the on-bank to turn on.
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
void QTRSensors::calibrate(uint8_t readMode)
{
  // manual emitter control is not supported
  if (readMode == QTR_EMITTERS_MANUAL) { return; }

  if (readMode == QTR_EMITTERS_ON ||
      readMode == QTR_EMITTERS_ON_AND_OFF)
  {
    calibrateOnOrOff(&calibratedMinimumOn, &calibratedMaximumOn,
                     QTR_EMITTERS_ON);
  }
  else if (readMode == QTR_EMITTERS_ODD_EVEN ||
           readMode == QTR_EMITTERS_ODD_EVEN_AND_OFF)
  {
    calibrateOnOrOff(&calibratedMinimumOn, &calibratedMaximumOn,
                     QTR_EMITTERS_ODD_EVEN);
  }

  if (readMode == QTR_EMITTERS_ON_AND_OFF ||
      readMode == QTR_EMITTERS_ODD_EVEN_AND_OFF ||
      readMode == QTR_EMITTERS_OFF)
  {
      calibrateOnOrOff(&calibratedMinimumOff, &calibratedMaximumOff,
                       QTR_EMITTERS_OFF);
  }
}


void QTRSensors::calibrateOnOrOff(uint16_t **calibratedMinimum,
                                  uint16_t **calibratedMaximum,
                                  uint8_t readMode)
{
  uint16_t sensorValues[16];
  uint16_t maxSensorValues[16];
  uint16_t minSensorValues[16];

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
    read(sensorValues, readMode);

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
void QTRSensors::read(uint16_t *sensorValues, uint8_t readMode)
{
  switch (readMode)
  {
    case QTR_EMITTERS_OFF:
      emittersOff();
      // fall through

    case QTR_EMITTERS_MANUAL:
      readPrivate(sensorValues);
      return;

    case QTR_EMITTERS_ON:
    case QTR_EMITTERS_ON_AND_OFF:
      emittersOn();
      readPrivate(sensorValues);
      emittersOff();
      break;

    case QTR_EMITTERS_ODD_EVEN:
    case QTR_EMITTERS_ODD_EVEN_AND_OFF:
      // Turn on odd emitters and read the odd-numbered sensors.
      // (readPrivate takes a 0-based array index, so start = 0 to start with
      // the first sensor)
      emitterBankSelect(QTR_BANK_ODD);
      readPrivate(sensorValues, QTR_BANK_ODD);

      // Turn on even emitters and read the even-numbered sensors.
      // (readPrivate takes a 0-based array index, so start = 1 to start with
      // the second sensor)
      emitterBankSelect(QTR_BANK_EVEN);
      readPrivate(sensorValues, QTR_BANK_EVEN);

      emittersOff();
      break;

    default: // invalid - do nothing
      return;
  }

  if (readMode == QTR_EMITTERS_ON_AND_OFF ||
      readMode == QTR_EMITTERS_ODD_EVEN_AND_OFF)
  {
    // Take a second set of readings and return the values (on + max - off).

    uint16_t offValues[QTR_MAX_SENSORS];
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
void QTRSensors::readCalibrated(uint16_t *sensorValues, uint8_t readMode)
{
  // manual emitter control is not supported
  if (readMode == QTR_EMITTERS_MANUAL) { return; }

  // if not calibrated, do nothing

  if (readMode == QTR_EMITTERS_ON ||
      readMode == QTR_EMITTERS_ON_AND_OFF ||
      readMode == QTR_EMITTERS_ODD_EVEN_AND_OFF)
  {
    if ((calibratedMinimumOn == nullptr) || (calibratedMaximumOn == nullptr))
    {
      return;
    }
  }

  if (readMode == QTR_EMITTERS_OFF ||
      readMode == QTR_EMITTERS_ON_AND_OFF ||
      readMode == QTR_EMITTERS_ODD_EVEN_AND_OFF)
  {
    if ((calibratedMinimumOff == nullptr) || (calibratedMaximumOff == nullptr))
    {
      return;
    }
  }

  // read the needed values
  read(sensorValues, readMode);

  for (uint8_t i = 0; i < _sensorCount; i++)
  {
    uint16_t calmin, calmax;

    // find the correct calibration
    if (readMode == QTR_EMITTERS_ON ||
        readMode == QTR_EMITTERS_ODD_EVEN)
    {
      calmax = calibratedMaximumOn[i];
      calmin = calibratedMinimumOn[i];
    }
    else if (readMode == QTR_EMITTERS_OFF)
    {
      calmax = calibratedMaximumOff[i];
      calmin = calibratedMinimumOff[i];
    }
    else // QTR_EMITTERS_ON_AND_OFF, QTR_EMITTERS_ODD_EVEN_AND_OFF
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
int QTRSensors::readLine(uint16_t *sensorValues, uint8_t readMode,
                         uint8_t whiteLine)
{
  bool onLine = false;
  uint32_t avg = 0; // this is for the weighted total
  uint16_t sum = 0; // this is for the denominator, which is <= 64000

  // manual emitter control is not supported
  if (readMode == QTR_EMITTERS_MANUAL) { return; }

  readCalibrated(sensorValues, readMode);

  avg = 0;
  sum = 0;

  for (uint8_t i = 0; i < _sensorCount; i++)
  {
    uint16_t value = sensorValues[i];
    if (whiteLine) { value = 1000 - value; }

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

// bank defaults to QTR_BANK_ALL
void QTRSensors::readPrivate(uint16_t *sensorValues, uint8_t bank)
{
  // starting value and increment for sensorValues array index
  uint8_t start = 0, step = 1;

  switch (bank)
  {
    case QTR_BANK_ALL:  // start = 0, step = 1 (no change)
      break;

    case QTR_BANK_EVEN: // start = 1, step = 2
      start = 1;
      // fall through to set step

    case QTR_BANK_ODD:  // start = 0, step = 2
      step = 2;
      break;

    default: // QTR_BANK_NONE or invalid - do nothing
      return;
  }

  if (_sensorPins == nullptr) { return; }

  switch (_type)
  {
    case QTR_TYPE_RC:
      for (uint8_t i = start; i < _sensorCount; i += step)
      {
        sensorValues[i] = _maxValue;
        pinMode(_sensorPins[i], OUTPUT);    // make sensor line an output (drives low briefly, but doesn't matter)
        digitalWrite(_sensorPins[i], HIGH); // drive sensor line high
      }

      delayMicroseconds(10);                // charge lines for 10 us

      for (uint8_t i = start; i < _sensorCount; i += step)
      {
        pinMode(_sensorPins[i], INPUT);     // make sensor line an input
        digitalWrite(_sensorPins[i], LOW);  // important: disable internal pull-up!
      }

      {
        uint32_t startTime = micros();
        uint16_t time = 0;
        while (time < _maxValue)
        {
          time = micros() - startTime;
          for (uint8_t i = start; i < _sensorCount; i += step)
          {
            if ((digitalRead(_sensorPins[i]) == LOW) && (time < sensorValues[i]))
            {
              // record the first time the line reads low
              sensorValues[i] = time;
            }
          }
        }
      }
      return;

    case QTR_TYPE_ANALOG:
      // reset the values
      for (uint8_t i = start; i < _sensorCount; i += step)
      {
        sensorValues[i] = 0;
      }

      for (uint8_t j = 0; j < _samplesPerSensor; j++)
      {
        for (uint8_t i = start; i < _sensorCount; i += step)
        {
          sensorValues[i] += analogRead(_sensorPins[i]); // add the conversion result
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


// the destructor frees up allocated memory
QTRSensors::~QTRSensors()
{
  releaseEmitterPins();

  if (_sensorPins)          { free(_sensorPins); }
  if (calibratedMaximumOn)   { free(calibratedMaximumOn); }
  if (calibratedMaximumOff)  { free(calibratedMaximumOff); }
  if (calibratedMinimumOn)   { free(calibratedMinimumOn); }
  if (calibratedMinimumOff)  { free(calibratedMinimumOff); }
}
