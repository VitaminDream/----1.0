#include "oled_display.h"


#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64


#define OLED_RESET -1


Adafruit_SSD1306 display(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    OLED_RESET
);



bool oledReady = false;



// ==============================
// OLED初始化
// ==============================

void oledBegin()
{


    Serial.println("[OLED] Initializing...");


    if(!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C
    ))
    {

        Serial.println("[OLED] Failed");

        oledReady = false;

        return;
    }



    oledReady = true;


    display.clearDisplay();


    display.setTextSize(1);

    display.setTextColor(
        SSD1306_WHITE
    );


    display.setCursor(
        0,
        0
    );


    display.println(
        "BUOY V1.3"
    );


    display.println(
        "OLED READY"
    );


    display.display();


    delay(1000);



    Serial.println("[OLED] Ready");


}



// ==============================
// OLED显示更新
// ==============================

void oledUpdate(
    const ImuData &imu,
    const EnvData &env,
    TinyGPSPlus &gps,
    bool sdOK
)

{

    if(!oledReady)
        return;



    display.clearDisplay();



    display.setTextSize(1);

    display.setTextColor(
        SSD1306_WHITE
    );



    int y = 0;



    display.setCursor(
        0,
        y
    );

    display.println(
        "BUOY DATA"
    );



    y += 10;



    display.setCursor(
        0,
        y
    );


    display.printf(
        "T:%.1f C",
        env.temperatureC
    );



    display.setCursor(
        70,
        y
    );


    display.printf(
        "P:%.0f",
        env.pressureHpa
    );



    y += 10;



    display.setCursor(
        0,
        y
    );


    display.printf(
        "H:%.1f%%",
        env.humidityPct
    );



    y += 10;



    display.setCursor(
        0,
        y
    );


    display.printf(
        "R:%.1f",
        imu.rollDeg
    );



    display.setCursor(
        70,
        y
    );


    display.printf(
        "Y:%.1f",
        imu.pitchDeg
    );



    y += 10;



    display.setCursor(
        0,
        y
    );


    if(
        gps.location.isValid()
    )
    {

        display.println(
            "GPS:FIX"
        );

    }
    else
    {

        display.println(
            "GPS:SEARCH"
        );

    }



    y += 10;



    display.setCursor(
        0,
        y
    );


    if(sdOK)
    {

        display.println(
            "SD:OK"
        );

    }
    else
    {

        display.println(
            "SD:ERR"
        );

    }



    display.display();

}