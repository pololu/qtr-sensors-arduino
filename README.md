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

For documentation, please see the
[user's guide](https://www.pololu.com/docs/0J19) for the library on
Pololu's website.

## Version history
* 3.0.0 (2016-08-16): Updated library to work with the Arduino Library Manager.
* 2.1.2 (2015-12-03): Corrected `readLine()` behavior to work with multiple instances (thanks to Tandy Carmichael).
* 2.1.1 (2014-05-02): Minor improvements to `read()` behavior and whitespace cleanup.
* 2.1.0 (2013-04-18): Improved existing examples and added two new examples for printing raw values.
* 2.0.2 (2013-04-17): Made constructors initialize pointers to avoid possible problems.
* 2.0.1 (2012-09-13): Added a 200 us delay after emitter state changes to ensure sensors do not start being sampled before the LEDs turn on/off.
* 2.0.0 (2012-02-14): Initial release of library on GitHub (with Arduino 1.0 compatibility).
