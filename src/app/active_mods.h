// active_mods.h — mods 注册集中点声明
//
// 设计目的: 用户加 car / theme mod 只动 active_mods.cpp 一个文件,
// 不动 main.cpp. 多人 PR 时 main.cpp 不冲突.
//
// 实现在 active_mods.cpp, 由 main.cpp 在 backend::startAll() 之前调一次.

#pragma once

void register_active_mods();
