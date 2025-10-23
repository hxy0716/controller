cmake_minimum_required(VERSION 3.10)
project(AnomalyMonitoringController)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
add_executable(anomaly_monitoring_controller 
    main.cpp 
    anomaly_monitoring_controller.cpp
    anomaly_monitoring_controller.h
)

# 设置包含目录（确保可以找到头文件）
target_include_directories(anomaly_monitoring_controller PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 设置编译选项
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(anomaly_monitoring_controller PRIVATE -Wall -Wextra -O2)
elseif(MSVC)
    target_compile_options(anomaly_monitoring_controller PRIVATE /W4 /O2)
endif()