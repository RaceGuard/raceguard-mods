"""
link_core.py - PIO extra_script 在 link 阶段追加核心库

由 platformio.ini 的 extra_scripts = post:scripts/link_core.py 触发.
作用: 找到 lib/raceguard_core/libraceguard-core-<version>-<env>.a 并追加到链接命令.

未找到 .a (v0.0.x 阶段还没发布) 时只警告, 不报错 - 让 build 继续到 link 阶段
自然报 "undefined reference to raceguard::..." 让用户知道要先跑 fetch_core.sh.
"""

import glob
import os

Import("env")  # noqa: F821

LIB_DIR = os.path.join(env["PROJECT_DIR"], "lib", "raceguard_core")  # noqa: F821
ENV_NAME = env["PIOENV"]  # noqa: F821

# 主仓 pack_lib.sh 输出 libraceguard-core-<version>-<env>.a, 按完整 env 名匹配
pattern = os.path.join(LIB_DIR, f"libraceguard-core-*-{ENV_NAME}.a")
matches = sorted(glob.glob(pattern))

if not matches:
    print(f"[link_core] ⚠️  未找到 {pattern}")
    print(f"[link_core] ⚠️  请先执行: ./scripts/fetch_core.sh <version>")
    print(f"[link_core] ⚠️  v0.0.x 阶段 .a 尚未发布到 Releases, 本警告可忽略.")
    raise SystemExit(0)

# 取最新版本 (按文件名排序最后一个)
a_file = matches[-1]
print(f"[link_core] ✅ 链接 {os.path.basename(a_file)}")

# --whole-archive 强制 ld 保留 .a 内所有符号 (即使没被 .o 引用).
# 否则因为 PIO 把 LINKFLAGS 放在 .o 之前, ld 先扫 .a 时 .o 的 undefined refs 还
# 没出现, .a 里的符号会被丢弃, 之后 .o 再用到就报 undefined reference.
env.Append(LINKFLAGS=[  # noqa: F821
    "-Wl,--whole-archive",
    a_file,
    "-Wl,--no-whole-archive",
])
