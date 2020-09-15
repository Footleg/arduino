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

#include "golife.h"

#include <iostream>
#include <unistd.h>

// default constructor
GameOfLife::GameOfLife(RGBMatrixRenderer &renderer_, uint8_t fadeSteps_, int delay_)
    : renderer(renderer_)
{
    // Allocate memory
    cells = new uint8_t*[renderer.getGridWidth()];
    for (int x=0; x<renderer.getGridWidth(); ++x) {
        cells[x] = new uint8_t[renderer.getGridHeight()];
    }

    //Initialise member variables
    fadeSteps = fadeSteps_;
    delay = delay_;

    panelSize = renderer.getGridHeight();
    if (renderer.getGridWidth() < panelSize)
        panelSize = renderer.getGridWidth();

} //GameOfLife

// default destructor
GameOfLife::~GameOfLife()
{
    for (int x=0; x<renderer.getGridWidth(); ++x) {
        delete [] cells[x];
    }
    delete [] cells;
} //~GameOfLife

void GameOfLife::runCycle()
{
    int8_t x, y, xt, yt, xi, yi, neighbours;
    uint8_t maxRepeatsCount, maxContributor;

    //Get highest repeating frame count for repeating patterns > 5 frames
    maxRepeatsCount = 0;
    maxContributor = 0;
    for(int8_t i = 5; i < maxRepeatCycle; ++i)
    {
        if ( unchangedPopulation[i] > maxRepeatsCount )
        {
            maxRepeatsCount = unchangedPopulation[i];
            maxContributor = i;
        }
    }
    
    /* Reinitialise simulation on the following conditions:
    *  - All cells dead.
    *  - No changes have occurred between consecutive frames (static pattern)
    *  - Pattern alternates between just 2 different states 
    *  - Pattern cycles between 3 different states 
    *  - Population remains constant at 5 cells for 4xPanel size consecutive frames (gliding pattern)
    *  - Population remains constant at >5 cells 10xPanel size frames (as glider may collide with something)
    *  - Population cycles over a 4 step cycle for over 3xPanel size frames
    *  - Pattern cycles over 6-24 frames for over 150 cycles
    */
    if ( (alive == 0) || (unchangedCount > 5 ) || (repeat2Count > 6) || (repeat3Count > 35) 
        || (unchangedPopulation[0] > panelSize*10) || ( (unchangedPopulation[0] > panelSize*4) && (alive == 5 ) ) 
        || (unchangedPopulation[3] > panelSize*3) || (maxRepeatsCount > 150) )
    {
        //Update min and max iterations counters
        if (iterations > 0)
        {
            if (iterations < iterationsMin) iterationsMin = iterations;
            if (iterations > iterationsMax) iterationsMax = iterations;

        }
        //Debug end condition detected
        std::string terminalCause;
        if (alive == 0)
            terminalCause = "All died\n";
        else if (unchangedCount > 5 )
            terminalCause = "Static pattern for 5 frames\n";
        else if (repeat2Count > 6)
            terminalCause = "Pattern repeated over 2 frames\n";
        else if (repeat3Count > 35)
            terminalCause = "Pattern repeated over 3 frames\n";
        else if (unchangedPopulation[0] > panelSize*10)
        {
            terminalCause = "Population static over ";
            terminalCause += std::to_string(panelSize*10);
            terminalCause += " frames\n";
        }
        else if ( (unchangedPopulation[0] > panelSize*4) && (alive == 5 ) )
        {
            terminalCause = "Population static over ";
            terminalCause += std::to_string(panelSize*4);
            terminalCause += " frames with 5 cells exactly\n";
        }
        else if (unchangedPopulation[3] > panelSize*3) 
        {
            terminalCause = "Population repeated over 4 step cycle ";
            terminalCause += std::to_string(panelSize*3);
            terminalCause += "x\n";
        }
        else if (maxRepeatsCount > 150)
        {
            terminalCause = "Population repeated over ";
            terminalCause += std::to_string(maxContributor+1);
            terminalCause += " step cycle 120x";
        }
        fprintf(stderr,"Pattern terminated after %d iterations (min: %d, max: %d): %s",
                iterations, iterationsMin, iterationsMax, const_cast<char*>(terminalCause.c_str()));
        
        initialiseGrid(0);
        
    }

    //fprintf(stderr, "Iteration %d alive=%d\n", iterations, alive);
     if ((delay < 5) && ( (alive == 0) || (unchangedCount > 5 ) || (repeat2Count > 6) || (repeat3Count > 10) 
        || (unchangedPopulation[0] > 10)  
        || (unchangedPopulation[3] > 10) || (maxRepeatsCount > 20) ) )
    {
        //Debug delay
        usleep(100 * 1000);
    }

    //Apply rules of Game of Life to determine cells dying and being born
    for(y = 0; y < renderer.getGridHeight(); ++y)
    {
        for(x = 0; x < renderer.getGridWidth(); ++x)
        {
            //For each cell, count neighbours, including wrapping over grid edges
            neighbours = -1;
            for(xi = -1; xi < 2; ++xi)
            {
                xt = renderer.newPositionX(x, xi);
                for(yi = -1; yi < 2; ++yi)
                {
                    yt = renderer.newPositionY(y, yi);
                    if ( (cells[xt][yt] & CELL_ALIVE) != 0 )
                    {
                        ++neighbours;
                    }
                }
            }

            //Reset changes arrays for this cell
            cells[x][y] &= ~CELL_BIRTH;
            cells[x][y] &= ~CELL_DEATH;
            if ( ((cells[x][y] & CELL_ALIVE) != 0) && (neighbours < 2) )
            {
                //Populated cell with too few neighbours, so it will die
                cells[x][y] |= CELL_DEATH; //turn on kill bit

            }
            else if ( ((cells[x][y] & CELL_ALIVE) == 0) && (neighbours == 2) ) 
            {
                //Empty cell with exactly 3 neighbours (count = 2 as did not count itself so was initialised as -1)
                cells[x][y] |= CELL_BIRTH; //turn on spawn bit
            }
            else if ( ((cells[x][y] & CELL_ALIVE) != 0) && (neighbours > 3) )
            {
                //Populated cell with too many neighbours, so it will die
                cells[x][y] |= CELL_DEATH; //turn on kill bit
            }
        }
    }

    /* /Debug info
    for (int x=0;x<3;x++) {
        for (int y=0;y<3;y++) {
            fprintf(stderr,"Cell [%d,%d] value %d, alive %d, ", x, y, cells[x][y], (cells[x][y] & CELL_ALIVE) );
            fprintf(stderr,"birth %d, ", (cells[x][y] & CELL_BIRTH) );
            fprintf(stderr,"death %d\n", (cells[x][y] & CELL_DEATH) );
        }
    } */
    if (fadeSteps > 1)
        fadeInChanges();

    applyChanges();
    
    if (alive == 0)
    {
      //Pause to show end of population before it gets reset
      usleep(4000); 
    }

    iterations++;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Initialise Grid
///////////////////////////////////////////////////////////////////////////////////////////////////
void GameOfLife::initialiseGrid(uint8_t pattern)
{
    const bool X = true;
    const bool O = false;

    alive = 0;
    iterations = 0;
    unchangedCount = 0;
    for (int x = 0; x < maxRepeatCycle; ++x) unchangedPopulation[x] = 0;
    repeat2Count = 0;
    repeat3Count = 0;
    for (int x = 0; x < popHistorySize; ++x) population[x] = 0;

    //New random colour
    renderer.setRandomColour();
    if (fadeSteps > 4) {
        //Reject colours which are too close to red or green
        while ( (renderer.r > 180 && renderer.g < 100 && renderer.b < 100) 
            || (renderer.r < 100 && renderer.g > 180 && renderer.b < 100) ) 
            renderer.setRandomColour();
    }

    if (pattern == 0) {
        //Random
        for(int y = 0; y < renderer.getGridHeight(); ++y)
        {
            for(int x = 0; x < renderer.getGridWidth(); ++x)
            {
                uint8_t randNumber = rand()%100;
                if (randNumber < 15)
                {
                    cells[x][y] = CELL_ALIVE;
                    renderer.setPixel(x, y, renderer.r, renderer.g, renderer.b);
                    alive++;
                }
                else
                {
                    cells[x][y] = 0;
                    renderer.setPixel(x, y, 0, 0, 0);
                }
            }
        }
    }

    //Debug info
    //fprintf(stderr,"\nCell [1,1] is %d\n", (cells[1][1] & CELL_ALIVE) );

}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Update cells array with changes for next iteration
///////////////////////////////////////////////////////////////////////////////////////////////////
void GameOfLife::applyChanges()
{
    uint8_t x, y, changes, i, j, gap;
    int8_t popChk,prevPopChk;
    
    changes = 0;
    bool compare2 = true;
    bool compare3 = true;

    for(y = 0; y < renderer.getGridHeight(); ++y)
    {
        for(x = 0; x < renderer.getGridWidth(); ++x)
        {
            //Update last 3 iterations history for this cell
            if ((cells[x][y] & CELL_PREV2) != 0)
                cells[x][y] |= CELL_PREV3;
            else
                cells[x][y] &= ~CELL_PREV3;
            
            if ((cells[x][y] & CELL_PREV1) != 0)
                cells[x][y] |= CELL_PREV2;
            else
                cells[x][y] &= ~CELL_PREV2;

            if ((cells[x][y] & CELL_ALIVE) != 0)
                cells[x][y] |= CELL_PREV1;
            else
                cells[x][y] &= ~CELL_PREV1;

            //Create new cells
            if ((cells[x][y] & CELL_BIRTH) != 0)
            {
                cells[x][y] |= CELL_ALIVE;
                if (fadeSteps < 2) 
                    renderer.setPixel(x, y, renderer.r, renderer.g, renderer.b);
                ++changes;
                ++alive;
            }
            else if ((cells[x][y] & CELL_DEATH) != 0)
            {
                //Kill dying cells
                cells[x][y] &= ~CELL_ALIVE;
                if (fadeSteps < 2) 
                    renderer.setPixel(x, y, 0, 0, 0);
                ++changes;
                --alive;
            }
            
            //Compare cell to state 2 and 3 iterations ago
            if (compare2 && ( ((cells[x][y] & CELL_ALIVE) == 0) != ((cells[x][y] & CELL_PREV2) == 0) )) compare2 = false;
            if (compare3 && ( ((cells[x][y] & CELL_ALIVE) == 0) != ((cells[x][y] & CELL_PREV3) == 0) )) compare3 = false;
        }
    }

    popCursor++;
    if (popCursor > popHistorySize-1) popCursor = 0;
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
        popChk = popHistorySize-1;
    else
        popChk = popCursor - 1;

    if (population[popChk] == alive)
        ++unchangedPopulation[0];
    else
        unchangedPopulation[0] = 0;

    //Check for repeating population cycles
    bool gapCheck = false;
    for (gap = 4; gap < maxRepeatCycle+1; ++gap)
    {
        for (i = 1; i < popHistorySize/gap; ++i)
        {
        
        for (j = 0; j < gap; ++j)
        {
            popChk = popCursor - 1 - (gap * i) - j;
            if (popChk < 0) popChk += popHistorySize;
            prevPopChk = popChk + (gap * i);
            if (prevPopChk > popHistorySize-1) prevPopChk -= popHistorySize;
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

    if (gapCheck == true) ++unchangedPopulation[gap-1];
    else unchangedPopulation[gap-1] = 0;
  
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Fade births in green, and death to red
///////////////////////////////////////////////////////////////////////////////////////////////////
void GameOfLife::fadeInChanges()
{
    uint8_t halfSteps = fadeSteps / 2; 
    uint8_t bR;
    uint8_t bG;
    uint8_t bB;
    uint8_t dR;
    uint8_t dG;
    uint8_t dB;
    int fadeDelay = delay;

    for(int i = 1; i < fadeSteps+1; ++i)
    {
        if (i < halfSteps)
        {
            //Set fade from black to green for cells being born
            bR = 0;
            bG = renderer.blendColour(0, renderer.getMaxBrightness(), i, halfSteps);
            bB = 0;
            //Set fade cells dying out from current colour to red
            dR = renderer.blendColour(renderer.r, renderer.getMaxBrightness(), i, halfSteps);
            dG = renderer.blendColour(renderer.g, 0, i, halfSteps);
            dB = renderer.blendColour(renderer.b, 0, i, halfSteps);
        }
        else
        {
            //Set fade from green to active cell colour for cells being born
            bR = renderer.blendColour(0, renderer.r, i-halfSteps, halfSteps);
            bG = renderer.blendColour(renderer.getMaxBrightness(), renderer.g, i-halfSteps, halfSteps);
            bB = renderer.blendColour(0, renderer.b, i-halfSteps, halfSteps);
            //Set fade colour for cells dying out from red to black
            dR = renderer.blendColour(renderer.getMaxBrightness(), 0, i-halfSteps, halfSteps);
            dG = 0;
            dB = 0;
        }
        
        for(int y = 0; y < renderer.getGridHeight(); ++y)
        {
            for(int x = 0; x < renderer.getGridWidth(); ++x)
            {
                if ((cells[x][y] & CELL_BIRTH) != 0)
                {
                    renderer.setPixel(x, y, bR, bG, bB);
                }
                else if ((cells[x][y] & CELL_DEATH) != 0)
                {
                    renderer.setPixel(x, y, dR, dG, dB);
                }
            }
        }
        
        if (delay * fadeSteps > 1000) fadeDelay = 1000 / fadeSteps;
        usleep(fadeDelay * 1000);
    }
}
