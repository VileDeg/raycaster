#include <iostream>

#include "app.h"

static int ROWS = 128;
static int COLS = 128;

int main(int argc, char** argv)
{
    Application app(ROWS, COLS);

    const int pix_x = 4;
    const int pix_y = 4;

    if (!app.Construct(ROWS, COLS, pix_y, pix_x)) {
        return 1;
    }

    app.Start();
    return 0;
}