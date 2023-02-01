#include <iostream>
using namespace std;

#include "libraries/inky_frame/inky_frame.hpp"
using namespace pimoroni;

#include "JPEGDEC.h"

InkyFrame inky;

JPEGDEC jpeg;
struct {
  int x, y, w, h;
} jpeg_decode_options;

void *jpegdec_open_callback(const char *filename, int32_t *size) {
    FIL *fil = new FIL;
    if (f_open(fil, filename, FA_READ)) { return nullptr; }
    *size = f_size(fil);

    cout << "opened file; size: " << *size << endl;

    return (void *)fil;
}

void jpegdec_close_callback(void *handle) {
  f_close((FIL *)handle);
  delete (FIL *)handle;
}

int32_t jpegdec_read_callback(JPEGFILE *jpeg, uint8_t *p, int32_t c) {
    uint br;
    f_read((FIL *)jpeg->fHandle, p, c, &br);

    cout << "Read " << br << " bytes" << endl;

    return br;
}

int32_t jpegdec_seek_callback(JPEGFILE *jpeg, int32_t p) {

    cout << "Seek to " << p << endl;

    return f_lseek((FIL *)jpeg->fHandle, p) == FR_OK ? 1 : 0;
}

int jpegdec_draw_callback(JPEGDRAW *draw) {
  uint16_t *p = draw->pPixels;

  int xo = jpeg_decode_options.x;
  int yo = jpeg_decode_options.y;

  for(int y = 0; y < draw->iHeight; y++) {
    for(int x = 0; x < draw->iWidth; x++) {
      int sx = ((draw->x + x + xo) * jpeg_decode_options.w) / jpeg.getWidth();
      int sy = ((draw->y + y + yo) * jpeg_decode_options.h) / jpeg.getHeight();

      if(xo + sx > 0 && xo + sx < inky.bounds.w && yo + sy > 0 && yo + sy < inky.bounds.h) {
        inky.set_pixel_dither({xo + sx, yo + sy}, RGB(RGB565(*p)));
      }

      p++;
    }
  }

  return 1; // continue drawing
}

void draw_jpeg(std::string filename, int x, int y, int w, int h) {

  // TODO: this is a horrible way to do it but we need to pass some parameters
  // into the jpegdec_draw_callback() method somehow and the library isn't
  // setup to allow any sort of user data to be passed around - yuck
  jpeg_decode_options.x = x;
  jpeg_decode_options.y = y;
  jpeg_decode_options.w = w;
  jpeg_decode_options.h = h;

  jpeg.open(
    filename.c_str(),
    jpegdec_open_callback,
    jpegdec_close_callback,
    jpegdec_read_callback,
    jpegdec_seek_callback,
    jpegdec_draw_callback);

    jpeg_decode_options.w = jpeg.getWidth();
    jpeg_decode_options.h = jpeg.getHeight();

  jpeg.setPixelType(RGB565_BIG_ENDIAN);

  cout << "- starting jpeg decode.." << endl;
  int start = millis();
  jpeg.decode(0, 0, 0);
  cout << "done in " << int(millis() - start) << " ms!" << endl;

  jpeg.close();
}

int main() {
    // TODOs
    // - Do something with the LEDs
    // - Use wifi to download pics
    // - Figure out how not to stretch the JPEG if it's not precisely 600x448
    inky.init();

    stdio_init_all();
    sleep_ms(1000);

    for (int i = 0; i < 100; i++) {
        cout << i << endl;
        sleep_ms(10);
    }

    cout << "initialising inky frame.. " << endl;
    cout << "done!\n" << endl;

    cout << "Mounting SD card filesystem.. " << endl;
    FATFS fs;
    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        cout << "Failed to mount SD card filesystem, error: " << fr << endl;
        return 0;
    }
    cout << "Filesystem mounted!" << endl;

    cout << "Listing sd card contents.." << endl;
    FILINFO file;
    auto dir = new DIR();
    f_opendir(dir, "/");
    while(f_readdir(dir, &file) == FR_OK && file.fname[0]) {
        cout << file.fname << " " << file.fsize << endl;
    }
    f_closedir(dir);
    cout << "Listing done!" << endl;


    // inky.led(InkyFrame::LED_ACTIVITY, 0);

    // inky.led(InkyFrame::LED_CONNECTION, 0);

    cout << "Drawing JPEG..." << endl;
    inky.clear();
    draw_jpeg("IMG_20160529_181423035.jpg_scaled.jpg", 0, 0, 600, 448);
    cout << "JPEG done" << endl;

        // cout << "About to draw image to buffer..." << endl;

        // inky.set_pen(0);
        // inky.clear();

        // for (int y = 0; y < 448; y++) {
        //         uint colour = (y / 16) % 8;
        //         inky.set_pen(colour);
        //         inky.line(Point(0, y), Point(599, y));
        //         cout << y << ",";
        // }

        // cout << endl;
        // cout << "Finished drawing image to buffer" << endl;
    cout << "Updating screen..." << endl;
    inky.update();
    cout << "Updated screen" << endl << endl;
    sleep_ms(500);
}
