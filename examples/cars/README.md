# 车型适配示例 / 贡献区

每个子目录对应一款车型的 OBD 适配: PID 列表 / DTC 描述 / 告警阈值.

## 目录约定

```
examples/cars/
├── nissan_gtr_r35/         (官方示例, 实车验证过)
│   ├── profile.cpp
│   ├── README.md
│   └── tested_with/
├── honda_civic_fk7/        (社区贡献)
├── bmw_m3_f80/             (社区贡献)
└── ...
```

## 命名规则

`<品牌_车型_代号>/` (全小写, 下划线分隔)

例:
- `nissan_gtr_r35`
- `toyota_supra_a90`
- `porsche_911_991`
- `volkswagen_golf_mk7`

## 怎么贡献你的车

详见 [`docs/ADD_CAR_PROFILE.md`](../../docs/ADD_CAR_PROFILE.md) (待发布).

简要流程:

1. Fork 本仓库
2. 在 `examples/cars/<你的车>/` 下新建目录, 参考 `nissan_gtr_r35/` 模板
3. 实车验证你声明的 PID 都能正确读取 (连续 ≥30 分钟, 覆盖各种工况)
4. 提交 PR, 附实测视频 / 日志
5. Review 合并后, 进入下一版社区车型库

## 合并标准

- ✅ 实车验证 (必须)
- ✅ PID / DTC 字段对应正确
- ✅ 告警阈值合理 (不能误报频繁)
- ✅ 代码风格遵循 SDK 模板
- ✅ README 清晰说明覆盖的工况和已知限制
