/**************************************************************************************************
 *  Conway's Game of Life 
 * 
 * This implementation was written for the Teensy using the TOZERO board to run a 
 * UNICORN HAT HD RGB LED array. The TOZERO and UNICORNHATHD libraries can be found at  
 * https://github.com/ZodiusInfuser/ToZeroAdapters/tree/master/examples/ToZero_UnicornHATHD
 *
 * Copyright (C) 2020 Paul Fretwell - aka 'Footleg'
 * 
 * This file is part of my Ardunio code examples repo:
 * https://github.com/Footleg/arduino
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
 * along with Cave Converter.  If not, see <http://www.gnu.org/licenses/>.
 */

/* NOTE: This code resets the simulation when repeating end conditions occur.
         The code to detect longer cycle repeating patterns is not complete here
     so patterns repeating in over 4 cycles will continue forever. I am still
     testing the code which detects 5-16 cycle patterns.
*/

#define NO_TO_ZERO_WARNINGS

/***** Library Includes *****/
#include <FastLED.h>
#include <ToZero.h>

/***** Project Includes *****/
#include "UnicornHD.h"

/***** Global Constants *****/
static uint8_t const gridSize = 16;
static uint8_t const maxBrightness = 128;
static uint8_t const maxGap = 16;

/***** Global Variables *****/
UnicornHD unicorn;

bool cells[gridSize][gridSize] = {};
bool cellsLast1[gridSize][gridSize] = {};
bool cellsLast2[gridSize][gridSize] = {};
bool cellsLast3[gridSize][gridSize] = {};
bool cellsBorn[gridSize][gridSize] = {};
bool cellsDying[gridSize][gridSize] = {};

uint8_t alive = 0;
uint8_t population[48] = {};
uint8_t popCursor = 47; //Set to last position as gets incremented before use
uint8_t unchangedCount = 0;
uint8_t repeat2Count = 0;
uint8_t repeat3Count = 0;
uint8_t unchangedPopulation[maxGap] = {};
uint32_t iterations = 0;
uint32_t iterationsMin = 999999999999;
uint32_t iterationsMax = 0;

CRGB cellColour;

///////////////////////////////////////////////////////////////////////////////////////////////////
// SETUP
///////////////////////////////////////////////////////////////////////////////////////////////////
void setup()
{
  uint8_t x;
  
  // Open serial communications and wait for port to open:
  Serial.begin(19200);
  /*
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Native USB only
  }
  */
  randomSeed(analogRead(0));
  unicorn.Begin();
  unicorn.SetBrightness(maxBrightness);
  cellColour = CRGB::Red;
  for (x = 0; x < maxGap; ++x) unchangedPopulation[x] = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// LOOP
///////////////////////////////////////////////////////////////////////////////////////////////////
void loop()
{
  int8_t x, y, xt, yt, xi, yi, neighbours;

  /* Reinitialise simulation on the following conditions:
   *  - All cells dead.
   *  - No changes have occurred between consecutive frames (static pattern)
   *  - Pattern alternates between just 2 different states 
   *  - Pattern cycles between 3 different states 
   *  - Population remains constant for 50 consecutive frames (gliding pattern)
   *    * Allow 150 frames if more than 5 cells as glider may collide with something
   *  - Populate size repeats over a 4 cycle period for over 40 cycles
   */
  if ( (alive == 0) || (unchangedCount > 5 ) || (repeat2Count > 6) || (repeat3Count > 35) 
    || (unchangedPopulation[0] > 150) || ( (unchangedPopulation[0] > 50) && (alive == 5 ) ) 
    || (unchangedPopulation[3] > 40) )
  {
    //Update min and max iterations counters
    if (iterations > 0)
    {
      if (iterations < iterationsMin) iterationsMin = iterations;
      if (iterations > iterationsMax) iterationsMax = iterations;

    }
    //Debug end condition detected
    Serial.print("Pattern terminated after ");
    Serial.print(iterations);
    Serial.print(" iterations (min: ");
    Serial.print(iterationsMin);
    Serial.print(", max: ");
    Serial.print(iterationsMax);
    Serial.print("): ");
    if (alive == 0)
      Serial.println("All died");
    else if (unchangedCount > 5 )
      Serial.println("Static pattern for 5 frames");
    else if (repeat2Count > 6)
      Serial.println("Pattern repeated over 2 frames");
    else if (repeat3Count > 35)
      Serial.println("Pattern repeated over 3 frames");
    else if (unchangedPopulation[0] > 150)
      Serial.println("Population static over 150 frames");
    else if ( (unchangedPopulation[0] > 50) && (alive == 5 ) )
      Serial.println("Population static over 50 frames with 5 cells exactly");
    else if (unchangedPopulation[3] > 40)
      Serial.println("Population repeated on 4 step cycle");
    
    initialiseGrid(0);

    updatePixels();
    unicorn.Show();
  }

  //Show status of simulation on LEDs
  updatePixels();
  unicorn.Show();
  
  
  //Apply rules of Game of Life to determine cells dying and being born
  for(y = 0; y < gridSize; ++y)
  {
    for(x = 0; x < gridSize; ++x)
    {
      //For each cell, count neighbours, including wrapping over grid edges
      neighbours = -1;
      for(xi = -1; xi < 2; ++xi)
      {
        xt = x + xi;
        if (xt < 0)
        {
          xt += gridSize;
        }
        else if (xt >= gridSize)
        {
          xt -= gridSize;
        }
        for(yi = -1; yi < 2; ++yi)
        {
          yt = y + yi;
          if (yt < 0)
          {
            yt += gridSize;
          }
          else if (yt >= gridSize)
          {
            yt -= gridSize;
          }
          if (cells[xt][yt])
          {
            ++neighbours;
          }
        }
      }

      //Reset changes arrays for this cell
      cellsBorn[x][y] = false;
      cellsDying[x][y] = false;
      if ( (cells[x][y]) && (neighbours < 2) )
      {
        //Populated cell with too few neighbours, so it will die
        cellsDying[x][y] = true; //Kill
      }
      else if ( (cells[x][y] == false) && (neighbours == 2) ) 
      {
        //Empty cell with exactly 3 neighbours (count=2 as didn't count itself
        //since cell is empty, and counter was initialised as -1)
        cellsBorn[x][y] = true; //Spawn
      }
      else if ( (cells[x][y]) && (neighbours > 3) )
      {
        //Populated cell with too many neighbours, so it will die
        cellsDying[x][y] = true; //Kill
      }
    }
  }

  fadeInChanges();
  
  applyChanges();
  
  if (alive == 0)
  {
    //Show end of population before it gets reset
    updatePixels();
    unicorn.Show();
    //delay(4000);
  }

  iterations++;
  delay(10); //Use 10 with fade effects on
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Initialise Grid
///////////////////////////////////////////////////////////////////////////////////////////////////
void initialiseGrid(uint8_t pattern)
{
  uint8_t x, y, col;

  alive = 0;
  iterations = 0;
  unchangedCount = 0;
  for (x = 0; x < maxGap; ++x) unchangedPopulation[x] = 0;
  repeat2Count = 0;
  repeat3Count = 0;
  for (x = 0; x < 48; ++x) population[x] = 0;

  col = random(0,6);
  switch(col)
  {
    case 0:
      cellColour = CRGB::Purple;
      break;
    case 1:
      cellColour = CRGB::Magenta;
      break;
    case 2:
      cellColour = CRGB::Blue;
      break;
    case 3:
      cellColour = CRGB::Yellow;
      break;
    case 4:
      cellColour = CRGB::Cyan;
      break;
    case 5:
      cellColour = CRGB::White;
      break;
  }

  if (pattern == 0) {
    //Random
    for(y = 0; y < gridSize; ++y)
    {
      for(x = 0; x < gridSize; ++x)
      {
        uint8_t randNumber = random(0, 100);
        if (randNumber < 15)
        {
           cells[x][y] = true;
           alive++;
        }
        else
        {
          cells[x][y] = false;
        }
      }
    }
  }
  else
  {
    switch(pattern)
    {
      case 1:
        cells[6][4] = true;
        cells[4][5] = true;
        cells[8][5] = true;
        cells[9][6] = true;
        cells[4][7] = true;
        cells[9][7] = true;
        cells[5][8] = true;
        cells[6][8] = true;
        cells[7][8] = true;
        cells[8][8] = true;
        cells[9][8] = true;
        break;
      default:
        cells[7][6] = true;
        cells[6][7] = true;
        cells[8][7] = true;
        cells[5][8] = true;
        cells[9][8] = true;
        break;
    }

    //Count cells in pattern
    for(y = 0; y < gridSize; ++y)
    {
      for(x = 0; x < gridSize; ++x)
      {
        if (cells[x][y]) alive++;
      }
    }
  }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Fade births in green, and death to red
///////////////////////////////////////////////////////////////////////////////////////////////////
void fadeInChanges()
{
  uint8_t i, x, y;
  const uint8_t fadeSteps = 20; //Controls speed of simulation. Around 24 steps is reasonable speed.
  const uint8_t halfSteps = fadeSteps / 2; 
  uint16_t idx = 0;
  CRGB colourB, colourD;
  
  for(i = 0; i < fadeSteps; ++i)
  {
    idx = 0;

    if (i < halfSteps)
    {
      //Set fade from black to green for cells being born
      colourB.red = 0;
      colourB.green = maxBrightness*i/halfSteps;
      colourB.blue = 0;
      //Set fade cells dying out from current colour to red
      colourD.red = cellColour.r + (maxBrightness-cellColour.r)*i/halfSteps;
      colourD.green = (halfSteps-i)*cellColour.g/halfSteps;
      colourD.blue = (halfSteps-i)*cellColour.b/halfSteps;
    }
    else
    {
      //Set fade from green to active cell colour for cells being born
      colourB.red = (i-halfSteps)*cellColour.r/halfSteps;
      colourB.green = maxBrightness - ((maxBrightness-cellColour.g)*(i-halfSteps)/halfSteps);
      colourB.blue = (i-halfSteps)*cellColour.b/halfSteps;
      //Set fade colour for cells dying out
      colourD.red = (halfSteps-(i-halfSteps))*maxBrightness/halfSteps-1;
      colourD.green = 0;
      colourD.blue = 0;
    }
    
    for(y = 0; y < gridSize; ++y)
    {
      for(x = 0; x < gridSize; ++x)
      {
        //Fade births up to green
        if (cellsBorn[x][gridSize-1-y])
        {
          unicorn.Pixels()[idx] = colourB;
        }
        //Fade dying to red
        if (cellsDying[x][gridSize-1-y])
        {
          unicorn.Pixels()[idx] = colourD;
        }
        ++idx;
      }
    }
    unicorn.Show();
    delay(50);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Update cells array with changes for next iteration
///////////////////////////////////////////////////////////////////////////////////////////////////
void applyChanges()
{
  uint8_t x, y, changes, i, j, gap;
  int8_t popChk,prevPopChk;
  
  changes = 0;
  bool compare2 = true;
  bool compare3 = true;
  
  for(y = 0; y < gridSize; ++y)
  {
    for(x = 0; x < gridSize; ++x)
    {
      //Update last 3 iterations history for this cell
      cellsLast3[x][y] = cellsLast2[x][y];
      cellsLast2[x][y] = cellsLast1[x][y];
      cellsLast1[x][y] = cells[x][y];

      if (cellsBorn[x][y])
      {
        cells[x][y] = true;
        ++changes;
        ++alive;
      }
      
      if (cellsDying[x][y]) 
      {
          cells[x][y] = false;
          ++changes;
          --alive;
      }
      
      //Compare cell to state 2 and 3 iterations ago
      if (cells[x][y] != cellsLast2[x][y]) compare2 = false;
      if (cells[x][y] != cellsLast3[x][y]) compare3 = false;
    }
  }

  popCursor++;
  if (popCursor > 47) popCursor = 0;
  population[popCursor] = alive;
  
  //Increment counter if no changes made
  if (changes == 0)
    ++unchangedCount;
  else
    unchangedCount = 0;

  //Increment counter if last frame was identical to 2 frames ago (2 cycle repeat)
  if (compare2)
    ++repeat2Count;
  else
    repeat2Count = 0;

  //Increment counter if last frame was identical to 3 frames ago (3 cycle repeat)
  if (compare3)
    ++repeat3Count;
  else
    repeat3Count = 0;

  //Increment counter of unchanging population size
  if (popCursor == 0)
    popChk = 47;
  else
    popChk = popCursor - 1;

  if (population[popChk] == alive)
    ++unchangedPopulation[0];
  else
    unchangedPopulation[0] = 0;
  
  //Check for repeating population cycles
  bool gapCheck = false;
  for (gap = 4; gap < maxGap+1; ++gap)
  {
    for (i = 1; i < 48/gap; ++i)
    {
      popChk = popCursor - 1 - (gap * i);
      for (j = 0; j < gap; ++j)
      {
        popChk -= j;
        if (popChk < 0) popChk += 48;
        prevPopChk = popChk + (gap * i);
        if (prevPopChk > 47) prevPopChk -= 48;
        gapCheck = ( (population[popChk] > 0) && (population[popChk] == population[prevPopChk]) );

        if (gapCheck == false)
        {
          break;
        }
      }
      if (gapCheck == false)
      {
        break;
      }
    }
    if (gapCheck == true) break;
  }

  if (gapCheck == true)
  {
    ++unchangedPopulation[gap-1];
  }
  else
    unchangedPopulation[gap-1] = 0;
    
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Update Pixels
///////////////////////////////////////////////////////////////////////////////////////////////////
void updatePixels()
{
  uint16_t idx = 0;
  uint8_t x;
  uint8_t y;

  for(y = 0; y < gridSize; ++y)
  {
    for(x = 0; x < gridSize; ++x)
    {
      if (cells[x][gridSize-1-y])
      {
         unicorn.Pixels()[idx] = cellColour;
      }
      else {
        unicorn.Pixels()[idx] = CRGB::Black;
      }
      idx++;
    }
  }
}
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////