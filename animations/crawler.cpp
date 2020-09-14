/**************************************************************************************************
 * Simple Crawler animator class 
 * 
 * Generates a simple animation starting at a random point in the grid, and crawling across the
 * grid at random. This is a simple example of an animator class where the RGB matrix hardware
 * rendering is passed in, so it can be used with any RGB array display by writing an 
 * implementation of a renderer class to set pixels/LED colours on the hardware.
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
 * along with Cave Converter.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "crawler.h"

#include <iostream>

// default constructor
Crawler::Crawler(RGBMatrixRenderer &renderer_)
    : renderer(renderer_)
{
    // Store grid size
    gridWidth = renderer.getGridWidth();
    gridHeight = renderer.getGridHeight();

    srand(time(NULL));

    //Pick random start point
    x = rand()%gridWidth;
    y = rand()%gridHeight;

    //Start random direction
    direction = rand()%4;

    //Initial random colour
    renderer.setRandomColour();

    std::cout << "Starting position " << x << "," << y << std::endl;
    std::cout << "Starting direction " << direction << std::endl;

} //Crawler

// default destructor
Crawler::~Crawler()
{
} //~Crawler

//Run Cycle is called once per frame of the animation
void Crawler::runCycle()
{
    //Set current position pixel
    renderer.setPixel(x, y, renderer.r, renderer.g, renderer.b);
    
    //Update direction
    int c = rand()%8;
    switch(c) {
        case 0: //Turn left
            direction--;
            break;
        case 1: //Turn right
            direction++;
            break;

    }
    if (direction > 3)
        direction = 0;
    else if (direction < 0)
        direction = 3;

    switch(direction) {
        case 0: //Up
            y--;
            if (y < 0) y = gridHeight - 1;
            break;
        case 1: //Right
            x++;
            if (x >= gridWidth) x = 0;
            break;
        case 2: //Down
            y++;
            if (y >= gridHeight) y = 0;
            break;
        case 3: //Left
            x--;
            if (x < 0) x = gridWidth - 1;
            break;
    }

    //Update colour every 50 steps
    colChg++;
    if (colChg >= 50) {
        colChg = 0;
        renderer.setRandomColour();
    }

}