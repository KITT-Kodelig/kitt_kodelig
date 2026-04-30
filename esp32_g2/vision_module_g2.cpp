#include "vision_module.h"

// Kamera pin tanımlamaları (AI-Thinker modeli ESP32-CAM için standarttır)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

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
    
    // RGB565 formatı, görüntü işlemede piksel okumak için en verimli olandır
    config.pixel_format = PIXFORMAT_RGB565; 
    // QQVGA (160x120). G2 Görevi için düşük çözünürlük = Yüksek Hız
    config.frame_size = FRAMESIZE_QQVGA; 
    config.jpeg_quality = 12;
    config.fb_count = 1;

    // Kamerayı başlat
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Kamera baslatilamadi! Hata Kodu: 0x%x", err);
        return false;
    }
    return true;
}

// Basit ve hızlı RGB renk tespit mantığı (Eşik değerleri sahada kalibre edilmeli)
ColorType detectPixelColor(uint8_t r, uint8_t g, uint8_t b) {
    // Sahanın aydınlatmasına göre bu değerler değişebilir (Şu an temel eşikler girildi)
    if (r > 150 && g < 100 && b < 100) return COLOR_RED;
    if (b > 150 && r < 100 && g < 100) return COLOR_BLUE;
    if (r > 150 && g > 150 && b < 100) return COLOR_YELLOW;
    if (g > 150 && r < 100 && b < 150) return COLOR_GREEN;
    return COLOR_NONE;
}

QuadrantResult processImageQuadrants() {
    QuadrantResult result = {COLOR_NONE, COLOR_NONE, COLOR_NONE, COLOR_NONE};
    
    // Kameradan kareyi (frame) al
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Kare (Frame) alinamadi!");
        return result;
    }

    int half_w = fb->width / 2;
    int half_h = fb->height / 2;

    // Her bölge için renk sayaçları [Red, Blue, Yellow, Green]
    int votes_TL[5] = {0}, votes_TR[5] = {0}, votes_BL[5] = {0}, votes_BR[5] = {0};

    // RGB565 formatında her piksel 2 byte yer kaplar
    for (int i = 0; i < fb->len; i += 2) {
        // RGB565'ten 8-bit R, G, B değerlerini çıkarma
        uint8_t high_byte = fb->buf[i];
        uint8_t low_byte = fb->buf[i+1];
        uint16_t pixel = (high_byte << 8) | low_byte;
        
        uint8_t r = (pixel >> 8) & 0xF8;
        uint8_t g = (pixel >> 3) & 0xFC;
        uint8_t b = (pixel << 3) & 0xF8;

        // X ve Y koordinatını hesapla
        int current_pixel = i / 2;
        int x = current_pixel % fb->width;
        int y = current_pixel / fb->width;

        // Rengi bul
        ColorType detectedColor = detectPixelColor(r, g, b);

        // İlgili bölgenin (Quadrant) oy sandığına oyu at
        if (detectedColor != COLOR_NONE) {
            if (y < half_h && x < half_w)      votes_TL[detectedColor]++; // Sol Üst
            else if (y < half_h && x >= half_w) votes_TR[detectedColor]++; // Sağ Üst
            else if (y >= half_h && x < half_w) votes_BL[detectedColor]++; // Sol Alt
            else                               votes_BR[detectedColor]++; // Sağ Alt
        }
    }

    // Kareyi hafızadan serbest bırak (Memory leak olmaması için çok kritik!)
    esp_camera_fb_return(fb);

    // En çok oy alan renkleri belirleme algoritması (Gürültü eşiği: en az 200 piksel)
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