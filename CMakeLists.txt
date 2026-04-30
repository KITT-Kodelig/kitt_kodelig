cmake_minimum_required(VERSION 3.10)
project(kodelig_otonom_sistem)

# C++17 Standardı
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# 1. DIŞ BAĞIMLILIKLAR (ROS 2 & OpenCV)
# ---------------------------------------------------------
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)

# ---------------------------------------------------------
# 2. MAVLINK BAŞLIK DOSYALARI (MAVSDK yerine)
# ---------------------------------------------------------
# Not: MAVLink kütüphanesi derleme gerektirmez, sadece include edilir.
# Sisteminize göre bu yolu güncelleyin. Varsayılan olarak yaygın kullanılan yollar eklenmiştir.
set(MAVLINK_INCLUDE_DIR "/usr/local/include/mavlink/v2.0" CACHE PATH "Path to MAVLink headers")

# ---------------------------------------------------------
# 3. ÇALIŞTIRILABİLİR DOSYA VE KAYNAKLAR
# ---------------------------------------------------------
add_executable(otonom_node
    main.cpp
    mission_1.cpp
    vision_module.cpp
)

# ---------------------------------------------------------
# 4. INCLUDE DİZİNLERİ
# ---------------------------------------------------------
target_include_directories(otonom_node PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    ${MAVLINK_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---------------------------------------------------------
# 5. KÜTÜPHANELERİ BAĞLA (Linking)
# ---------------------------------------------------------
# ROS 2 bağımlılıkları
ament_target_dependencies(otonom_node
    rclcpp
    sensor_msgs
    cv_bridge
)

# OpenCV ve Sistem kütüphaneleri (MAVSDK artık burada yok)
target_link_libraries(otonom_node
    ${OpenCV_LIBS}
    pthread
)

# ---------------------------------------------------------
# 6. YÜKLEME VE PAKETLEME
# ---------------------------------------------------------
install(TARGETS otonom_node DESTINATION lib/${PROJECT_NAME})
ament_package()
