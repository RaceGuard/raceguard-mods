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

# 按 env 后缀过滤 (round-led-21 → led; p4-bar-dsi → p4)
if "led" in ENV_NAME:
    suffix = "led"
elif "p4" in ENV_NAME:
    suffix = "p4"
else:
    print(f"[link_core] 未知 env '{ENV_NAME}', 跳过 .a 链接")
    raise SystemExit(0)

pattern = os.path.join(LIB_DIR, f"libraceguard-core-*-{suffix}.a")
matches = sorted(glob.glob(pattern))

if not matches:
    print(f"[link_core] ⚠️  未找到 {pattern}")
    print(f"[link_core] ⚠️  请先执行: ./scripts/fetch_core.sh <version>")
    print(f"[link_core] ⚠️  v0.0.x 阶段 .a 尚未发布到 Releases, 本警告可忽略.")
    raise SystemExit(0)

# 取最新版本 (按文件名排序最后一个)
a_file = matches[-1]
print(f"[link_core] ✅ 链接 {os.path.basename(a_file)}")

# 追加到 LINKFLAGS - 用 -l: 让 ld 接受任意 .a 文件名
env.Append(LINKFLAGS=[a_file])  # noqa: F821
