# fcprobe

在小霸王学习机 SB2000（SB-DOS）的 DOS 下运行，在 FC 兼容模式里测试内存写入、PPU、软驱与键盘；用 fcdev 开发，C 与 ca65 汇编。

它是 bbg-loader 的前期探测程序，结论是 FC 模式走不通，结果和结论见 [machines/bbg/docs/计划与进度.md](../../../machines/bbg/docs/计划与进度.md)。

## 使用

探测版只在调试用模拟器里运行，运行后不会返回 DOS，只能硬复位。

```bash
npm run build    # 生成 build/sb2000/FCPROBE.EXE
npm run disk     # 生成含 FCPROBE.EXE 的软盘镜像 build/sb2000/FCPROBE.img
```

读出测试结果（在仓库里 `machines/bbg` 目录下运行，用 Ember 启动学习机并经调试服务读内存；Ember 的位置取环境变量 `EMBER_EXE`）：

```bash
node tools/probe-fc.mjs
```

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/fcprobe.c` | 在 DOS 下运行：关中断，把第 3 个 128K 窗口预置成 `$AA`，改向量，复制探测桩与跳板到 ERAM，写 `$5006`、`$4301` 进入 FC 模式 |
| `src/probe-stub.s` | 探测桩，复制到 ERAM `$6100` 运行，结果写到 `$7F00` 起，各项的含义见文件头 |
| `fcdev.json` | fcdev 配置：目标 sb2000，程序名 fcprobe，固件指向 `ember/roms/sb2000_bios.bin` |
