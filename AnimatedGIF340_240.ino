// Master Animated GIF on a 320x240 SPI LCD Screen
// Youtube Tutorial:
// Tested with Espressif ESP32 Arduino Core v3.0.2
// Using ESP32-S3 with 8MB of PSRAM
//
#include "esp_flash.h"
#include "esp_partition.h"

#include <bb_spi_lcd.h>  // Install this library with the Arduino IDE Library Manager
                         // Tested on version 2.5.4
#include <AnimatedGIF.h> // Install this library with the Arduino IDE Library Manager
                         // Tested on version 2.1.1

// GIF files
#include "gif_files\hud_a.h"                 //GIF size in FLASH memory is 1.7MB
#include "gif_files\x_wing.h"                //GIF size in FLASH memory is 0.9MB
#include "gif_files\death_star.h"            //GIF size in FLASH memory is 1.7MB
#include "gif_files\star_destroyer.h"        //GIF size in FLASH memory is 1MB
#include "gif_files\star_destroyer_planet.h" //GIF size in FLASH memory is 2.3MB
#include "gif_files\cat.h"                   //GIF size in FLASH memory is 1.1MB
#include "gif_files\star_trek_hud.h"         //GIF size in FLASH memory is 1.6MB
#include "gif_files\jedi_battle.h"           //GIF size in FLASH memory is 3.3MB (use partitions.csv for this one, if you your ESP32 board has 4MB Flash size)

BB_SPI_LCD tft; // Main object for the display driver

// GIF to display
#define GifData x_wing // Change image to display (image name in gif_files\[image header file].h)

void setup()
{
  Serial.begin(115200);
  tft.begin(LCD_ILI9341, FLAGS_NONE, 40000000, 8, 3, 9, -1, -1, 17, 18); //
  tft.setRotation(LCD_ORIENTATION_270);                                  // Make sure you have the right orientation based on your GIF
                                                                         // or the GIF will show incorrectly
                                                                         // Values : LCD_ORIENTATION_0, LCD_ORIENTATION_90, LCD_ORIENTATION_180 or LCD_ORIENTATION_270
  tft.fillScreen(TFT_BLACK);

  printFlashInfo();

  AnimatedGIF *gif;
  gif = openGif((uint8_t *)GifData, sizeof(GifData));
  if (gif == NULL)
  {
    Serial.println("Cannot open GIF");
    while (true)
    {
      //  no need to continue
    }
  }

  while (true)
  {
    gif->playFrame(false, NULL);
  }
}

void loop()
{
  delay(1);
}

// Open Gif and allocate memory
AnimatedGIF *openGif(uint8_t *gifdata, size_t gifsize)
{
  AnimatedGIF *gif;
  gif = (AnimatedGIF *)malloc(sizeof(AnimatedGIF));
  if (gif == NULL)
  {
    Serial.println("Not RAM Enough memory for GIF structure");
    return NULL;
  }

  gif->begin(GIF_PALETTE_RGB565_BE); // Set the cooked output type we want (compatible with SPI LCDs)

  if (gif->open(gifdata, gifsize, GIFDraw))
  {
    Serial.printf("Successfully opened GIF; Canvas size = %d x %d\n", gif->getCanvasWidth(), gif->getCanvasHeight());
    Serial.printf("GIF memory size is %ld (%2.2f MB)", gifsize, (float)gifsize / (1024 * 1024));
    gif->setDrawType(GIF_DRAW_COOKED); // We want the Animated GIF library to generate ready-made pixels
    if (gif->allocFrameBuf(GIFAlloc) != GIF_SUCCESS)
    {
      Serial.println("Not Enough RAM memory for frame buffer");
      return NULL;
    }
    return gif;
  }
  else
  {
    printGifErrorMessage(gif->getLastError());
    return NULL;
  }
}

//
// The memory management functions are needed to keep operating system
// dependencies out of the core library code
//
// memory allocation callback function
void *GIFAlloc(uint32_t u32Size)
{
  return malloc(u32Size);
} /* GIFAlloc() */
// memory free callback function
void GIFFree(void *p)
{
  free(p);
}

// Draw callback from the AnimatedGIF decoder
void GIFDraw(GIFDRAW *pDraw)
{
  if (pDraw->y == 0)
  { // set the memory window (once per frame) when the first line is rendered
    tft.setAddrWindow(pDraw->iX, pDraw->iY, pDraw->iWidth, pDraw->iHeight);
  }
  // For all other lines, just push the pixels to the display. We requested 'COOKED'big-endian RGB565 and
  tft.pushPixels((uint16_t *)pDraw->pPixels, pDraw->iWidth);
}

// Get human-readable error related to GIF
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

// Get information about the flash chip
void printFlashInfo(void)
{
  esp_flash_t *flash = esp_flash_default_chip;
  uint32_t flash_size;
  if (esp_flash_get_size(flash, &flash_size) != ESP_OK)
  {
    Serial.println("Failed to get flash size");
    return;
  }
  Serial.printf("\nTotal flash size: %u bytes (%2.2f MB)\n", flash_size, (float)(flash_size / (1024 * 1024)));

  // Calculate used flash memory
  uint32_t used_flash = 0;
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (it != NULL)
  {
    do
    {
      const esp_partition_t *partition = esp_partition_get(it);
      used_flash += partition->size;
      // Serial.printf("Partition: %s, Type: 0x%02X, Subtype: 0x%02X, Address: 0x%08X, Size: %u bytes\n",
      //               partition->label, partition->type, partition->subtype, partition->address, partition->size);
      it = esp_partition_next(it);
    } while (it != NULL);
    esp_partition_iterator_release(it);
  }

  Serial.printf("Used flash size: %u bytes (%2.2f MB)\n", used_flash, (float)used_flash / (1024 * 1024));
  Serial.printf("Free flash size: %u bytes (%2.2f MB)\n", flash_size - used_flash, (float)(flash_size - used_flash) / (1024 * 1024));
}