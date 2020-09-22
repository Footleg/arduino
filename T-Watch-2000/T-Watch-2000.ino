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
 * I have refactored the example framework to be more structured. The main loop controls the 
 * actions the watch constantly monitors in the background. Screen update are all handled by
 * apps, which should return control to the main program loop so background monitoring continues
 * while apps are displayed. The displayTime class is the main watch screen app. The UI has been 
 * kept deliberately basic so you can see how the code works with the watch hardware features. It 
 * just displays the time (including seconds), date and battery level. The inactivity counter 
 * (counting the seconds since the user last touched the screen) is shown to help you see how this 
 * works.
 * 
 * The main loop monitors the touch screen for input, and counts the inactivity time in order to
 * put the watch into power saving mode, and wake it up again on touching the screen. This 
 * currently extends the battery life to around 24 hours. I think further improvements can be made, 
 * but this is a good starting point and I am publishing it now so others can build on it too. 
 * 
 * The menu system is just another app. I have made the menu auto-close after the inactivity time out 
 * so that the menu being left active does not prevent the watch going into sleep mode. Note that 
 * other apps will keep the screen on and the watch in 'active' mode until you exit them. So the 
 * battery will still run down if an app is left open. These apps which currently block the main
 * program loop will be replaced with classes which hand back to the main loop on each cycle so the
 * background features of the watch keep going. I have integrated the blocking apps from the 
 * original project for now just so the watch works. But I recommend using the displayTime class
 * as a pattern to follow if you are developing any apps yourself. 
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

//#define ARDUINO 10813  //DO NOT COMPILE WITH THIS DEFINE ENABLED! It is to help VSCODE spot code problems.

#include "watchdef.h"
#include <soc/rtc.h>
//#include "esp_wifi.h"
//#include "esp_sleep.h"
#include "displayTime.cpp"

TTGOClass *ttgo; //Main watch class, used to access all watch features

//Global constants and variables
const uint16_t inactivityTimeout = 10000;    //Interval in msec after which the watch is put into low power mode (and screen turned off)
const uint16_t batteryCheckInterval = 30000; //Interval in msec between reads of the battery voltage

//App name constants (makes the code much more readable)
const uint8_t APP_TIMESHOW  = 0;
const uint8_t APP_MENU      = 255; //App not selectable from menu, it is the menu :)
const uint8_t APP_ACCELDEMO = 1;
const uint8_t APP_BATTINFO  = 2;
const uint8_t APP_TOUCHDEMO = 3;
const uint8_t APP_TIMESET   = 4;

uint8_t activeApp = APP_TIMESHOW;   //Track which app is active
bool active = true;                 //Flag to track if screen is active for updates/power saving
uint32_t lastActiveTime = 0;        //Tracks time of last interaction from user (to enable sleep after inactivity)
uint32_t nextUpdateTime = 0;        //TODO Move this into time app. Time which next display update should not occur before (avoids updating time display more than once per second)
uint32_t lastBattChkTime = 0;       //Time when the battery level was last read

//Declare variables for all watch app classes here. These will be initialised in the setup function after the watch object has been created.
AppTimeDisplay *appDisplayTime;

void setup() {
    Serial.begin(19200); //Serial port maybe uses power? Suggest comment out unless debugging

    //Initialise watch hardware class, and clear screen
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

    //Initialise action tracker variables. These are used to determine when it is time to
    //perform actions which only need to happen periodically. e.g. Putting the watch into
    //low power mode after the last interaction from the user.
    lastActiveTime = millis();
    lastBattChkTime = millis();
    
    //Create time display app class. This app displays the main watch time screen.
    appDisplayTime = new AppTimeDisplay(ttgo);
    appDisplayTime->lastActiveTime = lastActiveTime;
    //Read battery level before first display update
    appDisplayTime->batteryLevel = readBattery();

    appDisplayTime->updateScreen(true); //Update display to show the time

    ttgo->openBL(); // Turn on the backlight

}

/**************************************************************************************************
 * Main Loop
 * 
 * This controls the watch power saving mode/sleep mode and tracks which app currently has control
 * of the screen. It is also responsible for any 'always active' background tasks, such as counting
 * steps, monitoring for touch screen interaction from the user
 *************************************************************************************************/
void loop() {

    //Update display if watch is not in sleep mode (power saving mode)
    if (active) {
        // Check if inactivity timeout reached
        if (millis() - lastActiveTime > inactivityTimeout) {
            //Put into low energy mode
            low_energy();
        }
        else {
            //Update display
            switch(activeApp) {
                case APP_TIMESHOW:
                    appDisplayTime->lastActiveTime = lastActiveTime;
                    //Check if we need to update the time display yet (no point in doing so more than once per second)
                    if (nextUpdateTime < millis()) {
                        nextUpdateTime = millis() + 1000;
                        if (millis() - lastBattChkTime > batteryCheckInterval) {
                            appDisplayTime->batteryLevel = readBattery();
                            //Force a full refresh so battery level is updated on screen
                            appDisplayTime->updateScreen(true); 
                        }
                        else {
                            appDisplayTime->updateScreen(false); // Called every second but only update time every minute
                            //TODO Make control of screen update frequency a job of the app class, not the main loop
                        }
                    }
                    else {
                        //Pause to save processing
                        delay(200);
                    }
                    break;
               
                case APP_MENU:
                    //Get next active app
                    activeApp = modeMenu();
                    lastActiveTime = millis(); //Reset lastActiveTime tracker
                    lastBattChkTime = 0; //Reset this to force full display refresh if returning to clock display
                    break;
                
                case APP_ACCELDEMO:
                    //Some original apps take full control, so call them then revert to time app
                    appAccel();
                    activeApp = APP_TIMESHOW;
                    lastActiveTime = millis(); //Reset lastActiveTime tracker
                    lastBattChkTime = 0; //Reset this to force full display refresh
                    break;
                
                case APP_BATTINFO:
                    //Some original apps take full control, so call them then revert to time app
                    appBattery();
                    activeApp = APP_TIMESHOW;
                    lastActiveTime = millis(); //Reset lastActiveTime tracker
                    lastBattChkTime = 0; //Reset this to force full display refresh
                    break;
                
                case APP_TOUCHDEMO:
                    //Some original apps take full control, so call them then revert to time app
                    appTouch();
                    activeApp = APP_TIMESHOW;
                    lastActiveTime = millis(); //Reset lastActiveTime tracker
                    lastBattChkTime = 0; //Reset this to force full display refresh
                    break;
                
                case APP_TIMESET:
                    //Some original apps take full control, so call them then revert to time app
                    appSetTime();
                    activeApp = APP_TIMESHOW;
                    lastActiveTime = millis(); //Reset lastActiveTime tracker
                    lastBattChkTime = 0; //Reset this to force full display refresh
                    break;
                
             }
        }
    }

    //Check for touch screen interaction whether in sleep mode or not (so watch can be woken up from sleep)
    int16_t x, y;
    if (ttgo->getTouch(x, y)) {
        while (ttgo->getTouch(x, y)) {} // wait for user to release

        //Check if watch is in low power mode
        if (!active) {
            //Just wake up the watch if asleep
            wake_up(); 
            lastActiveTime = millis(); //Reset lastActiveTime tracker

            //Active app will update screen on next pass around loop, so
            //only take actions here for apps which need special action
            //after sleep
            switch(activeApp) {
                case APP_MENU:
                case APP_TIMESET:
                    //These apps should exit if watch was asleep
                    activeApp = APP_TIMESHOW; 
                    break;
            }
        }
        else {
            //Watch was already awake, so take action based on active app
            switch(activeApp) {
                case APP_TIMESHOW:
                    //Screen touch in time show app, activate menu
                    activeApp = APP_MENU;
                    break;
            }
        }
    }

    //Do any other always active background tasks here
    //e.g. Increment step counter
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

/******************************************************************************
 * Put the watch into low energy mode.
 * This code needs more work. I am not sure what some of these things do. This
 * code is based on the SimpleWatch example but some aspects are commented out
 * where some dependent libraries are not included in this project (yet?)
 *****************************************************************************/
void low_energy()
{
    Serial.println("ENTERING LIGHT SLEEP MODE");

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

    setCpuFrequencyMhz(10);
    gpio_wakeup_enable ((gpio_num_t)AXP202_INT, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable ((gpio_num_t)BMA423_INT1, GPIO_INTR_HIGH_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_light_sleep_start();

    active = false;
}

void wake_up()
{
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

    Serial.println("AWAKE");
}
