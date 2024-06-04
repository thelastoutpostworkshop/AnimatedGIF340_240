// Your Eyes on Round Display
//

#include <TFT_eSPI.h>    // Install this library with the Arduino IDE Library Manager
                         // Don't forget to configure the driver for the display!
#include <AnimatedGIF.h> // Install this library with the Arduino IDE Library Manager

// Eyes GIF files
#include "gif_files\radar.h"
#include "gif_files\hud_a.h"
#include "gif_files\x_wing.h"
#include "gif_files\death_star.h"
#include "gif_files\star_destroyer.h"
#include "gif_files\star_destroyer_planet.h"

TFT_eSPI tft = TFT_eSPI();

#define DISPLAY_WIDTH tft.width()
#define DISPLAY_HEIGHT tft.height()
#define BUFFER_SIZE 256
uint16_t usTemp[BUFFER_SIZE];

// GIF
#define GIF_Rotation 3      // Adjust Rotation of your screen (0-3)
#define GifData star_destroyer_planet  // Change image to display (image name in gif_files\[image header file].h)

AnimatedGIF gif;

void setup()
{
  Serial.begin(115200);
  tft.init();
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(GIF_Rotation);
  if (!openGif())
  {
    Serial.println("Cannot open GIF");
    printGifErrorMessage(gif.getLastError());
    while (true)
    {
    }
  }
}

void loop()
{
  playGifFrame();
}

void playGifFrame()
{
  tft.startWrite();
  gif.playFrame(true, NULL);
  tft.endWrite();
}

int openGif()
{
  gif.begin(BIG_ENDIAN_PIXELS);
  // eye->pTurboGIFBuffer = (uint8_t *)heap_caps_malloc(TURBO_BUFFER_SIZE + (imageWidth * imageHeight), MALLOC_CAP_8BIT);
  // if (eye->pTurboGIFBuffer == NULL)
  // {
  //   Serial.println("Could not allocate pTurboBuffer");
  //   return 0;
  // }
  // eye->frameGIFBuffer = (uint8_t *)malloc(imageWidth * imageHeight * 2);
  // if (eye->frameGIFBuffer == NULL)
  // {
  //   Serial.println("Could not allocate frameBuffer");
  //   return 0;
  // }
  // eye->gif.setDrawType(GIF_DRAW_COOKED);
  // eye->gif.setTurboBuf(eye->pTurboGIFBuffer);
  // eye->gif.setFrameBuf(eye->frameGIFBuffer);
  return gif.open((uint8_t *)GifData, sizeof(GifData), GIFDraw);
}

// Draw a line from the current frame
void GIFDraw(GIFDRAW *pDraw)
{
  uint8_t *s;
  uint16_t *d, *usPalette;
  int x, y, iWidth, iCount;

  // Displ;ay bounds chech and cropping
  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX > DISPLAY_WIDTH)
    iWidth = DISPLAY_WIDTH - pDraw->iX;
  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y; // current line
  if (y >= DISPLAY_HEIGHT || pDraw->iX >= DISPLAY_WIDTH || iWidth < 1)
    return;

  // Old image disposal
  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) // restore to background color
  {
    for (x = 0; x < iWidth; x++)
    {
      if (s[x] == pDraw->ucTransparent)
        s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  // Apply the new pixels to the main image
  if (pDraw->ucHasTransparency) // if transparency used
  {
    uint8_t *pEnd, c, ucTransparent = pDraw->ucTransparent;
    pEnd = s + iWidth;
    x = 0;
    iCount = 0; // count non-transparent pixels
    while (x < iWidth)
    {
      c = ucTransparent - 1;
      d = &usTemp[0];
      while (c != ucTransparent && s < pEnd && iCount < BUFFER_SIZE)
      {
        c = *s++;
        if (c == ucTransparent) // done, stop
        {
          s--; // back up to treat it like transparent
        }
        else // opaque
        {
          *d++ = usPalette[c];
          iCount++;
        }
      } // while looking for opaque pixels
      if (iCount) // any opaque pixels?
      {
        // DMA would degrtade performance here due to short line segments
        tft.setAddrWindow(pDraw->iX + x, y, iCount, 1);
        tft.pushPixels(usTemp, iCount);
        x += iCount;
        iCount = 0;
      }
      // no, look for a run of transparent pixels
      c = ucTransparent;
      while (c == ucTransparent && s < pEnd)
      {
        c = *s++;
        if (c == ucTransparent)
          x++;
        else
          s--;
      }
    }
  }
  else
  {
    s = pDraw->pPixels;

    // Unroll the first pass to boost DMA performance
    // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
    if (iWidth <= BUFFER_SIZE)
      for (iCount = 0; iCount < iWidth; iCount++)
        usTemp[iCount] = usPalette[*s++];
    else
      for (iCount = 0; iCount < BUFFER_SIZE; iCount++)
        usTemp[iCount] = usPalette[*s++];

    tft.setAddrWindow(pDraw->iX, y, iWidth, 1);
    tft.pushPixels(&usTemp[0], iCount);

    iWidth -= iCount;
    // Loop if pixel buffer smaller than width
    while (iWidth > 0)
    {
      // Translate the 8-bit pixels through the RGB565 palette (already byte reversed)
      if (iWidth <= BUFFER_SIZE)
        for (iCount = 0; iCount < iWidth; iCount++)
          usTemp[iCount] = usPalette[*s++];
      else
        for (iCount = 0; iCount < BUFFER_SIZE; iCount++)
          usTemp[iCount] = usPalette[*s++];

      tft.pushPixels(&usTemp[0], iCount);
      iWidth -= iCount;
    }
  }
}

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