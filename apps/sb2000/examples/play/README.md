# play

在 SB-DOS 下运行 `.nes` 游戏，支持 mapper 0 与 mapper 4（MMC3），PRG、CHR 各不超过 128K。

## 构建与运行

用 VS Code 的“fcdev：构建”任务构建，产物在 `build/sb2000/PLAY.EXE`，同时生成软盘镜像 `build/sb2000/PLAY.img`（`fcdev.json` 的 `diskName`，Ember 的测试用它）。

在 SB-DOS 的 `A:\>` 下输入：

```text
play 文件名.nes
```

运行时软盘上要有 `.nes` 文件，可以把它放进项目的 `disk/` 目录后重新构建。
