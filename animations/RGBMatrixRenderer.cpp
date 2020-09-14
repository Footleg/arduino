#include "RGBMatrixRenderer.h"
#include <stdint.h>
#include <iostream>
using namespace std;

// default constructor
RGBMatrixRenderer::RGBMatrixRenderer(int width, int height)
    : gridWidth(width), gridHeight(height)
{} //RGBMatrixRenderer

// default destructor
RGBMatrixRenderer::~RGBMatrixRenderer()
{
} //~RGBMatrixRenderer

void RGBMatrixRenderer::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    cout << "Base setPixel" << endl;
}

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
    
    //cout << "New RGB " << r << "," << g << "," << b << endl;

}