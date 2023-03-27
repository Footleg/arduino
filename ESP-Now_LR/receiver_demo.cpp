/*
  Example using ESP-Now with the LR protocol for low latency realtime communication between 
  a pair of ESP32 boards over distances of at least 124m. 
  
  This receiver example takes data messages representing the inputs from a 3-axis joystick with 
  a button. So the data structure contains 3 floats representing the x, y and z axis postitions 
  (z is a twist action on these joysticks), and a byte for the button. We are only using a single 
  bit for the button so the same code could handle 8 buttons, each using a different bit in the 
  unsigned 8 bit int. The floats are set between -1.0 and 1.0 to represent the full range of 
  movement for each axis.

  To display the incoming data visually, this demo uses a 241 neopixel disk. You can use any
  neopixel strip or array to test with, but the position display will only make sense on a disk
  or spiral of 241 neopixels where the centre pixel is at the end of the chain. This demo will
  also work fine without any neopixels connected as the data is output on the serial console.

  To indicate whether a transmitter is connected, the time of the last received message is tracked
  and if > 2 seconds has passed since the last message then a flag is set to indicate the program is
  not under control. (Think of a robot being remote controlled, where we treat it as out of control
  if no instructions are received for over 2 seconds. If that happened you would want to stop the
  motors so it does not run away if it lost connection. Or maybe if a flying robot, trigger it to
  hover, begin a safe descent or move towards home to try and re-establish a connection). When in
  the out-of-control state, this demo program flashes a bright red circle pattern on the neopixel
  disk. When under control, a spot is displayed to indicate the position of the x + y axes, with
  the z axis setting the LED colour. Pressing the button on the joystick causes an expanding 
  circular pulse animation to be displayed in the current colour set on the z axis.

  Testing with a pair of generic ESP32 WROOM32 boards with built in PCB antennas the maximum
  range for reliable communication was 27 metres using ESP-Now without the LR protocol configured.
  Adding the LR protocol config to both the transmitter and receiver code increased the range to
  65m. Switching to an ESP32 WROOM32-U board with small external stick antenna increased the range
  to 124m (the receiver was still an on-board PCB antenna). This was not the limit, just the 
  furthest distance I could get line of site to confirm my receiver was responding during the test.
  The same 124m range was achieved with a little SEEED XIAO ESP32-C board using a small flat 
  external antenna as the transmitter. The maximum range has not been determined.

  Released under the MIT License

  Copyright (c) 2023 Paul 'Footleg' Fretwell  https://github.com/Footleg

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  This is an extension of ideas presented in a tutorial published at 
  https://RandomNerdTutorials.com/esp-now-two-way-communication-esp32/

*/
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include "pixel_disk.cpp"
 
#define ANIMATION_INTERVAL  50 // Milliseconds interval between animation frame updates
#define FLASH_INTERVAL     500 // Milliseconds interval between animation frame updates
#define PIN_NEO_PIXEL       27 // Arduino pin that connects to NeoPixels data in

uint32_t lastDataTime;
uint32_t lastAnimationUpdate;
uint32_t lastFlashingAnimationUpdate;

PixelDisk pixdisk(PIN_NEO_PIXEL, 12);
uint8_t ringFlash = 0;
uint8_t noDataToggle = 0;
bool underControl = true;
 
// Data structure used to receive data messages.
// Must match the data structure used on the transmitter to send messages.
typedef struct struct_message {
    float x1;
    float y1;
    float z1;
    uint8_t b1;
} struct_message;

// Create a struct_message to hold incoming data
struct_message incomingReadings;

// Callback function. Will be called when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) 
{
    // Copy the data from the message into the data structure
    memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

    // Set flag to show our receiver is still connected
    underControl = true;

    // Track the time of the last message received
    lastDataTime = millis();

    // Display the incoming data in the serial console
    Serial.print("Bytes received: ");
    Serial.print(len);
    Serial.print(" Data: ");
    Serial.print(incomingReadings.x1);
    Serial.print(", ");
    Serial.print(incomingReadings.y1);
    Serial.print(", ");
    Serial.print(incomingReadings.z1);
    Serial.print(", ");
    Serial.println(incomingReadings.b1);
}

// Convert a joystick axis position to a neopixel colour
void colourFromZ(float z, uint8_t &r, uint8_t &g, uint8_t &b )
{
    float za = abs(z);
    if (za < 0.2) {
        r = 255;
        g = float(255 * za / 0.2);
        b = 0;
    }
    else if (za < 0.4) {
        r = float(255 - 255 * (za-0.2) / 0.2);
        g = 255;
        b = 0;
    }
    else if (za < 0.6) {
        r = 0;
        g = 255;
        b = float(255 * (za-0.4) / 0.2);
    }
    else if (za < 0.8) {
        r = 0;
        g = float(255 - 255 * (za-0.6) / 0.2);
        b = 255;
    }
    else {
        r = float(255 * (za-0.8) / 0.2);
        g = 0;
        b = 255;
    }
}


void setup(){
    Serial.begin(115200);
    Serial.println("Initializing ESP-NOW receiver");

    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
  
    // Set protocol to LR for longer range (at lower data rate)
    esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_LR );

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register the a callback function that will be called when data is received
    esp_now_register_recv_cb(OnDataRecv);

    // Initialise time trackers
    lastDataTime = millis();
    lastAnimationUpdate = millis();
    lastFlashingAnimationUpdate = millis();
}


void loop(){

    if (millis() - lastDataTime > 2000) {
        // No data communication for 2 seconds, so not under control
        underControl = false;

        // Update flash animation when interval has elapsed
        if (millis() - lastFlashingAnimationUpdate >  FLASH_INTERVAL) {
            // While not receiving data, output the MAC address (needed by the transmitter
            // program) to the serial console, and flash the neopixels red.
            Serial.print("No data in last 2 seconds. MAC: ");
            Serial.println(WiFi.macAddress());
            lastFlashingAnimationUpdate = millis();

            //Flash no data signal (use noDataToggle variable as a toggle)
            if (noDataToggle == 0) {
                noDataToggle = 1;
                pixdisk.setPixelRing(2,255, 0, 0);
                pixdisk.setPixelRing(3,255, 0, 0);
                pixdisk.setPixelRing(7,255, 0, 0);
                pixdisk.setPixelRing(8,255, 0, 0);
            }
            else {
                pixdisk.clear();
                noDataToggle = 0;
            }
            pixdisk.show();
        }
    }
    else if (underControl) {
        // When under the control of a transmitter (connection has received data in the last 2 seconds)
        // display the position of the joystick on the neopixels. The postions can be read at any time
        // from the incomingReadings global variable (which is updated in the callback function when
        // any data message is received).
        float angle,distance;

        // Update position display on pixel disk
        pixdisk.clear(); // set all pixel colors to 'off'. It only takes effect if pixels.show() is called

        //Convert x,y into angle and distance
        pixdisk.cartesianToPolar(incomingReadings.x1, incomingReadings.y1, distance, angle);

        //Set LED ring distance
        int16_t ringDist = distance * 8;
        uint16_t angleSpread = 0;
        switch(ringDist) {
            case 0:
            angleSpread = 0;
            break;
            case 1:
            angleSpread = 360 / 8;
            break;
            case 2:
            angleSpread = 360 / 12;
            break;
            case 3:
            angleSpread = 360 / 16;
            break;
            case 4:
            angleSpread = 360 / 24;
            break;
            case 5:
            angleSpread = 360 / 32;
            break;
            case 6:
            angleSpread = 360 / 40;
            break;
            case 7:
            angleSpread = 360 / 48;
            break;
            default:
            angleSpread = 360 / 60;
            break;
        }

        //Set colour
        uint8_t r,g,b;
        colourFromZ(incomingReadings.z1,r,g,b);

        //Set size of target
        float z = (incomingReadings.z1 + 1)/2;
        // Centre of target
        pixdisk.setPixelRadial(angle,ringDist,r, g, b);

        if (ringDist > 0) {
            // Larger cluster
            if (z > 0.25) {
                pixdisk.setPixelRadial(angle+angleSpread,ringDist,r, g, b);
                pixdisk.setPixelRadial(angle-angleSpread,ringDist,r, g, b);
                pixdisk.setPixelRadial(angle+0.5*angleSpread,ringDist+1,r, g, b);
                pixdisk.setPixelRadial(angle+0.5*angleSpread,ringDist-1,r, g, b);
                pixdisk.setPixelRadial(angle-0.5*angleSpread,ringDist+1,r, g, b);
                pixdisk.setPixelRadial(angle-0.5*angleSpread,ringDist-1,r, g, b);
            }
            // Largest cluster
            if (z > 0.8) {
                pixdisk.setPixelRadial(angle+2*angleSpread,ringDist,r, g, b);
                pixdisk.setPixelRadial(angle-2*angleSpread,ringDist,r, g, b);
                pixdisk.setPixelRadial(angle+0.5*angleSpread,ringDist+2,r, g, b);
                pixdisk.setPixelRadial(angle+0.5*angleSpread,ringDist-2,r, g, b);
                pixdisk.setPixelRadial(angle+1.5*angleSpread,ringDist+2,r, g, b);
                pixdisk.setPixelRadial(angle+1.5*angleSpread,ringDist-2,r, g, b);
                pixdisk.setPixelRadial(angle-0.5*angleSpread,ringDist+2,r, g, b);
                pixdisk.setPixelRadial(angle-0.5*angleSpread,ringDist-2,r, g, b);
                pixdisk.setPixelRadial(angle-1.5*angleSpread,ringDist+2,r, g, b);
                pixdisk.setPixelRadial(angle-1.5*angleSpread,ringDist-2,r, g, b);
            }
        }
        else {
            // 2nd ring
            if (z > 0.25) {
                pixdisk.setPixelRing(1,r, g, b);
            }
            // 3rd ring
            if (z > 0.8) {
                pixdisk.setPixelRing(2,r, g, b);
            }
        }

        if (incomingReadings.b1 == 0 && ringFlash > 8) {
            ringFlash = 0;
        }

        //Show ring flash animation when active
        if (ringFlash < 9) {
            uint8_t r,g,b;
            colourFromZ(incomingReadings.z1,r,g,b);
            pixdisk.setPixelRing(ringFlash,r, g, b);
        }

        // Update any animations when interval has elapsed
        if (millis() - lastAnimationUpdate >  ANIMATION_INTERVAL) {
            lastAnimationUpdate = millis();

            //Update ring flash animation when active
            if (ringFlash < 9) {
                ringFlash++;
            }
        }

        pixdisk.show();

    }

    delay(5); // Allow time for callback function (may not be neccesary if it is interupt driven)

}
