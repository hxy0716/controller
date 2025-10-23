// anomaly_monitoring_controller.cpp
#include "anomaly_monitoring_controller.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>

// 常量定义
constexpr std::chrono::milliseconds MONITORING_INTERVAL(100); // 监测间隔100ms

// 构造函数
AnomalyMonitoringController::AnomalyMonitoringController() 
    : enabled_(false),
      normal_voltage_(220.0),
      normal_frequency_(50.0),
      max_hydrogen_concentration_(1.0),
      max_hydrogen_pressure_(1.5),
      anomaly_duration_threshold_ms_(5000), // 5秒
      running_(false) {}

// 析构函数
AnomalyMonitoringController::~AnomalyMonitoringController() {
    running_ = false; // 停止运行标志
}

// 初始化系统
bool AnomalyMonitoringController::initialize() {
    running_ = true; // 设置运行标志
    return true;
}

// 1.主监测循环
void AnomalyMonitoringController::runMonitoringLoop() {
    while (running_) {
        // 1.检查是否使能
        if (!enabled_) {
            std::this_thread::sleep_for(std::chrono::seconds(1)); // 未使能时休眠1秒
            continue;
        }
        // 2.检查异常
        checkAnomalies();
        // 3.检查异常恢复
        std::vector<std::string> resolved_anomalies; // 已解决的异常列表
        {
            std::lock_guard<std::mutex> lock(anomaly_mutex_); // 加锁保护异常数据
            for (auto& [id, anomaly] : active_anomalies_) {
                if (isAnomalyResolved(anomaly)) { // 检查异常是否已解决
                    resolved_anomalies.push_back(id); // 添加到已解决列表
                    anomaly.end_time = std::chrono::system_clock::now(); // 设置结束时间
                    anomaly_history_.push_back(anomaly); // 添加到历史记录
                    
                    if (status_callback_) {
                        status_callback_("异常已解除: " + anomaly.description); // 回调通知
                    }
                }
            }
            // 移除已解除的异常
            for (const auto& id : resolved_anomalies) {
                active_anomalies_.erase(id); // 从活动异常中移除
                
                // 处理异常恢复
                auto it = std::find_if(anomaly_history_.begin(), anomaly_history_.end(),
                                      [&](const AnomalyInfo& a) { return a.description == id; });
                if (it != anomaly_history_.end()) {
                    handleAnomalyRecovery(*it); // 处理异常恢复
                }
            }
        }
        std::this_thread::sleep_for(MONITORING_INTERVAL); // 按监测间隔100s休眠
    }
}

// 2.检查异常
void AnomalyMonitoringController::checkAnomalies() {
    SystemStatus status;
    {
        std::lock_guard<std::mutex> lock(status_mutex_); // 加锁获取当前状态
        status = current_status_;
    }
    
    auto now = std::chrono::system_clock::now(); // 当前时间
    
    // 检查设备故障
    if (status.pv_inverter_fault) {
        AnomalyInfo anomaly;
        anomaly.type = AnomalyType::DEVICE_FAULT;
        anomaly.level = AnomalyLevel::CRITICAL;
        anomaly.device_id = "PV_Inverter";
        anomaly.description = "光伏逆变器故障";
        anomaly.start_time = now;
        anomaly.is_handled = false;
        anomaly.needs_manual_confirmation = false;
        
        handleAnomaly(anomaly); // 处理异常
    }
    
    if (status.wind_controller_fault) {
        AnomalyInfo anomaly;
        anomaly.type = AnomalyType::DEVICE_FAULT;
        anomaly.level = AnomalyLevel::CRITICAL;
        anomaly.device_id = "Wind_Controller";
        anomaly.description = "风机控制器故障";
        anomaly.start_time = now;
        anomaly.is_handled = false;
        anomaly.needs_manual_confirmation = false;
        
        handleAnomaly(anomaly);
    }
    
    if (status.ess_pcs_fault) {
        AnomalyInfo anomaly;
        anomaly.type = AnomalyType::DEVICE_FAULT;
        anomaly.level = AnomalyLevel::CRITICAL;
        anomaly.device_id = "ESS_PCS";
        anomaly.description = "储能PCS故障";
        anomaly.start_time = now;
        anomaly.is_handled = false;
        anomaly.needs_manual_confirmation = false;
        
        handleAnomaly(anomaly);
    }
    
    if (status.electrolyzer_fault) {
        AnomalyInfo anomaly;
        anomaly.type = AnomalyType::DEVICE_FAULT;
        anomaly.level = AnomalyLevel::CRITICAL;
        anomaly.device_id = "Electrolyzer";
        anomaly.description = "电解槽故障";
        anomaly.start_time = now;
        anomaly.is_handled = false;
        anomaly.needs_manual_confirmation = false;
        
        handleAnomaly(anomaly);
    }
    
    // 检查电网电压异常
    if (status.grid_voltage >= 1.1 * normal_voltage_ || status.grid_voltage <= 0.9 * normal_voltage_) {
        AnomalyInfo anomaly;
        anomaly.type = AnomalyType::GRID_FAULT;
        anomaly.level = determineAnomalyLevel(AnomalyType::GRID_FAULT, status.grid_voltage);
        anomaly.device_id = "Grid";
        anomaly.description = "电网电压异常: " + std::to_string(status.grid_voltage) + "V";
        anomaly.start_time = now;
        anomaly.is_handled = false;
        anomaly.needs_manual_confirmation = false;
        
        handleAnomaly(anomaly);
    }
    // 检查电网频率异常
    if (status.grid_frequency >= 50.5 || status.grid_frequency <= 49.5) {
        AnomalyInfo anomaly;
        anomaly.type = AnomalyType::GRID_FAULT;
        anomaly.level = determineAnomalyLevel(AnomalyType::GRID_FAULT, status.grid_frequency);
        anomaly.device_id = "Grid";
        anomaly.description = "电网频率异常: " + std::to_string(status.grid_frequency) + "Hz";
        anomaly.start_time = now;
        anomaly.is_handled = false;
        anomaly.needs_manual_confirmation = false;
        
        handleAnomaly(anomaly);
    }
    
    // 检查安全异常
    //检查氢浓度
    if (status.hydrogen_concentration >= max_hydrogen_concentration_) {
        AnomalyInfo anomaly;
        anomaly.type = AnomalyType::SAFETY_FAULT;
        anomaly.level = determineAnomalyLevel(AnomalyType::SAFETY_FAULT, status.hydrogen_concentration);
        anomaly.device_id = "Hydrogen_System";
        anomaly.description = "氢浓度异常: " + std::to_string(status.hydrogen_concentration) + "%";
        anomaly.start_time = now;
        anomaly.is_handled = false;
        anomaly.needs_manual_confirmation = true;
        
        handleAnomaly(anomaly);
    }
    //检查氢罐压力
    if (status.hydrogen_tank_pressure >= max_hydrogen_pressure_) {
        AnomalyInfo anomaly;
        anomaly.type = AnomalyType::SAFETY_FAULT;
        anomaly.level = determineAnomalyLevel(AnomalyType::SAFETY_FAULT, status.hydrogen_tank_pressure);
        anomaly.device_id = "Hydrogen_System";
        anomaly.description = "氢罐压力异常: " + std::to_string(status.hydrogen_tank_pressure) + "MPa";
        anomaly.start_time = now;
        anomaly.is_handled = false;
        anomaly.needs_manual_confirmation = true;
        
        handleAnomaly(anomaly);
    }
}

// 3.处理异常
void AnomalyMonitoringController::handleAnomaly(const AnomalyInfo& anomaly) {
    std::lock_guard<std::mutex> lock(anomaly_mutex_); // 加锁保护异常数据
    // 检查是否已存在相同异常
    if (active_anomalies_.find(anomaly.description) != active_anomalies_.end()) {
        return; // 异常已存在，不重复处理
    }
    // 添加新异常
    active_anomalies_[anomaly.description] = anomaly;
    // 检查异常持续时间
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - anomaly.start_time);//异常持续时间
    if (duration.count() >= anomaly_duration_threshold_ms_) {
        // 根据异常等级处理
        switch (anomaly.level) {
            case AnomalyLevel::INFO:
                // 提示级：仅记录和通知
                if (status_callback_) {
                    status_callback_("提示级异常: " + anomaly.description);
                }
                break;
                
            case AnomalyLevel::WARNING:
                // 一般级：设备功率减半
                if (anomaly.device_id == "PV_Inverter") {
                    double new_power = current_status_.pv_power * 0.5;
                    if (control_callback_) {
                        control_callback_("PV", new_power);
                    }
                } else if (anomaly.device_id == "Wind_Controller") {
                    double new_power = current_status_.wind_power * 0.5;
                    if (control_callback_) {
                        control_callback_("WIND", new_power);
                    }
                } else if (anomaly.device_id == "ESS_PCS") {
                    double new_power = current_status_.ess_power * 0.5;
                    if (control_callback_) {
                        control_callback_("ESS", new_power);
                    }
                }
                
                if (status_callback_) {
                    status_callback_("一般级异常: " + anomaly.description + ", 设备功率减半");
                }
                break;
                
            case AnomalyLevel::CRITICAL:
                // 事故级：设备停机或特殊处理
                if (anomaly.device_id == "PV_Inverter") {
                    if (control_callback_) {
                        control_callback_("PV", 0);
                    }
                } else if (anomaly.device_id == "Wind_Controller") {
                    if (control_callback_) {
                        control_callback_("WIND", 0);
                    }
                } else if (anomaly.device_id == "ESS_PCS") {
                    if (control_callback_) {
                        control_callback_("ESS", 0);
                    }
                } else if (anomaly.device_id == "Electrolyzer") {
                    if (control_callback_) {
                        control_callback_("HYDROGEN", 0);
                    }
                } else if (anomaly.device_id == "Grid") {
                    if (control_callback_) {
                        control_callback_("GRID", 0);
                    }
                    
                    if (current_status_.is_island_mode) {
                        if (status_callback_) {
                            status_callback_("孤岛模式，保障重要负荷");
                        }
                    }
                } else if (anomaly.device_id == "Hydrogen_System") {
                    if (control_callback_) {
                        control_callback_("HYDROGEN", 0);
                    }
                    
                    if (anomaly.description.find("氢浓度异常") != std::string::npos) {
                        if (safety_callback_) {
                            safety_callback_("启动通风系统");
                        }
                    } else if (anomaly.description.find("氢罐压力异常") != std::string::npos) {
                        if (safety_callback_) {
                            safety_callback_("启动泄压系统");
                        }
                    }
                    
                    if (safety_callback_) {
                        safety_callback_("转入保安全模式");
                    }
                }
                
                if (status_callback_) {
                    status_callback_("事故级异常: " + anomaly.description + ", 设备已停机");
                }
                break;
        }
        // 标记为已处理
        active_anomalies_[anomaly.description].is_handled = true;
    }
}

// 4.处理异常恢复
void AnomalyMonitoringController::handleAnomalyRecovery(const AnomalyInfo& anomaly) {
    // 安全异常需要人工确认
    if (anomaly.type == AnomalyType::SAFETY_FAULT && anomaly.needs_manual_confirmation) {
        if (status_callback_) {
            status_callback_("安全异常恢复需要人工确认: " + anomaly.description);
        }
        return;
    }
    
    // 按5%/分钟速率恢复系统出力
    if (status_callback_) {
        status_callback_("开始恢复系统出力: " + anomaly.description);
    }
    
   // 计算恢复速率 (5%/分钟 = 0.0833%/秒)
   double recovery_rate_per_second = 5.0 / 60.0;
    
   if (anomaly.device_id == "PV_Inverter") {
       double target_power = 100.0; // 假设额定功率100kW
       double current_power = 0.0;
       
       while (current_power < target_power) {
           double increment = target_power * (recovery_rate_per_second / 100.0);
           current_power = std::min(current_power + increment, target_power);
           
           if (control_callback_) {
               control_callback_("PV", current_power);
           }
           
           std::this_thread::sleep_for(std::chrono::seconds(1));
       }
   } else if (anomaly.device_id == "Wind_Controller") {
       double target_power = 100.0; // 假设风机额定功率100kW
       double current_power = 0.0;
       
       while (current_power < target_power) {
           double increment = target_power * (recovery_rate_per_second / 100.0);
           current_power = std::min(current_power + increment, target_power);
           
           if (control_callback_) {
               control_callback_("WIND", current_power);
           }
           
           std::this_thread::sleep_for(std::chrono::seconds(1));
       }
   } else if (anomaly.device_id == "ESS_PCS") {
       double target_power = 100.0; // 假设储能额定功率100kW
       double current_power = 0.0;
       
       while (current_power < target_power) {
           double increment = target_power * (recovery_rate_per_second / 100.0);
           current_power = std::min(current_power + increment, target_power);
           
           if (control_callback_) {
               control_callback_("ESS", current_power);
           }
           
           std::this_thread::sleep_for(std::chrono::seconds(1));
       }
   } else if (anomaly.device_id == "Electrolyzer") {
       double target_power = 100.0; // 假设电解槽额定功率100kW
       double current_power = 0.0;
       
       while (current_power < target_power) {
           double increment = target_power * (recovery_rate_per_second / 100.0);
           current_power = std::min(current_power + increment, target_power);
           
           if (control_callback_) {
               control_callback_("HYDROGEN", current_power);
           }
           
           std::this_thread::sleep_for(std::chrono::seconds(1));
       }
   } else if (anomaly.device_id == "Grid") {
       // 电网恢复：先闭合并网开关，再逐步恢复功率
       if (safety_callback_) {
           safety_callback_("闭合并网开关");
       }
       
       double target_power = 100.0; // 假设额定上网功率100kW
       double current_power = 0.0;
       
       while (current_power < target_power) {
           double increment = target_power * (recovery_rate_per_second / 100.0);
           current_power = std::min(current_power + increment, target_power);
           
           if (control_callback_) {
               control_callback_("GRID", current_power);
           }
           
           std::this_thread::sleep_for(std::chrono::seconds(1));
       }
       
       if (status_callback_) {
           status_callback_("电网恢复完成，已并网并恢复正常功率输出");
       }
   } else if (anomaly.device_id == "Hydrogen_System") {
       double target_power = 100.0; // 假设制氢系统额定功率100kW
       double current_power = 0.0;
       
       while (current_power < target_power) {
           double increment = target_power * (recovery_rate_per_second / 100.0);
           current_power = std::min(current_power + increment, target_power);
           
           if (control_callback_) {
               control_callback_("HYDROGEN", current_power);
           }
           
           std::this_thread::sleep_for(std::chrono::seconds(1));
       }
   }
   
   if (status_callback_) {
       status_callback_("系统出力恢复完成: " + anomaly.description);
   }
    
 
}

// 5.确定异常等级
AnomalyLevel AnomalyMonitoringController::determineAnomalyLevel(AnomalyType type, double value) {
    switch (type) {
        case AnomalyType::DEVICE_FAULT:
            return AnomalyLevel::CRITICAL; // 设备故障均为事故级
            
        case AnomalyType::GRID_FAULT:
            // 电网异常根据偏离程度确定等级
            if (value >= 1.15 * normal_voltage_ || value <= 0.85 * normal_voltage_ ||
                value >= 51.0 || value <= 49.0) {
                return AnomalyLevel::CRITICAL;
            } else if (value >= 1.1 * normal_voltage_ || value <= 0.9 * normal_voltage_ ||
                       value >= 50.5 || value <= 49.5) {
                return AnomalyLevel::WARNING;
            } else {
                return AnomalyLevel::INFO;
            }
            
        case AnomalyType::SAFETY_FAULT:
            // 安全异常根据偏离程度确定等级
            if (value >= 2.0 * max_hydrogen_concentration_ || value >= 2.0 * max_hydrogen_pressure_) {
                return AnomalyLevel::CRITICAL;
            } else if (value >= 1.5 * max_hydrogen_concentration_ || value >= 1.5 * max_hydrogen_pressure_) {
                return AnomalyLevel::WARNING;
            } else {
                return AnomalyLevel::INFO;
            }
    }
    
    return AnomalyLevel::INFO; // 默认返回提示级
}

// 6.检查异常是否已解决
bool AnomalyMonitoringController::isAnomalyResolved(const AnomalyInfo& anomaly) {
    SystemStatus status;
    {
        std::lock_guard<std::mutex> lock(status_mutex_); // 加锁获取当前状态
        status = current_status_;
    }
    
    // 根据异常类型检查是否已解决
    switch (anomaly.type) {
        case AnomalyType::DEVICE_FAULT:
            if (anomaly.device_id == "PV_Inverter") {
                return !status.pv_inverter_fault;
            } else if (anomaly.device_id == "Wind_Controller") {
                return !status.wind_controller_fault;
            } else if (anomaly.device_id == "ESS_PCS") {
                return !status.ess_pcs_fault;
            } else if (anomaly.device_id == "Electrolyzer") {
                return !status.electrolyzer_fault;
            }
            break;
            
        case AnomalyType::GRID_FAULT:
            if (anomaly.description.find("电压异常") != std::string::npos) {
                return status.grid_voltage >= 0.9 * normal_voltage_ && 
                       status.grid_voltage <= 1.1 * normal_voltage_;
            } else if (anomaly.description.find("频率异常") != std::string::npos) {
                return status.grid_frequency >= 49.5 && status.grid_frequency <= 50.5;
            }
            break;
            
        case AnomalyType::SAFETY_FAULT:
            if (anomaly.description.find("氢浓度异常") != std::string::npos) {
                return status.hydrogen_concentration < max_hydrogen_concentration_;
            } else if (anomaly.description.find("氢罐压力异常") != std::string::npos) {
                return status.hydrogen_tank_pressure < max_hydrogen_pressure_;
            }
            break;
    }
    
    return false; // 默认返回未解决
}

// 7.更新系统状态
void AnomalyMonitoringController::updateSystemStatus(const SystemStatus& status) {
    std::lock_guard<std::mutex> lock(status_mutex_); // 加锁更新状态
    current_status_ = status;
}

// 8.确认安全异常恢复
void AnomalyMonitoringController::confirmSafetyAnomalyRecovery(const std::string& anomaly_id) {
    std::lock_guard<std::mutex> lock(anomaly_mutex_); // 加锁保护异常数据
    
    for (auto& [id, anomaly] : active_anomalies_) {
        if (anomaly.description == anomaly_id && anomaly.type == AnomalyType::SAFETY_FAULT) {
            anomaly.needs_manual_confirmation = false; // 标记为已确认
            
            if (status_callback_) {
                status_callback_("安全异常恢复已确认: " + anomaly.description);
            }
            
            // 处理异常恢复
            anomaly.end_time = std::chrono::system_clock::now();
            anomaly_history_.push_back(anomaly);
            active_anomalies_.erase(id);
            
            handleAnomalyRecovery(anomaly); // 处理恢复过程
            break;
        }
    }
}

// 设置控制参数
void AnomalyMonitoringController::setControlParameters(double normal_voltage,
                                                     double normal_frequency,
                                                     double max_hydrogen_concentration,
                                                     double max_hydrogen_pressure,
                                                     int anomaly_duration_threshold_ms) {
    normal_voltage_ = normal_voltage;
    normal_frequency_ = normal_frequency;
    max_hydrogen_concentration_ = max_hydrogen_concentration;
    max_hydrogen_pressure_ = max_hydrogen_pressure;
    anomaly_duration_threshold_ms_ = anomaly_duration_threshold_ms;
}

// 使能监测功能
void AnomalyMonitoringController::enableMonitoring(bool enabled) {
    enabled_ = enabled;
}

// 设置状态回调函数
void AnomalyMonitoringController::setStatusCallback(StatusCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    status_callback_ = callback;
}

// 设置控制回调函数
void AnomalyMonitoringController::setControlCallback(ControlCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    control_callback_ = callback;
}

// 设置安全回调函数
void AnomalyMonitoringController::setSafetyCallback(SafetyCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    safety_callback_ = callback;
}