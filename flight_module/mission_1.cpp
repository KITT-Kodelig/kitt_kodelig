#include "navigation.h"
#include "config.h"
#include "vision_module.h"
#include <mavsdk/plugins/offboard/offboard.h>

int main() {
    // Sadece bir kez tanımlama yapın ve Mavsdk::ComponentType kullanın
    Mavsdk::Configuration config(Mavsdk::ComponentType::CompanionComputer);
    
    Mavsdk mavsdk{config};
    
    // Bağlantı fonksiyonunu çağır[cite: 4]
    std::shared_ptr<System> drone_system = connect_to_drone(mavsdk);
    
    if (!drone_system) {
        return 1; 
    }

    auto telemetry = Telemetry{drone_system};
    auto action = Action{drone_system};
    auto offboard = Offboard{drone_system};

    // Sensörlerin hazır olmasını bekle
    while (!telemetry.health_all_ok()) {
        std::cout << "Sensörler ve GPS hazırlanıyor...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // Görev adımları[cite: 3, 4]
    if (arm_drone(action)) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        if (takeoff_drone(action, 3.0f)) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            
            // config.h içindeki START_LAT ve START_LON üzerinden ENU koordinatına git[cite: 2, 4]
            goto_local_xy(action, telemetry, 6.25f, 6.25f, 0.0f);
        }
    }
    
    std::cout << "Hedefe ulaşıldı. İniş aranıyor...\n";
    precision_landing(offboard, telemetry, action, TargetColor::BLUE);


    return 0;
}
