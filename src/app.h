#pragma once

#include "olcPixelGameEngine/olcPixelGameEngine.h"

class Application : public olc::PixelGameEngine 
{
public:
    Application(int rows, int cols) 
        : ROWS(rows), COLS(cols)
    {
        sAppName = "Raycaster";
    }
public:
	bool OnUserCreate() override;

	bool OnUserUpdate(float fElapsedTime) override;

private:
    int ROWS = 128;
    int COLS = 128;    
};
