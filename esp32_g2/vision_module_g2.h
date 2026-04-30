#ifndef VISION_MODULE_H
#define VISION_MODULE_H

#include <Arduino.h>
#include "esp_camera.h"

// Tespit edilebilecek renkler
enum ColorType {
    COLOR_NONE = 0,
    COLOR_RED,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_GREEN
};

// 4 hücrenin (çeyreğin) sonucunu tutacak yapı
struct QuadrantResult {
    ColorType topLeft;
    ColorType topRight;
    ColorType bottomLeft;
    ColorType bottomRight;
};

// Fonksiyon prototipleri
bool initCamera();
QuadrantResult processImageQuadrants();
String colorToString(ColorType color);

#endif // VISION_MODULE_H