# 仪表风格示例 / 贡献区

每个子目录对应一套仪表盘视觉风格: PNG 底图 / 指针样式 / 字体配色 / 卡片布局.

## 目录约定

```
examples/themes/
├── classic_3gauge/         (官方示例)
│   ├── gauges.yaml         (烘焙配置)
│   ├── needle_style.json   (指针配置)
│   ├── README.md
│   └── preview.png
├── jdm_retro/              (社区贡献)
├── minimalist_dark/        (社区贡献)
└── ...
```

## 命名规则

`<风格名_细分>/` (全小写, 下划线分隔)

例:
- `classic_3gauge` — 经典三表布局
- `jdm_retro` — JDM 复古机械表
- `digital_modern` — 现代数字仪表
- `minimalist_dark` — 极简暗色

## 怎么贡献你的设计

详见 [`docs/ADD_GAUGE_THEME.md`](../../docs/ADD_GAUGE_THEME.md) (待发布).

简要流程:

1. Fork 本仓库
2. 用 [`tools/gauge_bakery/`](../../tools/gauge_bakery/) 烘焙你的 PNG 仪表底图
3. 在 `examples/themes/<风格名>/` 下放配置 + 烘焙产物 + README + preview
4. 提交 PR, 附设备实拍图
5. Review 合并后, 进入下一版社区主题库

## 合并标准

- ✅ 设备实拍效果良好 (无偏色 / 字体溢出 / 卡顿)
- ✅ 配色对比度足够 (车内日间 / 夜间都可读)
- ✅ **不要使用第三方品牌商标** (避免法律风险, 比如不能用真实 Ferrari / Porsche 等 logo)
- ✅ README 包含设计灵感 + 适用场景说明
- ✅ Preview PNG (设备实拍, 不要 mockup)
