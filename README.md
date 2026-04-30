# KİTT Otonom İHA Sistemleri - KODELİG

Bu depo (repository), **KİTT Turan** ve **Aldebaran** bağımsız İHA takımlarının KODELİG yarışması için geliştirdiği otonom uçuş, seyrüsefer ve gömülü görüntü işleme yazılımlarını içermektedir.

Sistem, ArduPilot (SpeedyBee FC) uçuş kontrolcüsü ile ESP32-CAM modülü arasındaki asenkron seri haberleşme mimarisine dayanır. Otonomi algoritmaları, Görev 1 (Seyrüsefer) ve Görev 2 (Sürüyü Bul) için modüler olarak tasarlanmıştır.

## 🚀 Donanım & Yazılım Mimarisi
*   **Uçuş Kontrolcüsü:** SpeedyBee FC (ArduPilot / MAVLink)
*   **Görüntü İşleme / Görev Yükü:** ESP32-CAM (OV2640)
*   **Görüntü Formatı:** RGB565 / QQVGA (Donanım seviyesinde optimizasyon)
*   **Yer İstasyonu Araçları:** Python (Tkinter & WGS84 Dönüşüm Algoritmaları)
*   **Dil:** C++ (Gömülü Sistemler), Python

---

## 📂 Klasör Yapısı ve Modüller

Proje, sahada hızlı kod değişimi (flashing) ve hata ayıklama yapılabilmesi için donanım hedeflerine göre 4 ana modüle ayrılmıştır:

### 1. `tools/` (Yer İstasyonu Araçları)
Saha kurulumu sırasında kullanılan masaüstü araçlarını içerir.
*   **`waypoint_calculator.py`:** İHA'nın kalkış yaptığı (2,2) matının referans GPS verisini (Enlem/Boylam) ve sahanın gerçek kuzey sapma açısını (Yaw) alarak, 9x9 metrelik sahanın tüm kesişim noktalarının **WGS84 dünya koordinatlarını** hesaplar. Mission Planner'a waypoint atamak için saniyeler içinde `.txt` çıktısı üretir.

### 2. `flight_module/` (Uçuş Mantığı)
Otopilot ve companion computer tarafında çalışacak temel navigasyon ve görev yönetim C++ kütüphaneleridir.
*   `config.h`, `navigation.h`, `mission_1.cpp` 

### 3. `esp32_g1/` (Görev 1: Seyrüsefer Payload)
Görev 1 için kullanılan standart vizyon ve hedef tespit kodlarıdır. ESP32-CAM'e doğrudan flashlanabilir formattadır.

### 4. `esp32_g2/` (Görev 2: Sürüyü Bul Payload)
Görev 2'deki 120 saniyelik zaman kısıtlamasını aşmak için geliştirilmiş **"Dörtlü Tarama (Quadrant Scanning)"** algoritmasını içerir.
*   İHA, sahadaki kırmızı kesişim noktalarına gider (AltHold ~2.5m).
*   ESP32-CAM tek bir kare çeker ve RGB565 formatında hızlı piksel taraması yapar.
*   Görüntüyü 4 çeyreğe (Sol Üst, Sağ Üst, Sol Alt, Sağ Alt) böler, HSV eşikleme mantığıyla matların renklerini tespit eder.
*   Uçuş kontrolcüsüne seri port üzerinden `<TL:KIRMIZI,TR:BOS,BL:MAVI,BR:SARI>` formatında karar verilerini anlık olarak basar.

---

## 🛠 Kurulum ve Kullanım

### Waypoint Hesaplayıcıyı Çalıştırmak (Python)
Yer istasyonunuzda (Ubuntu/Windows) hiçbir ek kütüphane kurmadan çalıştırabilirsiniz:
```bash
cd tools
python3 waypoint_calculator.py
