/**************************************************************************************************
 * Conway's Game of Life 
 * 
 * This implementation was written as a reusable animator class where the RGB matrix hardware
 * rendering class is passed in, so it can be used with any RGB array display by writing an 
 * implementation of a renderer class to set pixels/LED colours on the hardware.
 *
 * Based on code which was originally published in my Ardunio code examples repo:
 * https://github.com/Footleg/arduino
 * 
* Copyright (C) 2020 Paul Fretwell - aka 'Footleg'
 * 
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software.  If not, see <http://www.gnu.org/licenses/>.
 */

/* NOTE: This code resets the simulation when repeating end conditions occur.
         The code detects repeating patterns of up to 24 cycles.
		 The reason each simulation is terminated is output to stderr.
*/

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#elif defined(ARDUINO)
#include "WProgram.h"
#else
#include <stdio.h>
#endif

#include <tuple>

#include "RGBMatrixRenderer.h"

const uint16_t N_GRAINS=256; // Number of grains of sand

class FallingSand
{
    //variables
    public:
    protected:
    private:
        int delayms;
        RGBMatrixRenderer &renderer;

        struct Grain {
            int16_t  x,  y; // Position
            int16_t vx, vy; // Velocity
        } grain[N_GRAINS];

        int maxX;
        int maxY;
        uint8_t* img; // Internal 'map' of pixels
        int16_t accelX;
        int16_t accelY;
        int16_t accelAbs;
        int16_t shake;
        float velCap;
    //functions
    public:
        FallingSand(RGBMatrixRenderer&,int16_t);
        ~FallingSand();
        void runCycle();
        void setAcceleration(int16_t,int16_t);
    protected:
    private:
}; //FallingSand
