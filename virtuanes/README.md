# 模拟器（VirtuaNES 调试版）

fcdev 使用的调试用模拟器，由 VirtuaNES 改造而来（GPLv3，许可文件见 [LICENSE](LICENSE)），带调试服务，能运行 NES 游戏，以及 SB2000 等学习机的固件与程序。

底版是 VirtuaNES 的上游较新版本（带 SB2K、BBK、YuXing 等机型的映射器，mapper 号 171 变体 1 是 SB2K）。

## 目录

```text
virtuanes/
  VirtuaNES.slnx        Visual Studio 2026 解决方案
  build.ps1             命令行编译脚本
  README.md  LICENSE    文档与许可
  VirtuaNES/            工程与全部源码
    VirtuaNES.vcxproj   工程文件
    lang/Chinese.vlp    中文语言包，编译后复制到 exe 旁
    NES/ res/ s_lib/ zlib/ tools/  模拟核心、资源、第三方库与小工具
```

## 相对上游的改动

以下文件都在 `VirtuaNES/` 下。

- `DebugServer.cpp/.h`：调试服务，Node 客户端见 `packages/fcdev/src/emulator/debug-client.mjs`，命令行入口是 `fcdev debug`。
- `NES/Cpu.cpp`：增加指令、内存访问与中断钩子（用于采集 CDL 与断点）。
- `EmuThread.cpp`、`WinMain.cpp`：调试服务启动，以及调试模式下按命令授权的帧数运行。
- `WinMain.cpp`：设置了 `FCDEV_DEBUG_PORT`（调试模式）时，不把命令行转交给已运行的实例，调试模式的模拟器可以与普通实例同时运行。
- `DebugServer.cpp`：新增 `live` 命令，把调试模式切换为自由运行（fcdev 的 `run` 命令使用）。
- `Config.h`、`MainFrame.cpp`：“后台运行”默认开启；窗口失去焦点或最小化时不再暂停模拟（原先“后台运行”关闭时会暂停，调试模式下 `run` 会因此一直等不到帧），失活时只停止读取键盘，“后台运行”选项对此不再起作用。
- `Config.h`：“显示 240 线”“以 4:3 比例显示”默认开启。
- `AboutDlg.cpp`、`AboutDlg.h`、`VirtuaNESres.h`：版本信息对话框显示“v1.0.0 调试版”（只改显示，`VIRTUANES_VERSION` 仍是 0x0097，录像与联机版本校验不受影响），QQ 号（普通文字，不是链接）放在原“E-mail”一行并把标签改成“QQ”，原“网址”一行隐藏；不再使用的 `CHyperLink.h` 已删除。
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
- `NES/FDC.cpp/.h`、`DebugServer.cpp`：新增 `fdc` 调试命令（软驱故障注入：取盘、写保护、没有索引脉冲、磁头卡住、控制器不响应、指定扇区校验错），用法见 [05-模拟器与调试](../packages/fcdev/docs/05-模拟器与调试.md) 第 5.1 节的命令表。已知隐患：`cmFormatTrack` 用的 `sectorSize` 没有按命令里的 N 参数设置，要先有过一次读写命令才是 512。
- 没有改的上游行为：`NES/LPT.cpp` 的 `InitPrinterList` 把 `EnumPrinters` 的缓冲传成了 `NULL`，打印机选择列表里只有“PNG image”，看不到系统打印机；SB2K 映射器 `Reset` 里固定使用 Dendy 时序（CPU 1773447 Hz、312 行、50 Hz），游戏选项里改的制式在硬复位后会被改回。

## 编译

Windows，Visual Studio 2026（含“使用 C++ 的桌面开发”与 v145 工具集）。只支持 Win32，可在 Visual Studio 里打开 `VirtuaNES.slnx`，或用脚本编译：

```powershell
.\build.ps1 -Configuration Release
```

输出在仓库根目录的 `build\virtuanes\<配置>\bin\VirtuaNES.exe`，同目录有 `Chinese.vlp` 与运行后生成的 `VirtuaNES.ini`；编译前只结束调试实例（调试服务监听 `127.0.0.1` 端口的 `VirtuaNES.exe`）；用户自己打开的模拟器不会被结束，它们占用输出文件时编译报错，需先自行关闭。没有 `C:\DirectXSDK` 时用 Windows SDK 自带的 DirectInput 头文件即可编译。

语言包用相对文件名加载（上游行为），所以要从 exe 所在目录启动（工作目录是该目录），否则界面不显示中文。`build.ps1 -Run` 按此启动。

## 调试服务命令

命令表在 [05-模拟器与调试](../packages/fcdev/docs/05-模拟器与调试.md) 第 5.1 节，随 `fcdev` 包发布，只在那里维护。

## 配置

| 名称 | 说明 | 默认值 | 必填 |
| --- | --- | --- | --- |
| `FCDEV_DEBUG_PORT` | 模拟器调试服务的端口，未设置则不启动调试服务 | 无（fcdev 默认使用 5699） | 否 |
