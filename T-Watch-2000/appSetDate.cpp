/*
 * App class to set the date
 * 
 * Based on the set time app.
 * 
 * This is a 'blocking app' in that is does not allow code in the main watch
 * loop to execute while the app it active. So it has an inactivity timeout
 * so that the app exits automatically if  no touch is detected for the set 
 * inactivity period.
 */
#include "watchdef.h"

class AppSetDate {
    public:
    private:
        TTGOClass *ttgo;
        unsigned long lastActiveTime = 0;   //Tracks time of last interaction from user (to enable sleep after inactivity)
        uint16_t inactivityTimeout;
        uint8_t hh, mm, ss, mmonth, dday; // H, M, S variables
        uint16_t yyear; // Year is 16 bit int
    public:
        AppSetDate(TTGOClass *ttgo_, uint16_t timeout) 
            : ttgo(ttgo_), inactivityTimeout(timeout)
        {}

        ~AppSetDate(){}

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

        //Loop waiting for 'SET' button press, or inactivity timeout
        while ((wl != 13) && (millis() - lastActiveTime < inactivityTimeout)) {
            wl = getTnum();
            //If touch was detected
            if (wl != -1) {
                if (wl != 13) {
                    //Digit pressed, so update if valid
                    int testVal;
                    switch (curnum) {
                        case 1:
                            testVal = wl * 10 + dday % 10;
                            //Prevent days over 3X
                            if (wl < 4) dday = testVal;
                            break;
                        case 2:
                            testVal = int(dday / 10) * 10 + wl;
                            if (testVal < 32) dday = testVal;
                            //Cap day to 31 (we don't know what month is going to be set yet)
                            if (dday > 31) dday = 31;
                            break;
                        case 3:
                            testVal = wl * 10 + mmonth % 10;
                            //Prevent months over 1X
                            if (wl < 2) mmonth = testVal;
                            break;
                        case 4:
                            testVal = int(mmonth / 10) * 10 + wl;
                            if (testVal < 13) mmonth = testVal;
                            //Cap months to 12
                            if (mmonth > 12) mmonth = 12;
                            break;
                        case 5:
                            testVal = int(yyear / 100) * 100 + wl*10 + yyear % 10;
                            yyear = testVal;
                            break;
                        case 6:
                            testVal = int(yyear / 10) * 10 + wl;
                            yyear = testVal;
                            break;
                    }                    
                    //Wait for touch release
                    while (getTnum() != -1) {}
                    curnum += 1;
                    if (curnum > 6) curnum = 1;
                }
                prtTime(curnum);
            }
        }
        //Wait for touch release
        while (getTnum() != -1) {}

        //Update time if DONE pressed
        if (wl == 13) {
            Serial.print("Setting date/time: ");
            Serial.print(dday);
            Serial.print("/");
            Serial.print(mmonth);
            Serial.print("/");
            Serial.print(yyear);
            Serial.print("  ");
            Serial.print(hh);
            Serial.print(":");
            Serial.print(mm);
            Serial.print(":");
            Serial.println(ss);

            ttgo->rtc->setDateTime(yyear, mmonth, dday, hh, mm, 0);
        }
    }

    // prtTime will display the current selected date and highlight
    // the current digit to be updated in yellow

    void prtTime(byte digit) {
        ttgo->tft->fillRect(0, 0, 205, 34, TFT_BLACK);
        if (digit == 1)   ttgo->tft->setTextColor(TFT_YELLOW);
        else   ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawNumber(int(dday / 10), 5, 5, 2);
        if (digit == 2)   ttgo->tft->setTextColor(TFT_YELLOW);
        else   ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawNumber(dday % 10, 25, 5, 2);
        ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawString("/",  45, 5, 2);
        if (digit == 3)   ttgo->tft->setTextColor(TFT_YELLOW);
        else   ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawNumber(int(mmonth / 10), 65 , 5, 2);
        if (digit == 4)   ttgo->tft->setTextColor(TFT_YELLOW);
        else   ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawNumber(mmonth % 10, 85, 5, 2);
        ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawString("/",  105, 5, 2);
        //Date only supports 2 digits in hardware, so only allow 2 to be set
        ttgo->tft->drawNumber(int(yyear / 1000), 125, 5, 2);
        ttgo->tft->drawNumber(int((yyear%1000) / 100), 145, 5, 2);
        if (digit == 5)   ttgo->tft->setTextColor(TFT_YELLOW);
        else   ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawNumber(int((yyear%100) / 10), 165, 5, 2);
        if (digit == 6)   ttgo->tft->setTextColor(TFT_YELLOW);
        else   ttgo->tft->setTextColor(TFT_WHITE);
        ttgo->tft->drawNumber(yyear % 10, 185, 5, 2);
            Serial.print("Pending date/time: ");
            Serial.print(dday);
            Serial.print("/");
            Serial.print(mmonth);
            Serial.print("/");
            Serial.print(yyear);
            Serial.print("  ");
            Serial.print(hh);
            Serial.print(":");
            Serial.print(mm);
            Serial.print(":");
            Serial.println(ss);
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
