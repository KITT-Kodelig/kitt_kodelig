#include "vision_module.h"

void VisionModule::bgrToHsv(uint8_t b, uint8_t g, uint8_t r, int &h, int &s, int &v) {
    uint8_t rgbMin = r < g ? (r < b ? r : b) : (g < b ? g : b);
    uint8_t rgbMax = r > g ? (r > b ? r : b) : (g > b ? g : b);
    uint8_t diff = rgbMax - rgbMin;

    v = (rgbMax * 100) / 255;
    if (rgbMax == 0) { s = 0; h = 0; return; }
    s = (diff * 100) / rgbMax;
    if (s == 0) { h = 0; return; }

    if (rgbMax == r) h = 0 + 60 * (g - b) / diff;
    else if (rgbMax == g) h = 120 + 60 * (b - r) / diff;
    else h = 240 + 60 * (r - g) / diff;

    if (h < 0) h += 360;
}

// HELPER: Renk Karar Merkezi
TargetColor VisionModule::matchColor(int h, int s, int v) {
    if (s < HSV_SAT_MIN || v < HSV_VAL_MIN) return NONE;

    if (h < HUE_RED_MAX || h > HUE_RED_MIN) return RED;
    if (h > HUE_YELLOW_MIN && h < HUE_YELLOW_MAX) return YELLOW;
    if (h > HUE_GREEN_MIN && h < HUE_GREEN_MAX) return GREEN;
    if (h > HUE_BLUE_MIN && h < HUE_BLUE_MAX) return BLUE;

    return NONE;
}

// HSV'Lİ VE GÜVENLİ VERSİYON
TargetColor VisionModule::getCenterColor(const uint8_t* buffer) {
    int cx = IMG_WIDTH / 2;
    int cy = IMG_HEIGHT / 2;
    
    long totalB = 0, totalG = 0, totalR = 0;
    int count = 0;

    // Yatay Eksen (Sabit Y, Değişken X)
    for (int x = cx - CROSS_ARM_LENGTH; x <= cx + CROSS_ARM_LENGTH; ++x) {
        if (x < 0 || x >= IMG_WIDTH) continue; // Güvenlik kontrolü
        int idx = (cy * IMG_WIDTH + x) * IMG_CHANNELS;
        totalB += buffer[idx + CH_B];
        totalG += buffer[idx + CH_G];
        totalR += buffer[idx + CH_R];
        count++;
    }

    // Dikey Eksen (Sabit X, Değişken Y)
    for (int y = cy - CROSS_ARM_LENGTH; y <= cy + CROSS_ARM_LENGTH; ++y) {
        if (y == cy || y < 0 || y >= IMG_HEIGHT) continue; // Merkezi iki kez sayma ve güvenliği sağla
        int idx = (y * IMG_WIDTH + cx) * IMG_CHANNELS;
        totalB += buffer[idx + CH_B];
        totalG += buffer[idx + CH_G];
        totalR += buffer[idx + CH_R];
        count++;
    }

    if (count == 0) return NONE;

    int h = 0, s = 0, v = 0;
    bgrToHsv(totalB / count, totalG / count, totalR / count, h, s, v);

    return matchColor(h, s, v);
}

TargetColor VisionModule::analyzeCell(const uint8_t* buffer, int cx, int cy, int cellSize) {
    long totalB = 0, totalG = 0, totalR = 0;
    int count = 0;
    int halfSize = cellSize / 2;

    for (int y = cy - halfSize; y <= cy + halfSize; ++y) {
        for (int x = cx - halfSize; x <= cx + halfSize; ++x) {
            if (x < 0 || x >= IMG_WIDTH || y < 0 || y >= IMG_HEIGHT) continue;

            int idx = (y * IMG_WIDTH + x) * IMG_CHANNELS;
            totalB += buffer[idx + CH_B];
            totalG += buffer[idx + CH_G];
            totalR += buffer[idx + CH_R];
            count++;
        }
    }

    if (count == 0) return NONE;

    int h = 0, s = 0, v = 0;
    bgrToHsv(totalB / count, totalG / count, totalR / count, h, s, v);

    return matchColor(h, s, v); // Helper fonksiyon kullanımı
}

CrossScanResult VisionModule::scanCrossNeighbors(const uint8_t* buffer, int originX, int originY) {
    TargetColor tempColor;

    tempColor = analyzeCell(buffer, originX, originY - CROSS_OFFSET, SCAN_CELL_SIZE);
    if (tempColor != NONE) return {tempColor, NORTH};

    tempColor = analyzeCell(buffer, originX, originY + CROSS_OFFSET, SCAN_CELL_SIZE);
    if (tempColor != NONE) return {tempColor, SOUTH};

    tempColor = analyzeCell(buffer, originX - CROSS_OFFSET, originY, SCAN_CELL_SIZE);
    if (tempColor != NONE) return {tempColor, WEST};

    tempColor = analyzeCell(buffer, originX + CROSS_OFFSET, originY, SCAN_CELL_SIZE);
    if (tempColor != NONE) return {tempColor, EAST};

    return {NONE, CENTER};
}

Direction VisionModule::estimateRelativePosition(int pixelX, int pixelY) {
    int thirdX = IMG_WIDTH / 3;
    int thirdY = IMG_HEIGHT / 3;

    int col = pixelX / thirdX; 
    int row = pixelY / thirdY; 

    if (row == 0 && col == 0) return NORTH_WEST;
    if (row == 0 && col == 1) return NORTH;
    if (row == 0 && col == 2) return NORTH_EAST;
    
    if (row == 1 && col == 0) return WEST;
    if (row == 1 && col == 1) return CENTER;
    if (row == 1 && col == 2) return EAST;

    if (row == 2 && col == 0) return SOUTH_WEST;
    if (row == 2 && col == 1) return SOUTH;
    if (row == 2 && col == 2) return SOUTH_EAST;

    return CENTER; 
}

LandingAngles VisionModule::getLandingAngles(const uint8_t* buffer, TargetColor targetColor) {
    // Hücre boyutu (10x10)
    int cellSize = 10; 
    
    // Görüntü merkez noktası
    int cx = IMG_WIDTH / 2;
    int cy = IMG_HEIGHT / 2;

    // Hücrelerin merkez koordinatlarını belirle (Kenarlardan biraz içeride olsun)
    // Örn: Sol üst hücrenin merkezi (15, 15) gibi.
    int offset = 20; 
    
    // 9 Hücrenin Koordinatları (x, y)
    int cells[9][2] = {
        {cx, cy},                               // 0: MERKEZ
        {cx, offset},                           // 1: KUZEY (Üst)
        {IMG_WIDTH - offset, offset},           // 2: KUZEY DOĞU (Sağ Üst)
        {IMG_WIDTH - offset, cy},               // 3: DOĞU (Sağ)
        {IMG_WIDTH - offset, IMG_HEIGHT - offset},// 4: GÜNEY DOĞU (Sağ Alt)
        {cx, IMG_HEIGHT - offset},              // 5: GÜNEY (Alt)
        {offset, IMG_HEIGHT - offset},          // 6: GÜNEY BATI (Sol Alt)
        {offset, cy},                           // 7: BATI (Sol)
        {offset, offset}                        // 8: KUZEY BATI (Sol Üst)
    };

    // Her bir hücreyi test et
    for (int i = 0; i < 9; i++) {
        TargetColor cellColor = analyzeCell(buffer, cells[i][0], cells[i][1], cellSize);

        // Eğer bu hücrede aradığımız renk (MAVİ) varsa, o yöne doğru açı hesapla
        if (cellColor == targetColor) {
            // Hedefin bulunduğu hücrenin merkez koordinatları
            float target_x = (float)cells[i][0];
            float target_y = (float)cells[i][1];

            // NED referansına göre açı hesabı (Eski kodundakiyle aynı mantık)
            float angle_x = (cy - target_y) * (FOV_Y / (float)IMG_HEIGHT);
            float angle_y = (target_x - cx) * (FOV_X / (float)IMG_WIDTH);

            return {angle_x, angle_y, target_x, target_y, true};
        }
    }

    // Hiçbir hücrede hedef bulunamadı
    return {0.0f, 0.0f, 0.0f, 0.0f, false};
}
