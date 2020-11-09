//***********************************************************************
// Uncanny eyes for Adafruit 1.5" OLED
//   modified by markwtech to simplify/eliminate all the unused
//   options that were present in the original code except
//   things necessary for Teensy 3.2 and SSD1351 OLED display
//***********************************************************************

//***********************************************************************
// Library Includes
//***********************************************************************
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>  // OLED display library


//***********************************************************************
// Graphics Includes - determines appearance of the eyes
//   enable only ONE of these - these are huge graphics tables
//***********************************************************************
#include "graphics/defaultEye.h"      // Standard human-ish hazel eye -OR-
//#include "graphics/dragonEye.h"     // Slit pupil fiery dragon/demon eye -OR-
//#include "graphics/noScleraEye.h"   // Large iris, no sclera -OR-
//#include "graphics/goatEye.h"       // Horizontal pupil goat/Krampus eye -OR-
//#include "graphics/newtEye.h"       // Eye of newt -OR-
//#include "graphics/terminatorEye.h" // Git to da choppah!
//#include "graphics/catEye.h"        // Cartoonish cat (flat "2D" colors)


//***********************************************************************
// Constants
//***********************************************************************
#define PHOTOCELL       A1            // pin for photocell input

#define TFT_SPI        SPI            // display hardware SPI setting

#define DISPLAY_DC       7            // Data/command pin for ALL displays
#define DISPLAY_RESET    8            // Reset pin for ALL displays
#define SPI_FREQ 24000000             // OLED: 24 MHz on 72 MHz Teensy

#define IRIS_MIN      120             // minimum iris size (0-1023)
#define IRIS_MAX      720             // maximum iris size (0-1023)

#define SCREEN_X_START 0              // screen x axis start
#define SCREEN_X_END   SCREEN_WIDTH   // width is defined in the graphics h file
#define SCREEN_Y_START 0              // screen y axis start
#define SCREEN_Y_END   SCREEN_HEIGHT  // height is defined in the graphics h file

#define NOBLINK 0                     // Not currently engaged in a blink
#define ENBLINK 1                     // Eyelid is currently closing
#define DEBLINK 2                     // Eyelid is currently opening

#define DISPLAY_OFF 0                 // State values
#define DISPLAY_ON  1
#define WAITING_TO_TURN_OFF_DISPLAY 2
#define DISPLAY_OFF_LED_ON 3
#define DISPLAY_OFF_LED_FLASHING 4
#define DISPLAY_ON_LED_FLASHING 5

#define DISPLAY_TIMEOUT 15            // display and LED timeout values in seconds
#define LED_OFF_TIMEOUT 3

#define LIGHT_THRESHOLD 30           // Alexa photocell light detection threshold

const uint8_t ease[] =                // eye animation movement curve
{
    0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  2,  2,  2,  3,   // T
    3,  3,  4,  4,  4,  5,  5,  6,  6,  7,  7,  8,  9,  9, 10, 10,   // h
   11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 18, 19, 20, 21, 22, 23,   // x
   24, 25, 26, 27, 27, 28, 29, 30, 31, 33, 34, 35, 36, 37, 38, 39,   // 2
   40, 41, 42, 44, 45, 46, 47, 48, 50, 51, 52, 53, 54, 56, 57, 58,   // A
   60, 61, 62, 63, 65, 66, 67, 69, 70, 72, 73, 74, 76, 77, 78, 80,   // l
   81, 83, 84, 85, 87, 88, 90, 91, 93, 94, 96, 97, 98,100,101,103,   // e
  104,106,107,109,110,112,113,115,116,118,119,121,122,124,125,127,   // c
  128,130,131,133,134,136,137,139,140,142,143,145,146,148,149,151,   // J
  152,154,155,157,158,159,161,162,164,165,167,168,170,171,172,174,   // a
  175,177,178,179,181,182,183,185,186,188,189,190,192,193,194,195,   // c
  197,198,199,201,202,203,204,205,207,208,209,210,211,213,214,215,   // o
  216,217,218,219,220,221,222,224,225,226,227,228,228,229,230,231,   // b
  232,233,234,235,236,237,237,238,239,240,240,241,242,243,243,244,   // s
  245,245,246,246,247,248,248,249,249,250,250,251,251,251,252,252,   // o
  252,253,253,253,254,254,254,254,254,255,255,255,255,255,255,255    // n
};


//***********************************************************************
// Global variables
//***********************************************************************
uint32_t startTime;                           // For FPS indicator

uint32_t lightStart;                          // for decoding the Alexa LED states
uint32_t darkStart;
uint32_t onTime;
uint32_t offTime;
uint8_t  State;
uint8_t  LEDon;
uint8_t  blankDisplay;

uint32_t photocellReading;                    // photocell variables
uint32_t photocellIndex;
uint32_t photocellArray[5/0];
uint32_t photocellSum;
uint32_t photocellAverage;
int photoDiff;

uint32_t timeOfLastBlink = 0L;                // blink variables
uint32_t timeToNextBlink = 0L;

uint16_t oldIris = (IRIS_MIN + IRIS_MAX)/2;   // Autonomous iris scaling variables
uint16_t newIris;


//***********************************************************************
// Structures and object declarations
//***********************************************************************
typedef struct          // Struct must be defined before including config.h
{
  int8_t  select;       // pin numbers for each eye's screen select line
  uint8_t rotation;     // also display rotation.
} eyeInfo_t;

eyeInfo_t eyeInfo[] =
{
  { 9, 4 }, // LEFT EYE display-select pin, rotation
  {  10, 4 }, // RIGHT EYE display-select pin, rotation
};

typedef Adafruit_SSD1351 displayType; // Using OLED display(s)

typedef struct
{
  uint8_t  state;       // NOBLINK/ENBLINK/DEBLINK
  uint32_t duration;    // Duration of blink state (micros)
  uint32_t startTime;   // Time (micros) of last state change
} eyeBlink;

#define NUM_EYES (sizeof eyeInfo / sizeof eyeInfo[0])

struct
{                       // One-per-eye structure
  displayType *display; // -> OLED/TFT object
  eyeBlink     blink;   // Current blink/wink state
} eye[NUM_EYES];

SPISettings settings(SPI_FREQ, MSBFIRST, SPI_MODE0);  // Configure SPI port


//***********************************************************************
// Main Loop
//***********************************************************************
void loop()
{
//  if (digitalRead(DISPLAY_BLANKING) == 0)
//  {
//    digitalWrite(9, HIGH);  // Deselect eye
//    digitalWrite(10, HIGH); // Deselect eye
//  }
//  else
  {
    newIris = random(IRIS_MIN, IRIS_MAX);
    split(oldIris, newIris, micros(), 10000000L, IRIS_MAX - IRIS_MIN);
    oldIris = newIris;
  }
}
