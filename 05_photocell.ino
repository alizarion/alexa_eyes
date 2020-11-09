//***********************************************************************
// Process photocell to drive system states
//***********************************************************************
void photocell()
{
  // get photocell reading and average value using a 50 sample array
  photocellReading = analogRead(PHOTOCELL);                     // get new photocell reading
  photocellSum = photocellSum - photocellArray[photocellIndex]; // subtract oldest value from total
  photocellArray[photocellIndex] = photocellReading;            // insert new value into array
  photocellSum = photocellSum + photocellReading;               // add newest value into total
  photocellIndex = photocellIndex + 1;                          // increment index
  if(photocellIndex > 49) photocellIndex = 0;                   // constrain index
  photocellAverage = photocellSum / 50;                         // compute average over the 50 samples
  photoDiff = photocellReading - photocellAverage;

  // if LED is on, set the flag, else clear it
  if (abs(photoDiff) > LIGHT_THRESHOLD)
  {
    LEDon = true;
  }
  else
  {
    LEDon = false;
  }

  // if display is currently off
  if (State == DISPLAY_OFF)
  {
    blankDisplay = 1;                                         // turn off the eyes
    offTime = (millis() - darkStart) / 1000;                  // compute seconds since LED was turned off
    if ((LEDon == true) && (offTime < LED_OFF_TIMEOUT))       // if LED is on, but it was off for only a short time
    {
      State = DISPLAY_OFF_LED_FLASHING;                       // then the LED is flashing
    }
    else if ((LEDon == true) && (offTime >= LED_OFF_TIMEOUT)) // if LED is on, and it was off for a long time
    {
      State = DISPLAY_ON;                                     // set display state to 'on'
      lightStart = millis();                                  // get timestamp for when LED was turned on
    }
  }

  // if display is currently on
  else if (State == DISPLAY_ON)
  {
    blankDisplay = 0;                                         // turn on the eyes
    onTime = (millis() - lightStart) / 1000;                  // compute seconds since LED was turned on
    if (onTime > DISPLAY_TIMEOUT)                             // if display has been on more than timeout seconds
    {
      State = DISPLAY_OFF_LED_ON;                             // set display state to off even though the LED is still on
    }
    if (LEDon == false)                                       // if LED turns off
    {
      State = WAITING_TO_TURN_OFF_DISPLAY;                    // set display state to waiting for timer to expire
      darkStart = millis();                                   // get timestamp for when LED was turned off
    }
  }

  // if display is currently waiting for timer to expire
  else if (State == WAITING_TO_TURN_OFF_DISPLAY)
  {
    blankDisplay = 0;                                         // turn on the eyes
    offTime = (millis() - darkStart) / 1000;                  // compute seconds since LED was turned off
    if ((LEDon == true) && (offTime < LED_OFF_TIMEOUT))       // if LED is on, but it was off for only a short time
    {
      State = DISPLAY_ON_LED_FLASHING;                        // then the LED is flashing
    }
    else if ((LEDon == true) && (offTime >= LED_OFF_TIMEOUT)) // if LED is on, and it was off for a long time
    {
      State = DISPLAY_ON;                                     // set display state to 'on'
      lightStart = millis();                                  // get timestamp for when LED was turned on
    }
    onTime = (millis() - lightStart) / 1000;                  // compute seconds since LED was turned on
    if (onTime > DISPLAY_TIMEOUT)                             // if display has been on more than timeout seconds
    {
      State = DISPLAY_OFF;                                    // set display state to 'off'
      darkStart = millis();                                   // get timestamp for when LED was turned off
    }
  }

  // if display is currently off because the LED was on a long time
  else if (State == DISPLAY_OFF_LED_ON)
  {
    blankDisplay = 1;                                         // turn off the eyes
    if (LEDon == false)                                       // if the LED turns off
    {
      State = DISPLAY_OFF;                                    // set display state to 'off'
      darkStart = millis();                                   // get timestamp for when LED was turned off
    }
  }

  // if display is on but the LED is flashing
  else if (State == DISPLAY_ON_LED_FLASHING)
  {
    blankDisplay = 0;                                         // turn on the eyes
    onTime = (millis() - lightStart) / 1000;                  // compute seconds since LED was turned on
    if (onTime > DISPLAY_TIMEOUT)                             // if display has been on more than timeout seconds
    {
      State = DISPLAY_OFF;                                    // set display state to 'off'
      darkStart = millis();                                   // get timestamp for when LED was turned off
    }
    if (LEDon == false)                                       // if LED turns off
    {
      State = WAITING_TO_TURN_OFF_DISPLAY;                    // set display state to waiting for timer to expire
      darkStart = millis();                                   // get timestamp for when LED was turned off
    }
  }

  // if display is off but the LED is flashing
  else if (State == DISPLAY_OFF_LED_FLASHING)
  {
    blankDisplay = 1;                                         // turn off the eyes
    if (LEDon == false)                                       // if LED turns off
    {
      State = DISPLAY_OFF;                                    // set display state to 'off'
      darkStart = millis();                                   // get timestamp for when LED was turned off
    }
  }

  //debug print statements
  // Serial.print(photocellReading);
  // Serial.print(" ");
  // Serial.print(photocellAverage);
  // Serial.print(" ");
  Serial.print(abs(photoDiff));
  Serial.println(" ");
  
  // Serial.print(photocellAverage);
  // Serial.print(onTime);
  //Serial.print(" ");
  // Serial.print(offTime);
  // Serial.print(" ");
  // Serial.print(LEDon);
  // Serial.print(" ");
  // Serial.print(blankDisplay);
  // Serial.print(" ");
  // Serial.println(State);
}
