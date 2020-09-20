# Arduino Code
This is a repository of code examples and libraries I have written for the Ardunio development environment, and other reusable C++ code.

## Contents
### animations - Projects to generate RGB LED matrix display animations
A set of animator classes to generate LED array animations. These are reusable with any LED matrix or other display device which supports RGB colour values per pixel or LED. A Makefile is provided to compile these for the RGB Matrix library by Henner Zeller to run on a Raspberry Pi.

### GOL.cpp - Conway's Game of Life
An example written to run on the Unicorn HAT HD, using the Teensy To Zero adapter board from ZodiusInfuser. It depends on the libraries in this repo. https://github.com/ZodiusInfuser/ToZeroAdapters/tree/master/examples/ToZero_UnicornHATHD

### T-Watch-2000
A starting development framework for building a UI and apps for the Lilygo T-Watch-2000 ESP32 based smart watch.
Building on work by Dan Geiger, which was based on code by Lewis He.

See Dan's Instructable guide to setting up the Arduino IDE and some more apps you can drop into 
the menus. https://www.instructables.com/id/Lilygo-T-Watch-2020-Arduino-Framework

The code by Lewis He does a lot of lower level stuff and provides a very nice looking UI, but I
found it very difficult to get started with. You can find it here:
https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library
