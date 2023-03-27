/*
  Example using ESP-Now with the LR protocol for low latency realtime communication between 
  a pair of ESP32 boards over distances of at least 124m. 
  
  This transmitter example broadcasts data for a 3-axis joystick with a button. So the data
  structure contains 3 floats representing the x, y and z axis postitions (z is a twist action
  on these joysticks), and a byte for the button. We are only using a single bit for the button
  so the same code could handle 8 buttons, each using a different bit in the unsigned 8 bit int.
  The floats are set between -1.0 and 1.0 to represent the full range of movement for each axis.

  To make it simple to test of different boards, the joystick is simulated here. So no hardware 
  needs to be attached to the board. In reality, the joystick axes would each be connected to an
  analogue input on the ESP32 to read the position and convert it to a float in the range -1.0 < 1.0
  with 0.0 being the centre position.

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
#include <Arduino.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

// Receiver MAC Address. (Change this to the actual MAC address of the board you are using
// as your receiver. This will be output on the serial console by the receiver when it
// does not have a connection).
uint8_t broadcastAddress[] = {0x01, 0xA2, 0xB3, 0xC4, 0xD5, 0xE6};

// Variable to store if sending data was successful
String success;

// Data structure used to send data messages.
// Must match the data structure used on the receiver to decode the messages.
typedef struct struct_message {
    float x1;
    float y1;
    float z1;
    uint8_t b1;
} struct_message;

// Create a struct_message to hold joystick readings
struct_message Joy1Readings;

int period = 34;
int btnperiod = 2000;

unsigned long time_now = 0;
unsigned long last_time_start = 0;
unsigned long last_btn_time = 0;
float change = 0.015;

esp_now_peer_info_t peerInfo;

// Callback function called when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nLast Packet Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    if (status == 0){
        success = "Delivery Success :)";
    }
    else{
        success = "Delivery Fail :(";
    }
}

void setup()
{
    Serial.begin(115200);
    
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
    
    // Set protocol to LR for longer range (at lower data rate)
    esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_LR );

    // Initialise ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    // Once ESPNow is successfully intialised, register the Send callback function to
    // get the status of transmitted packets
    esp_now_register_send_cb(OnDataSent);
    
    // Register peer
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;    
    peerInfo.encrypt = false;
    
    // Add peer                
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("Failed to add peer");
        return;
    }

    // Initialise simulated joystick values to send
    Joy1Readings.x1 = 0.0;
    Joy1Readings.y1 = 0.0;
    Joy1Readings.z1 = 0.0;
    Joy1Readings.b1 = 1;     // On this joystick, the button goes low when pressed
}


void loop()
{
    // Simulate joystick inputs to transmit. Here we cycle the z axis between 0.0 and 1.0
    // and press the button for 200ms every 2 seconds.
    float x,y,z;
    time_now = millis();

    if (time_now > last_time_start + period) {
        last_time_start = time_now;
        Joy1Readings.z1 = Joy1Readings.z1 + change;
        if ((Joy1Readings.z1 > 1.0) or (Joy1Readings.z1 < 0.0)) {
            Joy1Readings.z1 = Joy1Readings.z1 - change;
            change = -change;
        }
    }
     
    if (Joy1Readings.b1 == 0) {
        // Turn off button after another 200ms since it was pressed
        if (time_now > last_btn_time + btnperiod + 200) {
            last_btn_time = time_now;
            Serial.println("Released button");
            Joy1Readings.b1 = 1;
        }
    } 
    else {
        if (time_now > last_btn_time + btnperiod) {
            // Press button
            Serial.println("Pressed button");
            Joy1Readings.b1 = 0;
        }
    }

    // Send the data held in the Joy1Readings data structure as a message via ESP-NOW
    esp_err_t result = esp_now_send( broadcastAddress, (uint8_t *) &Joy1Readings, sizeof(Joy1Readings) );
     
    if (result == ESP_OK) {
        Serial.println("Sent with success");
    }
    else {
        Serial.println("Error sending the data");
    }

    delay(60);         // off time so we don't send excessive amounts of data
}
