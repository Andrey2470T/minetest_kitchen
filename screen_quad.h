#pragma once

#include "globals.h"

class ScreenQuad {
public:
    ScreenQuad(video::IVideoDriver* driver);

    void Render();

    video::SMaterial mat;
private:
    video::IVideoDriver* mDriver;
    video::S3DVertex2TCoords mVertices[4];
};
