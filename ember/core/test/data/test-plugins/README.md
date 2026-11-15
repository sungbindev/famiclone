# test-plugins

只用于测试的 mapper 插件源文件，测试时由 `core/test/support/plugin-test-support.hpp` 的 `testPluginPackage` 现场打成 `.emap` 包，不随程序发布，也不放进 `plugins/`（那里只放官方插件）。

## 来源

由本项目自己编写，不是第三方数据。

| 目录 | 内容 |
| --- | --- |
| `cnrom/` | `test-cnrom`：mapper 3（CNROM），用声明式寄存器写 `$8000-$FFFF` 选择 8K 的 CHR 页（没有脚本），用来测试声明式寄存器、没有 `main.js` 的包，并让 `ppu_read_buffer` 这个 mapper 3 的 ROM 能运行 |

## 用途

- `core/test/hardware/ppu-rom-test.cpp`：`ppu_read_buffer` 用它装配卡带。
- `core/test/mappers/mapper-chip-test.cpp`：寄存器写入切换 CHR 页、复位后回到第 0 页。
- `core/test/bootstrap/core-test.cpp`：经 `mappers.install` 安装。

## 结果

通过。
