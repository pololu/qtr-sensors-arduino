Usage Notes
===========

Calibration
-----------

This library allows you to use the QTRSensors::calibrate() method to easily calibrate your sensors for the particular conditions it will encounter. Calibrating your sensors can lead to substantially more reliable sensor readings, which in turn can help simplify your code. As such, we recommend you build a calibration phase into your application’s initialization routine. This can be as simple as a fixed duration over which you repeatedly call the `calibrate()` method. During this calibration phase, you will need to expose each of your reflectance sensors to the lightest and darkest readings they will encounter. For example, if you have made a line follower, you will want to slide it across the line during the calibration phase so the each sensor can get a reading of how dark the line is and how light the ground is. A sample calibration routine would be:

```cpp
#include <QTRSensors.h>

QTRSensors qtr;

void setup()
{
  // Optional: wait for some input from the user, such as a button press.

  // Initialize the sensors.
  // In this example we have three sensors on pins A0 - A2.
  qtr.setTypeRC(); // or setTypeAnalog()
  qtr.setSensorPins((const uint8_t[]){A0, A1, A2}, 3);

  // Optional: change parameters like RC timeout, set an emitter control pin...

  // Then start the calibration phase and move the sensors over both reflectance
  // extremes they will encounter in your application. This calibration should
  // take about 5 seconds (250 iterations * 20 ms per iteration).
  //
  // Passing a QTRReadMode to calibrate() is optional; it should match the mode
  // you plan to use when reading the sensors.
  for (uint8_t i = 0; i < 250; i++)
  {
    qtr.calibrate();
    delay(20);
  }

  // Optional: signal that the calibration phase is now over and wait for
  // further input from the user, such as a button press.
}
```

Reading the sensors
-------------------

This library gives you a number of different ways to read the sensors. All of these take an optional `mode` argument that determines how the IR emitters behave during the reading. (The options available depend on what sensor board you have; see ::QTRReadMode for details.)

1. You can request raw sensor values using the QTRSensors::read() method.

2. You can request calibrated sensor values using the QTRSensors::readCalibrated() method. Calibrated sensor values will always range from 0 to 1000, with 0 corresponding to the most reflective (brightest) surface encountered during calibration, and 1000 corresponding to the least reflective (darkest).

3. For line-detection applications, you can request the line location using the QTRSensors::readLineBlack() or QTRSensors::readLineWhite() methods. These are intended for use with a black line on a white background or a white line on a black background, respectively. Each of these methods provides calibrated readings for each sensor and returns an integer that tells you where it thinks the line is. If you are using N sensors, a returned value of 0 means it thinks the line is on or to the outside of sensor 0, and a returned value of 1000 &times; (N&minus;1) means it thinks the line is on or to the outside of sensor N&minus;1. As you slide your sensors across the line, the line position will change monotonically from 0 to 1000 &times; (N&minus;1), or vice versa. This line-position value can be used for closed-loop PID control.

A sample routine to obtain the sensor values and perform rudimentary line following would be:

```cpp
void loop()
{
  uint16_t sensors[3];
  // Get calibrated sensor values returned in the sensors array, along with the
  // line position, which will range from 0 to 2000, with 1000 corresponding to
  // a position under the middle sensor.
  int16_t position = qtr.readLineBlack(sensors);

  // If all three sensors see very low reflectance, take some appropriate action
  // for this  situation.
  if ((sensors[0] > 750) && (sensors[1] > 750) && (sensors[2] > 750))
  {
    // Do something. Maybe this means we're at the edge of a course or about to
    // fall off a table, in which case we might want to stop moving, back up,
    // and turn around.
    return;
  }

  // Compute our "error" from the line position. We will make it so that the
  // error is zero when the middle sensor is over the line, because this is our
  // goal. Error will range from -1000 to +1000. If we have sensor 0 on the left
  // and sensor 2 on the right,  a reading of -1000 means that we see the line
  // on the left and a reading of +1000 means we see the line on the right.
  int16_t error = position - 1000;

  int16_t leftMotorSpeed = 100;
  int16_t rightMotorSpeed = 100;
  if (error < -500)
  {
    // the line is on the left
    leftMotorSpeed = 0;  // turn left
  }
  if (error > 500)
  {
    // the line is on the right
    rightMotorSpeed = 0;  // turn right
  }

  // set motor speeds using the two motor speed variables above
}
```

PID Control
-----------

The integer value returned by QTRSensors::readLineWhite() or QTRSensors::readLineBlack() can be easily converted into a measure of your position error for line-following applications, as was demonstrated in the previous code sample. The function used to generate this position/error value is designed to be monotonic, which means the value will almost always change in the same direction as you sweep your sensors across the line. This makes it a great quantity to use for PID (proportional&ndash;integral&ndash;derivative) control.

Explaining PID control is beyond the scope of this document, but Wikipedia has a good [article](https://en.wikipedia.org/wiki/PID_controller) on the subject.

The following code gives a very simple example of PD (proportional&ndash;derivative) control (we find the integral term is usually not necessary for line following). The specific constants will be determined by your particular application, but note that the derivative constant Kd is usually much bigger than the proportional constant Kp. This is because the derivative (rate of change) of the error is a much smaller quantity than the error itself, so in order to produce a meaningful correction, it needs to be multiplied by a much larger constant.

```cpp

void loop()
{
  static uint16_t lastError = 0;

  uint16_t sensors[3];
  // Get calibrated sensor values returned in the sensors array, along with the
  // line position, which will range from 0 to 2000, with 1000 corresponding to
  // a position under the middle sensor
  int16_t position = qtr.readLineBlack(sensors);

  // Compute our "error" from the line position. We will make it so that the
  // error is zero when the middle sensor is over the line, because this is our
  // goal. Error will range from -1000 to +1000. If we have sensor 0 on the left
  // and sensor 2 on the right, a reading of -1000 means that we see the line on
  // the left and a reading of +1000 means we see the line on the right.
  int16_t error = position - 1000;

  // Set the motor speed based on proportional and derivative PID terms:
  // KP is the floating-point proportional constant (maybe start with a value around 0.1)
  // KD is the floating-point derivative constant (maybe start with a value around 5)
  // Note that when doing PID, it is very important you get your signs right, or
  // else the control loop will be unstable.
  int16_t motorSpeed = KP * error + KD * (error - lastError);
  lastError = error;

  // M1 and M2 are base motor speeds (the speeds the motors should run if you
  // are perfectly on the line with no error). If your motors are well matched,
  // M1 and M2 will be equal. When you start testing your PID loop, it might
  // help to start with small values for M1 and M2. You can then increase the
  // speed as you fine-tune your PID constants KP and KD.
  int16_t m1Speed = M1 + motorSpeed;
  int16_t m2Speed = M2 - motorSpeed;

  // It might help to keep the speeds positive (this is optional). You might
  // also want to add a similar check to keep the speeds from exceeding a
  // maximum limit.
  if (m1Speed < 0) { m1Speed = 0; }
  if (m2Speed < 0) { m2Speed = 0; }

  // set motor speeds using the two motor speed variables above
}
```
