/**
 * @file lv_conf.h
 * @brief LVGL 配置 — ESP32-S3 + Waveshare 2.1" RGB LCD (480x480)
 *
 * LVGL v8.3.11，对齐官方 demo 配置
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_CONF_SKIP

#include <stdint.h>

/* ============ 屏幕分辨率 ============ */
#define LV_HOR_RES_MAX 480
#define LV_VER_RES_MAX 480

/* ============ 颜色设置 ============ */
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0          /* RGB 并口不需要 byte swap（SPI 才需要） */
#define LV_COLOR_SCREEN_TRANSP 0

/* ============ 内存设置 ============ */
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_ALLOC malloc
#define LV_MEM_CUSTOM_FREE free
#define LV_MEM_CUSTOM_REALLOC realloc
#define LV_MEM_SIZE (512 * 1024)

/* ============ 显示设置 ============ */
#define LV_DPI_DEF 120
#define LV_DISP_DEF_REFR_PERIOD 16  /* ~60 FPS 上限 */

/* ============ 输入设备设置 ============ */
#define LV_INDEV_DEF_READ_PERIOD 30

/* ============ 字体设置（仅保留实际使用的） ============ */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1     /* 小标签 */
#define LV_FONT_MONTSERRAT_16 1     /* 默认字体 */
#define LV_FONT_MONTSERRAT_18 1     /* 校准/标签 */
#define LV_FONT_MONTSERRAT_20 1     /* Fuel Trim 标题 */
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1     /* 校准/标签 */
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 1     /* G-Ball 数值 */
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 1     /* G-Ball 数值加大 */
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_48 1     /* Timing 大字 / 通用仪表数值 */
#define LV_FONT_MONTSERRAT_52 0
#define LV_FONT_DEFAULT &lv_font_montserrat_16


/* ============ 日志设置 ============ */
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

/* ============ 断言设置（生产环境关闭） ============ */
#define LV_USE_ASSERT_NULL 0
#define LV_USE_ASSERT_MALLOC 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0
#define LV_USE_ASSERT_STYLE 0

/* ============ 组件启用 ============ */
#define LV_USE_ARC 1
#define LV_USE_ANIMIMG 0
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CALENDAR 0
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_CHART 0
#define LV_USE_DROPDOWN 0
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 0
#define LV_USE_SLIDER 0
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 0
#define LV_USE_TABLE 0

/* ============ 高级组件 ============ */
#define LV_USE_METER 1
#define LV_USE_MSGBOX 1
#define LV_USE_LIST 1
#define LV_USE_PAGE 0
#define LV_USE_SPAN 0
#define LV_USE_SPINBOX 0
#define LV_USE_SPINNER 0
#define LV_USE_TABVIEW 0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN 0
#define LV_USE_KEYBOARD 0
#define LV_USE_LED 1
#define LV_USE_SCALE 0

/* ============ 主题设置 ============ */
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_GROW 0
#define LV_THEME_DEFAULT_DARK 1
#define LV_USE_THEME_MONO 0
#define LV_USE_THEME_BASIC 0

/* ============ 布局设置 ============ */
#define LV_USE_FLEX 1
#define LV_USE_GRID 0

/* ============ 动画设置 ============ */
#define LV_USE_ANIMATION 1
#define LV_ANIM_TICK_ENABLE 1

/* ============ 其他设置 ============ */
#define LV_USE_SNAPSHOT 0
#define LV_USE_MONKEY 0
#define LV_USE_GRIDNAV 0
#define LV_USE_FRAGMENT 0
#define LV_USE_IMGFONT 0
#define LV_USE_SJPG 0
#define LV_USE_PNG 1            /* LVGL 8.3 内置 lodepng decoder，导航 Bitmap + 仪表底图 */
/* Image cache：缓存 8 张 480×480 解码后位图（PngGaugeCard 长按切表 zero-cost）
 * 单张约 460KB（RGB565），8 张 ~3.7MB，从 PSRAM 分配 */
#define LV_IMG_CACHE_DEF_SIZE 8
#define LV_USE_LODEPNG 0        /* v9 风格，8.3 忽略 */
#define LV_USE_LIBPNG 0
#define LV_USE_BMP 0
#define LV_USE_RLE 0
#define LV_USE_LIBJPEG_TURBO 0
#define LV_USE_GIF 0
#define LV_USE_QRCODE 0
#define LV_USE_BARCODE 0
#define LV_USE_FREETYPE 0
#define LV_USE_TINY_TTF 0

/* ============ 绘制优化 ============ */
#define LV_DRAW_BUF_STRIDE_ALIGN 1
#define LV_USE_DRAW_SIMPLE 1

/* ============ 文件系统驱动 (v0.2.0+: FS 主题包) ============ */
/* LVGL 8.3 自带 lv_fs_posix, 包装 POSIX open/read/seek API.
 * Arduino-ESP32 的 LittleFS 已挂载到 /littlefs/ VFS 路径, 可直接用 POSIX 访问.
 * letter='L' → 路径形如 "L:/littlefs/themes/dark_minimal/gauge_coolant.png"
 * 配合 LV_USE_PNG=1, lv_image_set_src(obj, "L:/...png") 会自动 fread + lodepng 解.
 */
#define LV_USE_FS_POSIX 1
#define LV_FS_POSIX_LETTER 'L'
#define LV_FS_POSIX_PATH ""              /* 不加全局前缀, 全 letter:/abs-path 形式 */
#define LV_FS_POSIX_CACHE_SIZE 0         /* 不缓存 (LVGL image cache 已缓解码后位图) */

#endif /* LV_CONF_H */
