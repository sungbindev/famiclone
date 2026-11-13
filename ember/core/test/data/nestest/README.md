# nestest

CPU 核的验证数据：`nestest.nes` 是 Kevin Horton 编写的 6502 指令测试 ROM，`nestest.log` 是它在自动模式下由真机验证过的逐条执行日志。

## 来源

| 文件 | 来源 |
| --- | --- |
| `nestest.nes` | https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/other/nestest.nes |
| `nestest.log` | https://raw.githubusercontent.com/christopherpow/nes-test-roms/master/other/nestest.log |

两个文件都是测试程序的公开分发版本，原样放在这里，没有修改。

## 用途

`core/test/hardware/nestest-test.cpp` 用它们验证 `cpu-6502`：

- 自动模式：PC 从 `$C000` 起，SP 为 `$FD`，状态为 `$24`，累计周期 7，不需要 PPU。
- 逐行比对日志里的 PC、A、X、Y、P、SP 与累计周期（CYC），忽略 PPU 列；日志共 8991 行，必须全部一致。
- 运行结束后核对 `$0002`、`$0003` 为 `00`（测试通过）。
- ROM 的复位向量指向交互模式入口 `$C004`，测试里的内存把复位向量读成 `$C000`，让 CPU 经完整的复位序列到达自动模式的入口。
