#include <Arduino.h>

#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>

// (DIED)) "EC:94:CB:70:55:14" 
// WROVER MAC: 08:3A:F2:B1:C1:E4
// Transmitter mac: "3C:61:05:65:6B:B8"

// Receiver MAC Address
uint8_t broadcastAddress[] = {0x08, 0x3A, 0xF2, 0xB1, 0xC1, 0xE4};

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
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

// Callback when data is sent
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
  esp_wifi_set_protocol( WIFI_IF_AP, WIFI_PROTOCOL_LR );

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
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

  // Initialise values to send
  Joy1Readings.x1 = 0;
  Joy1Readings.y1 = 0;
  Joy1Readings.z1 = 0;
  Joy1Readings.b1 = 1;
}


void loop()
{
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

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send( broadcastAddress, (uint8_t *) &Joy1Readings, sizeof(Joy1Readings) );
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }

  delay(60);     // off time
}
