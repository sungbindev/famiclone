# Floppy Emu 金字塔固件（修改版）

这是 BMOW Floppy Emu 虚拟软驱（Model B / Model C，Apple II 固件 250225A）的修改版固件，**新增了对金字塔 PEC-9588 学习机软盘的支持**。

原版 Floppy Emu 只认苹果格式的磁盘镜像，金字塔的 40 磁道 `.nib` 软盘插上去认不出来。刷入本固件后，模式菜单里多出一个 **「9 Pyramid 40-track」** 模式，选它就能把金字塔的软盘镜像当真实软盘用：开机、`INIT` 格式化、`SAVE` 保存程序、`LOAD` 读回，和真软盘一样。

## 支持什么

| 项目 | 说明 |
| --- | --- |
| 机型 | 金字塔 PEC-9588（兼容 PEC-586 / PEC-686F 的 5.25 寸软驱接法） |
| 镜像格式 | `.nib`（Disk II 位流格式），**266240 字节 = 40 磁道 × 6656 半字节** |
| 空盘 | 仓库里的 [`floppy-images/pec9588/blank.nib`](../floppy-images/pec9588/blank.nib)（266240 字节全 0），挂上后在机器里 `INIT` 格式化即可当空盘用 |
| 硬件 | BMOW Floppy Emu Model B 或 Model C，Apple II 版固件（本修改基于 250225A） |

注意：

- 模式 9 只认 266240 字节的 `.nib`；金字塔的 143360 字节 `.dsk` 不支持（原版也不支持）。
- 苹果 35 磁道的 232960 字节 `.nib` 请切回原来的「4 Apple II 5.25 Floppy」模式用，原有 6 个模式的功能完全不变。
- 同一个 `.nib` 镜像也能在 Ember 模拟器（金字塔机型）里直接载入，两边通用。

## 怎么刷固件

1. 把 [`build/femu.bin`](build/femu.bin) 复制到 SD 卡**根目录**，文件名必须是 `femu.bin`（卡上原来的固件文件会被它替代）。
2. SD 卡插回 Floppy Emu。
3. **按住 SELECT 和 PREV 两个键不放，再按一下 RESET**，进入固件更新，等屏幕提示完成。
4. 更新完把 SD 卡上的 `femu.bin` 删掉（官方建议，避免误触发再次更新），重新开机。

刷坏了不要紧：把官方原版固件（本目录 [`bin/femu.bin`](bin/femu.bin) 就是逐字节一致的原件）放回 SD 卡根目录，按同样步骤重刷即可恢复。

## 怎么用

1. 开机进文件菜单，选 **Settings** → **Disk Emulation Mode**。
2. 模式列表现在是 7 项，**第 3 位是新增的「9 Pyramid 40-track」**：

   ```
   3 Apple II 3.5 Floppy
   4 Apple II 5.25 Floppy
   9 Pyramid 40-track      ← 新增
   8 Apple II Dual 5.25
   5 Smartport Hard Disk
   7 Smartport Unit 2
   6 Unidisk 3.5
   ```

3. 选「9 Pyramid 40-track」按 SELECT。选择会存进芯片，**以后每次开机自动就是这个模式**，不用再设。
4. 在文件菜单里选金字塔的 `.nib` 镜像（比如 `blank.nib`）挂载，类型会显示为 NIB。
5. Floppy Emu 接到金字塔的软驱口，开机进入 LM-DOS（主菜单按 `6`），空盘先 `INIT`（答 `Y`）格式化，之后 `CATALOG` 列文件、`SAVE 名字` 保存、`LOAD 名字` 读回。写盘内容会自动写回 SD 卡上的镜像文件。
6. 想切回苹果模式：同一处 Settings → Disk Emulation Mode 里选 3–8 任意一项即可，随时来回切。

## 目录说明

| 路径 | 内容 |
| --- | --- |
| `build/femu.bin` | **修改版固件（刷这个）** |
| `bin/femu.bin` | 官方原版固件 250225A（恢复用） |
| `bin/firmware.xvf` | 官方包内 CPLD 数据（本修改不动 CPLD） |
| `patch/` | 补丁工程：汇编源码、构建脚本与技术说明（[patch/README.md](patch/README.md)） |
| `disasm/` | 固件反汇编工程与核实摘录 |
| `tools/` | avr-gcc 工具链与检索脚本 |
| `NOTES.md`、`方案文档.md` | 反汇编笔记与修改方案 |

自己重新构建：`node patch/build.mjs`，产物写到 `build/femu.bin`，脚本会逐字节校验未改动区域与官方原件一致。

## 已知限制

- 模式 9 的 NIB 大小识别只认 266240；其他大小一律不认（这与模式 4 只认 232960 是对称的）。
- 修改版未经长期真机考验，如遇异常先刷回原版对照，再反馈现象。