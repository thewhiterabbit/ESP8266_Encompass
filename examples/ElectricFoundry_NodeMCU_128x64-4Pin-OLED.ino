/*
 Name:		ElectricFoundry_NodeMCU_128x64_4Pin_OLED.ino
 Created:	11/20/2020 2:33:10 PM
 Author:	ethan
*/




/*
 Temperature Control
*/
#include <SPI.h>
#include <Adafruit_MAX31855.h>

// Default connection is using software SPI, but comment and uncomment one of
// the two examples below to switch between software SPI and hardware SPI:

// Creating a thermocouple instance with software SPI on any three
// digital IO pins.
#define MAXDO   D0
#define MAXCS   D8
#define MAXCLK  D5
// Heater Coil Relay
#define HC      D3

// initialize the Thermocouple
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

/*
 END Temperature Control INCLUDES, CONSTANTS & FUNCTIONS
*/




/*
 Web Server
*/
#include "Encompass.h"
/*
 END Web Server INCLUDES, CONSTANTS & FUNCTIONS
*/




/*
  Graphics
*/
#include <U8g2lib.h>

// Contructor line
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /*reset=*/ U8X8_PIN_NONE);

/*
 END GRAPHICS INCLUDES, CONSTANTS & FUNCTIONS
*/




// Setup function runs once when you press reset or power the board
void setup() {
    Serial.begin(9600);





    //
    // START Graphics SETUP
    //
    u8g2.begin();
    u8g2.enableUTF8Print();
    //
    // END Graphics SETUP
    //




    //
    // START Error Control SETUP
    //
    int8 errc = 0;
    //
    // END Error Control SETUP
    //




    //
	// START Temperature Control MAX31855 SETUP
    //
    // Create Set Temperature variable
    //int8 st;

    // Set HC to output mode
    pinMode(HC, OUTPUT);

    // Set HC to low
    digitalWrite(HC, HIGH);

    while (!Serial) delay(1); // wait for Serial on Leonardo/Zero, etc

    Serial.println("MAX31855 test");
    // Wait for MAX chip to stabilize
    delay(500);
    Serial.print("Initializing sensor...");
    if (!thermocouple.begin()) {
        Serial.println("Thermocouple ERROR.");
        while (1) delay(10);
    }
    Serial.println("Thermocouple DONE.");

    //
    // END MAX31855 SETUP
    //




    //
    // START WEB SERVER SETUP
    //

    //
    // END WEB SERVER SETUP
    //
}
// END SETUP FUNCTION




// Loop function runs over and over again until power down or reset
void loop() {
    //
    // START Temperature Control MAX31855 LOOP
    //
    // Create set temperature variable
    int st;

    if (isnan(st))
    {

    }

    /* Create mode variable
        0 = Internal
        1 = Celsius
        2 = Fahrenheit
    */
    int8 m;

    // basic readout test, just print the current temp
    Serial.print("Internal Temp = ");
    Serial.println(thermocouple.readInternal());

    int c = thermocouple.readCelsius();
    int f = thermocouple.readFahrenheit();
    if (isnan(c) || isnan(f)) {
        Serial.println("Something wrong with thermocouple!");
    }
    else {
        Serial.print("C = ");
        Serial.println(c);
        Serial.print("F = ");
        Serial.println(f);
    }
    
    // Initiate heater coils
 
    //
    // END Temperature Control MAX31855 LOOP
    //




    //
    // START WEB SERVER LOOP
    //
    
    //
    // END WEB SERVER LOOP
    //




    //
    // START GRAPHICS LOOP
    //
    u8g2.clearBuffer();					// clear the internal memory
    u8g2.setFont(u8g2_font_ncenB14_tf);	// choose a suitable font
    u8g2.setCursor(2,16);
    u8g2.print(f);	// write something to the internal memory
    u8g2.print("°F");
    u8g2.sendBuffer();					// transfer internal memory to the display
    delay(1000);
    //
    // END GRAPHICS LOOP
    //
}
// END LOOP FUNCTION