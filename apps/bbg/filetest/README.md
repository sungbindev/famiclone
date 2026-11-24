# filetest（步步高）

列出 BBGDOS 默认盘根目录的文件，验证文件调用 FILE_FIRST（6）与 FILE_NEXT（7）的通道块用法：每行显示属性（十六进制）、11 字节文件名与大小（十六进制），结束时显示 `E=` 加出错码，`12` 表示没有更多文件。

## 使用

```text
filetest
```

用 FC Dev 的 bbg 目标构建，`fcdev.json` 的 `diskImage` 指向系统盘 `apps/bbg/BBGDOS.IMG`，构建后生成 `build/bbg/FILETEST.CMD` 与带它的软盘镜像 `build/bbg/bbg-filetest.img`（`fcdev.json` 的 `diskName`）。在 Ember 里以 `bbg1` 机型加 `ember/roms/bbg1_bios.bin` 开机，等到 `A:\>` 后输入 `filetest`。

## 原理

通道块 48 字节，`FH_COMMAND` 填 6，`FH_PAGE` 填程序所在页，`FH_BUFFER` 指向 11 个 `?`；调用文件入口 `$5803`（X 为块地址高字节，Y 为低字节）后，查找结果在块偏移 `$17` 起：属性、时间、日期、大小、文件名。之后把 `FH_COMMAND` 改为 7 反复调用，直到返回出错码 `$12`。接口细节见 [machines/bbg/docs/api](../../../machines/bbg/docs/api/README.md)。
