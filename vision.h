#ifndef VISION_H
#define VISION_H


#include <math.h>

// Renk dönüş tipleri
typedef enum {
    RED,
    GREEN,
    YELLOW,
    BLUE,
    OTHER_COLORS
} COLORS;

// Tarama bölgeleri
typedef enum {
    REGION_CENTER = 1,
    REGION_TOP_LEFT = 2,
    REGION_TOP_RIGHT = 3,
    REGION_BOTTOM_LEFT = 4,
    REGION_BOTTOM_RIGHT = 5
} REGION;



void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, float *h, float *s, float *v) {
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;
    
    float max_val = fmax(rf, fmax(gf, bf));
    float min_val = fmin(rf, fmin(gf, bf));
    float delta = max_val - min_val;

    *v = max_val; // Parlaklık
    *s = (max_val == 0.0f) ? 0.0f : (delta / max_val); // Doygunluk
    *h = 0.0f; // Renk özü

    if (delta > 0.0f) {
        if (max_val == rf) {
            *h = 60.0f * fmod(((gf - bf) / delta), 6.0f);
        } else if (max_val == gf) {
            *h = 60.0f * (((bf - rf) / delta) + 2.0f);
        } else if (max_val == bf) {
            *h = 60.0f * (((rf - gf) / delta) + 4.0f);
        }
        
        if (*h < 0.0f) {
            *h += 360.0f;
        }
    }
}


COLORS scanRegionForColor(uint8_t* fb_buf, REGION region) {
    int cx, cy;
    int width = 240;
    int height = 240;
    int span = 15; // Merkezden kollara uzanan piksel mesafesi (+ işaretinin boyutu)

    // Enum'a göre merkez koordinatları (cx, cy) belirle
    switch(region) {
        case REGION_CENTER:       cx = 120; cy = 120; break;
        case REGION_TOP_LEFT:     cx = 60;  cy = 60;  break;
        case REGION_TOP_RIGHT:    cx = 180; cy = 60;  break;
        case REGION_BOTTOM_LEFT:  cx = 60;  cy = 180; break;
        case REGION_BOTTOM_RIGHT: cx = 180; cy = 180; break;
        default:                  cx = 120; cy = 120; break;
    }

    long total_r = 0, total_g = 0, total_b = 0;
    int pixel_count = 0;

    // 1. Yatay Tarama (X ekseni)
    for (int x = cx - span; x <= cx + span; x++) {
        // Görüntü sınırları dışına çıkmamak için güvenlik kontrolü
        if (x < 0 || x >= width) continue; 
        
        int index = (cy * width + x) * 2;
        uint16_t pixel = (fb_buf[index] << 8) | fb_buf[index + 1];
        
        total_r += (pixel >> 8) & 0xF8;
        total_g += (pixel >> 3) & 0xFC;
        total_b += (pixel << 3) & 0xF8;
        pixel_count++;
    }

    // 2. Dikey Tarama (Y ekseni)
    for (int y = cy - span; y <= cy + span; y++) {
        // Merkeze denk gelen noktayı 2 kez saymamak ve sınırları aşmamak için kontrol
        if (y == cy || y < 0 || y >= height) continue;
        
        int index = (y * width + cx) * 2;
        uint16_t pixel = (fb_buf[index] << 8) | fb_buf[index + 1];
        
        total_r += (pixel >> 8) & 0xF8;
        total_g += (pixel >> 3) & 0xFC;
        total_b += (pixel << 3) & 0xF8;
        pixel_count++;
    }

    // RGB Ortalamalarını al
    uint8_t avg_r = total_r / pixel_count;
    uint8_t avg_g = total_g / pixel_count;
    uint8_t avg_b = total_b / pixel_count;

    // Ortalamayı HSV'ye dönüştür
    float h, s, v;
    rgbToHsv(avg_r, avg_g, avg_b, &h, &s, &v);

    // --- HSV Değerleri ile Renk Kararı ---
    
    // Eğer doygunluk (Saturation) veya parlaklık (Value) çok düşükse renk seçilemez (siyah/beyaz/gri)
    if (s < 0.3f || v < 0.2f) {
        return OTHER_COLORS; 
    }

    // Hue (Renk Özü) 0-360 derece arası değer alır
    if (h < 20.0f || h >= 330.0f) {
        return RED;
    } 
    else if (h >= 30.0f && h < 90.0f) {
        return YELLOW;
    } 
    else if (h >= 90.0f && h < 160.0f) {
        return GREEN;
    } 
    else if (h >= 170.0f && h < 250.0f) {
        return BLUE;
    }

    return OTHER_COLORS;
}









#endif
