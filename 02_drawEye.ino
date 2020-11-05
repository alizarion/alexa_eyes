//***********************************************************************
// Eye Rendering Function
//***********************************************************************
void drawEye
  ( 
  uint8_t  e,       // Eye array index; 0 or 1 for left/right
  uint16_t iScale,  // Scale factor for iris (0-1023)
  uint8_t  scleraX, // First pixel X offset into sclera image
  uint8_t  scleraY, // First pixel Y offset into sclera image
  uint8_t  uT,      // Upper eyelid threshold value
  uint8_t  lT       // Lower eyelid threshold value
  )
{
  uint8_t  screenX, screenY, scleraXsave;
  int16_t  irisX, irisY;
  uint16_t p, a;
  uint32_t d;
  uint8_t  irisThreshold = (128 * (1023 - iScale) + 512) / 1024;
  uint32_t irisScale     = IRIS_MAP_HEIGHT * 65536 / irisThreshold;

  // Set up raw pixel dump to entire screen.
  TFT_SPI.beginTransaction(settings);
  digitalWrite(eyeInfo[e].select, LOW);                // Chip select
  eye[e].display->writeCommand(SSD1351_CMD_SETROW);    // Y range
  eye[e].display->spiWrite(0); eye[e].display->spiWrite(SCREEN_HEIGHT - 1);
  eye[e].display->writeCommand(SSD1351_CMD_SETCOLUMN); // X range
  eye[e].display->spiWrite(0); eye[e].display->spiWrite(SCREEN_WIDTH  - 1);
  eye[e].display->writeCommand(SSD1351_CMD_WRITERAM);  // Begin write
  digitalWrite(eyeInfo[e].select, LOW);                // Re-chip-select
  digitalWrite(DISPLAY_DC, HIGH);                      // Data mode

  // issue raw 16-bit values for every pixel...
  scleraXsave = scleraX + SCREEN_X_START; // Save initial X value to reset on each line
  irisY       = scleraY - (SCLERA_HEIGHT - IRIS_HEIGHT) / 2;
  for(screenY=SCREEN_Y_START; screenY<SCREEN_Y_END; screenY++, scleraY++, irisY++) 
  {
    scleraX = scleraXsave;
    irisX   = scleraXsave - (SCLERA_WIDTH - IRIS_WIDTH) / 2;
    for(screenX=SCREEN_X_START; screenX<SCREEN_X_END; screenX++, scleraX++, irisX++) 
    {
      if((lower[screenY][screenX]<=lT)||(upper[screenY][screenX]<=uT))          // if covered by eyelid
      {
        p = 0;
      } 
      else if((irisY<0)||(irisY>=IRIS_HEIGHT)||(irisX<0)||(irisX>=IRIS_WIDTH))  // if in sclera
      {
        p = sclera[scleraY][scleraX];
      } 
      else                                                                      // else maybe iris...
      {      
        p = polar[irisY][irisX];                        // Polar angle/dist
        d = p & 0x7F;                                   // Distance from edge (0-127)
        if(d < irisThreshold)                           // if within scaled iris area
        {                    
          d = d * irisScale / 65536;                    // d scaled to iris image height
          a = (IRIS_MAP_WIDTH * (p >> 7)) / 512;        // Angle (X)
          p = iris[d][a];                               // Pixel = iris
        } 
        else                                            // Not in iris
        {
          p = sclera[scleraY][scleraX];                 // Pixel = sclera
        }
      }

      // SPI FIFO technique from Paul Stoffregen's ILI9341_t3 library:
      while(KINETISK_SPI0.SR & 0xC000);                 // Wait for space in FIFO
      KINETISK_SPI0.PUSHR = p | SPI_PUSHR_CTAS(1) | SPI_PUSHR_CONT;
    }                                                   // end column
  }                                                     // end scanline

  KINETISK_SPI0.SR |= SPI_SR_TCF;                       // Clear transfer flag
  while((KINETISK_SPI0.SR & 0xF000) ||                  // Wait for SPI FIFO to drain
       !(KINETISK_SPI0.SR & SPI_SR_TCF));               // Wait for last bit out

  digitalWrite(eyeInfo[e].select, HIGH);                // Deselect
  TFT_SPI.endTransaction();
}
