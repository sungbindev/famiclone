# femu.bin — Floppy Emu 金字塔修改版固件

BMOW Floppy Emu（Model B / Model C，Apple II 固件）的修改版，基于官方 250225A。新增模式 **「9 Pyramid 40-track」**（模式菜单第 3 位），支持金字塔 PEC-9588 学习机的 40 磁道 `.nib` 软盘镜像（266240 字节），挂上即可开机、`INIT` 格式化、`SAVE`/`LOAD`。原有 6 个模式功能不变。

详细说明见上级目录 [README.md](../README.md)。

## 怎么刷

1. 把本目录的 `femu.bin` 复制到 SD 卡**根目录**，文件名必须是 `femu.bin`。
2. SD 卡插回 Floppy Emu。
3. **按住 SELECT 和 PREV 不放，再按一下 RESET**，进入固件更新，等屏幕提示完成。
4. 更新完删掉 SD 卡上的 `femu.bin`，重新开机。

刷完在 Settings → Disk Emulation Mode 里选第 3 位「9 Pyramid 40-track」即可使用，选择会记住，以后开机自动生效。

刷坏了用官方原版（[`../bin/femu.bin`](../bin/femu.bin)）按同样步骤重刷即可恢复。