#ifndef NAVIGATION_H
#define NAVIGATION_H

#include <memory>
#include <chrono>
#include <iostream>
#include <thread>
#include <cmath>

// ROS 2 Kütüphaneleri
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>

// MAVSDK ve OpenCV Kütüphaneleri
#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <mavsdk/plugins/offboard/offboard.h>
#include <mavsdk/plugins/mavlink_passthrough/mavlink_passthrough.h>
#include <opencv2/opencv.hpp>
#include <mavlink.h>

#include "config.h" 
#include "vision_module.h"

using namespace mavsdk;
using namespace std::this_thread;
using namespace std::chrono;

// ==========================================
// ROS 2 KAMERA ABONE DÜĞÜMÜ (C++ Versiyonu)
// ==========================================
class CameraNode : public rclcpp::Node {
public:
    cv::Mat latest_frame;
    bool frame_received = false;

    CameraNode() : Node("vision_navigator_cpp") {
        // Python'daki qos_profile_sensor_data'nın C++ karşılığı
        rmw_qos_profile_t custom_qos = rmw_qos_profile_sensor_data;
        auto qos = rclcpp::QoS(rclcpp::QoSInitialization::from_rmw(custom_qos), custom_qos);

        subscription_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/iris/bottom_camera/image_raw", qos,
            [this](const sensor_msgs::msg::Image::SharedPtr msg) {
                try {
                    // ROS mesajını OpenCV Matrisine çevir (Python'daki imgmsg_to_cv2 gibi)
                    latest_frame = cv_bridge::toCvCopy(msg, "bgr8")->image;
                    frame_received = true;
                } catch (cv_bridge::Exception& e) {
                    RCLCPP_ERROR(this->get_logger(), "cv_bridge hatasi: %s", e.what());
                }
            });
    }
private:
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr subscription_;
};

// ==========================================
// YARDIMCI MATEMATİK FONKSİYONLARI
// ==========================================
const double EARTH_RADIUS = 6378137.0; 

inline double get_distance_meters(double lat1, double lon1, double lat2, double lon2) {
    double dlat = (lat2 - lat1) * M_PI / 180.0;
    double dlon = (lon2 - lon1) * M_PI / 180.0;
    double a = std::sin(dlat/2) * std::sin(dlat/2) +
               std::cos(lat1 * M_PI / 180.0) * std::cos(lat2 * M_PI / 180.0) *
               std::sin(dlon/2) * std::sin(dlon/2);
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
    return 6371000.0 * c; 
}

// ==========================================
// 0-3 ARASI MAVSDK UÇUŞ FONKSİYONLARI 
// ==========================================
inline std::shared_ptr<System> connect_to_drone(Mavsdk& mavsdk) {
    std::cout << "[BAĞLANTI] " << CONNECTION_STRING << " adresine baglaniliyor...\n";
    ConnectionResult connection_result = mavsdk.add_any_connection(CONNECTION_STRING);
    if (connection_result != ConnectionResult::Success) return nullptr;
    auto system_opt = mavsdk.first_autopilot(5.0);
    if (!system_opt) return nullptr;
    std::cout << "[BAŞARILI] Drone bulundu ve iletisim kuruldu!\n";
    return system_opt.value();
}

inline bool arm_drone(Action& action) {
    Action::Result result = action.arm();
    return (result == Action::Result::Success);
}

inline bool takeoff_drone(Action& action, float takeoff_alt_m = 1.5f) {
    action.set_takeoff_altitude(takeoff_alt_m);
    Action::Result result = action.takeoff();
    return (result == Action::Result::Success);
}

inline bool goto_local_xy(Action& action, Telemetry& telemetry, double x, double y, float altitude_offset_m = 5.0f) {
    std::cout << "[KOMUT] Yerel X: " << x << "m, Y: " << y << "m konumuna gidiliyor...\n";
    double E = x * std::cos(NORTH_OFFSET) + y * std::sin(NORTH_OFFSET);
    double N = -x * std::sin(NORTH_OFFSET) + y * std::cos(NORTH_OFFSET);
    double start_lat_rad = START_LAT * M_PI / 180.0;
    double delta_lat = (N / EARTH_RADIUS) * (180.0 / M_PI);
    double delta_lon = (E / (EARTH_RADIUS * std::cos(start_lat_rad))) * (180.0 / M_PI);
    double target_lat = START_LAT + delta_lat;
    double target_lon = START_LON + delta_lon;

    auto current_position = telemetry.position();
    float target_altitude_amsl = current_position.absolute_altitude_m + altitude_offset_m;
    
    action.goto_location(target_lat, target_lon, target_altitude_amsl, 0.0f);

    while (true) {
        auto current_pos = telemetry.position();
        double distance = get_distance_meters(current_pos.latitude_deg, current_pos.longitude_deg, target_lat, target_lon);
        if (distance < 0.5) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return true;
}

inline void send_landing_target_msg(mavsdk::MavlinkPassthrough& passthrough, float angle_x, float angle_y, float distance) {
    mavlink_message_t msg;
    mavlink_msg_landing_target_pack(1, 191, &msg, 0, 0, MAV_FRAME_LOCAL_NED, angle_x, angle_y, distance, 0, 0, 0, 0, 0, (const float[4]){0,0,0,0}, 2, 0);
    passthrough.queue_message([msg](MavlinkAddress address, uint8_t channel) { return msg; });
}

// ==========================================
// 4. PRECISION LANDING FONKSİYONU (ROS 2 İLE)
// ==========================================
inline bool precision_landing(mavsdk::MavlinkPassthrough& passthrough, Offboard& offboard, Telemetry& telemetry, Action& action, TargetColor target_color) {
    std::cout << "[KOMUT] Hassas Inis (Surekli Takip) baslatiliyor...\n";
    action.land(); 

    // ROS 2 Düğümümüzü Başlatıyoruz
    auto camera_node = std::make_shared<CameraNode>();
    std::cout << "[KAMERA] ROS 2 üzerinden '/iris/bottom_camera/image_raw' dinleniyor...\n";

    int empty_frame_count = 0;

    while (true) {
        // Cikis sarti: Otopilot motorlari kapatirsa
        if (!telemetry.armed() || telemetry.position().relative_altitude_m < 0.1f) {
            std::cout << "\n\n[BİLGİ] Gorev Basarili: Drone yere indi.\n";
            break;
        }

        // ROS 2 Ağını anlık olarak tara ve yeni paket varsa callback'i tetikle
        rclcpp::spin_some(camera_node);

        if (!camera_node->frame_received || camera_node->latest_frame.empty()) {
            empty_frame_count++;
            if (empty_frame_count % 20 == 0) { 
                std::cout << "\r[UYARI] ROS üzerinden goruntu bekleniyor...     " << std::flush;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        // Frame'i kopyala (İşlem sırasında ROS yeni frame basarsa çakışma olmasın)
        cv::Mat frame = camera_node->latest_frame.clone();

        cv::resize(frame, frame, cv::Size(IMG_WIDTH, IMG_HEIGHT));
        LandingAngles target = VisionModule::getLandingAngles(frame.data, target_color);
        
        float current_alt = telemetry.position().relative_altitude_m;

        if (target.target_found) {
            send_landing_target_msg(passthrough, target.angle_x, target.angle_y, current_alt);
            std::cout << "\r[KİLİT] Hedef takip ediliyor -> X: " << target.angle_x << " Y: " << target.angle_y << "          " << std::flush;
            cv::circle(frame, cv::Point(target.pixel_x, target.pixel_y), 8, cv::Scalar(0, 0, 255), 2);
        } else {
            send_landing_target_msg(passthrough, 0.0f, 0.0f, current_alt);
            std::cout << "\r[ARAMA] Hedef kayip! Duzleme (0,0) komutu basiliyor...  " << std::flush;
        }

        cv::Point center(IMG_WIDTH / 2, IMG_HEIGHT / 2);
        cv::line(frame, cv::Point(center.x - 10, center.y), cv::Point(center.x + 10, center.y), cv::Scalar(0, 255, 0), 1);
        cv::line(frame, cv::Point(center.x, center.y - 10), cv::Point(center.x, center.y + 10), cv::Scalar(0, 255, 0), 1);

        cv::imshow("Drone Analiz (ROS 2)", frame);
        if (cv::waitKey(1) == 27) break; 
    }
    
    cv::destroyAllWindows();
    return true;
}

#endif
