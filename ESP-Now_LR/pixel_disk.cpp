#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class PixelDisk {
    public:
    private:
        static const uint16_t NUM_PIXELS = 241; // The number of LEDs (pixels) on NeoPixel
        Adafruit_NeoPixel NeoPixel;
    public:
        PixelDisk(uint8_t IO_PIN, uint8_t brightness = 128) {
            NeoPixel = Adafruit_NeoPixel(NUM_PIXELS, IO_PIN, NEO_GRB + NEO_KHZ800);
            NeoPixel.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
            NeoPixel.setBrightness(brightness); // a value from 0 to 255
            // NeoPixel.clear(); // Initialize all pixels to 'off'
            // NeoPixel.show();
        }

        ~PixelDisk(){}

        void setPixelRadial(uint16_t angle, int16_t distance, uint8_t r, uint8_t g, uint8_t b)
        {
            uint16_t leds = 1;
            uint16_t ringStart = 240;

            if (distance >= 0) {
                switch(distance) {
                    case 0:
                        leds = 1;
                        ringStart = 240;
                        break;
                    case 1:
                        leds = 8;
                        ringStart = 232;
                        break;
                    case 2:
                        leds = 12;
                        ringStart = 220;
                        break;
                    case 3:
                        leds = 16;
                        ringStart = 204;
                        break;
                    case 4:
                        leds = 24;
                        ringStart = 180;
                        break;
                    case 5:
                        leds = 32;
                        ringStart = 148;
                        break;
                    case 6:
                        leds = 40;
                        ringStart = 108;
                        break;
                    case 7:
                        leds = 48;
                        ringStart = 60;
                        break;
                    default:
                        leds = 60;
                        ringStart = 0;
                        break;
                }

                if (leds == 1){
                    NeoPixel.setPixelColor(240, NeoPixel.Color(r,g,b));
                }
                else {
                    int16_t position = round(leds * (angle + 180/leds) / 360);
                    while (position < 0) {
                        position += leds;
                    }
                    while (position >= leds) {
                        position -= leds;
                    }
                    uint16_t pixel = ringStart + int(position);
                NeoPixel.setPixelColor(pixel, NeoPixel.Color(r,g,b));
                }
            }
            // Serial.print(" Dist: ");
            // Serial.print(distance);
            // Serial.print(" Angle: ");
            // Serial.print(angle);
        }


        void setPixelRing(int16_t distance, uint8_t r, uint8_t g, uint8_t b )
        {
            uint16_t ringStart = 240;
            uint16_t ringEnd = 0;

            if (distance >= 0) {
                switch(distance) {
                    case 0:
                        ringStart = 240;
                        ringEnd = 240;
                        break;
                    case 1:
                        ringStart = 232;
                        ringEnd = 239;
                        break;
                    case 2:
                        ringStart = 220;
                        ringEnd = 231;
                        break;
                    case 3:
                        ringStart = 204;
                        ringEnd = 219;
                        break;
                    case 4:
                        ringStart = 180;
                        ringEnd = 203;
                        break;
                    case 5:
                        ringStart = 148;
                        ringEnd = 179;
                        break;
                    case 6:
                        ringStart = 108;
                        ringEnd = 147;
                        break;
                    case 7:
                        ringStart = 60;
                        ringEnd = 107;
                        break;
                    default:
                        ringStart = 0;
                        ringEnd = 59;
                        break;
                }

                for (uint8_t idx = ringStart; idx <= ringEnd; idx++) {
                    NeoPixel.setPixelColor(idx, NeoPixel.Color(r,g,b));
                }
            }
        }

        void cartesianToPolar(float x, float y, float &d, float &a ) 
        {
            d = sqrt(x * x + y * y);
            a = atan(x / y) * 180 / PI;

            if (y < 0) {
                a += 180;
            }
            else if (x < 0) {
                a += 360;
            }
        }

        void clear()
        {
            NeoPixel.clear();
        }

        void show()
        {
            NeoPixel.show();
        }
};
