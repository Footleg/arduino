/**************************************************************************************************
 * Lilygo T-Watch-2000 Programmable Smart Watch framework
 * 
 * A starting development framework for building a UI and apps for the ESP32 based smart watch.
 * Building on work by Dan Geiger, which was based on code by Lewis He.
 *
 * See Dan's Instructable guide to setting up the Arduino IDE and some more apps you can drop into 
 * the menus. https://www.instructables.com/id/Lilygo-T-Watch-2020-Arduino-Framework
 * 
 * The code by Lewis He does a lot of lower level stuff and provides a very nice looking UI, but I
 * found it very difficult to get started with. You can find it here:
 * https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library
 * 
 * Building on Dan's work, I have added some of the basic capabilties needed to make the watch
 * usable. The main one being an inactivity time out and power saving mode. This extends the
 * battery life to around 24 hours. I think further improvements can be made, but this is a good
 * starting point and I am publishing it now so others can build on it too. The UI has been kept
 * deliberately basic so you can see how the code works with the watch hardware features. It just 
 * displays the time (including seconds), date and battery level. The inactivity counter (counting
 * the seconds since the user last touched the screen) is shown to help you see how this works.
 * I have also made the menu auto-close after the inactivity time out so that the menu being left
 * active does not prevent the watch going into sleep mode. Note that the other apps will keep the
 * screen on and the watch in 'active' mode until you exit them. So the battery will still run down
 * if an app is left open. You will want to consider how long you want to allow apps to remain 
 * active as you write your own.
 * 
 * You can find the latest version of my code here: https://github.com/Footleg/arduino/T-Watch-2000
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

//#define ARDUINO 10813

#include "watchdef.h"
#include <soc/rtc.h>
//#include "esp_wifi.h"
//#include "esp_sleep.h"
#include "displayTime.cpp"

TTGOClass *ttgo;

const int inactivityTimeout = 10000;    //Interval in msec after which the watch is put into low power mode (and screen turned off)
const int batteryCheckInterval = 30000; //Interval in msec between reads of the battery voltage

bool active = true;           //Flag to track if screen is active for updates/power saving
uint32_t lastActiveTime = 0;  //Tracks time of last interaction from user (to enable sleep after inactivity)
uint32_t nextUpdateTime = 0;  //Time which next display update should not occur before (avoids updating time display more than once per second)
uint32_t lastBattChkTime = 0; //Time when the battery level was last read

AppTimeDisplay *appDisplayTime;

void setup() {
    Serial.begin(19200); //Serial port maybe uses power? Suggest comment out unless debugging

    ttgo = TTGOClass::getWatch();
    ttgo->begin();
    ttgo->tft->setTextFont(1);
    ttgo->tft->fillScreen(TFT_BLACK);
    ttgo->tft->setTextColor(TFT_YELLOW, TFT_BLACK); // Note: the new fonts do not draw the background colour
    
    //Initialize lvgl
    ttgo->lvgl_begin();

    //Check if the RTC clock matches, if not, use compile time
    ttgo->rtc->check();

    //Synchronize time to system time
    ttgo->rtc->syncToSystem();

    //Initialise action trackers
    lastActiveTime = millis();
    lastBattChkTime = millis();
    
    //Create time display class
    appDisplayTime = new AppTimeDisplay(ttgo);
    appDisplayTime->lastActiveTime = lastActiveTime;
    //Read battery before first display update
    appDisplayTime->batteryLevel = readBattery();

    appDisplayTime->displayTime(true); //Update display to show the time

    ttgo->openBL(); // Turn on the backlight

}

void loop() {

  //Update display if active
  if (active) {
    // Check if inactivity timeout reached
    if (millis() - lastActiveTime > inactivityTimeout) {
      //Put into low energy mode
      low_energy();
      //Serial.print("ARDUINO define=");
      //Serial.println(ARDUINO);
    }
    else {
      //Update display
      if (nextUpdateTime < millis()) {
        nextUpdateTime = millis() + 1000;
        if (millis() - lastBattChkTime > batteryCheckInterval) {
          appDisplayTime->batteryLevel = readBattery();
          //Force a full refresh so battery level is updated on screen
          appDisplayTime->displayTime(true); 
        }
        else {
          appDisplayTime->displayTime(false); // Call every second but only update time every minute
        }
      }
    }
  }


  int16_t x, y;
  if (ttgo->getTouch(x, y)) {
    while (ttgo->getTouch(x, y)) {} // wait for user to release

    if (!active) 
      low_energy(); //Wake up
    else {
      //Active menu:
      // This is where the app selected from the menu is launched
      // If you add an app, follow the variable update instructions
      // at the beginning of the menu code and then add a case
      // statement on to this switch to call your paticular
      // app routine.

      //These modeMenu case statements have to match the order of the apps in the modemenu function
      switch (modeMenu()) { // Call modeMenu. The return is the desired app number
        case 0: // Zero is the clock, just exit the switch
          break;
        case 1:
          appAccel();
          break;
        case 2:
          appBattery();
          break;
        case 3:
          appTouch();
          break;
        case 4:
          appSetTime();
          break;
      }
    }

    //App returned control to main loop, or just woke up from sleep
    lastActiveTime = millis(); //Reset lastActiveTime tracker
    appDisplayTime->lastActiveTime = lastActiveTime;
    //Redraw display in full as last running app had taken over the screen
    appDisplayTime->displayTime(true); 
  }
}

int readBattery() 
{
  ttgo->power->adc1Enable(AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_BATT_VOL_ADC1, true);
  // get the values
  //float vbus_v = ttgo->power->getVbusVoltage();
  //float vbus_c = ttgo->power->getVbusCurrent();
  //float batt_v = ttgo->power->getBattVoltage();
  int percentPower = ttgo->power->getBattPercentage();
  lastBattChkTime = millis();
  
  return percentPower;
}

void low_energy()
{
    if (ttgo->bl->isOn()) {
        //xEventGroupSetBits(isr_group, WATCH_FLAG_SLEEP_MODE);
        ttgo->closeBL();
        ttgo->stopLvglTick();
        ttgo->bma->enableStepCountInterrupt(false);
        ttgo->displaySleep();

        /*
        if (!WiFi.isConnected()) {
            lenergy = true;
            WiFi.mode(WIFI_OFF);
            // rtc_clk_cpu_freq_set(RTC_CPU_FREQ_2M);
            setCpuFrequencyMhz(20);
        */ 
        {
            setCpuFrequencyMhz(10);
            Serial.println("ENTERING LIGHT SLEEP MODE");
            gpio_wakeup_enable ((gpio_num_t)AXP202_INT, GPIO_INTR_LOW_LEVEL);
            gpio_wakeup_enable ((gpio_num_t)BMA423_INT1, GPIO_INTR_HIGH_LEVEL);
            esp_sleep_enable_gpio_wakeup ();
            esp_light_sleep_start();
        }
        active = false;
    } else {
        ttgo->startLvglTick();
        ttgo->displayWakeup();
        ttgo->rtc->syncToSystem();
        //updateStepCounter(ttgo->bma->getCounter());
        //updateBatteryLevel();
        //updateBatteryIcon(LV_ICON_CALCULATION);
        lv_disp_trig_activity(NULL);
        ttgo->openBL();
        ttgo->bma->enableStepCountInterrupt();
        active = true;
        lastActiveTime = millis();
        appDisplayTime->lastActiveTime = lastActiveTime;
        Serial.println("AWAKE");
    }
}
