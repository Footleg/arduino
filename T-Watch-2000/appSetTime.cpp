/*
 * App class to set the time
 * 
 * Refactored Dan's code into a class, and fixed a bug where the DONE button 
 * press caused the code to add 13 to the highlighted digit of the time.
 * Also added inactivity timeout. So the app exits automatically without
 * changing the time if no touch is detected for the set inactivity period.
 * 
 * This is a 'blocking app' in that is does not allow code in the main watch
 * loop to execute while the app it active.
 */
#include "watchdef.h"

class AppSetTime {
    public:
    private:
        TTGOClass *ttgo;
        unsigned long lastActiveTime = 0;   //Tracks time of last interaction from user (to enable sleep after inactivity)
        uint16_t inactivityTimeout;
        uint8_t hh, mm, ss, mmonth, dday; // H, M, S variables
        uint16_t yyear; // Year is 16 bit int
    public:
        AppSetTime(TTGOClass *ttgo_, uint16_t timeout) 
            : ttgo(ttgo_), inactivityTimeout(timeout)
        {}

        ~AppSetTime(){}

    void showApp() {
        //Reset lastActiveTime tracker
        lastActiveTime = millis(); 

        // Get the current info
        RTC_Date tnow = ttgo->rtc->getDateTime();

        hh = tnow.hour;
        mm = tnow.minute;
        ss = tnow.second;
        dday = tnow.day;
        mmonth = tnow.month;
        yyear = tnow.year;

        //Set up the interface buttons
        ttgo->tft->fillScreen(TFT_BLACK);
        ttgo->tft->fillRect(0, 35, 80, 50, TFT_BLUE);
        ttgo->tft->fillRect(161, 35, 78, 50, TFT_BLUE);
        ttgo->tft->fillRect(81, 85, 80, 50, TFT_BLUE);
        ttgo->tft->fillRect(0, 135, 80, 50, TFT_BLUE);
        ttgo->tft->fillRect(161, 135, 78, 50, TFT_BLUE);
        ttgo->tft->fillRect(0, 185, 80, 50, TFT_BLUE);
        ttgo->tft->setTextColor(TFT_GREEN);
        ttgo->tft->drawNumber(1, 30, 40, 2);
        ttgo->tft->drawNumber(2, 110, 40, 2);
        ttgo->tft->drawNumber(3, 190, 40, 2);
        ttgo->tft->drawNumber(4, 30, 90, 2);
        ttgo->tft->drawNumber(5, 110, 90, 2);
        ttgo->tft->drawNumber(6, 190, 90, 2);
        ttgo->tft->drawNumber(7, 30, 140, 2);
        ttgo->tft->drawNumber(8, 110, 140, 2);
        ttgo->tft->drawNumber(9, 190, 140, 2);
        ttgo->tft->drawNumber(0, 30, 190, 2);
        ttgo->tft->fillRoundRect(120, 200, 119, 39, 6, TFT_WHITE);
        ttgo->tft->setTextSize(2);
        ttgo->tft->setCursor(0, 0);

        ttgo->tft->setCursor(155, 210);
        ttgo->tft->setTextColor(TFT_BLACK);
        ttgo->tft->print("SET");
        ttgo->tft->setTextColor(TFT_WHITE);
        int wl = 0; // Track the current number selected
        byte curnum = 1;  // Track which digit we are on

        prtTime(curnum); // Display the time for the current digit

        //Loop waiting for 'SET' press, or inactivity timeout
        while ((wl != 13) && (millis() - lastActiveTime < inactivityTimeout)) {
            wl = getTnum();
            //If touch was detected
            if (wl != -1) {
                if (wl != 13) {
                    //Digit pressed, so update if valid
                    int testVal;
                    switch (curnum) {
                        case 1:
                            testVal = wl * 10 + hh % 10;
                            //Prevent hours over 2X
                            if (wl < 3) hh = testVal;
                            break;
                        case 2:
                            testVal = int(hh / 10) * 10 + wl;
                            if (testVal < 24) hh = testVal;
                            //Cap hours to 23
                            if (hh > 23) hh = 23;
                            break;
                        case 3:
                            testVal = wl * 10 + mm % 10;
                            //Prevent minutes over 5X
                            if (wl < 6) mm = testVal;
                            break;
                        case 4:
                            testVal = int(mm / 10) * 10 + wl;
                            if (testVal < 60) mm = testVal;
                            //Cap minutes to 59
                            if (mm > 59) hh = 59;
                            break;
                    }
                    //Wait for touch release
                    while (getTnum() != -1) {}
                    curnum += 1;
                    if (curnum > 4) curnum = 1;
                }
                prtTime(curnum);
            }
        }
        //Wait for touch release
        while (getTnum() != -1) {}

        //Update time if SET pressed
        if (wl == 13) {
            Serial.print("Setting time: ");
            Serial.print(dday);
            Serial.print("/");
            Serial.print(mmonth);
            Serial.print("/");
            Serial.print(yyear);
            Serial.print("  ");
            Serial.print(hh);
            Serial.print(":");
            Serial.println(mm);

            ttgo->rtc->setDateTime(yyear, mmonth, dday, hh, mm, 0);
        }
    }

    // prtTime will display the current selected time and highlight
    // the current digit to be updated in yellow

    void prtTime(byte digit) {
        ttgo->tft->fillRect(0, 0, 100, 34, TFT_BLACK);
        if (digit == 1)   ttgo->tft->setTextColor(TFT_YELLOW);
        else   ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawNumber(int(hh / 10), 5, 5, 2);
        if (digit == 2)   ttgo->tft->setTextColor(TFT_YELLOW);
        else   ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawNumber(hh % 10, 25, 5, 2);
        ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawString(":",  45, 5, 2);
        if (digit == 3)   ttgo->tft->setTextColor(TFT_YELLOW);
        else   ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawNumber(int(mm / 10), 65 , 5, 2);
        if (digit == 4)   ttgo->tft->setTextColor(TFT_YELLOW);
        else   ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawNumber(mm % 10, 85, 5, 2);
    }

    // getTnum takes care of translating where we pressed into
    // a number that was pressed. Returns -1 for no press
    // and 13 for DONE
    int getTnum() {
        int16_t x, y;

        if (!ttgo->getTouch(x, y)) return - 1;

        //Reset lastActiveTime tracker as something was touched
        lastActiveTime = millis(); 

        if (y < 85) {
          if (x < 80) return 1;
          else if (x > 160) return 3;
          else return 2;
        }
        else if (y < 135) {
          if (x < 80) return 4;
          else if (x > 160) return 6;
          else return 5;
        }
        else if (y < 185) {
          if (x < 80) return 7;
          else if (x > 160) return 9;
          else return 8;
        }
        else if (x < 80) return 0;
        else return 13;
    }

};
