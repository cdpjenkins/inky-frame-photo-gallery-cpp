#include <iostream>
using namespace std;

#include "libraries/inky_frame/inky_frame.hpp"


using namespace pimoroni;


int main() {

    // TODOs
    // - Do something with the LEDs
    // - Use wifi to download pics
    // - Display actual JPEGs 

    InkyFrame inky;
    inky.init();

    stdio_init_all();
    sleep_ms(1000);

    cout << endl << endl << endl;

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

    cout << "About to draw image to buffer..." << endl;

    inky.set_pen(0);
    inky.clear();

    for (int y = 0; y < 448; y++) {
            uint colour = (y / 16) % 8;
            inky.set_pen(colour);
            inky.line(Point(0, y), Point(599, y));
            cout << y << ",";
    }

    cout << endl;
    cout << "Finished drawing image to buffer" << endl;
    cout << "Updating screen..." << endl;
    inky.update();
    cout << "Updated screen" << endl << endl;
    sleep_ms(500);
}
