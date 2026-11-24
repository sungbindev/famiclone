# 机型与开机后的操作

载入整机，或在整机里操作开机后的画面时使用。机型标识：`fc`、`sb97`、`sb2000`、`bbg1`、`bbg98`、`pec9588`、`pec586`、`yuxing`。

## NES 卡带（机型 `fc`）

- 载入：`emu_load` 的 `file`，载入后已通电、暂停；第一轮 `emu_run` 60 帧。
- 手柄用 `emu_input` 的 `kind: "pad"`；键盘按键的 `code` 由卡带的映射脚本翻译成手柄或键盘输入。
- 需要 Family BASIC 键盘的卡带，`emu_load` 加 `familyKeyboard: true`。

## 学习机 SB2000（SB-DOS）

### 自动运行程序

`emu_load` 的 `machine: "sb2000"`、`firmware`、`diskImage`、`program` 会依次做：开机运行 900 帧，按两次回车跳过日期与时间，输入程序名，回车。返回时程序刚启动，机器暂停。

### 只开机、手动输入

1. `emu_load` 不给 `program`。
2. `emu_run` 900 帧，`emu_screen` 确认画面在询问日期。
3. `emu_input` 的 `kind: "key"`、`code: "Enter"`、`downFrames: 8`、`upFrames: 8`，做两次。
4. 逐个字符输入程序名：字母用 `KeyA` 到 `KeyZ`，数字用 `Digit0` 到 `Digit9`，每次 `downFrames` 与 `upFrames` 都是 8。
5. 输入 `Enter` 运行。

### 要点

- 提示出现的时间取决于 BIOS 与磁盘；换了 BIOS 或磁盘后，先按“只开机、手动输入”做，第 2 步的画面不是日期提示时，按画面上的提示操作。
- 程序返回 DOS 会清屏，需要看输出的程序要在返回前等待按键。
- 从键盘启动的程序，模拟器会把回车当作手柄的选择加开始键；读手柄的程序要先等几十帧再初始化。
- 程序的内存布局、系统调用与易错点读 skill `fcdev-app-development` 的 `references/sb2000-development.md` 与 `sb2000-recipes.md`。

## SB97 程序（机型 `sb97`）

`emu_load` 的 `machine: "sb97"`、`firmware`、`diskImage`、`program` 会依次做：开机运行 200 帧，按空格跳过开机画面，运行 300 帧等菜单稳定，按左键选中 DOS，回车进入，运行 120 帧等提示符，输入程序名，回车。按键都是按下 8 帧、松开 8 帧。返回时程序刚启动，机器暂停。

- 菜单没稳定前按的左键会丢，所以要等满 300 帧。
- SB97 的 DOS 程序不能返回 DOS，运行后画面停在程序的输出上。
- 需要给程序传参数时（如 `RUN MARIO`），不给 `program`，按上面的步骤手动操作，再逐个字符输入命令行（空格用 `Space`，点用 `Period`）。

## 裕兴程序（机型 `yuxing`）

`emu_load` 的 `machine: "yuxing"`、`firmware`、`diskImage`、`program` 会依次做：开机运行 300 帧，按空格跳过开机画面，运行 120 帧进主菜单，按下键 3 次选中“学DOS”，回车，运行 300 帧让“DOS入门”动画播完，按空格，运行 120 帧，按下键 2 次选中“三.DOS系统操作”，回车进入，运行 120 帧等提示符，输入程序名加 `.EXE`，回车。按键都是按下 8 帧、松开 8 帧。返回时程序刚启动，机器暂停。

- 裕兴的 DOS 运行程序必须写扩展名（`hello.exe`），不写报 Bad command or file name；也不接受命令行参数。
- 裕兴的 DOS 程序不能返回 DOS，运行后画面停在程序的输出上。
- 提示符下 `dir` 要几百帧才列出，之后再输入命令。
- 开机时放入 D 型游戏盘“空心接龙”，选“学DOS”后进了游戏而不是 DOS（原因没查）；用普通的工具盘（如“拷贝工具.IMG”）或 FC Dev 生成的镜像。

## 步步高程序（机型 `bbg1`）

`emu_load` 的 `machine: "bbg1"`、`firmware`、`diskImage`、`program` 会依次做：开机运行 1200 帧（BIOS 从磁盘读入 DOS，系统盘的 AUTOEXEC.BAT 跑完），按两次回车（没有 AUTOEXEC.BAT 的系统盘会先问日期与时间；已在提示符下时只是两个空行），运行 60 帧，输入程序名，回车。按键都是按下 8 帧、松开 8 帧。返回时程序刚启动，机器暂停。

- 步步高的 DOS 在磁盘上，`diskImage` 必须是带 `BBGDOS.SYS` 与 `COMMAND.CMD` 的系统盘镜像；FC Dev 生成的镜像建在它上面。
- 运行程序不用写扩展名（`hello`），程序用 DOS 调用“程序结束返回”后回到 `A:\>` 提示符。
- 系统盘里的 AUTOEXEC.BAT 最后启动一个不返回的程序时，提示符出不来，输入的程序名会丢；换一张系统盘。

## 其他整机（`bbg1` 只开机、`bbg98`、`pec9588`、`pec586`）

载入：`emu_load` 的 `machine`、`firmware`，需要软盘时加 `diskImage`；不给 `program`。开机后的菜单、按键与等待时间因机型而异，本文不记录没有验证过的步骤。操作步骤按下面的顺序取得：

1. 用户给出的步骤：整理成“第几步、按键或动作、等待帧数、预期画面”的表，按表逐步做。
2. 项目 README 里写的步骤。
3. 都没有时按画面逐步试，规则如下：
   - 先 `emu_run` 120 帧，再 `emu_screen` 一张；
   - 按画面上的提示一次输入一个键，每次输入后 `emu_run` 30 帧，再 `emu_screen` 一张；
   - 菜单的光标位置看不清时，先截图确认再按键，不凭印象连按；
   - 同一个画面试了 3 个不同的键仍没有变化，就停下，汇报当前截图，请用户给出步骤。

每一步做完都用 `emu_screen` 与预期画面核对，一致才进入下一步；不一致时停在这一步，先查原因。

## 工具覆盖不到的事

- 把软驱里被程序改写的盘片存回镜像文件：没有对应的 `emu_*` 工具，请用户运行命令“FCDEV：调用调试请求”，请求 `media.floppySave`，参数 `drive=0`。
