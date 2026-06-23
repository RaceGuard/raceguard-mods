#pragma once

#include <cstdint>

namespace raceguard::log {

enum class Level {
    NONE    = 0,
    ERROR   = 1,
    WARN    = 2,
    INFO    = 3,
    DEBUG   = 4,
    VERBOSE = 5,
};

void init(uint32_t baud_rate = 115200);
void setLevel(Level level);
Level getLevel();
void write(Level level, const char* format, ...);

}  // namespace raceguard::log

#ifndef RG_LOG_DISABLE
#define RG_LOG_ERROR(fmt, ...) ::raceguard::log::write(::raceguard::log::Level::ERROR, fmt, ##__VA_ARGS__)
#define RG_LOG_WARN(fmt, ...)  ::raceguard::log::write(::raceguard::log::Level::WARN,  fmt, ##__VA_ARGS__)
#define RG_LOG_INFO(fmt, ...)  ::raceguard::log::write(::raceguard::log::Level::INFO,  fmt, ##__VA_ARGS__)
#define RG_LOG_DEBUG(fmt, ...) ::raceguard::log::write(::raceguard::log::Level::DEBUG, fmt, ##__VA_ARGS__)
#else
#define RG_LOG_ERROR(fmt, ...) ((void)0)
#define RG_LOG_WARN(fmt, ...)  ((void)0)
#define RG_LOG_INFO(fmt, ...)  ((void)0)
#define RG_LOG_DEBUG(fmt, ...) ((void)0)
#endif

// 短别名 - UI 框架代码继承自旧 debug.h, 用 LOG_INFO 等而非 RG_LOG_INFO
// 用户写的 mods 代码两者皆可
#ifndef LOG_ERROR
#define LOG_ERROR(fmt, ...) RG_LOG_ERROR(fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  RG_LOG_WARN(fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  RG_LOG_INFO(fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) RG_LOG_DEBUG(fmt, ##__VA_ARGS__)
#endif
