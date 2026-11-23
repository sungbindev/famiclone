RUN.EXE：在小霸王学习机 SB2000（SB-DOS）的 DOS 下运行 NES 游戏。

使用方法：
1. 把 RUN.EXE 和游戏文件放在同一张盘上，游戏文件名须是 8.3 名字（如 MARIO.NES）。
2. 进入 DOS，在提示符下输入：RUN MARIO.NES
3. 读入游戏需要一会儿，之后直接进入游戏。

注意：
- 支持 mapper 0 与 4（MMC3），PRG、CHR 各不超过 128K，不带 trainer，必须有 CHR。
- 文件超过 256K+16 字节时只读前面这部分。
- 游戏里不能回到 DOS，只能硬复位。
- 出错时显示英文提示，按任意键返回 DOS，如 Usage: RUN FILE.NES、Mapper not supported。
