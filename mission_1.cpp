#include "vision_module.h"
#include "navigation.h" // MAVLink ve uçuş fonksiyonların
#include "config.h"
#include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>

bool run_mission_1(mavsdk::MavlinkPassthrough& passthrough, Offboard& offboard, Telemetry& telemetry, Action& action, TargetColor target_color) {
    std::cout << "[GÖREV 1] Arama ve Tarama Başlıyor...\n";

    // 1. Başlangıç gridine intikal et
    // goto_local_xy(START_LAT, START_LON, ...); 

    // 2. Kamerayı başlat
    cv::VideoCapture cap("udp://127.0.0.1:5600", cv::CAP_GSTREAMER); // ESP/SITL kamera portu
    if (!cap.isOpened()) {
        std::cerr << "[HATA] Kamera açılamadı!\n";
        return false;
    }

    bool found = false;

    // 3. Arama Döngüsü
    while (!found) {
        // Drone'u 3D voxel haritana göre sıradaki arama noktasına yönlendir
        // navigate_to_next_probable_voxel();

        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) continue;

        // config.h parametrelerine göre yeniden boyutlandır
        cv::resize(frame, frame, cv::Size(IMG_WIDTH, IMG_HEIGHT));
        
        // Yeni fonksiyon ile hedefi kontrol et
        LandingAngles result = VisionModule::getLandingAngles(frame.data, target_color);

        if (result.target_found) {
            std::cout << "[GÖREV 1] Hedef tespit edildi! İniş sekansına devrediliyor...\n";
            found = true;
            break; // Arama döngüsünü bitir
        }

        // ESP'yi/işlemciyi boğmamak için bekleme
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Kamerayı hassas iniş (precision_landing) fonksiyonunun kullanabilmesi için serbest bırak
    cap.release(); 

    // 4. Hedef bulunduğunda doğrudan Hassas İnişi başlat
	return precision_landing(passthrough, offboard, telemetry, action, target_color);
}
