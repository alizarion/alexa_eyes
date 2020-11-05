//***********************************************************************
// Process motion for a single frame of left or right eye
//***********************************************************************
void frame (uint16_t iScale)    // Iris scale (0-1023) passed in
{     
  static uint32_t frames   = 0; // Used in frame rate calculation
  static uint8_t  eyeIndex = 0; // eye[] array counter
  int16_t         eyeX, eyeY;
  uint32_t        t = micros(); // Time at start of function

  if(!(++frames & 255))  // Every 256 frames...
  {
    uint32_t elapsed = (millis() - startTime) / 1000;
    if(elapsed) Serial.println(frames / elapsed); // Print FPS
  }

  if(++eyeIndex >= NUM_EYES) eyeIndex = 0; // Cycle through eyes, 1 per call

  // Autonomous X/Y eye motion
  // Periodically initiates motion to a new random point, random speed,
  // holds there for random period until next motion.
  static boolean  eyeInMotion      = false;
  static int16_t  eyeOldX=512, eyeOldY=512, eyeNewX=512, eyeNewY=512;
  static uint32_t eyeMoveStartTime = 0L;
  static int32_t  eyeMoveDuration  = 0L;

  int32_t dt = t - eyeMoveStartTime;      // uS elapsed since last eye event
  if(eyeInMotion)                         // Currently moving?
  {
    if(dt >= eyeMoveDuration)             // Time up?  Destination reached.
    {
      eyeInMotion      = false;           // Stop moving
      eyeMoveDuration  = random(3000000); // 0-3 sec stop
      eyeMoveStartTime = t;               // Save initial time of stop
      eyeX = eyeOldX = eyeNewX;           // Save position
      eyeY = eyeOldY = eyeNewY;
    } 
    else                                  // Move time is not yet fully elapsed -- interpolate position 
    { 
      int16_t e = ease[255 * dt / eyeMoveDuration] + 1;   // Ease curve
      eyeX = eyeOldX + (((eyeNewX - eyeOldX) * e) / 256); // Interp X
      eyeY = eyeOldY + (((eyeNewY - eyeOldY) * e) / 256); // and Y
    }
  } 
  else                                                    // Eye stopped
  {
    eyeX = eyeOldX;
    eyeY = eyeOldY;
    if(dt > eyeMoveDuration)                              // Time up?  Begin new move.
    {
      int16_t  dx, dy;
      uint32_t d;
      do                                                  // Pick new dest in circle
      {
        eyeNewX = random(1024);
        eyeNewY = random(1024);
        dx      = (eyeNewX * 2) - 1023;
        dy      = (eyeNewY * 2) - 1023;
      } 
      while((d = (dx * dx + dy * dy)) > (1023 * 1023));   // Keep trying
      eyeMoveDuration  = random(72000, 144000);           // ~1/14 - ~1/7 sec
      eyeMoveStartTime = t;                               // Save initial time of move
      eyeInMotion      = true;                            // Start move on next frame
    }
  }

  // Blinking
  // Similar to the autonomous eye movement above -- blink start times
  // and durations are random (within ranges).
  if((t - timeOfLastBlink) >= timeToNextBlink)            // Start new blink?
  {
    timeOfLastBlink = t;
    uint32_t blinkDuration = random(36000, 72000);        // ~1/28 - ~1/14 sec
    for(uint8_t e=0; e<NUM_EYES; e++)                     // Set up durations for both eyes
    {
      if(eye[e].blink.state == NOBLINK) 
      {
        eye[e].blink.state     = ENBLINK;
        eye[e].blink.startTime = t;
        eye[e].blink.duration  = blinkDuration;
      }
    }
    timeToNextBlink = blinkDuration * 3 + random(4000000);
  }

  if(eye[eyeIndex].blink.state)                         // Eye currently blinking?
  {
    // Check if current blink state time has elapsed
    if((t - eye[eyeIndex].blink.startTime) >= eye[eyeIndex].blink.duration) 
    {
      // increment blink state, unless Enblinking and display is being commanded to blank
      if((eye[eyeIndex].blink.state == ENBLINK) && (blankDisplay == 1))
      {
        // Don't advance state -- eye is held closed instead (used for display blanking)
      } 
      else 
      {
        if(++eye[eyeIndex].blink.state > DEBLINK)       // Deblinking finished?
        {
          eye[eyeIndex].blink.state = NOBLINK;          // No longer blinking
        } 
        else                                            // Advancing from ENBLINK to DEBLINK mode
        {
          eye[eyeIndex].blink.duration *= 2;            // DEBLINK is 1/2 ENBLINK speed
          eye[eyeIndex].blink.startTime = t;
        }
      }
    }
  } 
  else                                                  // Not currently blinking
  {
    if(blankDisplay == 1)                               // check if display is being commanded to blank
    {
      // Manually-initiated blinks have random durations like auto-blink
      uint32_t blinkDuration = random(36000, 72000);
      for(uint8_t e=0; e<NUM_EYES; e++) 
      {
        if(eye[e].blink.state == NOBLINK) 
        {
          eye[e].blink.state     = ENBLINK;
          eye[e].blink.startTime = t;
          eye[e].blink.duration  = blinkDuration;
        }
      }
    } 
  }

  // Process motion, blinking and iris scale into renderable values
  // Scale eye X/Y positions (0-1023) to pixel units used by drawEye()
  eyeX = map(eyeX, 0, 1023, 0, SCLERA_WIDTH  - 128);
  eyeY = map(eyeY, 0, 1023, 0, SCLERA_HEIGHT - 128);
  if(eyeIndex == 1) eyeX = (SCLERA_WIDTH - 128) - eyeX;   // Mirrored display

  // Horizontal position is offset so that eyes are very slightly crossed
  // to appear fixated (converged) at a conversational distance.  
  if(NUM_EYES > 1) eyeX += 4;
  if(eyeX > (SCLERA_WIDTH - 128)) eyeX = (SCLERA_WIDTH - 128);

  // Eyelids are rendered using a brightness threshold image.  This same
  // map can be used to simplify another problem: making the upper eyelid
  // track the pupil (eyes tend to open only as much as needed -- e.g. look
  // down and the upper eyelid drops).  Just sample a point in the upper
  // lid map slightly above the pupil to determine the rendering threshold.
  static uint8_t uThreshold = 128;
  uint8_t        lThreshold, n;

  //eyelid tracking
  int16_t sampleX = SCLERA_WIDTH  / 2 - (eyeX / 2),     // Reduce X influence
          sampleY = SCLERA_HEIGHT / 2 - (eyeY + IRIS_HEIGHT / 4);
  if(sampleY < 0) n = 0;    // Eyelid is slightly asymmetrical, so two readings are averaged
  else            n = (upper[sampleY][sampleX] +
                       upper[sampleY][SCREEN_WIDTH - 1 - sampleX]) / 2;
  uThreshold = (uThreshold * 3 + n) / 4; // Filter/soften motion
  // Lower eyelid doesn't track the same way, but seems to be pulled upward
  // by tension from the upper lid.
  lThreshold = 254 - uThreshold;

  // The upper/lower thresholds are then scaled relative to the current
  // blink position so that blinks work together with pupil tracking.
  if(eye[eyeIndex].blink.state)  // Eye currently blinking?
  {
    uint32_t s = (t - eye[eyeIndex].blink.startTime);
    if(s >= eye[eyeIndex].blink.duration) s = 255;   // At or past blink end
    else s = 255 * s / eye[eyeIndex].blink.duration; // Mid-blink
    s          = (eye[eyeIndex].blink.state == DEBLINK) ? 1 + s : 256 - s;
    n          = (uThreshold * s + 254 * (257 - s)) / 256;
    lThreshold = (lThreshold * s + 254 * (257 - s)) / 256;
  } 
  else 
  {
    n          = uThreshold;
  }

  // Pass all the derived values to the eye-rendering function:
  drawEye(eyeIndex, iScale, eyeX, eyeY, n, lThreshold);

  // check photocell once all eyes are rendered
  if(eyeIndex == (NUM_EYES - 1)) 
  {
    photocell();
  }
}
