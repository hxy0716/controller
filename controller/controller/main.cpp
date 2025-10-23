// test_main.cpp
#include "anomaly_monitoring_controller.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>

// 示例回调函数实现
void statusCallback(const std::string& status) {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;
    
    std::cout << "[" << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S")
              << "." << std::setw(3) << std::setfill('0') << now_ms.count() << "] "
              << "状态: " << status << std::endl;
}

void controlCallback(const std::string& device, double power) {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;
    
    std::cout << "[" << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S")
              << "." << std::setw(3) << std::setfill('0') << now_ms.count() << "] "
              << "控制 " << device << " 功率为: " << power << " kW" << std::endl;
}

void safetyCallback(const std::string& action) {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;
    
    std::cout << "[" << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S")
              << "." << std::setw(3) << std::setfill('0') << now_ms.count() << "] "
              << "安全操作: " << action << std::endl;
}

// 生成格式化的时间字符串
std::string currentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S")
       << "." << std::setw(3) << std::setfill('0') << now_ms.count() << "] ";
    return ss.str();
}

int main() {
    std::cout << currentTimeString() << "开始测试异常监测控制器..." << std::endl;
    
    AnomalyMonitoringController controller;

    // 初始化系统
    if (!controller.initialize()) {
        std::cerr << currentTimeString() << "初始化异常监测控制器失败!" << std::endl;
        return 1;
    }
    std::cout << currentTimeString() << "异常监测控制器初始化成功" << std::endl;

    // 设置回调函数
    controller.setStatusCallback(statusCallback);
    controller.setControlCallback(controlCallback);
    controller.setSafetyCallback(safetyCallback);
    std::cout << currentTimeString() << "回调函数设置完成" << std::endl;

    // 设置控制参数
    controller.setControlParameters(
        220.0,  // 额定电压220V
        50.0,   // 额定频率50Hz
        1.0,    // 最大氢浓度1%
        1.5,    // 最大氢罐压力1.5MPa
        5000    // 异常持续时间阈值5秒
    );
    std::cout << currentTimeString() << "控制参数设置完成" << std::endl;

    // 使能监测
    controller.enableMonitoring(true);
    std::cout << currentTimeString() << "监测功能已启用" << std::endl;

    // 启动监测循环线程
    std::thread monitoringThread([&controller]() {
        std::cout << currentTimeString() << "启动监测循环线程" << std::endl;
        controller.runMonitoringLoop();
        std::cout << currentTimeString() << "监测循环线程结束" << std::endl;
    });

    // 模拟实时数据更新
    std::cout << currentTimeString() << "开始模拟数据更新..." << std::endl;
    
    for (int i = 0; i < 120; i++) { // 模拟2分钟的运行
        SystemStatus status;
        
        // 基础正常状态
        status.pv_power = 80.0;
        status.wind_power = 60.0;
        status.ess_power = 40.0;
        status.hydrogen_power = 20.0;
        status.grid_voltage = 220.0;
        status.grid_frequency = 50.0;
        status.hydrogen_concentration = 0.5;
        status.hydrogen_tank_pressure = 1.0;
        status.pv_inverter_fault = false;
        status.wind_controller_fault = false;
        status.ess_pcs_fault = false;
        status.electrolyzer_fault = false;
        status.is_island_mode = false;

        // 模拟不同时间点的异常情况
        if (i >= 10 && i < 20) {
            // 第10-20秒：光伏逆变器故障
            status.pv_inverter_fault= true;
            std::cout << currentTimeString() << "模拟光伏逆变器故障" << std::endl;
        }
        
        if (i >= 30 && i < 40) {
            // 第30-40秒：电网电压异常
            status.grid_voltage = 250.0; // 超出正常范围
            std::cout << currentTimeString() << "模拟电网电压异常: " << status.grid_voltage << "V" << std::endl;
        }
        
        if (i >= 50 && i < 60) {
            // 第50-60秒：电网频率异常
            status.grid_frequency = 51.0; // 超出正常范围
            std::cout << currentTimeString() << "模拟电网频率异常: " << status.grid_frequency << "Hz" << std::endl;
        }
        
        if (i >= 70 && i < 80) {
            // 第70-80秒：氢浓度异常
            status.hydrogen_concentration = 1.2; // 超出正常范围
            std::cout << currentTimeString() << "模拟氢浓度异常: " << status.hydrogen_concentration << "%" << std::endl;
        }
        
        if (i >= 90 && i < 100) {
            // 第90-100秒：氢罐压力异常
            status.hydrogen_tank_pressure = 2.0; // 超出正常范围
            std::cout << currentTimeString() << "模拟氢罐压力异常: " << status.hydrogen_tank_pressure << "MPa" << std::endl;
        }
        
        if (i == 105) {
            // 第105秒：模拟人工确认安全异常恢复
            std::cout << currentTimeString() << "模拟人工确认安全异常恢复" << std::endl;
            controller.confirmSafetyAnomalyRecovery("氢浓度异常: 1.2%");
            controller.confirmSafetyAnomalyRecovery("氢罐压力异常: 2.0MPa");
        }

        // 更新系统状态
        controller.updateSystemStatus(status);
        
        // 每秒更新一次
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // 显示进度
        if (i % 10 == 0) {
            std::cout << currentTimeString() << "已运行 " << i << " 秒" << std::endl;
        }
    }

    // 停止系统
    std::cout << currentTimeString() << "停止监测系统..." << std::endl;
    controller.enableMonitoring(false);
    
    // 等待监测线程结束
    if (monitoringThread.joinable()) {
        monitoringThread.join();
    }
    
    std::cout << currentTimeString() << "测试完成" << std::endl;
    return 0;
}