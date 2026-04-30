#include "vision_module.h"

void setup() {
    // SpeedyBee FC ile haberleşme hızı (G2 ile aynı olmalı)
    Serial.begin(115200); 
    
    // Eski kütüphanendeki kamera başlatma fonksiyonu
    // Eğer eski kodunda initCamera() yoksa, G2'deki initCamera fonksiyonunu buraya da ekleyebilirsin.
    if(!initCamera()) {
        Serial.println("G1 Kamerasi baslatilamadi!");
    }
}

void loop() {
    /* 
     * DİKKAT: Aşağıdaki fonksiyon senin eski vision_module.h dosyanın
     * içinde G1 görevini (örneğin hedef tespiti) yapan fonksiyon olmalı.
     * Kendi kütüphanendeki isme göre burayı güncelle.
     */
     
    // Örnek: G1_HedefVerisi data = processG1Vision();
    
    // Elde edilen veriyi FC'ye gönder
    // Serial.print("<G1_DATA:"); Serial.print(data.hedefX); Serial.println(">");
    
    // Sistemin rahatlaması için ufak bekleme
    delay(100); 
}