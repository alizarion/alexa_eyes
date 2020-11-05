//***********************************************************************
// Initialization routine
//***********************************************************************
void setup(void)
{
  uint8_t e; // Eye index, 0 to NUM_EYES-1

  Serial.begin(115200);
  Serial.println("Init");
  randomSeed(analogRead(A3)); // Seed random() from floating analog input

  // user customization setup
  pinMode(PHOTOCELL, INPUT);                                  // set up photocell input analog
  State = DISPLAY_OFF;                                        // set display state to 'off'
  darkStart = millis();                                       // get initial timestamp for when LED was turned off
  photocellReading = analogRead(PHOTOCELL);                   // get initial photocell reading
  for(photocellIndex=0; photocellIndex<50; photocellIndex++)
    photocellArray[photocellIndex] = photocellReading;        // set up photocell average array
  photocellSum = photocellReading*50;
  photocellIndex = 0;

  // Initialize eye objects based on eyeInfo list in config.h:
  for(e=0; e<NUM_EYES; e++) 
  {
    Serial.print("Create display #"); Serial.println(e);
    eye[e].display     = new displayType(128, 128, &TFT_SPI,
                           eyeInfo[e].select, DISPLAY_DC, -1);
    eye[e].blink.state = NOBLINK;
    if(eyeInfo[e].select >= 0) 
    {
      pinMode(eyeInfo[e].select, OUTPUT);
      digitalWrite(eyeInfo[e].select, HIGH); // Deselect them all
    }
  }

  // perform manual reset to take care of both displays just once:
  Serial.println("Reset displays");
  pinMode(DISPLAY_RESET, OUTPUT);
  digitalWrite(DISPLAY_RESET, LOW);  delay(1);
  digitalWrite(DISPLAY_RESET, HIGH); delay(50);

  // After all-displays reset, now call init/begin func for each display:
  for(e=0; e<NUM_EYES; e++) 
  {
    eye[e].display->begin(SPI_FREQ);
    Serial.println("Rotate");
    eye[e].display->setRotation(eyeInfo[e].rotation);
  }
  Serial.println("done");

  // One of the displays is configured to mirror on the X axis.  
  const uint8_t rotateOLED[] = { 0x74, 0x77, 0x66, 0x65 },
                mirrorOLED[] = { 0x76, 0x67, 0x64, 0x75 }; // Mirror+rotate

  // If OLED, loop through ALL eyes and set up remap register
  for(e=0; e<NUM_EYES; e++) 
  {
    eye[e].display->sendCommand(SSD1351_CMD_SETREMAP, e ?
      &rotateOLED[eyeInfo[e].rotation & 3] :
      &mirrorOLED[eyeInfo[e].rotation & 3], 1);
  }

  // capture start time for frame-rate calculation
  startTime = millis();
}
