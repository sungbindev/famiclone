# 苹果 II 固件 ROM

苹果 II 各机型的系统 ROM 与视频字符发生器 ROM，从两个开源模拟器项目的真机 dump 取得并逐字节交叉核对。
Apple 没有官方发布过这些 ROM 的二进制，版权仍属 Apple；这里作为研究资料入库。
目录内的文件与来源的核对结论见下表。

## 系统 ROM

| 文件 | 机型 | 大小 | 地址范围 | 来源 | MD5 |
| --- | --- | --- | --- | --- | --- |
| `Apple2.rom` | Apple II（原版，Integer BASIC） | 12288 | $D000–$FFFF | apple2js `js/roms/system/original.ts`（去掉末尾 2 个填充字节） | `2A68F049D44E326574B95C0EC638C1CA` |
| `Apple2_Plus.rom` | Apple II+（Autostart，Applesoft） | 12288 | $D000–$FFFF | AppleWin `resource/Apple2_Plus.rom` | `572B3005A4FA49BC54917B069B82C1AB` |
| `Apple2_JPlus.rom` | Apple II J-Plus（日版） | 12288 | $D000–$FFFF | AppleWin `resource/Apple2_JPlus.rom` | `F3E8237ECA3809BEC2A42B984A1332A8` |
| `Apple2e.rom` | Apple //e（未增强） | 16384 | $C000–$FFFF | AppleWin `resource/Apple2e.rom` | `346BC782C6A08A531C460E33BC03DAF4` |
| `Apple2e_Enhanced.rom` | Apple //e（增强版，65C02） | 16384 | $C000–$FFFF | AppleWin `resource/Apple2e_Enhanced.rom` | `38063E08C778503FC03ECEBB979769E9` |

## 视频 ROM（字符发生器）

| 文件 | 用途 | 大小 | 来源 | MD5 |
| --- | --- | --- | --- | --- |
| `Apple2_Video.rom` | Apple II/II+ 文本字符点阵 | 2048 | AppleWin `resource/Apple2_Video.rom` | `9AC0DC8C4D0002EB45B0B84BE0BDE5EC` |
| `Apple2e_Enhanced_Video.rom` | 增强 //e 文本字符点阵（5×7，位 0 在最左），Ember 文本屏字模用它生成 | 4096 | AppleWin `resource/Apple2e_Enhanced_Video.rom` | `9123FFF3442C0E688CC6816BE88DD4AB` |

## 来源与核对说明

- 来源仓库：[AppleWin/AppleWin](https://github.com/AppleWin/AppleWin)（GPL-2.0）与
  [whscullin/apple2js](https://github.com/whscullin/apple2js)（MIT）。两个项目独立提取，可互相印证。
- `Apple2_JPlus.rom`、`Apple2e.rom`、`Apple2e_Enhanced.rom` 在两个来源里逐字节一致。
- `Apple2.rom` 采用 apple2js 的完整版：AppleWin 的同名文件里 $D800–$DFFF（Integer BASIC 区）
  约 2036 字节被填成 $FF，不是完整 dump，故不采用；其余部分与 apple2js 一致。
- `Apple2_Plus.rom` 采用 AppleWin 版（II+ 主板标准 ROM，IRQ 向量 $FFFE 为 $FA40）；
  apple2js 的 `fpbasic.ts` 是 Applesoft 固件卡变体（IRQ 向量为 $FF59），两者仅差这 2 个字节。
- `Apple2e_Enhanced_Video.rom` 是本目录最早收录的文件，与 AppleWin 现版逐字节一致，保留原件。
- 核对时间：2026-11-28。上游若更新，按 MD5 复核。

Ember 的字模生成脚本在 `ember/tools/dev/gen-apple2-font.mjs`，输入即本目录的 `Apple2e_Enhanced_Video.rom`。