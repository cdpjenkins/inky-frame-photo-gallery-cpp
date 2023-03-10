#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>
using namespace std;

#include "libraries/inky_frame/inky_frame.hpp"
using namespace pimoroni;

#include "JPEGDEC.h"

#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/apps/http_client.h"

#include "wifi_settings.h"
#include "HttpConnection.hpp"

InkyFrame inky;

FATFS fs;

JPEGDEC jpeg;
struct {
  int x, y, w, h;
} jpeg_decode_options;

vector<string> split_by_newlines(string &&content);

basic_string<char> read_text_file(const char *file_path);

void http_get_to_file(const char *ip_addres, int port, const char *path, const char *file_path);

void filesystem_list(const char *path);

void mount_sd_card_filesystem();

void connect_wifi();

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

    cout << "Starting jpeg decode.." << endl;
    int start = millis();
    jpeg.decode(0, 0, 0);
    cout << "done in " << int(millis() - start) << " ms!" << endl;

    jpeg.close();
}

int photo_gallery_main() {
    inky.init();

    stdio_init_all();
    sleep_ms(1000);

    cout << "*****************************" << endl;
    cout << "Welcome to the Photo Gallery!" << endl;
    cout << "*****************************" << endl << endl << endl;

    connect_wifi();

    mount_sd_card_filesystem();

    // filesystem_list("/");

    http_get_to_file("192.168.1.51", 8000, "/list.txt", "/list.txt");

    vector<string> files = split_by_newlines(read_text_file("/list.txt"));

    while (true) {
        for (const auto &jpeg_filename: files) {
            inky.led(pimoroni::InkyFrame::LED_CONNECTION, 100);
            string jpeg_path = "/" + jpeg_filename;
            string local_path = "/" + jpeg_filename;
            http_get_to_file("192.168.1.51", 8000, jpeg_path.c_str(), local_path.c_str());
            inky.led(pimoroni::InkyFrame::LED_CONNECTION, 0);

            inky.led(pimoroni::InkyFrame::LED_ACTIVITY, 100);
            cout << "Drawing JPEG: " << jpeg_filename << "... " << endl;
            inky.set_pen(0);
            inky.clear();
            draw_jpeg(jpeg_filename, 0, 0, 600, 448);
            cout << "Done drawing JPEG" << endl;
            inky.led(pimoroni::InkyFrame::LED_ACTIVITY, 0);

            cout << "Updating screen... " << endl;
            inky.update();
            cout << "Done updating screen" << endl << endl;

            sleep_ms(600000);
        }
        cout << "Listing done!" << endl;
    }

    // Possibly isn't much point doing this, given that this line will never be reached...
    cyw43_arch_deinit();
}

int main() {
    try {
        photo_gallery_main();
    } catch (runtime_error &e) {
        cout << endl << endl << endl;
        cout << "runtime_error:" << endl;
        cout << e.what() << endl;
    }
}

void connect_wifi() {
    int rc;

    cout << "Setting up WiFI" << endl;
    cout << "Initialising CYW43... ";
    rc = cyw43_arch_init_with_country(CYW43_COUNTRY_UK);
    if (rc) {
        cout << "Return code: " << rc;
        throw runtime_error("Failed to init CYW43");
    }
    cout << "Initialising CYW34 done." << endl;

    cyw43_arch_enable_sta_mode();

    cout << "Connecting to WiFi... ";
    rc = cyw43_arch_wifi_connect_blocking(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_MIXED_PSK);
    if (rc) {
        cout << "Return code: " << rc;
        throw runtime_error("Failed to connect to WiFi");
    }
    cout << "Connected!" << endl;

    cout << "IP address: " << ip4addr_ntoa(netif_ip_addr4(netif_default)) << endl;
    cout << "Netmask:    " << ip4addr_ntoa(netif_ip_netmask4(netif_default)) << endl;
    cout << "Gateway:    " << ip4addr_ntoa(netif_ip_gw4(netif_default)) << endl;
    cout << "Hostname:   " << netif_get_hostname(netif_default) << endl;
}

void mount_sd_card_filesystem() {
    cout << "Mounting SD card filesystem... ";
    FRESULT fr = f_mount(&fs, "", 1);
    if (fr != FR_OK) {
        cout << "fr: " << fr << endl;
        throw runtime_error("Failed to mount SD card filesystem");
    }
    cout << "Filesystem mounted!" << endl;
}

void filesystem_list(const char *path) {
    cout << "Listing sd card contents.." << endl;
    FILINFO file;
    auto dir = new DIR();
    f_opendir(dir, path);
    while(f_readdir(dir, &file) == FR_OK && file.fname[0]) {
        cout << file.fname << " " << file.fsize << endl;
    }
    f_closedir(dir);
    cout << "Listing done!" << endl;
}

void http_get_to_file(const char *ip_addres, int port, const char *path, const char *file_path) {
    cout << "Downloading " << path << " to " << file_path << "... ";
    HttpConnection connection(ip_addres, port, path, file_path);
    connection.do_request();
}

basic_string<char> read_text_file(const char *file_path) {
    const FSIZE_t LIST_MAX_SIZE = (1 << 14) - 1;

    char buffer[LIST_MAX_SIZE + 1];

    FIL list_file_handle;
    FRESULT rc = f_open(&list_file_handle, file_path, FA_OPEN_EXISTING | FA_READ);
    if (rc) {
        cout << "rc: " << rc << endl;
        throw runtime_error("Failed to open file for reading");
    }

    FSIZE_t list_txt_size = f_size(&list_file_handle);
    cout << "Size of list.txt is " << list_txt_size << endl;
    if (list_txt_size > LIST_MAX_SIZE) {

        cout << "Size of list.txt is too large: " << list_txt_size << " bytes" << endl;
        throw runtime_error("Size of list.txt is too large");
    }

    UINT bytes_read;
    FRESULT fresult = f_read(&list_file_handle, buffer, list_txt_size, &bytes_read);
    if (fresult != FR_OK) {
        cout << "Failed to read from " << file_path << ": " << fresult;
        throw runtime_error("Failed to read from file");
    }
    buffer[bytes_read] = '\0';

    f_close(&list_file_handle);

    return std::move(string{buffer});
}

vector<string> split_by_newlines(string &&content) {
    stringstream file_list{content};
    vector<string> files;
    string line;
    while (getline(file_list, line, '\n')) {
        files.push_back(line);
    }
    return files;
}

void display_images_by_listing_directory() {
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
