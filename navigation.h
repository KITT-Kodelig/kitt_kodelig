#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <memory>
#include <chrono>
#include <iostream>
#include <thread>
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <mavsdk/plugins/offboard/offboard.h>
#include <cmath>
#include "config.h" 
#include "vision_module.h"
#include <opencv2/opencv.hpp> //esp de yok


using namespace mavsdk;
using namespace std::this_thread;
using namespace std::chrono;


// ==========================================
// 0. BAĞLANTI VE DRONE KEŞİF FONKSİYONU
// ==========================================
std::shared_ptr<System> connect_to_drone(Mavsdk& mavsdk) {
    std::cout << "[BAĞLANTI] " << CONNECTION_STRING << " adresine bağlanılıyor...\n";
    
    // config.h içindeki CONNECTION_STRING makrosunu kullanıyoruz
    ConnectionResult connection_result = mavsdk.add_any_connection(CONNECTION_STRING);

    if (connection_result != ConnectionResult::Success) {
        std::cerr << "[HATA] Bağlantı kurulamadı. Hata Kodu: " << connection_result << '\n';
        return nullptr; // Başarısız olursa null döndür
    }

    std::cout << "[BAĞLANTI] Port açıldı. Sistem (Drone) bekleniyor...\n";

    // 5.0 saniye boyunca bir otopilot (drone) sisteminin kalp atışını (heartbeat) bekle
    auto system_opt = mavsdk.first_autopilot(5.0);

    if (!system_opt) {
        std::cerr << "[HATA] Zaman aşımı: Bağlantı açıldı ama drone bulunamadı!\n";
        return nullptr;
    }

    std::cout << "[BAŞARILI] Drone bulundu ve iletişim kuruldu!\n";
    
    // Bulunan drone nesnesini (System) döndür
    return system_opt.value();
}

// ==========================================
// 1. ARM ETME FONKSİYONU
// ==========================================
bool arm_drone(Action& action) {
    std::cout << "[KOMUT] Arm ediliyor...\n";
    Action::Result result = action.arm();
    
    if (result == Action::Result::Success) {
        std::cout << "[BAŞARILI] Drone Arm edildi.\n";
        return true;
    } else {
        std::cerr << "[HATA] Arm işlemi başarısız: " << result << '\n';
        return false;
    }
}

// ==========================================
// 2. KALKIŞ (TAKEOFF) FONKSİYONU
// ==========================================
// MAVSDK'da Action::takeoff(), ArduCopter'i otomatik olarak GUIDED moduna alır.
bool takeoff_drone(Action& action, float takeoff_alt_m = 2.5f) {
    std::cout << "[KOMUT] " << takeoff_alt_m << " metreye kalkış yapılıyor...\n";
    
    // Varsayılan kalkış irtifasını ayarla
    action.set_takeoff_altitude(takeoff_alt_m);
    
    Action::Result result = action.takeoff();
    
    if (result == Action::Result::Success) {
        std::cout << "[BAŞARILI] Kalkış komutu alındı.\n";
        return true;
    } else {
        std::cerr << "[HATA] Kalkış başarısız: " << result << '\n';
        return false;
    }
}

// ==========================================
// 3. KONUMA GİTME (GOTO GUIDED) FONKSİYONU
// ==========================================
// ArduCopter'in bu komutu kabul etmesi için drone'un havalanmış ve GUIDED modunda olması gerekir.


// Dünya yarıçapı (metre cinsinden)
const double EARTH_RADIUS = 6378137.0; 

// x ve y metre cinsinden yerel (ENU tabanlı) koordinatlardır
bool goto_local_xy(Action& action, Telemetry& telemetry, double x, double y, float altitude_offset_m = 5.0f) {
    std::cout << "[KOMUT] Yerel X: " << x << "m, Y: " << y << "m konumuna gidiliyor...\n";

    // 1. NORTH_OFFSET'e göre Ekseni Döndür (E ve N metrelerini bul)
    // Not: config.h dosyanızda NORTH_OFFSET tanımlı ama değeri yok. 
    // #define NORTH_OFFSET 0.0 gibi bir radyan değeri atadığınızdan emin olun.
    double E = x * std::cos(NORTH_OFFSET) + y * std::sin(NORTH_OFFSET);
    double N = -x * std::sin(NORTH_OFFSET) + y * std::cos(NORTH_OFFSET);

    // 2. Metreyi Dereceye (Enlem/Boylam Sapmasına) Çevir
    // START_LAT değerini radyana çevirmeliyiz çünkü boylam hesabı enleme göre daralır
    double start_lat_rad = START_LAT * M_PI / 180.0;
    
    double delta_lat = (N / EARTH_RADIUS) * (180.0 / M_PI);
    double delta_lon = (E / (EARTH_RADIUS * std::cos(start_lat_rad))) * (180.0 / M_PI);

    // 3. Hedef Global Koordinatları Belirle
    double target_lat = START_LAT + delta_lat;
    double target_lon = START_LON + delta_lon;

    // 4. İrtifa (Altitude) Hesabı (AMSL)
    auto current_position = telemetry.position();
    float target_altitude_amsl = current_position.absolute_altitude_m + altitude_offset_m;
    
    // Drone'un yönü (Yaw). Sıfır olursa Kuzeye bakar. 
    // Gittiği yöne bakmasını isterseniz: atan2(E, N) * (180.0/M_PI) yapabilirsiniz.
    float target_yaw = 0.0f; 

    // 5. MAVSDK goto_location komutunu gönder
    Action::Result result = action.goto_location(
        target_lat, 
        target_lon, 
        target_altitude_amsl, 
        target_yaw
    );

    if (result == Action::Result::Success) {
        std::cout << "[BAŞARILI] Hedef GPS: " << target_lat << ", " << target_lon << '\n';
        return true;
    } else {
        std::cerr << "[HATA] Konuma gitme başarısız: " << result << '\n';
        return false;
    }
}


// ==========================================
// 4. PRECISION LANDING FONKSİYONU
// ==========================================
bool precision_landing(Offboard& offboard, Telemetry& telemetry, Action& action, TargetColor target_color) {
    std::cout << "[KOMUT] Hassas İniş (Precision Landing) başlatılıyor...\n";

    // --- 1. KAMERAYI BAŞLAT ---
    // Eğer Gazebo UDP üzerinden yayın yapıyorsa (SITL standart):
    cv::VideoCapture cap("udp://127.0.0.1:5600", cv::CAP_GSTREAMER);
    
    // NOT: Eğer UDP çalışmazsa ve Gazebo görüntüyü ROS/virtual_cam üzerinden 
    // /dev/video1 gibi bir aygıta veriyorsa üstteki satırı şununla değiştirin:
    // cv::VideoCapture cap(1); // veya "/dev/video1"

    if (!cap.isOpened()) {
        std::cerr << "[UYARI] Gazebo kamera akışı bulunamadı! İniş iptal ediliyor.\n";
        action.land(); // Güvenlik için normal iniş yap
        return false;
    }

    // Offboard başlatma kodları...
    Offboard::VelocityNedYaw initial_velocity{};
    initial_velocity.down_m_s = 0.5f;
    offboard.set_velocity_ned(initial_velocity);
    offboard.start();

    const float DESCENT_SPEED = 0.5f;
    const float CORRECTION_SPEED = 0.3f;

    // --- 2. İNİŞ VE GÖRÜNTÜ İŞLEME DÖNGÜSÜ ---
    while (true) {
        auto position = telemetry.position();
        if (position.relative_altitude_m < 0.5f) {
            std::cout << "[BİLGİ] Yere çok yaklaşıldı. Motorlar kapatılıyor.\n";
            break;
        }

        // Kameradan yeni frame'i (çerçeveyi) çek
        cv::Mat frame;
        cap >> frame; // Görüntüyü al

        if (frame.empty()) {
            std::cerr << "[UYARI] Kameradan boş frame geldi!\n";
            continue; // Döngünün başına dön ve tekrar dene
        }

        // config.h dosyanızdaki ayarlara göre görüntüyü 240x240 boyutuna yeniden boyutlandır[cite: 5]
        cv::resize(frame, frame, cv::Size(IMG_WIDTH, IMG_HEIGHT));

        // OpenCV'nin 'frame.data' özelliği, görüntünün bellekteki saf uint8_t işaretçisidir!
        // Tam olarak VisionModule sınıfınızın istediği format.
        uint8_t* camera_buffer = frame.data;

        // VisionModule'e gönder ve hedef yönünü al[cite: 6]
        Direction correction = VisionModule::getLandingCorrection(camera_buffer, target_color);

        // --- Hız Vektörlerini Hazırla ---
        Offboard::VelocityNedYaw velocity{};
        velocity.down_m_s = DESCENT_SPEED;
        velocity.yaw_deg = 0.0f;

        switch (correction) {
            case CENTER:     velocity.north_m_s = 0.0f;              velocity.east_m_s = 0.0f;               break;
            case NORTH:      velocity.north_m_s = CORRECTION_SPEED;  velocity.east_m_s = 0.0f;               break;
            case SOUTH:      velocity.north_m_s = -CORRECTION_SPEED; velocity.east_m_s = 0.0f;               break;
            case EAST:       velocity.north_m_s = 0.0f;              velocity.east_m_s = CORRECTION_SPEED;   break;
            case WEST:       velocity.north_m_s = 0.0f;              velocity.east_m_s = -CORRECTION_SPEED;  break;
            // ... diğer ara yönler (NORTH_EAST vb.) ...
            default:         velocity.north_m_s = 0.0f;              velocity.east_m_s = 0.0f;               break;
        }

        offboard.set_velocity_ned(velocity);

        // Görüntü işlemenin ve döngünün çok hızlı çalışıp sistemi boğmaması için ufak bir gecikme
        sleep_for(milliseconds(50)); 
    }

    offboard.stop();
    action.land();
    return true;
}
















#endif
