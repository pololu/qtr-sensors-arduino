# Arduino library for the Pololu QTR Reflectance Sensors

Version: 3.0.0<br>
Release date: 2016-08-16<br>
[![Build Status](https://travis-ci.org/pololu/qtr-sensors-arduino.svg?branch=master)](https://travis-ci.org/pololu/qtr-sensors-arduino)<br>
[www.pololu.com](https://www.pololu.com/)

## Summary

This is a library for an
[Arduino-compatible controller](https://www.pololu.com/arduino) that
interfaces with the
[Pololu QTR reflectance sensors](https://www.pololu.com/catalog/category/123).


## Getting started

### Software

If you are using version 1.6.2 or later of the
[Arduino software (IDE)](https://www.arduino.cc/en/Main/Software), you can use
the Library Manager to install this library:

1. In the Arduino IDE, open the "Sketch" menu, select "Include Library", then
   "Manage Libraries...".
2. Search for "QTRSensors".
3. Click the QTRSensors entry in the list.
4. Click "Install".

If this does not work, you can manually install the library:

1. Download the
   [latest release archive from GitHub](https://github.com/pololu/qtr-sensors-arduino/releases)
   and decompress it.
2. Rename the folder "qtr-sensors-arduino-xxxx" to "QTRSensors".
3. Drag the "QTRSensors" folder into the "libraries"
   directory inside your Arduino sketchbook directory.  You can view
   your sketchbook location by opening the "File" menu and selecting
   "Preferences" in the Arduino IDE.  If there is not already a
   "libraries" folder in that location, you should make the folder
   yourself.
4. After installing the library, restart the Arduino IDE.

## Examples

Several example sketches are available that show how to use the
library. You can access them from the Arduino IDE by opening the
"File" menu, selecting "Examples", and then selecting
"QTRSensors". If you cannot find these examples, the library was
probably installed incorrectly and you should retry the installation
instructions above.

## Documentation

For documentation, please see the
[user's guide](https://www.pololu.com/docs/0J19) for the library on
Pololu's website.

## Version history
* 3.0.0 (2016-08-16): Updated library to work with the Arduino Library manager.
* 2.1.1 (2014-05-02): Minor improvements to @read()@ behavior and whitespace cleanup.
* 2.1.0 (2013-04-18): Improved existing examples and added two new examples for printing raw values.
* 2.0.2 (2013-04-17): Made constructors initialize pointers to avoid possible problems.
* 2.0.1 (2012-09-13): Added a 200 us delay after emitter state changes to ensure sensors do not start being sampled before the LEDs turn on/off.
* 2.0.0 (2012-02-14): Initial release of library on GitHub (with Arduino 1.0 compatibility).
