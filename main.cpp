#include <iostream>
using namespace std;

#include "libraries/inky_frame/inky_frame.hpp"
using namespace pimoroni;

#include "JPEGDEC.h"

#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/apps/http_client.h"

#include "wifi_settings.h"

InkyFrame inky;

JPEGDEC jpeg;
struct {
  int x, y, w, h;
} jpeg_decode_options;

bool global_http_thang_done = false;

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

    return br;
}

int32_t jpegdec_seek_callback(JPEGFILE *jpeg, int32_t p) {
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

void http_result(void *arg,
                  httpc_result_t httpc_result,
                  u32_t rx_content_len,
                  u32_t srv_res,
                  err_t err
) {
  cout << "Transfer complete" << endl;
  cout << "Local result: " << httpc_result << endl;
  cout << "HTTP result: " << srv_res << endl;

  global_http_thang_done = true;
}

err_t http_headers(httpc_state_t *connection,
              void *arg,
              struct pbuf *hdr,
              u16_t hdr_len,
              u32_t content_len
) {
  cout << "Headers received" << endl;
  cout << "Content length: " << content_len << endl;

  return ERR_OK;
}

err_t http_body(void *arg,
            struct altcp_pcb *conn,
            struct pbuf *p,
            err_t err) {
  char myBuff[4096];

  cout << "Body:" << endl;
  u16_t bytes_copied = pbuf_copy_partial(p, myBuff, p->tot_len, 0);
  cout << bytes_copied << " bytes copied" << endl;
  myBuff[bytes_copied] = 0;
  cout << myBuff << endl;

  return ERR_OK;
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
    
    cout << "Setting up WiFI" << endl;
    cout << "Initialising CYW43... ";
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_UK)) {
        cout << "Failed to init CYW43" << endl;
        return 1;
    }
    cout << "Done!" << endl;

    cyw43_arch_enable_sta_mode();

    cout << "Connecting to WiFi... ";
    if (cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK)) {
        cout << "Failed to connect to WiFi!" << endl;
        return 2;
    }
    cout << "Connected!" << endl;

    cout << "IP address: " << ip4addr_ntoa(netif_ip_addr4(netif_default)) << endl;
    cout << "Netmask:    " << ip4addr_ntoa(netif_ip_netmask4(netif_default)) << endl;
    cout << "Gateway:    " << ip4addr_ntoa(netif_ip_gw4(netif_default)) << endl;
    cout << "Hostname:   " << netif_get_hostname(netif_default) << endl;

    cout << "Mounting SD card filesystem... ";
    FATFS fs;
    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
      cout << "Failed to mount SD card filesystem, error: " << fr << endl;
      return 0;
    }
    cout << "Filesystem mounted!" << endl;

    cyw43_arch_lwip_begin();
    cout << "Doing HTTP request... ";
    global_http_thang_done = false;
    httpc_connection_t settings;
    settings.result_fn = http_result;
    settings.headers_done_fn = http_headers;

    ip_addr_t server_addr;
    ip4_addr_set_u32(&server_addr, ipaddr_addr("192.168.1.51"));

    err_t err = httpc_get_file(&server_addr,
                              8000,
                              "/list.txt",
                              &settings,
                              http_body,
                              nullptr,
                              nullptr);
    cyw43_arch_lwip_end();

    bool http_thang_done = false;
    while (!http_thang_done) {
      cyw43_arch_lwip_begin();
      if (global_http_thang_done) {
        http_thang_done = true;
      }
      cyw43_arch_lwip_end();
    }


    while (true) {
        cout << "Listing sd card contents.." << endl;
        FILINFO file;
        auto dir = new DIR();
        f_opendir(dir, "/");
        while(f_readdir(dir, &file) == FR_OK && file.fname[0]) {
            cout << file.fname << " " << file.fsize << endl;

            cout << "Drawing JPEG..." << endl;
            inky.clear();
            draw_jpeg(file.fname, 0, 0, 600, 448);
            cout << "JPEG done" << endl;

            cout << "Updating screen..." << endl;
            inky.update();
            cout << "Updated screen" << endl << endl;
            sleep_ms(600000);
        }
        f_closedir(dir);
        cout << "Listing done!" << endl;
    }

    // Possibly isn't much point doing this, given that this line will never be reached...
    cyw43_arch_deinit();

}
