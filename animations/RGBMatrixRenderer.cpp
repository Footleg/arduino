#include "RGBMatrixRenderer.h"
#include <stdint.h>
#include <iostream>

// default constructor
RGBMatrixRenderer::RGBMatrixRenderer(int width, int height)
    : gridWidth(width), gridHeight(height)
{} //RGBMatrixRenderer

// default destructor
RGBMatrixRenderer::~RGBMatrixRenderer()
{
} //~RGBMatrixRenderer

int RGBMatrixRenderer::getGridWidth()
{
    return gridWidth;
}

int RGBMatrixRenderer::getGridHeight()
{
    return gridHeight;
}

void RGBMatrixRenderer::setRandomColour()
{
    // Init colour randomly
    r = rand()%255;
    g = rand()%255;
    b = rand()%255;
    
    //Prevent colours being too dim
    if (r<150 && g<150 && b<150) {
        int c = rand()%3;
        switch (c) {
        case 0:
            r = 200;
            break;
        case 1:
            g = 200;
            break;
        case 2:
            b = 200;
            break;
        }
    }
    
    fprintf(stderr, "%s  %d %s  %d %s %d %s", "New RGB ", r, ",",  g, ",",  b, "\n" );

}

//Method to update a grid coordinate while keeping on the matrix, with optional wrapping
int RGBMatrixRenderer::changePosition(int position, int increment, int dimension, bool wrap)
{
    int newPos = position + increment;

    if (wrap) 
    {
        while (newPos < 0)
            newPos += dimension;
        while (newPos >= dimension)
            newPos -= dimension;
    }
    else
    {
        if (newPos >= dimension)
            newPos = dimension - 1;
        else if (newPos >= dimension)
            newPos = 0;
    }
    
    return newPos;
}

int RGBMatrixRenderer::changePositionX(int x, int increment, bool wrap = true)
{
    return changePosition(x, increment, gridWidth, wrap);
}

int RGBMatrixRenderer::changePositionY(int y, int increment, bool wrap = true)
{
    return changePosition(y, increment, gridHeight, wrap);
}