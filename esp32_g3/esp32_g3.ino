#include "vision_g3.h"

void setup() {
    Serial.begin(115200); 
    if(!initCamera()) {
        Serial.println("G3 Kamerasi baslatilamadi!");
    }
}

void loop() {
    QuadrantResult qr = processImageQuadrants();

    // Uçuş kontrolcüsüne Görev 3 için veri bastığımızı "G3_" etiketiyle belirtiyoruz
    Serial.print("<G3_TL:"); Serial.print(colorToString(qr.topLeft));
    Serial.print(",TR:"); Serial.print(colorToString(qr.topRight));
    Serial.print(",BL:"); Serial.print(colorToString(qr.bottomLeft));
    Serial.print(",BR:"); Serial.print(colorToString(qr.bottomRight));
    Serial.println(">");

    delay(300); 
}
