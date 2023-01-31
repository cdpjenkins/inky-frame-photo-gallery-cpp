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

    printf("\n");
    printf("\n");
    printf("\n");

    printf("initialising inky frame.. ");
    printf("done!\n");

    // inky.led(InkyFrame::LED_ACTIVITY, 0);

    // inky.led(InkyFrame::LED_CONNECTION, 0);

    printf("About to draw on the screen...\n");

    inky.set_pen(0);
    inky.clear();

    for (int y = 0; y < 448; y++) {
            uint colour = (y / 16) % 8;
            inky.set_pen(colour);
            inky.line(Point(0, y), Point(599, y));
            printf("%d,", y);
    }

    printf("\n");
    printf("Finished drawing image\n");
    inky.update();
    printf("Updated screen\n");
    printf("\n");
    sleep_ms(500);
}
