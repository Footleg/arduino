#include <stdint.h>

class RGBMatrixRenderer
{
    //variables
    public:
        int r;
        int g;
        int b;
    protected:
        int gridWidth;
        int gridHeight;
    private:

    //functions
    public:
        RGBMatrixRenderer(int, int);
        virtual ~RGBMatrixRenderer();
        int getGridWidth();
        int getGridHeight();
        virtual void setPixel(int, int, uint8_t, uint8_t, uint8_t) = 0;
        void setRandomColour();
        int newPositionX(int,int,bool=true);
        int newPositionY(int,int,bool=true);
    protected:
    private:
        int newPosition(int,int,int,bool);
}; //RGBMatrixRenderer