/*
  Keithley 2001 and 2002 VFD Display Replacement

  Version 1.02 - 26th July 2026 by Scott - aka The Defpom

  Created by The Defpom http://www.thedefpom.co.nz https://www.youtube.com/thedefpom

  I made a video about creating this project showing the reverse engineering, test builds, and the final build along with the installation of the display and Qualia module into my 2002.
  You can watch my video here: https://youtu.be/HHad28dQnug

  Inspired by the work of Le_Bassiste, and especially BennoG who provided some key information which gave me a starting point in understanding some of the byte structure.

  EEVblog thread here: https://www.eevblog.com/forum/projects/keithley-2001-display-substitute/
  BennoG Github here: https://github.com/BennoG/Keithley-2001-oled

  Written for the ESP32-S3 Adafruit Qualia running at 240MHz.

  Display used is a 4.58" 320x960 rectangle bar display along with the Adafruit Qualia ESP32 driver board, but you could easily adapt it to suit another device and display by changing the pin definitions.
  The Qualia and 4.58" RGB666 display are available here: https://learn.adafruit.com/adafruit-qualia-esp32-s3-for-rgb666-displays

  For the 2002 (and 2001) display data can be found at pin 8 of the IDC cable that goes to the front panel, alternative (especially for testing) is R603 next to the IDC on the digital board, the final connection should be made at pin 32 of the front panel uC, see my video for more information.

  I recommend using the JST connector on the Qualia as it has GND, 5V and A0 (through a 1K resistor, and shunted by a 3.6V zener), all the connections we need !

  Libraries and Boards Required:
  (board) ESP32 by Espressive Systems - add this to your board manager urls in settings: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  (library) Arduino GFX Library (built on 1.12.6) and its dependancies.

  Build settings:
  Board: Adafruit Qualia ESP32-S3 RGB666 using default settings
  Place fonts in the sketch folder as THISSKETCHFOLDER/Fonts/FONTSHERE
  It is HIGHLY recomended to use the fonts supplied, I modifed the Roboto set to add degrees, mu, and ohms symbols at the same hex as the Adafuit GFX default 5x7 font so both display rows work correctly!

  Note on font choice: this uses special chars for Ohms (0xEA 234), Degrees (0xF8 248), Micro (0xE6 230), as well as the bars used for the bar graph, as seen in getDisplayScreen,
  this makes font choice tricky, the default built-in font by the adafruit_gfx_library actually works well for this, but it is small and has to be scaled up,
  if I find a larger font which has the required chars I will change it, but far far I have not been successfull.
  I highly recommmend NOT changing the fonts included, unless you can find one that has all the required Chars, if you do find one, let me know as I want it too LOL.

  Revision history
  (Rev - Date - By - Changes)
  Rev 1.0 - 17th July 2026 - The Defpom - Initial release
  Rev 1.01 - 23rd July 2026 - The Defpom - Added space injection in main readout for 8.5 digit mode to improve readout ie: 1.23456789mV to 1.23456789 mV
  Rev 1.02 - 26rd July 2026 - The Defpom - Added display update timer to slow down display writing to sensible speed.

*/

#include <EEPROM.h> // EEPROM support
#define EEPROM_SIZE 32 // can change this up and down, but this should be plenty

#include <Arduino_GFX_Library.h> // display support

#include "Fonts/FreeMonoBold18pt7b.h"  // is saved in a Fonts folder within this sketches folder, not using the arduino library
#include "Fonts/FreeSansBold18pt7b.h"  // is saved in a Fonts folder within this sketches folder, not using the arduino library
#include "Fonts/Roboto16pt8b.h"  // DO NOT CHANGE THIS FONT unless you like spending time fixing broken chars, this is a modified font for degrees, mu, ohms symbols to match the classic 5x7 Adafruit GFX bitmap font
#include "Fonts/Roboto40pt8b.h"  // DO NOT CHANGE THIS FONT unless you like spending time fixing broken chars, this is a modified font for degrees, mu, ohms symbols to match the classic 5x7 Adafruit GFX bitmap font

// display support
Arduino_XCA9554SWSPI *expander = new Arduino_XCA9554SWSPI(
  PCA_TFT_RESET, PCA_TFT_CS, PCA_TFT_SCK, PCA_TFT_MOSI,
  &Wire, 0x3F);
// display support
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
  TFT_DE, TFT_VSYNC, TFT_HSYNC, TFT_PCLK,
  TFT_R1, TFT_R2, TFT_R3, TFT_R4, TFT_R5,
  TFT_G0, TFT_G1, TFT_G2, TFT_G3, TFT_G4, TFT_G5,
  TFT_B1, TFT_B2, TFT_B3, TFT_B4, TFT_B5,
  /* 4.58" display settings */
  1 /* hsync_polarity */, 30 /* hsync_front_porch */, 10 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
  1 /* vsync_polarity */, 15 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 17 /* vsync_back_porch */
);
// display support and size options (if changing display size, positions and text sizes will need altering to match !)
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  // 3.2" 320x820 rectangle bar display
  //    320 /* width */, 820 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
  //    expander, GFX_NOT_DEFINED /* RST */, tl032fwv01_init_operations, sizeof(tl032fwv01_init_operations));
  // 3.7" 240x960 rectangle bar display
  //    240 /* width */, 960 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
  //    expander, GFX_NOT_DEFINED /* RST */, HD371001C40_init_operations, sizeof(HD371001C40_init_operations), 120 /* col_offset1 */);
  // 4.58" 320x960 rectangle bar display
  320 /* width */, 960 /* height */, rgbpanel, 3 /* rotation */, true /* auto_flush */,
  expander, GFX_NOT_DEFINED /* RST */, HD458002C40_init_operations, sizeof(HD458002C40_init_operations), 80 /* col_offset1 */
  // needs also the Arduino_ESP32RGBPanel to have these pulse/sync values (above):
  //    1 /* hsync_polarity */, 30 /* hsync_front_porch */, 10 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
  //    1 /* vsync_polarity */, 15 /* vsync_front_porch */,  2 /* vsync_pulse_width */, 17 /* vsync_back_porch */
);



#define RXD2 17  // Read the Keithley display data bus - Using the A0 pin which is shunted by a 3.6V zener to clamp input, BUT if using the JST connector is also run through a 1K resistor, how convenient !
//#define TXD2 17 // not used

int textcharcount = 0;          // char count of display, used to know when to switch to 2nd row
byte incomingByte = 0;          // current byte, also byte tracking for byte sequence matching
byte lastincomingByte = 0;      // byte tracking for byte sequence matching
byte lastlastincomingByte = 0;  // byte tracking for byte sequence matching
String flagslist = "";          // flags builder - temporary
String flagslistlast = "";      // flags
String rowtext = "";            // text holder - temporary
String row1 = "";               // 1st row main display
String row2 = "";               // 2nd row sub display
byte flagsByte1 = 0;            // 1st flag byte
byte flagsByte2 = 0;            // 2nd flag byte
int flagsByte1int = 0;          // hex to int conversion of 1st flag byte (probably not needed)
int flagsByte2int = 0;          // hex to int conversion of 2nd flag byte (probably not needed)
byte flashing = 0;              // flag to highlight that a peice of text is flashing
byte booted = 0;                // flag boot screen checks (0 = showing 1st boot screen,1 = showing 2nd boot screen wtih Cal info, 2 = booted fully)

unsigned long lastmillis = 0;          // holder for millis since startup
unsigned long savedmillis = 0;         // holder for millis from last LED change
unsigned long displaytimermillis = 0;  // holder for display timer, used to work around lack of predictable end char of 2nd boot screen
unsigned long lastdisplaywritemillis = 0; // holder for display update delay

// default display text sizes, but are re-specified inside DoDisplay
int line1textsize = 1;  // line 1 text size default
int line2textsize = 1;  // line 2 text size default
int line3textsize = 4;  // line 3 text size default

// fix to make sure old text of blacked out if new text is shorter, specified inside DoDisplay
int targetLength1 = 0;
int targetLength2 = 0;
int targetLength3 = 0;

// tracks number of times a certain char is matched in a string, used for display blanking and overwriting
char targetChar = ' ';
int row1charmatchcount = 0;

// convert to a dedicated string to avoid the padding for spaces causing issue
String flagslistlastview = "";
String row1view = "";
String row2view = "";
// reemmber last used string per row, to only update if changed
String flagslistlastviewlast = "";
String row1viewlast = "";
String row2viewlast = "";

// specified chars used in char conversion when detecting a peice of flashing text
char searchFlashingStartChar = '[';
char searchFlashingEndChar = ']';
int flashingStartCharIndex = -1;
int flashingEndCharIndex = -1;

// used to break up row3 text into sections to handle colour changing for flashing text
String firsttextsection = "";
String secondtextsection = "";
String thirdtextsection = "";



// build a list of basic colours available to use, the selected set are saved and retrieved from EEPROM on bootup
// named colours can be found here: https://github.com/moononournation/Arduino_GFX/blob/master/src/Arduino_GFX.h
// Examples of named colours can be seen here: https://www.w3.org/wiki/CSS/Properties/color/keywords
//                                    1             2             3                 4             5              6                 7                 8              9           10                11
uint16_t fontcolourarray[] = {0,RGB565_YELLOW,RGB565_WHITE,RGB565_LIME,RGB565_MAGENTA,RGB565_SKYBLUE,RGB565_DARKKHAKI,RGB565_LIGHTSKYBLUE,RGB565_YELLOW,RGB565_CYAN,RGB565_DARKORANGE,RGB565_LAVENDER};
int numberofcolours = 11; // need to ensure this matches the above array count of NAMED COLOURS ONLY (excludes index 0), so the colour changer code can rotate through them correctly.


// default colours for use for each display line, and highlighting - note these are rotatable by pressing the UP button on the Qualia
byte flagscolour = 10; // row 1 colour
byte row1colour = 11; // row 2 colour
byte row2colour = 1; // row 2 colour
byte highlightcolour = 2; // text highlighting colour






void setup(){
  setCpuFrequencyMhz(240);  //Set CPU clock to 240/160/80MHz for example

  Serial.begin(115200);  // IDE serial monitor port, you can safely change this speed to anything you need. (can be commented out if not wanted)
  delay(50);             // always a good idea to add a delay after starting a port
  //Serial2.setRxBufferSize(2048); // if your device has less memory you can reduce this, or even comment out the line, I added it as a precaution.
  Serial2.setRxBufferSize(4096);  // if your device has less memory you can reduce this, or even comment out the line, I added it as a precaution.
  //Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); // Keithley connected serial port, must be 9600 baud 8N1, but yo can change ports (note TX is not actually used!)
  Serial2.begin(9600, SERIAL_8N1, RXD2);  // Keithley connected serial port, must be 9600 baud 8N1, but you can change ports (note TX is not actually used!)
  delay(200);                             // always a good idea to add a delay after starting a port


  #ifdef GFX_EXTRA_PRE_INIT
    GFX_EXTRA_PRE_INIT();
  #endif

  // Init Display
  Wire.setClock(400000);  // speed up I2C

  if (!gfx->begin()) {
    Serial.println("gfx->begin() failed!");
    while (1) yield();
  }

  // fill the screen with black
  gfx->fillScreen(RGB565_BLACK);

  //turn off text wrapping, so we can pad with lots of spaces to overwrite old text to hide it, rather than doing screen blanking and causing it to flicker.
  gfx->setTextWrap(false);

  // turn on the display backlight
  expander->pinMode(PCA_TFT_BACKLIGHT, OUTPUT);
  expander->digitalWrite(PCA_TFT_BACKLIGHT, HIGH);

  // set the font
  gfx->setFont(&FreeMonoBold18pt7b);

  // show a basic splash to show it kind of works
  gfx->setCursor(40, 170);
  gfx->setTextSize(2);
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  //gfx->println("Created By The Defpom");
  gfx->println("Display By The Defpom");
  gfx->setCursor(240, 220);
  gfx->setTextSize(1);
  gfx->setTextColor(RGB565_YELLOW, RGB565_BLACK);
  gfx->println("https://thedefpom.co.nz");

  delay(1200);  // hold the credit splash on screen briefly

  // fill the screen with black
  gfx->fillScreen(RGB565_BLACK);

  // optional button controls built into Qualia, using these to allow font test anc colour changes
  expander->pinMode(PCA_BUTTON_UP, INPUT);
  expander->pinMode(PCA_BUTTON_DOWN, INPUT);

  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);


  // FORCE COLOURS TO DEFAULT - fixes oddness if messing with adding/removing colours, shouldn't normally need this !
  /*
  EEPROM.write(5, flagscolour); // address (up to 511), value (up to 255)
  EEPROM.write(11, row1colour); // address (up to 511), value (up to 255)
  EEPROM.write(22, row2colour); // address (up to 511), value (up to 255)
  EEPROM.write(29, highlightcolour); // address (up to 511), value (up to 255)
  EEPROM.commit(); // stores the data only if it is different to what is already there- TBC
  */


  // read saved colours from EEPROM
  byte epromflagscolour = EEPROM.read(5); // read stored for row 1 colour
  if(epromflagscolour >= 0 && epromflagscolour <= numberofcolours){ // limit data range, outside of this range is random data so ignore it (first power up)
    flagscolour = epromflagscolour;
  }
  else{ // not saved yet, so save as default
    EEPROM.write(5, flagscolour); // address (up to 511), value (up to 255)
    EEPROM.commit(); // stores the data only if it is different to what is already there- TBC
  }
  byte epromrow1colour = EEPROM.read(11); // read stored for row 1 colour
  if(epromrow1colour >= 0 && epromrow1colour <= numberofcolours){ // limit data range, outside of this range is random data so ignore it (first power up)
    row1colour = epromrow1colour;
  }
  else{ // not saved yet, so save as default
    EEPROM.write(11, row1colour); // address (up to 511), value (up to 255)
    EEPROM.commit(); // stores the data only if it is different to what is already there- TBC
  }
  byte epromrow2colour = EEPROM.read(22); // read stored for row 1 colour
  if(epromrow2colour >= 0 && epromrow2colour <= numberofcolours){ // limit data range, outside of this range is random data so ignore it (first power up)
    row2colour = epromrow2colour;
  }
  else{ // not saved yet, so save as default
    EEPROM.write(22, row2colour); // address (up to 511), value (up to 255)
    EEPROM.commit(); // stores the data only if it is different to what is already there- TBC
  }
  byte epromhighlightcolour = EEPROM.read(29); // read stored for row 1 colour
  if(epromhighlightcolour >= 0 && epromhighlightcolour <= numberofcolours){ // limit data range, outside of this range is random data so ignore it (first power up)
    highlightcolour = epromhighlightcolour;
  }
  else{ // not saved yet, so save as default
    EEPROM.write(29, highlightcolour); // address (up to 511), value (up to 255)
    EEPROM.commit(); // stores the data only if it is different to what is already there- TBC
  }
}


// FONT CHAR SET TESTER !!!
void fontCharTest(){
  gfx->fillScreen(RGB565_BLACK); // fill the screen with black to clear it
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK); // white text on black background

  gfx->setTextSize(1); // set the scaling of the font (as will be used normally)
  gfx->setFont(&Roboto16pt8b); // your font - must be included earlier in sketch
  //gfx->setFont();  // set default built-in 5x7 font

  // font test layout - may need to adjust this to suit your font
  int fontteststartnum = 128; // starting number
  int fonttestendnum = 255; // ending number
  int fonttestnumcols = 8; // num of columns to show across the screen
  int fonttestnumrows = 8; // num of rows to show down the screen before redrawing
  int fonttestnumloops = 5; // num of times to loop through all screens before returning to normal operation

  // loop to show on screen specified number of times
  for(int h = 1; h <= fonttestnumloops; h++){
    for(int i = fontteststartnum; i <= fonttestendnum;){
      gfx->setCursor(1, 30); // move curser to start position x,y
      gfx->fillScreen(RGB565_BLACK); // fill the screen with black to clear it
      // build a page of rows
      for(int j = 1; j <= fonttestnumrows; j++){
        // build a single row
        for(int k = 1; k <= fonttestnumcols; k++){
          gfx->print("  "); // Add some space before char, to pad away from screen left edge
          gfx->write(i);
          gfx->print(" "); // Add a space between data
          gfx->print(i);
          gfx->print("   "); // Add extra space between characters
          i++;
        }
        gfx->println(""); // end row
      }
      // end page
      delay(12000); // delay to show chars on screen for reading
    }
  }
  gfx->fillScreen(RGB565_BLACK); // fill the screen with black to clear it
}






// allow changing between colour sets
void handleColourChanges(){
  // simply rotates through all the colours, and loops back again
  // currently only 10 colours set, excludes array item 0
  if(flagscolour > 0 && flagscolour < numberofcolours){
    flagscolour++;
  }
  else{
    flagscolour = 1;
  }
  if(row1colour > 0 && row1colour < numberofcolours){
    row1colour++;
  }
  else{
    row1colour = 1;
  }
  if(row2colour > 0 && row2colour < numberofcolours){
    row2colour++;
  }
  else{
    row2colour = 1;
  }
  if(highlightcolour > 0 && highlightcolour < numberofcolours){
    highlightcolour++;
  }
  else{
    highlightcolour = 1;
  }

  // fallback in case of a bug/problem
  if(row1colour == highlightcolour || row2colour == highlightcolour){
    flagscolour = 1;
    row1colour = 2;
    row2colour = 3;
    highlightcolour = 4;
  }

  // save new colours to memory to use them next time
  EEPROM.write(5, flagscolour); // address (up to 511), value (up to 255)
  EEPROM.write(11, row1colour); // address (up to 511), value (up to 255)
  EEPROM.write(22, row2colour); // address (up to 511), value (up to 255)
  EEPROM.write(29, highlightcolour); // address (up to 511), value (up to 255)
  EEPROM.commit(); // stores the data only if it is different to what is already there- TBC


  // force an update to all rows
  flagslistlastviewlast = "";
  row1viewlast = "";
  row2viewlast = "";

  doDisplay();


  Serial.println(flagscolour);
  Serial.println(row1colour);
  Serial.println(row2colour);
  Serial.println(highlightcolour);
  Serial.println(" ");
}





// send display information to the serial monitor
void sendSerialMonitor(){
  Serial.println(flagslistlast);
  Serial.println(row1);
  Serial.println(row2);
  Serial.println(" ");
}






// get serial data from DMM
void handleSerial() {
  // waits for serial data to arrive (non blocking, but doesn't matter here)
  //if(Serial2.available()){
  while (Serial2.available()) {
    // Read a single incoming byte from the Keithley
    incomingByte = Serial2.read();

    // handle the display routines
    doDisplayTypeChecks();

    //now move the bytes into memory, used for next 2 loops for checking flags - must be last thing done
    lastlastincomingByte = lastincomingByte;
    lastincomingByte = incomingByte;
  }
}







// handle the display routines
void doDisplayTypeChecks(){
  // get the boot screen data ready to display - uses incomingByte
  if (booted == 0) {  // 1st boot screen
    getBootScreen();
  }
  if (booted == 1) {  // 2nd boot screen showing calibration (if used)
    get2ndBootScreen();
  }
  if (booted == 2) {  // get the normal screen data ready to display - uses incomingByte, only gets normal display once bootup screen has been handled
    // work out the display flags - uses incomingByte
    getFlags();

    // get the normal screen data ready to display - uses incomingByte
    getDisplayScreen();
  }
}






// capture the boot up screen display and handle it
void getBootScreen(){
  // get start of bootup screen text block for 1st screen and 2nd screen (if used)
  if (incomingByte == 0x0F && textcharcount == 0) {
    textcharcount = 1;  // start tracking char count to detect rows
    rowtext = "";       // clear existing row text before starting

    // fill the screen with black
    gfx->fillScreen(RGB565_BLACK);
  }

  // capture display text
  if (textcharcount >= 1) {
    if (incomingByte >= 0x20 && incomingByte <= 255) {  // only add printable chars, normal or flashing chars
      rowtext = rowtext + char(incomingByte);
    }

    // detect which row the text should be put on
    if (textcharcount <= 21) {
      row1 = rowtext;
      // reset string ready for next row
      if (textcharcount == 21) {
        rowtext = "";
      }
    } else {
      row2 = rowtext;
    }

    // count how many chars have been printed (includes the 00 and 01 blank char)
    textcharcount = textcharcount + 1;
  }
  // end of textcharcount block
  ////////


  // detect end of text block (actually looking at start of 2nd boot screen display flags "0x0D 0x03" OR normal display flags "0x0D 0x04"
  // and trigger sending to serial/new display
  //if(lastincomingByte == 0x0D && (incomingByte == 0x03 || incomingByte == 0x04) ){
  if (lastincomingByte != 0x0D && incomingByte == 0x0D && textcharcount >= 1) {  // catch only the first instance of 0x0D, as it does 0x0D at the end, and again at the start of 2nd boot screen block "0x0D 0x03"
    // update display and send over serial
    doDisplay();

    // reset text block char counter ready for next block
    textcharcount = 0;
  }

  /*
  // NOTE if the calibration information is not enabled at boot up it will go straight to the 0x04 normal screen, not the 0x03 2nd boot screen
  */

  // switch in case calibration boot screen is not enabled and it goes straight to normal display instead
  if (lastincomingByte == 0x0D && incomingByte == 0x03) {  // is starting to build the 2nd boot screen for calibration info, so show the first one and set flag to switch over
    // set flag to show that 1st boot screen has been completed and it should now build the 2nd screen
    booted = 1;

    // BUG TEST
    //Serial.println("got booted 1");
  } else if (lastincomingByte == 0x0D && incomingByte == 0x04) {  // is starting to display normal screen
    // set flag to show it should now build normal screen
    booted = 2;
  }
}






void get2ndBootScreen(){
  /*
  // NOTE if the calibration information is not enabled at boot up it will go straight to the 0x04 normal screen, not the 0x03 2nd boot screen
  */

  // get start of bootup screen text block
  if (incomingByte == 0x03 && textcharcount == 0) {
    textcharcount = 1;  // start tracking char count to detect rows
    rowtext = "";       // clear existing row text before starting

    // fill the screen with black
    //gfx->fillScreen(RGB565_BLACK);

    // ugly hack to force updating of screen as a result of only updating text if changed
    flagslistlastviewlast = "";
    row1viewlast = "";
    row2viewlast = "";

    // holder for display timer, used to work around lack of predictable end char of 2nd boot screen
    displaytimermillis = millis() + 600;  // wait 0.6 second after start of serial data match, it will then blindly try to display it.

    // BUG TEST
    //Serial.println("got 2nd start");
  }

  // capture display text
  if (textcharcount >= 1) {
    if (incomingByte >= 0x20 && incomingByte <= 255) {  // only add printable chars, normal or flashing chars
      rowtext = rowtext + char(incomingByte);
    }

    // detect which row the text should be put on
    if (textcharcount <= 21) {
      row1 = rowtext;
      // reset string ready for next row
      if (textcharcount == 21) {
        rowtext = "";
      }
    } else {
      row2 = rowtext;
    }

    // count how many chars have been printed (includes the 00 and 01 blank char)
    textcharcount = textcharcount + 1;
  }
  // end of textcharcount block
  ////////

  // detect end of text block (actually looking at start of normal display flags "0x0D 0x04"
  if (lastincomingByte == 0x0D && incomingByte == 0x04) {  // is starting to display normal screen
    booted = 2;

    // BUG TEST
    //Serial.println("got 2nd booted 2");

    // added delay to give more time to see 2nd boot screen
    delay(1500);

    // fill the screen with black
    gfx->fillScreen(RGB565_BLACK);

    // handle the display routines
    doDisplayTypeChecks();
  }
}






// work out the display flags
void getFlags(){
  // get display flags, 1st byte
  if (lastincomingByte == 0x06) {
    // clear before use
    flagslist = "";

    // convert to int
    flagsByte1int = (int)incomingByte;

    if (flagsByte1int > 127) {
      flagslist = flagslist + "REL   ";
      flagsByte1int = flagsByte1int - 128;
    }
    if (flagsByte1int > 63) {
      flagslist = flagslist + "REAR   ";
      flagsByte1int = flagsByte1int - 64;
    }
    if (flagsByte1int > 31) {
      flagslist = flagslist + "SRQ   ";
      flagsByte1int = flagsByte1int - 32;
    }
    if (flagsByte1int > 15) {
      flagslist = flagslist + "LSTN   ";
      flagsByte1int = flagsByte1int - 16;
    }
    if (flagsByte1int > 7) {
      flagslist = flagslist + "TALK   ";
      flagsByte1int = flagsByte1int - 8;
    }
    if (flagsByte1int > 3) {
      flagslist = flagslist + "REM   ";
      flagsByte1int = flagsByte1int - 4;
    }
    if (flagsByte1int > 1) {
      flagslist = flagslist + "ERR   ";
      flagsByte1int = flagsByte1int - 2;
    }
    if (flagsByte1int > 0) {
      flagslist = flagslist + "EDIT   ";
      flagsByte1int = flagsByte1int - 1;
    }
  }

  // 2nd byte of display flags
  if (lastlastincomingByte == 0x06) {
    // convert to int
    flagsByte2int = (int)incomingByte;

    if (flagsByte2int > 127) {
      flagslist = flagslist + "SMPL   ";
      flagsByte2int = flagsByte2int - 128;
    }
    if (flagsByte2int > 63) {
      flagslist = flagslist + "*   ";
      flagsByte2int = flagsByte2int - 64;
    }
    if (flagsByte2int > 31) {
      flagslist = flagslist + "TRIG   ";
      flagsByte2int = flagsByte2int - 32;
    }
    if (flagsByte2int > 15) {
      flagslist = flagslist + "ARM   ";
      flagsByte2int = flagsByte2int - 16;
    }
    if (flagsByte2int > 7) {
      flagslist = flagslist + "AUTO   ";
      flagsByte2int = flagsByte2int - 8;
    }
    if (flagsByte2int > 3) {
      flagslist = flagslist + "4W   ";
      flagsByte2int = flagsByte2int - 4;
    }
    if (flagsByte2int > 1) {
      flagslist = flagslist + "MATH   ";
      flagsByte2int = flagsByte2int - 2;
    }
    if (flagsByte2int > 0) {
      flagslist = flagslist + "FILT ";
      flagsByte2int = flagsByte2int - 1;
    }

    // catch and discard next 2 bytes as they are part of 0x07 - ignoring as I am not sure this is actually needed unless I want to do error checking, as it is an XOR of 0x06 (inverted)
    /*
    if(incomingByte == 0x07){
      incomingByte = Serial2.read(); // now get next byte ready for next check as if this section never happened
      delay(3);
      incomingByte = Serial2.read(); // now get next byte ready for next check as if this section never happened
      delay(3);
      incomingByte = Serial2.read(); // now get next byte ready for next check as if this section never happened
      delay(3);
    }
    */

    flagslistlast = flagslist;
  }
}






// capture the normal screen display and handle it
void getDisplayScreen(){
  // get start of text block
  if (incomingByte == 0x04 && textcharcount == 0) {
    textcharcount = 1;  // start tracking char count to detect rows
    rowtext = "";       // clear existing row text before starting
  }

  // only capture text after the 0x04 and 0x00 (or 0x01) have been detected so it ignores the 0x00 and 0x01 bytes, structure is: 0x04 0x00/0x01 then the actual text bytes
  if (lastincomingByte != 0x04) {
    // capture display text
    if (textcharcount >= 1) {
      // check for flashing text
      if (lastincomingByte == 0x0B && incomingByte == 0x01) {  // start of flashing text so insert special char to highlight it
        rowtext = rowtext + "[";                               // (
        flashing = 1;
      } else if (flashing == 1 && lastincomingByte == 0x0B && incomingByte == 0x00) {  // end of flashing text so insert special char to highlight it
        rowtext = rowtext + "]";                                                       // )
        flashing = 0;
      }

      // try to automatically add a space between readout number and suffix ie mV, V, µA etc ONLY if there is not already a space
      // check for a being in row 1, and number being right before the text without a space. - might need something like this instead? incomingByte == 0x12 || incomingByte == 0x10
      if(textcharcount <= 21 && lastincomingByte >= 0x30 && lastincomingByte <= 0x39 && incomingByte >= 0x41){ 
        // add a space before adding the text
        rowtext = rowtext + " ";
      }

      // handle special chars and HEX conversions -
      // NOTE if using a differnt font it MAY need to be modified to place the degrees, mu, and ohm symbols at the correct hex locations,
      // otherwise the main readout OR sub readout may not work correctly due to missing chars, I modified the supplied font to add them were required.
      if (incomingByte == 0x1C) {
        rowtext = rowtext + char(0x11);  // char(0xDC) or can use a <
      }
      else if (incomingByte == 0x1D) {
        rowtext = rowtext + char(0x10);  // char(0xDC) or can use a >
      }
      else if (incomingByte == 0x1B) {
        rowtext = rowtext + char(0x1F);  // char(0x1F) Down Arrow, alternative could be char(0x19)
      }
      else if (incomingByte == 0x1A) {
        rowtext = rowtext + char(0x1E);  // char(0x1E) Up Arrow, alternative could be char(0x18)
      }
      else if (incomingByte == 0x19) {
        rowtext = rowtext + char(0xDC);  // char(0xDC) char(220) Full Lower Half block, or can use # or *
      }
      else if (incomingByte == 0x18) {
        rowtext = rowtext + char(0xDC);  // char(0xDC) char(220) Right Half Lower Half block, or can use # or *
      }
      else if (incomingByte == 0x17) {
        rowtext = rowtext + char(0xDC);  // char(0xDC) char(220) Left Half Lower Half block, or can use # or *
      }
      else if (incomingByte == 0x16) {
        rowtext = rowtext + char(0xDB);  // char(0xDB) char(219) Full block, or can use # or *
      }
      else if (incomingByte == 0x15) {
        rowtext = rowtext + char(0xDE);  // char(0xDE) char(222) Right half block, or can use # or *
      }
      else if (incomingByte == 0x14) {
        rowtext = rowtext + char(0xDD);  // char(0xDD) char(221) Left half block, or can use # or *
      }
      else if (incomingByte == 0x13) {
        rowtext = rowtext + char(0xF8);  // char(0xB0) when using default greek charset 8859-7, USE char(0xF8) char(248) degrees symbol "°"
      }
      else if (incomingByte == 0x12) {
        rowtext = rowtext + char(0xEA);  // char(0xD9) when using default greek charset 8859-7, USE char(0xEA) char(234)? "Ω" symbol
      }
      else if (incomingByte == 0x10) {
        rowtext = rowtext + char(0xE6);  // char(0xCC) when using default greek charset 8859-7, USE char(0xE6) "µ" char(230)
      }
      //else if(incomingByte >= 0x20 && incomingByte <= 255){ // only add printable chars, normal or flashing chars
      else if (incomingByte >= 0x20 && incomingByte <= 127) {  // only add printable chars, normal or flashing chars
        rowtext = rowtext + char(incomingByte);
      }

      // detect which row the text should be put on
      if(textcharcount <= 21) {
        row1 = rowtext;
        // reset string ready for next row
        if (textcharcount == 21) {
          rowtext = "";
        }
      } else {
        row2 = rowtext;
      }

      // if not flaging a flashing text block or some other invisible char, then increment char counter
      if (incomingByte != 0x0B && incomingByte != 0x00 && incomingByte != 0x01) {
        // track how many chars have been printed (NOW EXcludes the B0,  00 and 01 blank char)
        textcharcount = textcharcount + 1;
      }
    }
    // end of textcharcount block
    ////////
  }
  // end of 0x04 0x00 filter
  /////////

  // detect end of text block (actually looking at start of flags "EDIT ERR REM TALK LSTN SRQ REAR REL FILT MATH 4W AUTO ARM TRIG * SMPL" which are 0x06 xx xx)
  // and trigger sending to serial/new display
  if (lastlastincomingByte == 0x0B && lastincomingByte == 0x00 && incomingByte == 0x06) {
    // update display and send over serial
    doDisplay();

    // reset text block char counter ready for next block
    textcharcount = 0;
  }
}






// update the new display and send over serial
void doDisplay(){
  if(lastdisplaywritemillis <= millis()){
    // update serial monitor (can be commented out if serial not wanted)
    //sendSerialMonitor();

    // handle updating of new display
    //flagslistlast
    //row1
    //row2


    line1textsize = 1;  // line 1 text size default
    line2textsize = 1;  // line 2 text size default
    line3textsize = 4;  // line 3 text size default

    /* 4.58" Display Version with Qualia */
    //gfx->fillScreen(RGB565_BLACK); // might need to remove this
    //gfx->fillRect(1, 1, 960, 320, RGB565_BLACK); // try to overwrite old text to stop ghosting, without blanking the entire display

    // fix to make sure old text of blacked out if new text is shorter
    targetLength1 = 32; // can be anything really, depends on font size
    targetLength2 = 24; // should only be 20
    targetLength3 = 32; // should only be 32

    // convert to a dedicated string to avoid the padding for spaces causing issue
    flagslistlastview = flagslistlast;
    row1view = row1;
    row2view = row2;

    // now check for number of spaces, to try and only add more only if it actually needs it !
    for(unsigned int i = 0; i < row1view.length(); i++) {
      targetChar = ' ';
      if(row1view.charAt(i) == targetChar){
        row1charmatchcount++;
      }
    }
    // loop to add spaces for each space found
    for(unsigned int i = 0; i < row1charmatchcount; i++){
      row1view = row1view + "   "; // now add some extra spaces, to match a normal char width
    }
    row1charmatchcount = 0; // reset counter ready for next check


    // Check length and append spaces
    while (flagslistlastview.length() < targetLength1) {
      flagslistlastview += "        "; // due to variable width font used, it needs lots of spaces to replace a normal char
    }
    /*
    // Check length and append spaces - REPLACED BY THE ABOVE, WHICH ONLY ADDS WHAT IT NEEDS
    while (row1view.length() < targetLength2) {
      row1view += " "; // due to variable width font used, it needs lots of spaces to replace a normal char
    }
    */
    // Check length and append spaces
    while (row2view.length() < targetLength3) {
      row2view += "  "; // if this uses a variable width font used, it will need lots more spaces to replace a normal char, currently using a fixed width 5x7 font, so it is OK.
    }


    if (flagslistlastview != "" && flagslistlastviewlast != flagslistlastview) {  // only try to print text if there is something to do
      // set the font
      //gfx->setFont(); // set default built-in font
      gfx->setFont(&FreeSansBold18pt7b);
      gfx->setCursor(10, 60);
      gfx->setTextSize(line1textsize);
      gfx->setTextColor(fontcolourarray[flagscolour], RGB565_BLACK);
      //gfx->fillRect(1, 20, 959, 30, RGB565_BLACK); // try to overwrite old text to stop ghosting, without blanking the entire display
      gfx->println(flagslistlastview);
      delay(5);

      // save string to compare on next look, to only update it if changes
      flagslistlastviewlast = flagslistlastview;
    }
    if (row1view != "" && row1viewlast != row1view) {  // only try to print text if there is something to do
      // set the font
      gfx->setFont(&Roboto40pt8b);
      //gfx->setFont();  // set default built-in font
      gfx->setCursor(10, 170);
      gfx->setTextSize(line2textsize);
      gfx->setTextColor(fontcolourarray[row1colour], RGB565_BLACK);
      //gfx->fillRect(1, 130, 959, 100, RGB565_BLACK); // try to overwrite old text to stop ghosting, without blanking the entire display
      //gfx->drawRect(1, 130, 959, 100, RGB565_BLUE); // make a box outline

      flashingStartCharIndex = row1view.indexOf(searchFlashingStartChar);

      // handle changing of colour to highlight selected text, assumes a bracket is a start/end of "flashing" text
      if (flashingStartCharIndex != -1) {  // check for starting char [
        firsttextsection = row1view.substring(0, flashingStartCharIndex);
        secondtextsection = row1view.substring(flashingStartCharIndex + 1);
        flashingEndCharIndex = secondtextsection.indexOf(searchFlashingEndChar);

        if (firsttextsection != "") {  // handles the start of the text being the flashing text, so nothing is there to print
          gfx->print(firsttextsection);
        }
        gfx->setTextColor(fontcolourarray[highlightcolour], RGB565_BLACK);
        gfx->print(secondtextsection.substring(0, flashingEndCharIndex));
        gfx->setTextColor(fontcolourarray[row2colour], RGB565_BLACK);
        gfx->print(secondtextsection.substring(flashingEndCharIndex + 1));

      } else {
        gfx->println(row1view);
      }
      delay(5);

      // save string to compare on next look, to only update it if changes
      row1viewlast = row1view;
    }
    if (row2view != "" && row2viewlast != row2view) {  // only try to print text if there is something to do
      // set the font
      gfx->setFont();  // set default built-in font
      gfx->setCursor(10, 220);
      gfx->setTextSize(line3textsize);
      gfx->setTextColor(fontcolourarray[row2colour], RGB565_BLACK);
      //gfx->fillRect(1, 265, 959, 40, RGB565_BLACK); // try to overwrite old text to stop ghosting, without blanking the entire display

      flashingStartCharIndex = row2view.indexOf(searchFlashingStartChar);

      // handle changing of colour to highlight selected text, assumes a bracket is a start/end of "flashing" text
      if (flashingStartCharIndex != -1) {  // check for starting char [
        firsttextsection = row2view.substring(0, flashingStartCharIndex);
        secondtextsection = row2view.substring(flashingStartCharIndex + 1);
        flashingEndCharIndex = secondtextsection.indexOf(searchFlashingEndChar);

        if (firsttextsection != "") {  // handles the start of the text being the flashing text, so nothing is there to print
          gfx->print(firsttextsection);
        }
        gfx->setTextColor(fontcolourarray[highlightcolour], RGB565_BLACK);
        gfx->print(secondtextsection.substring(0, flashingEndCharIndex));
        gfx->setTextColor(fontcolourarray[row2colour], RGB565_BLACK);
        gfx->print(secondtextsection.substring(flashingEndCharIndex + 1));

      } else {
        gfx->println(row2view);
      }
      delay(5);

      // save string to compare on next look, to only update it if changes
      row2viewlast = row2view;
    }
    lastdisplaywritemillis = millis() + 40;
  }
}







void loop(){
  // keep track of millis since start up, can be used for various non delay based timing features.
  lastmillis = millis();

  // handle updating of 2nd boot screen, bit of a bodge to work around a problem triggering it
  // after the timer finishes trigger a display update, used to work around lack of predictable end char of 2nd boot screen
  if (booted == 1 && displaytimermillis <= lastmillis && textcharcount >= 1) {

    // fill the screen with black
    //gfx->fillScreen(RGB565_BLACK);
    gfx->fillRect(1, 1, 960, 320, RGB565_BLACK);  // try to overwrite old text to stop ghosting, wihtout blanking the entire display

    // update display and send over serial
    doDisplay();
    // reset text block char counter ready for next block and to prevent this triggering more than once
    textcharcount = 0;
  }

  // fallback in case the boot sequence has trouble, to at least get a normal display after 11 seconds, hopefully not needed but just in case LOL
  if (booted != 2 && lastmillis >= 11000) {
    booted = 2;
  }

  // get serial data one byte at a time and check rather than a serial while loop to try and get more accurate responses
  handleSerial();


  // handle button to test font charset
  if(!expander->digitalRead(PCA_BUTTON_DOWN)){ // input is inverted, so a NOT made is active
    fontCharTest(); // call the function to run the test
    delay(500); // debounce
  }
  // handle button for changing of colour
  if(!expander->digitalRead(PCA_BUTTON_UP)){ // input is inverted, so a NOT made is active
    handleColourChanges();
    delay(500); // debounce
  }

}
