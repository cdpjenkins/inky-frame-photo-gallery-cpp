#include <iostream>
using namespace std;

#include "libraries/inky_frame/inky_frame.hpp"


using namespace pimoroni;


int main() {

    // TODOs
    // - set up stdio over UART
    // - Do something with the LEDs
    // - Filesystem on SD card
    // - Use wifi to download pics
    // - Display actual JPEGs 

    InkyFrame inky;
    inky.init();

    stdio_init_all();
    sleep_ms(500);

    cout << endl << endl << endl;

    cout << "initialising inky frame.. " << endl;
    cout << "done!\n" << endl;

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
