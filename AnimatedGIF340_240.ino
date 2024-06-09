// Master Animated GIF
//

#include <bb_spi_lcd.h>  // Install this library with the Arduino IDE Library Manager
                         // Tested on version...
#include <AnimatedGIF.h> // Install this library with the Arduino IDE Library Manager

#define USE_TURBO_MODE // Comment this line if you have insufficient memory at run time or if the ESP32 crashes when playing the GIF

// Eyes GIF files
#include "gif_files\radar.h"
#include "gif_files\hud_a.h"
#include "gif_files\x_wing.h"
#include "gif_files\death_star.h"
#include "gif_files\star_destroyer.h"
#include "gif_files\star_destroyer_planet.h"

uint8_t *pTurboBuffer, *frameBuffer;

BB_SPI_LCD tft;
AnimatedGIF gif;

// GIF
#define GIF_Rotation 3 // Adjust Rotation of your screen (0-3)
#define GifData hud_a  // Change image to display (image name in gif_files\[image header file].h)

void setup()
{
  Serial.begin(115200);
  tft.begin(LCD_ILI9341, FLAGS_NONE, 40000000, 8, 3, 9, -1, -1, 17, 18);
  if (!allocateBufferForGif(tft.width(), tft.height()))
  {
    Serial.println("Not Enough memory!");
    while (true)
      ;
  }
  tft.setRotation(LCD_ORIENTATION_270); // Make sure you have the right orientation based on your GIF
  tft.fillScreen(TFT_BLACK);

  if (!openGif())
  {
    Serial.println("Cannot open GIF");
    printGifErrorMessage(gif.getLastError());
    while (true)
    {
      //  no need to continue
    }
  }
}

void loop()
{
  playGifFrame();
}

void playGifFrame()
{
  gif.playFrame(false, NULL);
}

int openGif()
{
  gif.begin(GIF_PALETTE_RGB565_BE); // Set the cooked output type we want (compatible with SPI LCDs)

  int openGif;
  openGif = gif.open((uint8_t *)GifData, sizeof(GifData), GIFDraw);
  if (openGif)
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif.getCanvasWidth(), gif.getCanvasHeight());
    gif.setDrawType(GIF_DRAW_COOKED); // We want the library to generate ready-made pixels
    gif.setFrameBuf(frameBuffer);
#ifdef USE_TURBO_MODE
    gif.setTurboBuf(pTurboBuffer); // Turbo Mode
#endif
  }
  return openGif;
}

bool allocateBufferForGif(int width, int height)
{
  int frameBufferSize = width * (height + 3);
  frameBuffer = (uint8_t *)malloc(frameBufferSize);
  if (frameBuffer == NULL)
  {
    Serial.println("Not enough memory to allocate the frame buffer");
    return false;
  }

  int iTurboSize = TURBO_BUFFER_SIZE + (width * height);
  pTurboBuffer = (uint8_t *)malloc(iTurboSize);
  if (pTurboBuffer == NULL)
  {
    Serial.println("Not enough memory to allocate the turbo buffer");
    return false;
  }
  return true;
}

//
// Draw callback from the AnimatedGIF decoder
//
// Called once for each line of the current frame
// MCUs with minimal RAM would have to process "RAW" pixels into "COOKED" here.
// "Cooking" involves testing for disposal methods, merging non-transparent pixels
// and translating the raw pixels through the palette to generate the final output.
// The code for MCUs with enough RAM is much simpler because the AnimatedGIF library can
// generate "cooked" pixels that are ready to send to the display as-is.
//
void GIFDraw(GIFDRAW *pDraw)
{
  if (pDraw->y == 0)
  { // set the memory window (once per frame) when the first line is rendered
    tft.setAddrWindow(pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight);
  }
  // For all other lines, just push the pixels to the display. We requested 'COOKED'big-endian RGB565 and
  // the library provides them here. No need to do anything except push them right to the display
  tft.pushPixels((uint16_t *)pDraw->pPixels, pDraw->iWidth);
} /* GIFDraw() */

void printGifErrorMessage(int errorCode)
{
  switch (errorCode)
  {
  case GIF_DECODE_ERROR:
    Serial.println("GIF Decoding Error");
    break;
  case GIF_TOO_WIDE:
    Serial.println("GIF Too Wide");
    break;
  case GIF_INVALID_PARAMETER:
    Serial.println("Invalid Parameter for gif open");
    break;
  case GIF_UNSUPPORTED_FEATURE:
    Serial.println("Unsupported feature in GIF");
    break;
  case GIF_FILE_NOT_OPEN:
    Serial.println("GIF File not open");
    break;
  case GIF_EARLY_EOF:
    Serial.println("GIF early end of file");
    break;
  case GIF_EMPTY_FRAME:
    Serial.println("GIF with empty frame");
    break;
  case GIF_BAD_FILE:
    Serial.println("GIF bad file");
    break;
  case GIF_ERROR_MEMORY:
    Serial.println("GIF memory Error");
    break;
  default:
    Serial.println("Unknown Error");
    break;
  }
}