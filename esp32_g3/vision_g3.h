#ifndef VISION_G3_H
#define VISION_G3_H

#include <Arduino.h>
#include "esp_camera.h"

enum ColorType { COLOR_NONE = 0, COLOR_RED, COLOR_BLUE, COLOR_YELLOW, COLOR_GREEN };

struct QuadrantResult {
    ColorType topLeft;
    ColorType topRight;
    ColorType bottomLeft;
    ColorType bottomRight;
};

bool initCamera();
QuadrantResult processImageQuadrants();
String colorToString(ColorType color);

#endif
