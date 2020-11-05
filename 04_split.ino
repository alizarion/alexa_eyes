//***********************************************************************
// Perform Autonomous Iris Scaling
//***********************************************************************
void split             // Subdivides motion path into two sub-paths w/randomization
  (
  int16_t  startValue, // Iris scale value (IRIS_MIN to IRIS_MAX) at start
  int16_t  endValue,   // Iris scale value at end
  uint32_t startTime,  // micros() at start
  int32_t  duration,   // Start-to-end time, in microseconds
  int16_t  range       // Allowable scale value variance when subdividing
  )
{
  if(range >= 8)       // Limit subdvision count, because recursion
  {
    range    /= 2;     // Split range & time in half for subdivision,
    duration /= 2;     // then pick random center point within range:
    int16_t  midValue = (startValue + endValue - range) / 2 + random(range);
    uint32_t midTime  = startTime + duration;
    split(startValue, midValue, startTime, duration, range);  // First half
    split(midValue  , endValue, midTime  , duration, range);  // Second half
  } 
  else                 // No more subdivisons, do iris motion...
  {
    int32_t dt;        // Time (micros) since start of motion
    int16_t v;         // Interim value
    while((dt = (micros() - startTime)) < duration) 
    {
      v = startValue + (((endValue - startValue) * dt) / duration);
      if(v < IRIS_MIN)      v = IRIS_MIN;   // Clip just in case
      else if(v > IRIS_MAX) v = IRIS_MAX;
      frame(v);        // Draw frame w/interim iris scale value
    }
  }
}
