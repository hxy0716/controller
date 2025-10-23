// anomaly_monitoring_controller.h
#ifndef ANOMALY_MONITORING_CONTROLLER_H
#define ANOMALY_MONITORING_CONTROLLER_H

#include <vector>
#include <string>
#include <chrono>
#include <mutex>
#include <atomic>
#include <functional>
#include <map>
#include <deque>

// 异常等级枚举
enum class AnomalyLevel {
    INFO,       // 提示级
    WARNING,    // 一般级
    CRITICAL    // 事故级
};

// 异常类型枚举
enum class AnomalyType {
    DEVICE_FAULT,   // 设备故障
    GRID_FAULT,     // 电网异常
    SAFETY_FAULT    // 安全异常
};

// 异常信息结构体
struct AnomalyInfo {
    AnomalyType type;                               // 异常类型
    AnomalyLevel level;                             // 异常等级
    std::string device_id;                          // 设备标识
    std::string description;                        // 异常描述
    std::chrono::system_clock::time_point start_time; // 异常开始时间
    std::chrono::system_clock::time_point end_time;   // 异常结束时间
    bool is_handled;                                // 是否已处理
    bool needs_manual_confirmation;                 // 是否需要人工确认
};

// 系统状态结构体
struct SystemStatus {
    double pv_power;                 // 光伏功率(kW)
    double wind_power;               // 风电功率(kW)
    double ess_power;                // 储能功率(kW)
    double hydrogen_power;           // 制氢功率(kW)
    double grid_voltage;             // 电网电压(V)
    double grid_frequency;           // 电网频率(Hz)
    double hydrogen_concentration;   // 氢浓度(%)
    double hydrogen_tank_pressure;   // 氢罐压力(MPa)
    bool pv_inverter_fault;          // 光伏逆变器故障标志
    bool wind_controller_fault;      // 风机控制器故障标志
    bool ess_pcs_fault;              // 储能PCS故障标志
    bool electrolyzer_fault;         // 电解槽故障标志
    bool is_island_mode;             // 是否孤岛模式标志
};

// 异常监测控制器类
class AnomalyMonitoringController {
public:
    AnomalyMonitoringController();
    ~AnomalyMonitoringController();
    
    // 初始化系统
    bool initialize();
    
    // 主监测循环
    void runMonitoringLoop();
    
    // 更新系统状态
    void updateSystemStatus(const SystemStatus& status);
    
    // 确认安全异常恢复
    void confirmSafetyAnomalyRecovery(const std::string& anomaly_id);
    
    // 设置控制参数
    void setControlParameters(double normal_voltage,
                             double normal_frequency,
                             double max_hydrogen_concentration,
                             double max_hydrogen_pressure,
                             int anomaly_duration_threshold_ms);
    
    // 功能使能控制
    void enableMonitoring(bool enabled);
    
    // 回调函数类型定义
    using StatusCallback = std::function<void(const std::string& status)>;
    using ControlCallback = std::function<void(const std::string& device, double power)>;
    using SafetyCallback = std::function<void(const std::string& action)>;
    
    // 注册回调函数
    void setStatusCallback(StatusCallback callback);
    void setControlCallback(ControlCallback callback);
    void setSafetyCallback(SafetyCallback callback);

private:
    // 内部方法
    void checkAnomalies();                          // 检查异常
    void handleAnomaly(const AnomalyInfo& anomaly); // 处理异常
    void handleAnomalyRecovery(const AnomalyInfo& anomaly); // 处理异常恢复
    AnomalyLevel determineAnomalyLevel(AnomalyType type, double value); // 确定异常等级
    bool isAnomalyResolved(const AnomalyInfo& anomaly); // 检查异常是否已解决
    
    // 成员变量
    std::atomic<bool> enabled_;                     // 监测使能标志
    
    SystemStatus current_status_;                   // 当前系统状态
    std::mutex status_mutex_;                       // 状态数据互斥锁
    
    std::map<std::string, AnomalyInfo> active_anomalies_; // 活动异常映射表
    std::deque<AnomalyInfo> anomaly_history_;       // 异常历史记录
    std::mutex anomaly_mutex_;                      // 异常数据互斥锁
    
    // 控制参数（原子操作保证线程安全）
    std::atomic<double> normal_voltage_;            // 正常电压值(V)
    std::atomic<double> normal_frequency_;          // 正常频率值(Hz)
    std::atomic<double> max_hydrogen_concentration_; // 最大氢浓度(%)
    std::atomic<double> max_hydrogen_pressure_;     // 最大氢罐压力(MPa)
    std::atomic<int> anomaly_duration_threshold_ms_; // 异常持续时间阈值(ms)
    
    std::mutex callback_mutex_;                     // 回调函数互斥锁
    StatusCallback status_callback_;                // 状态回调函数
    ControlCallback control_callback_;              // 控制回调函数
    SafetyCallback safety_callback_;                // 安全回调函数
    
    std::atomic<bool> running_;                     // 运行状态标志
};

#endif // ANOMALY_MONITORING_CONTROLLER_H