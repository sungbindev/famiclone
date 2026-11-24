GAME.EXE：在小霸王 97（SB97）的 DOS 下运行 NES 游戏。

使用方法：
1. 把 GAME.EXE 和游戏文件放在同一张软盘上，游戏文件名须是 8.3 名字（如 MARIO.NES）。
2. 进入 DOS，在提示符下输入：
   GAME        不带参数：显示游戏菜单，手柄上下键选择，按 Start 运行
   GAME MARIO  带参数：直接运行（也可写 GAME MARIO.NES）
3. 进入游戏后按手柄的 Start 开始。

注意：
- 只支持 mapper 0 的游戏：PRG 16K 或 32K，CHR 8K。
- 游戏里不能回到 DOS，只能硬复位。
- 出错时画面第 10 行显示英文提示并停住，需要重新开机。

作者：sungbin
GitHub：https://github.com/sungbindev/famiclone/tree/master/apps/sb97/run-game
