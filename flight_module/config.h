#ifndef CONFIG_H
#define CONFIG_H

//bazı koordinatlar (şu an gazebo için)
#define START_LAT -35.363261 // 2,2 (BAŞLANGIÇ GRİDİ)
#define START_LON 149.165230 //2,2 (BAŞLANGIÇ GRİDİ)

#define END_LAT -35.363205
#define END_LON 149.165300

#define NORTH_OFFSET 0.0 //bizim +y olarak verdiğimiz yönün gerçek northla arasındaki fark (radian)


#define CONNECTION_STRING "udp://:14550"












#include <stdint.h>

// --- KAMERA VE GÖRÜNTÜ AYARLARI ---
#define IMG_WIDTH       240
#define IMG_HEIGHT      240
#define IMG_CHANNELS    3
#define CROSS_ARM_LENGTH 5   // Merkez artı taramasının her bir kolunun piksel uzunluğu

// BGR vs RGB Dizilimi (Görüntü BGR geliyorsa B_CH 0, R_CH 2 yapılır)
#define CH_B            0
#define CH_G            1
#define CH_R            2

// --- RENK TESPİT (HSV) EŞİKLERİ ---
#define HSV_SAT_MIN     30   // Minimum doygunluk (Grileri elemek için)
#define HSV_VAL_MIN     30   // Minimum parlaklık (Siyahları elemek için)

#define HUE_RED_MIN     345
#define HUE_RED_MAX     15
#define HUE_YELLOW_MIN  45
#define HUE_YELLOW_MAX  75
#define HUE_GREEN_MIN   100
#define HUE_GREEN_MAX   140
#define HUE_BLUE_MIN    200
#define HUE_BLUE_MAX    260

// --- TARAMA VE İNİŞ PARAMETRELERİ ---
// 1. Görev (Artı şeklinde tarama) parametreleri
#define SCAN_CELL_SIZE  5    // x ekseninde ve y ekseninde 5'er piksel (Toplam 25 piksel)
#define CROSS_OFFSET    20   // Merkezden sağa/sola/alta/üste ne kadar uzaklıkta taranacak

// 3. Görev (İniş hizalaması) parametreleri
#define LANDING_CELL    10   // İniş tespiti için kullanılacak hücre boyutu (10x10)
#define GRID_OFFSET_X   60   // İniş için 9'lu gridin X ekseni aralığı
#define GRID_OFFSET_Y   60   // İniş için 9'lu gridin Y ekseni aralığı













#endif
