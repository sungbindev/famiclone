# poke

按偏移写文件：把文字（大写）写进已有文件的指定偏移处。

## 构建与运行

用 VS Code 的“fcdev：构建”任务构建，产物在 `build/sb2000/POKE.EXE`，同时生成软盘镜像 `build/sb2000/POKE.img`。

在 SB-DOS 的 `A:\>` 下输入：

```text
poke 文件名 十六进制偏移 文字
```
