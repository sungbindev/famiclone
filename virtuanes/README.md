# 模拟器（VirtuaNES 调试版）

曾供 fcdev 使用的调试用模拟器，由 VirtuaNES 改造而来（GPLv3，许可文件见 [LICENSE](LICENSE)），带调试服务，能运行 NES 游戏，以及 SB2000 等学习机的固件与程序。`fcdev` 现在改用新模拟器 Ember（见 `ember/`），这里保留作学习机阶段的参考，`fcdev` 不再使用它。

底版是 VirtuaNES 的上游较新版本（带 SB2K、BBK、YuXing 等机型的映射器，mapper 号 171 变体 1 是 SB2K）。

## 目录

```text
virtuanes/
  VirtuaNES.slnx        Visual Studio 2026 解决方案
  build.ps1             命令行编译脚本
  README.md  LICENSE  CHANGELOG.md  文档、许可与更新记录
  VirtuaNES/            工程与全部源码
    VirtuaNES.vcxproj   工程文件
    lang/Chinese.vlp    中文语言包，编译后复制到 exe 旁
    NES/ res/ s_lib/ zlib/ tools/  模拟核心、资源、第三方库与小工具
```

## 相对上游的改动

以下文件都在 `VirtuaNES/` 下。

- `DebugServer.cpp/.h`：调试服务，协议与命令表见下面的“调试服务命令”。
- `NES/Cpu.cpp`：增加指令、内存访问与中断钩子（用于采集 CDL 与断点）。
- `EmuThread.cpp`、`WinMain.cpp`：调试服务启动，以及调试模式下按命令授权的帧数运行。
- `WinMain.cpp`：设置了 `FCDEV_DEBUG_PORT`（调试模式）时，不把命令行转交给已运行的实例，调试模式的模拟器可以与普通实例同时运行。
- `DebugServer.cpp`：新增 `live` 命令，把调试模式切换为自由运行（原来 fcdev 的 `run` 命令使用）。
- `Config.h`、`MainFrame.cpp`：“后台运行”默认开启；窗口失去焦点或最小化时不再暂停模拟（原先“后台运行”关闭时会暂停，调试模式下 `run` 会因此一直等不到帧），失活时只停止读取键盘，“后台运行”选项对此不再起作用。
- `Config.h`：“显示 240 线”“以 4:3 比例显示”默认开启。
- `AboutDlg.cpp`、`AboutDlg.h`、`VirtuaNESres.h`：版本信息对话框显示“v1.1.0 调试版”（只改显示，`VIRTUANES_VERSION` 仍是 0x0097，录像与联机版本校验不受影响），QQ 号（普通文字，不是链接）放在原“E-mail”一行并把标签改成“QQ”，原“网址”一行隐藏；不再使用的 `CHyperLink.h` 已删除。
- `NES/Nes.cpp/.h`、`Config.h`、`VirtuaNES.vcxproj`：删除“双击版本信息里的网址把 NES 转成 BBK 的 GAM”功能（`NEStoGAM`、`GAM_out` 目录配置与 `NEStoGAM.rar`）。
- `VirtuaNESres.h`：窗口标题由“VirtuaNES_BBK/Subor/YX”改为“VirtuaNES 调试版”，以便和其他 VirtuaNES 版本区分。
- `MainFrame.cpp/.h`：窗口最小大小等于 1 倍画面（以 4:3 显示时客户区 320×240），不能再缩小；不以 4:3 显示且 1 倍画面比菜单栏单行宽度窄时，宽度取菜单栏单行宽度、高度按画面比例放大，画面不变形，菜单栏不换行；有鼠标的游戏中，点击画面捕获鼠标，双击鼠标右键退出捕获（原先是按鼠标中键）。
- `MainFrame.cpp`、`Wnd.cpp`：第一次运行（没有保存的窗口位置）时主窗口放到桌面工作区中央，对话框按父窗口居中，因此也在桌面中央；没有父窗口的对话框直接放到桌面工作区中央。
- `NES/PAD.cpp`、`NES/PadEX/EXPAD_SB2K.cpp`：把调试服务注入的手柄、键盘按键并入真实输入。
- `NES/PadEX/EXPAD_BBK_FD1.cpp`：步步高键盘也并入调试服务用 `key` 命令注入的按键（原先只有 SB2K 键盘支持）。
- `NES/Mapper/MapperSB2K.cpp/.h`：增加物理地址解析与空间读取、声音最快播放开关，并修了 DAC 线程清零命令时可能丢掉复位命令的竞态。
- `NES/FDC.cpp/.h`：SB2K 的软驱改用上游的 NRS 软驱控制器（模拟 μPD765/82077 的命令阶段、结果阶段、寻道、就绪与写保护等状态），原先给 SB2K 单独用的 fanoble 那套已删除，同一控制器也供其他机型的映射器使用。改动：`LoadDiskImage` 的“镜像已修改”标志改放成员变量（上游传了指向栈上局部变量的指针）；`reset()` 不再清掉“镜像待保存”标志；每条命令的第一个扇区记入磁盘访问钩子（`disklog`）。
- `NES/Mapper/MapperSB2K.cpp/.h`：软驱端口接到 NRS 控制器：`$4100` 是数据率、`$4101` 是 DOR（`writeIO(7)`、`writeIO(2)`），`$4200-$4207` 对应 NRS 的 DMA 与状态寄存器；NRS 在非 DMA 模式每传一个字节都置一次中断标志，所以 SB2K 的软驱中断在映射器里按上升沿触发，并受 DOR 位 3（允许中断）控制。
- `NES/Mapper/Mapper169.cpp`：删掉只服务于 fanoble 控制器的选择分支。
- `NES/Mapper/MapperFactory.cpp`、`NES/Mapper/MapperPLQ.cpp`、`s_lib/s_intf.h`：启用 mapper 172（PLQ 声像磁带机，如 `qls.nes`）；上游把它注释掉了，且播放库函数名多了下划线，与 `s_lib.lib` 导出的 `player_*` 不符，已改回。磁带文件（`.m4a`、`.mp3`、`.wav` 等）拖进窗口载入。
- `NES/FDC.cpp/.h`、`DebugServer.cpp`：新增 `fdc` 调试命令（软驱故障注入：取盘、写保护、没有索引脉冲、磁头卡住、控制器不响应、指定扇区校验错），用法见下面“调试服务命令”的命令表。已知隐患：`cmFormatTrack` 用的 `sectorSize` 没有按命令里的 N 参数设置，要先有过一次读写命令才是 512。
- `DebugServer.cpp`：`trace` 命令新增 `unique` 选项（每个 CPU 地址的指令、每个中断入口只记第一次）；新增 `dialogs` 命令与监视线程（调试服务启动后，本进程弹出的消息框、文件框会被自动按下确定、取消、否、忽略或关闭，文字记录可用 `dialogs` 查看、`dialogs clear` 清空，避免调试停在模态对话框上；文件对话框直接按取消，因为没有文件名时确定按钮不起作用）；新增 `tape play|rec <文件>`、`tape stop`、`tape status` 命令（数据录音机 `.vtp`），用法见下面“调试服务命令”的命令表。
- `NES/Mapper/MapperSB2K.cpp/.h`、`NES/Nes.cpp`、`DebugServer.cpp`：按真机实测（`apps/sb2000/memtest`）对齐 SB2K 的内存与 DMA：6576 模式下内部 RAM 只有 2K（`$0800-$1FFF` 镜像 `$0000-$07FF`），`$4400-$5FFF` 读出 `$68`、写入无效（`$5006` 的写入仍有效果），DMA 在 `$404B` 位 7 为 0 时不传输、目的在程序窗口（`$8000` 以上）时写不进去；FC 兼容模式不变。
- `NES/PPU_UM6576.cpp/.h`、`NES/Mapper/MapperSB2K.cpp/.h`、`DebugServer.cpp`：按真机实测（`apps/sb2000/hwtest`）继续对齐：UM6576 PPU 实现 `$2001` 位 0、1、2、5、6、7（极性与 NES 相同，增强位按 PAL 顺序：位 5 绿、位 6 红、位 7 蓝）、精灵属性位 7（垂直翻转）、位 6（水平镜像）、位 5（在背景之后）、8×16 精灵图案号低位忽略、`$2002` 位 6、5 与低 5 位的总线值、`$2008` 位 0、OAM 属性读回的位 4-2 恒为 1、`$4023` 读 `$80 | 位 0`、空读返回 CPU 数据总线最近的值、`$4081` 读 `$60`、调色板读回 `(值 & $3F) | $40`；MapperSB2K 的 DMA 长度按“长度 + 1”、`$4020` 位 7 为 0 时目的总是显存，定时器一次计时下溢后保持预置值，`$4035` 可读；调试服务的 `shot` 按每行的黑白与增强位出图。
- 没有改的上游行为：`NES/LPT.cpp` 的 `InitPrinterList` 把 `EnumPrinters` 的缓冲传成了 `NULL`，打印机选择列表里只有“PNG image”，看不到系统打印机；SB2K 映射器 `Reset` 里固定使用 Dendy 时序（CPU 1773447 Hz、312 行、50 Hz），游戏选项里改的制式在硬复位后会被改回。

## 编译

Windows，Visual Studio 2026（含“使用 C++ 的桌面开发”与 v145 工具集）。只支持 Win32，可在 Visual Studio 里打开 `VirtuaNES.slnx`，或用脚本编译：

```powershell
.\build.ps1 -Configuration Release
```

输出在仓库根目录的 `build\virtuanes\<配置>\bin\VirtuaNES.exe`，同目录有 `Chinese.vlp` 与运行后生成的 `VirtuaNES.ini`；编译前只结束调试实例（调试服务监听 `127.0.0.1` 端口的 `VirtuaNES.exe`）；用户自己打开的模拟器不会被结束，它们占用输出文件时编译报错，需先自行关闭。没有 `C:\DirectXSDK` 时用 Windows SDK 自带的 DirectInput 头文件即可编译。

语言包用相对文件名加载（上游行为），所以要从 exe 所在目录启动（工作目录是该目录），否则界面不显示中文。`build.ps1 -Run` 按此启动。

## 调试服务命令

`fcdev` 已改用 Ember，不再经这套协议与模拟器通信，命令表保留在这里供学习机阶段对照。

- 模拟器只在设置了环境变量 `FCDEV_DEBUG_PORT` 时启动调试服务，只监听 `127.0.0.1`。
- 一行一个命令；应答若干行文本，以单独的 `.` 行结束；应答首行以 `OK` 或 `ERR` 开头。一次只处理一条命令。
- 直接发给服务的路径必须是绝对路径（模拟器的工作目录与终端不同）；数字默认按十六进制解析（地址、扫描码、寄存器），帧数、长度、次数按十进制。

| 命令 | 说明 |
| --- | --- |
| `status` | 运行状态、寄存器与 bank 寄存器 |
| `run <帧数>` | 运行指定帧数，遇到断点提前停止 |
| `step <条数>` | 单步指定条数的指令 |
| `bp x\|r\|w <地址> [结束地址]` | 添加执行、读、写断点；`bp list`、`bp del <编号\|all>` |
| `regs [pc=.. a=.. x=.. y=.. s=.. p=..]` | 读取或修改寄存器 |
| `read <空间> <地址> <长度>` | 读内存；空间为 `cpu`、`rom`、`pram`、`vram`、`cram`、`eram`、`wram`、`ram` |
| `write <空间> <地址> <十六进制字节>` | 写内存 |
| `resolve <地址>` | 把 CPU 地址解析为物理空间与偏移 |
| `key <扫描码> <0\|1>`、`key clear` | 按下或松开键盘键（DirectInput 扫描码） |
| `mouse move <dx> <dy>`、`mouse btn <掩码>` | 鼠标移动与按键 |
| `shot <文件.png>` | 截图 |
| `trace <文件> [最大行数] [unique]`、`trace off` | 记录每条指令与中断；加 `unique` 时每个 CPU 地址的指令、每个中断入口只记第一次，最大行数按实际写出的行数算（只按 CPU 地址去重，换页后同一地址的其他代码会漏记） |
| `io <文件> [最大行数] [w]`、`io off` | 记录 `$2000-$4FFF` 的读写；加 `w` 只记录写 |
| `pad <0-3> <十六进制掩码>`、`pad <0-3> clear` | 注入手柄按键（A=01、B=02、Select=04、Start=08、上=10、下=20、左=40、右=80），与真实输入合并 |
| `disklog <文件> [最大行数]`、`disklog off` | 记录软驱控制器读写的 LBA |
| `calllog <文件> <pc> <地址:长度[,...]> [最大次数] [skip=地址=值[,...]]`、`calllog off` | 每次执行到 `pc` 就记录寄存器、栈顶与内存段；`skip` 表示该地址字节等于值（十六进制）时不记录；可添加多条，`off` 全部关闭 |
| `cdl on\|off\|clear\|save <前缀>` | 代码数据标记，保存为 `<前缀>-rom.cdl`、`<前缀>-pram.cdl` |
| `dialogs [clear]` | 列出调试期间被自动关闭的对话框（标题、文字、按下的按钮），`clear` 同时清空记录 |
| `tape play <文件>`、`tape rec <文件>`、`tape stop`、`tape status` | 数据录音机（`.vtp`）的播放与录制；`rec` 的文件在 `stop` 后才完整 |
| `disk <镜像>`、`savedisk` | 挂载或保存软盘镜像 |
| `fdc eject\|insert\|wp <0\|1>\|noindex <0\|1>\|seekfail <0\|1>\|dead <0\|1>\|bad <逻辑扇区...>\|clear\|status` | 软驱故障注入：`eject`、`insert` 取出、放回盘片；`wp` 写保护；`noindex` 没有索引脉冲；`seekfail` 磁头卡住；`dead` 控制器收到命令字节后不再处理；`bad` 指定的逻辑扇区（十进制）读时报数据校验错；`clear` 关闭全部故障；`status` 显示当前状态。`disk` 挂载新镜像时也会清掉全部故障 |
| `audio fast\|real` | 声音播放速度：`fast` 时语音与 DAC 的采样直接丢弃，模拟器内部时序不变；`real` 为默认 |
| `reset`、`softreset`、`savestate <文件>`、`loadstate <文件>` | `reset` 是硬复位（回到 BIOS），`softreset` 只复位 CPU、手柄与 APU、保留映射器状态；另有状态存取 |
| `live` | 切换为自由运行：模拟器按正常速度连续运行，不再由 `run` 授权帧数 |
| `quit` | 关闭模拟器 |

对 `virtuanes/` 的约定：改动处按文件头约定标明，改动清单写在上面的“相对上游的改动”；调试模式下 `WinMain.cpp` 不把命令行转交给已运行的实例；`WinMain.cpp` 含 Shift-JIS 编码的内容，不要用编辑工具整体改写。

## 配置

| 名称 | 说明 | 默认值 | 必填 |
| --- | --- | --- | --- |
| `FCDEV_DEBUG_PORT` | 模拟器调试服务的端口，未设置则不启动调试服务 | 无（fcdev 默认使用 5699） | 否 |
