cmake_minimum_required(VERSION 3.8)
project(drone_kontrol)

set(CMAKE_CXX_STANDARD 17)

# ROS 2 ve Diğer Kütüphaneleri Bul
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(MAVSDK REQUIRED)

# Tüm cpp dosyalarını derle
add_executable(drone_kontrol
  main.cpp
  mission_1.cpp
  vision_module.cpp
)

# Kendi başlık dosyalarının (c_library_v2 vs) yolunu göster
target_include_directories(drone_kontrol PRIVATE ./c_library_v2/common)

# ROS 2 Kütüphanelerini Bağla
ament_target_dependencies(drone_kontrol
  rclcpp
  sensor_msgs
  cv_bridge
)

# MAVSDK'yı Bütün Olarak (Tek Parça) Bağla
target_link_libraries(drone_kontrol
  MAVSDK::mavsdk
  ${OpenCV_LIBS}
)
