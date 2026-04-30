#include "vision_g3.h"

// ESP32-CAM Pin Tanımlamaları
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

bool initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    
    // Yüksek Hız için RGB565 ve QQVGA (160x120)
    config.pixel_format = PIXFORMAT_RGB565; 
    config.frame_size = FRAMESIZE_QQVGA; 
    config.jpeg_quality = 12;
    config.fb_count = 1;

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) return false;
    return true;
}

ColorType detectPixelColor(uint8_t r, uint8_t g, uint8_t b) {
    // Sahadaki ışığa göre ince ayar (tuning) gerektirecek RGB eşikleri
    if (r > 150 && g < 100 && b < 100) return COLOR_RED;
    if (b > 150 && r < 100 && g < 100) return COLOR_BLUE;
    if (r > 150 && g > 150 && b < 100) return COLOR_YELLOW;
    if (g > 150 && r < 100 && b < 150) return COLOR_GREEN;
    return COLOR_NONE;
}

QuadrantResult processImageQuadrants() {
    QuadrantResult result = {COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE};
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) return result;

    int half_w = fb->width / 2;
    int half_h = fb->height / 2;
    int votes_TL[5] = {0}, votes_TR[5] = {0}, votes_BL[5] = {0}, votes_BR[5] = {0};

    for (int i = 0; i < fb->len; i += 2) {
        uint8_t high_byte = fb->buf[i];
        uint8_t low_byte = fb->buf[i+1];
        uint16_t pixel = (high_byte << 8) | low_byte;
        
        uint8_t r = (pixel >> 8) & 0xF8;
        uint8_t g = (pixel >> 3) & 0xFC;
        uint8_t b = (pixel << 3) & 0xF8;

        int current_pixel = i / 2;
        int x = current_pixel % fb->width;
        int y = current_pixel / fb->width;

        ColorType detectedColor = detectPixelColor(r, g, b);
        if (detectedColor != COLOR_NONE) {
            if (y < half_h && x < half_w)      votes_TL[detectedColor]++;
            else if (y < half_h && x >= half_w) votes_TR[detectedColor]++;
            else if (y >= half_h && x < half_w) votes_BL[detectedColor]++;
            else                               votes_BR[detectedColor]++;
        }
    }

    esp_camera_fb_return(fb); // Memory Leak Koruması!

    const int THRESHOLD = 200; 
    auto getWinner = [&](int votes[]) -> ColorType {
        int maxVotes = 0;
        ColorType winner = COLOR_NONE;
        for (int c = 1; c <= 4; c++) {
            if (votes[c] > maxVotes && votes[c] > THRESHOLD) {
                maxVotes = votes[c];
                winner = static_cast<ColorType>(c);
            }
        }
        return winner;
    };

    result.topLeft = getWinner(votes_TL);
    result.topRight = getWinner(votes_TR);
    result.bottomLeft = getWinner(votes_BL);
    result.bottomRight = getWinner(votes_BR);

    return result;
}

String colorToString(ColorType color) {
    switch (color) {
        case COLOR_RED: return "KIRMIZI";
        case COLOR_BLUE: return "MAVI";
        case COLOR_YELLOW: return "SARI";
        case COLOR_GREEN: return "YESIL";
        default: return "BOS";
    }
}
