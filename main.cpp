#include "navigation.h"
#include "config.h"
#include "vision_module.h"
#include <mavsdk/plugins/offboard/offboard.h>
#include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>

// mission_1.cpp içindeki fonksiyonumuzu derleyiciye tanıtıyoruz
extern bool run_mission_1(Offboard& offboard, Telemetry& telemetry, Action& action, TargetColor target_color);

int main() {
	rclcpp::init(0, nullptr);
    Mavsdk::Configuration config(Mavsdk::ComponentType::CompanionComputer);
    Mavsdk mavsdk{config};
    
    std::shared_ptr<System> drone_system = connect_to_drone(mavsdk);
    
    if (!drone_system) {
        return 1; 
    }

    auto telemetry = Telemetry{drone_system};
    auto action = Action{drone_system};
    auto offboard = Offboard{drone_system};
    
    
    // Passthrough eklentisini başlatıyoruz
    auto passthrough = mavsdk::MavlinkPassthrough{drone_system};

    while (!telemetry.health_all_ok()) {
        std::cout << "Sensörler ve GPS hazırlanıyor...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    if (arm_drone(action)) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        if (takeoff_drone(action, 1.5f)) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            
            // config.h içindeki değerlere göre hedefe intikal
            goto_local_xy(action, telemetry, 6.25f, 6.25f, 0.0f);
        }
    }
    
    std::cout << "Hedefe Ulaşıldı. İniş Başlatılıyor...\n";
    
    // Direkt iniş yapmak yerine, artık önce hedefi arayacak olan görev döngümüzü başlatıyoruz
    //run_mission_1(offboard, telemetry, action, TargetColor::BLUE);
	precision_landing(passthrough, offboard, telemetry, action, TargetColor::BLUE);

	rclcpp::shutdown();
    return 0;
}
