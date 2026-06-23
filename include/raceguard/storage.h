#pragma once

#include "../../src/types/car_data.h"

namespace raceguard::storage {

bool init();                            // SD 卡 + LittleFS 初始化
bool isReady();                         // SD 卡可写

// 异步入队 (Core 0 写入任务持有 SD mutex, 主循环不阻塞).
void enqueueLog(const CarData& data);

// 强制 sync (掉电前 / 用户主动). 阻塞直到队列清空.
void flush();

}  // namespace raceguard::storage
