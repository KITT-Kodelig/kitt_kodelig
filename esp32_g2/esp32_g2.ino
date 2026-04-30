#include "vision_g2.h"

void setup() {
    // SpeedyBee FC'nin seri port hızıyla aynı olmalı (örn: 115200)
    Serial.begin(115200); 
    
    // Kamerayı başlat, başarısız olursa seri porttan uyar
    if(!initCamera()) {
        Serial.println("Kamera baslatilamadi!");
    }
}

void loop() {
    // Fotoğrafı çek ve 4 çeyreği (Quadrant) analiz et
    QuadrantResult qr = processImageQuadrants();

    // ArduPilot'un (veya FC'nin) okuyacağı formatta TX pininden bas
    // Örnek Çıktı: <TL:KIRMIZI,TR:BOS,BL:MAVI,BR:SARI>
    Serial.print("<TL:"); Serial.print(colorToString(qr.topLeft));
    Serial.print(",TR:"); Serial.print(colorToString(qr.topRight));
    Serial.print(",BL:"); Serial.print(colorToString(qr.bottomLeft));
    Serial.print(",BR:"); Serial.print(colorToString(qr.bottomRight));
    Serial.println(">");

    // İşlemciyi boğmamak ve FC'ye veri işleme süresi tanımak için bekleme (Saniyede ~3 kare)
    delay(300); 
}