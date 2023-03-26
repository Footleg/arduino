// ESP-Now communication based on tutorial at https://RandomNerdTutorials.com/esp-now-two-way-communication-esp32/

#include <esp_now.h>
#include <esp_wifi.h>
#include "WiFi.h"
#include "pixel_disk.cpp"
 
#define ANIMATION_INTERVAL  50 // Milliseconds interval between animation frame updates
#define FLASH_INTERVAL  500 // Milliseconds interval between animation frame updates
#define PIN_NEO_PIXEL        27 // Arduino pin that connects to NeoPixel

//WROVER MAC: 08:3A:F2:B1:C1:E4

uint32_t lastDataTime;
uint32_t lastAnimationUpdate;
uint32_t lastFlashingAnimationUpdate;

PixelDisk pixdisk(PIN_NEO_PIXEL, 12);
uint8_t ringFlash = 0;
uint8_t noDataToggle = 0;
bool underControl = true;
 
//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    float x1;
    float y1;
    float z1;
    uint8_t b1;
} struct_message;

// Create a struct_message to hold incoming data
struct_message incomingReadings;

// esp_now_peer_info_t peerInfo;

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) 
{
    memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));

    underControl = true;
    lastDataTime = millis();

    Serial.print("Bytes received: ");
    Serial.print(len);
    Serial.print("Joystick1: ");
    Serial.print(incomingReadings.x1);
    Serial.print(", ");
    Serial.print(incomingReadings.y1);
    Serial.print(", ");
    Serial.println(incomingReadings.z1);
    Serial.print(", ");
    Serial.println(incomingReadings.b1);
}

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
    esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_LR );

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Register for a callback function that will be called when data is received
    esp_now_register_recv_cb(OnDataRecv);

    lastDataTime = millis();
    lastAnimationUpdate = millis();
    lastFlashingAnimationUpdate = millis();

    incomingReadings.z1 = -0.6;
}
  
void loop(){

    if (millis() - lastDataTime > 2000) {
        // No data communcation, so not under control
        underControl = false;

        // Update flash animation when interval has elapsed
        if (millis() - lastFlashingAnimationUpdate >  FLASH_INTERVAL) {
            Serial.print("No data in last 2 seconds. Time: ");
            Serial.print(millis());
            Serial.print(" Last anim: ");
            Serial.print(millis() - lastFlashingAnimationUpdate);
            Serial.print("ms ago. MAC: ");
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
            incomingReadings.b1 = 1; //TEST CODE, TO BE DELETED!
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

        // Display debug info in Serial Monitor
        Serial.print("Elasped: ");
        Serial.print(millis() - lastDataTime);
        Serial.print("ms, Ring Pulse: ");
        Serial.print(ringFlash);
        Serial.print(" Joystick1: ");
        Serial.print(incomingReadings.x1);
        Serial.print(", ");
        Serial.print(incomingReadings.y1);
        Serial.print(", ");
        Serial.print(incomingReadings.z1);
        Serial.print(" Button: ");
        Serial.println(incomingReadings.b1);
    }

    delay(5);     // off time

}