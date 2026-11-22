# peek

按偏移读文件：显示文件里从指定偏移起的 30 个字节。

## 构建与运行

用 VS Code 的“fcdev：构建”任务构建，产物在 `build/sb2000/PEEK.EXE`，同时生成软盘镜像 `build/sb2000/PEEK.img`。

在 SB-DOS 的 `A:\>` 下输入：

```text
peek 文件名 十六进制偏移
```
