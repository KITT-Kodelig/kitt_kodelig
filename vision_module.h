#ifndef VISION_MODULE_H
#define VISION_MODULE_H

#include "config.h"

enum TargetColor { NONE = 0, RED, GREEN, BLUE, YELLOW };

struct LandingAngles {
    float angle_x;
    float angle_y;
    float pixel_x; // Çizim için eklendi
    float pixel_y; // Çizim için eklendi
    bool target_found;
};

enum Direction {
    CENTER = 0, NORTH, NORTH_EAST, EAST, SOUTH_EAST, 
    SOUTH, SOUTH_WEST, WEST, NORTH_WEST
};

struct CrossScanResult {
    TargetColor color;
    Direction foundAt; 
};

class VisionModule {
private:
    static void bgrToHsv(uint8_t b, uint8_t g, uint8_t r, int &h, int &s, int &v);
    
    // YENİ HELPER: H, S, V değerlerini alıp hangi renge ait olduğunu döndürür
    static TargetColor matchColor(int h, int s, int v);
    
    static TargetColor analyzeCell(const uint8_t* buffer, int cx, int cy, int cellSize);

public:
    // İLK KODUMUZ: Sadece merkeze 10x10'luk bir artı (cross) atarak kameranın tam neye baktığını söyler
    static TargetColor getCenterColor(const uint8_t* buffer);

    static CrossScanResult scanCrossNeighbors(const uint8_t* buffer, int originX, int originY);
    static Direction estimateRelativePosition(int pixelX, int pixelY);
    static LandingAngles getLandingAngles(const uint8_t* buffer, TargetColor targetColor);
};

#endif // VISION_MODULE_H
