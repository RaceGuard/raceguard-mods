#pragma once

#include <cstdint>
#include <cmath>

// 数据来源标识
enum DataSource : uint8_t {
    DATA_SIMULATED  = 0,
    DATA_BLE_OBD    = 1,
    DATA_CAN_DIRECT = 2
};

// 无数据标记
#define NO_DATA_U8   0xFF
#define NO_DATA_U16  0xFFFF

// 车辆遥测数据结构（按采样优先级排列）
struct CarData {
    uint32_t timestamp;         // 时间戳（毫秒）
    DataSource source;          // 数据来源

    // === P0 Critical — 每轮必查 ===
    uint16_t rpm;                       // 发动机转速
    float intake_manifold_pressure;     // 进气歧管压力 (kPa)

    // === P1 Dynamic — 2:1 采样 ===
    float timing_advance;       // 点火提前角 (°)
    float short_fuel_trim_b1;   // 短期燃油修正 Bank1 (%)
    float short_fuel_trim_b2;   // 短期燃油修正 Bank2 (%)
    float throttle_pos;         // 节气门位置 (%)
    uint8_t vehicle_speed;      // 车速 (km/h)

    // === P1 Dynamic 扩展 ===
    float accel_pedal_d;        // 油门踏板位置 D (%, 0x49)

    // === P2 Status — 4:1 采样 ===
    float engine_load;          // 发动机负载 (%)
    float maf_rate;             // 进气流量 (g/s, 0x10 或 0x66)
    float coolant_temp;         // 冷却液温度 (°C)
    float air_fuel_ratio;       // 空燃比 (lambda)
    float commanded_throttle;   // 命令节气门开度 (%)
    float fuel_pressure;        // 燃油压力 (kPa)
    float absolute_load;        // 绝对负荷值 (%)

    // === P3 Thermal — 10:1 采样 ===
    float oil_temp;             // 机油温度 (°C)
    float intake_temp;          // 进气温度 (°C)
    float battery_voltage;      // 电池电压 (V)
    float fuel_level;           // 剩余油量 (%)
    float long_fuel_trim_b1;    // 长期燃油修正 Bank1 (%)
    float long_fuel_trim_b2;    // 长期燃油修正 Bank2 (%)
    float barometric_pressure;  // 大气压力 (kPa)
    float ambient_temp;         // 环境温度 (°C)
    float accel_pedal_e;        // 油门踏板位置 E (%, 0x4A, 冗余传感器)
    float catalyst_temp_b1s1;   // 催化器温度 B1S1 (°C, 0x3C)
    float catalyst_temp_b2s1;   // 催化器温度 B2S1 (°C, 0x3D)
    uint8_t dtc_count;          // 故障码数量
    uint16_t dtc_codes[16];     // 故障码列表（最多 16 个）

    // === 派生值 ===
    float boost_pressure;       // 涡轮增压压力 (kPa, 歧管压力-大气压)

    // 默认构造 — 所有值初始化为"无数据"
    CarData() : timestamp(0), source(DATA_SIMULATED),
                // P0
                rpm(NO_DATA_U16), intake_manifold_pressure(NAN),
                // P1
                timing_advance(NAN),
                short_fuel_trim_b1(NAN), short_fuel_trim_b2(NAN),
                throttle_pos(NAN), vehicle_speed(NO_DATA_U8),
                accel_pedal_d(NAN),
                // P2
                engine_load(NAN), maf_rate(NAN), coolant_temp(NAN),
                air_fuel_ratio(NAN), commanded_throttle(NAN),
                fuel_pressure(NAN), absolute_load(NAN),
                // P3
                oil_temp(NAN), intake_temp(NAN), battery_voltage(NAN),
                fuel_level(NAN), long_fuel_trim_b1(NAN), long_fuel_trim_b2(NAN),
                barometric_pressure(NAN), ambient_temp(NAN),
                accel_pedal_e(NAN), catalyst_temp_b1s1(NAN), catalyst_temp_b2s1(NAN),
                dtc_count(NO_DATA_U8), dtc_codes{},
                // 派生
                boost_pressure(NAN) {}

    // 判断字段是否有数据
    static bool hasValue(float v) { return !std::isnan(v); }
    static bool hasValue(uint8_t v) { return v != NO_DATA_U8; }
    static bool hasValue(uint16_t v) { return v != NO_DATA_U16; }
};

// 本次会话峰值追踪（重启或 OBD 重连时重置）
struct SessionPeaks {
    float stft_b1_max;      // STFT B1 最大正偏差
    float stft_b1_min;      // STFT B1 最大负偏差
    float stft_b2_max;
    float stft_b2_min;
    float timing_min;       // 点火提前角最小值（最大退角=最大爆震保护）
    float timing_max;       // 点火提前角最大值
    float afr_min;          // 空燃比最小值
    float afr_max;          // 空燃比最大值

    SessionPeaks() { reset(); }

    void reset() {
        stft_b1_max = NAN;
        stft_b1_min = NAN;
        stft_b2_max = NAN;
        stft_b2_min = NAN;
        timing_min = NAN;
        timing_max = NAN;
        afr_min = NAN;
        afr_max = NAN;
    }

    void update(const CarData& d) {
        // STFT B1
        if (CarData::hasValue(d.short_fuel_trim_b1)) {
            if (!CarData::hasValue(stft_b1_max) || d.short_fuel_trim_b1 > stft_b1_max)
                stft_b1_max = d.short_fuel_trim_b1;
            if (!CarData::hasValue(stft_b1_min) || d.short_fuel_trim_b1 < stft_b1_min)
                stft_b1_min = d.short_fuel_trim_b1;
        }
        // STFT B2
        if (CarData::hasValue(d.short_fuel_trim_b2)) {
            if (!CarData::hasValue(stft_b2_max) || d.short_fuel_trim_b2 > stft_b2_max)
                stft_b2_max = d.short_fuel_trim_b2;
            if (!CarData::hasValue(stft_b2_min) || d.short_fuel_trim_b2 < stft_b2_min)
                stft_b2_min = d.short_fuel_trim_b2;
        }
        // 点火提前角
        if (CarData::hasValue(d.timing_advance)) {
            if (!CarData::hasValue(timing_min) || d.timing_advance < timing_min)
                timing_min = d.timing_advance;
            if (!CarData::hasValue(timing_max) || d.timing_advance > timing_max)
                timing_max = d.timing_advance;
        }
        // 空燃比
        if (CarData::hasValue(d.air_fuel_ratio)) {
            if (!CarData::hasValue(afr_min) || d.air_fuel_ratio < afr_min)
                afr_min = d.air_fuel_ratio;
            if (!CarData::hasValue(afr_max) || d.air_fuel_ratio > afr_max)
                afr_max = d.air_fuel_ratio;
        }
    }

    // 取 STFT 峰值绝对偏差较大的那个方向
    float stft_b1_peak() const {
        float mx = CarData::hasValue(stft_b1_max) ? stft_b1_max : 0;
        float mn = CarData::hasValue(stft_b1_min) ? stft_b1_min : 0;
        return (mx >= -mn) ? mx : mn;  // 返回绝对值较大的（带符号）
    }
    float stft_b2_peak() const {
        float mx = CarData::hasValue(stft_b2_max) ? stft_b2_max : 0;
        float mn = CarData::hasValue(stft_b2_min) ? stft_b2_min : 0;
        return (mx >= -mn) ? mx : mn;
    }
};
