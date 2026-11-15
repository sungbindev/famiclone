# FC mapper 移植任务书

把 MesenCE（`D:\GitCode\MesenCE`，GPL-3.0）里支持的 FC 卡带 mapper，按它的 mapper 设计逐个做成 Ember 的 mapper 插件。本文是写给执行者（以后的我）看的任务书：写清范围、总量、要读的源码、Mesen 的设计要点、Ember 现状与缺口、统一做法、阶段、台账与坑。用户还没有说“开始”，**现在不执行**；等用户说开始后，按本文一路做完。

状态说明同 [01-软件架构设计.md](01-软件架构设计.md)：标“已定”的是用户确认过的，标“拟定”的是我的建议，实施前要向用户确认（第 10 节列全）。

本文是任务书，所以直接写了 MesenCE 的类名、文件名与对比（与“文档不写出处”的项目约定不同）。做完后本文删除，规范性内容并入 [03-mapper插件规范.md](03-mapper插件规范.md)，那里与插件、代码里仍不写出处，来源与许可只登记在 `third-party.json`。

## 1. 用户要求与范围

### 1.1 用户原话

- 第一次：“下面开始把 D:\GitCode\MesenCE里支持的mapper都做成插件。”
- 第二次：“只做fc的mapper，其他都不变，参考mesen的mapper设计引入本项目中做成插件，这个先不做，你先做成一个文档，把相关资料信息写进去，等过两天直接执行，这个文档是要给你看的，所以你要写详细。”

### 1.2 范围

- 只做 FC 卡带的 mapper：按 iNES 的 mapper 号与子 mapper（变体）匹配，插件放在 `plugins/<id>/`，随安装包预装，包格式 `.emap` 不变。
- 其他都不变：整机机型与 BIOS、学习机、调试服务、界面、现有的官方插件 `nrom`、`qls`、`plq` 与测试插件 `test-cnrom` 的行为都不改（改动只限为支持新插件而新增的宿主能力，且只加不改）。
- 已定的约定沿用：宿主放芯片库、插件放卡带接线（`MapperCircuit` 契约与 `core/src/logic/hardware/mapper-chips/`）；寄存器读写、查表与计时优先写成清单声明，由宿主原生执行；脚本只在声明写不出的地方使用；`apiVersion` 仍为 1，只加字段不改已有字段的含义。
- 开发阶段：不考虑修改期间可不可用、旧数据迁移与向后兼容；成本、复杂度、工作量都不是取舍依据。

### 1.3 不做的项

| 项 | 原因 |
| --- | --- |
| FDS（`65535`）、NSF（`65534`） | 不是卡带；FDS 要磁盘机与 RAM 适配器，NSF 是播放器 |
| StudyBox（`186`、`65533`） | Mesen 自己也当 BIOS 处理，不是 NES 文件 |
| Famicom Network System（`65532`，类 `FnsMmc1`） | 要网络适配器，不是普通卡带 |
| VS System（`99`，类 `VsSystem`） | 街机主板，不是 FC 卡带；有投币、DIP 与专用手柄 |
| mapper `171`（类 `Kaiser7058`） | 03 第 4.2 节规定它是学习机 BIOS 的 mapper 号，没有匹配的插件时必须报 `mappers.noMatch`，不能做成卡带插件 |
| UNIF 板（`UnifBoards::*`，共 9 个：`Ac08`、`Cc21`、`Ghostbusters63in1`、`Gs2013`、`Malee`、`SssNrom256`（`FamicomBox`）、`Unl255in1`、`Unl8237A`、`UnlPuzzle`） | Ember 只读 iNES（`ines-parser` 对 UNIF 报“不支持的格式”），没有对应的 mapper 号 |
| Mesen 自己没有实现的号：`256`、`257`、`271`、`295`、`308`、`315`、`322`、`327`、`335`、`337`-`342`、`344`、`345`、`350`、`524`-`528` | 没有可移植的行为 |

- 外设部分不做，只做总线部分：`BandaiFcg`（16、153、157、159）的 Datach 条码机、`BandaiKaraoke`（188）的麦克风、`OekaKids`（96）的手写板、`Racermate`（168）的自行车。Ember 的 `devices` 目前只有 `mouse`；这些卡带的存储、寄存器、IRQ、EEPROM 照做，外设写进待办备忘。

## 2. 总量与统计方法

统计在 2026-11-16 做，口径是静态扫描（解析 `MapperFactory.cpp` 的 `switch`，沿继承链收集特性），特性标记是近似值，**每个 mapper 动手前都要读源码核对**，不能只信台账。同一方法能在做完后复核总量。

| 项 | 数量 |
| --- | --- |
| `Core/NES/Mappers` 下的头文件 | 290 个（另有 `Rainbow.cpp`），共 22306 行，含 FDS、NSF、声音、闪存等辅助文件 |
| 工厂里的 `case` 标签 | 323 个：24 个 `break`（Mesen 未实现），9 个 UNIF 板，4 个特殊号（FDS、NSF、StudyBox、FNS），286 个有类的 iNES 号 |
| iNES 号 | 286 个（`4` 与 `34` 各有两个类，按子 mapper 或 CHR 是否存在区分，所以台账有 288 行） |
| 去掉不做的（`99`、`171`） | 284 个号，251 个类 |
| 要做的插件 | 251 个（每个类一个，见第 10 节第 1 项），其中 `nrom` 已有，新增 250 个 |
| 台账里这些类的头文件合计 | 19289 行（只算类自己的文件，不含基类） |

按阶段分（阶段含义见第 9 节）：

| 阶段 | 内容 | 号 | 类 |
| --- | --- | --- | --- |
| 2 | 只要 bank 切换与镜像、读寄存器，没有钩子与声音 | 149 | 140 |
| 3 | 逐个名称表映射、自定义 CHR 读 | 11 | 6 |
| 4a | MMC3 家族（继承链含 `MMC3`） | 62 | 58 |
| 4b | 其余带 IRQ、CPU 周期钩子、PPU 地址钩子的 | 47 | 37 |
| 5 | EEPROM 与闪存 | 6 | 3 |
| 6 | 扩展声音 | 9 | 7 |
| - | 不做（`99`、`171`） | 2 | 2 |
| 合计 | | 286 | 253 |

按特性标记分（不含不做的；括号里是号的数量）：

| 标记 | 含义 | 类 | 号 |
| --- | --- | --- | --- |
| `C` | CPU 周期钩子（`EnableCpuClockHook` 或 `ProcessCpuClock`） | 37 | 52 |
| `V` | PPU 地址钩子（`EnableVramAddressHook` 或 `NotifyVramAddressChange`） | 71 | 78 |
| `I` | 有 IRQ（`TriggerIrq`、`SetIrqSource`、IRQ 计数器） | 92 | 106 |
| `A` | 扩展声音 | 7 | 9 |
| `N` | 逐个名称表映射（`SetNametable`、`GetNametableCount`、把 `$2000` 起的地址映射到 RAM 或 CHR） | 16 | 25 |
| `R` | 重写了 `ReadRegister`（读寄存器） | 42 | 54 |
| `K` | 自定义 PPU 读或 CPU 读（`EnableCustomVramRead`、`EnableCustomRamRead`） | 5 | 7 |
| `E` | EEPROM、I2C、闪存 | 4 | 7 |
| `B` | 总线冲突固定为真（`HasBusConflicts` 返回 `true`）；按子 mapper 决定的另有 4 个类（`UNROM`、`CNROM`、`AXROM`、`UnRom512`），不在此列 | 11 | 13 |
| `D` | DIP 开关（`GetDipSwitchCount`） | 12 | 13 |
| `S` | 重写了存档 RAM 大小（`GetSaveRamSize`、`ForceSaveRamSize`、`ForceChrBattery`） | 66 | 76 |
| `W` | 重写了工作 RAM 大小 | 11 | 12 |
| `H` | 重写了 CHR RAM 大小（`GetChrRamSize`） | 15 | 22 |
| `M` | 有 mapper 自带 RAM（`GetMapperRamSize`） | 3 | 4 |

- 同一个类用在多个号上的（一个插件用多项 `match`，脚本按 `ctx.header.mapper` 分支）：`FrontFareast`（6、8、17）、`ColorDreams`（11、144）、`BandaiFcg`（16、153、157、159）、`Namco163`（19、210）、`VRC2_4`（21、22、23、25、27、183）、`VRC6`（24、26）、`NovelDiamond`（54、201）、`Kaiser202`（56、142）、`Bandai74161_7432`（70、152）、`JalecoJf17_19`（72、92）、`MMC3_ChrRam`（74、119、191、192、194、195）、`VRC1`（75、151）、`Nina03_06`（79、113、146）、`TaitoX1005`（80、207）、`TaitoX1017`（82、552）、`JalecoJfxx`（87、101）、`JyCompany`（90、209、211）、`Sachen8259`（137、138、139、141）、`Sachen74LS374N`（150、243）、`Subor166`（166、167）。构造参数不同的（如 `VRC6(VRC6a)`、`Sachen8259(Sachen8259B)`）见台账的类列。
- 同一个号有两个类的：`4`（`MMC3`；子 mapper 3 是 `McAcc`）、`34`（子 mapper 0 时按有无 CHR ROM 选，有 CHR ROM 是 `Nina01`，没有是 `BnRom`；子 mapper 1 是 `Nina01`，2 是 `BnRom`）。
- Mesen 的类继承：`MMC3` 与它的派生类共 58 个（4a 阶段），`MMC1` 派生 `MMC1_105`、`MMC1_155`、`FaridSlrom`，`Rambo1` 派生 `Rambo1_158`，`MMC2`、`MMC4` 各自独立。

## 3. 开工前必读

文档与规则：

- [03-mapper插件规范.md](03-mapper插件规范.md)：清单、脚本接口、限制，全文必读；第 6.4 节说了“电路构件”的方向。
- [02-硬件模块.md](02-硬件模块.md) 第 3.4 节（`MapperCircuit`）、第 4 节（名称表与显存控制线）、第 5 节（存储区规则）。
- [06-构建与模块登记.md](06-构建与模块登记.md)：模块登记与依赖检查。
- `ember/.github/copilot-instructions.md`：测试分层（开发中只跑相关用例，改了 mapper 宿主这类底层或交付前才跑全量 `npm run check`）、注释不写出处、不暂存。
- copilot-plugins 的规则：`large-task-planning`（计划写进笔记，确认一次后一路做完）、`code-header-comments`（每个 `.js`、`.cpp`、`.hpp` 要有文件头与中文注释）、`code-testing`、`decision-scope`。

Ember 的代码（都在 `core/` 下）：

| 文件 | 内容 |
| --- | --- |
| `src/contracts/hardware/cartridge.hpp`、`cartridge-slot.hpp`、`bus-device.hpp`、`irq-output.hpp`、`scheduler.hpp`、`mapper-circuit.hpp` | 卡带、卡槽、总线设备、IRQ 引脚、调度器与 mapper 电路的契约 |
| `src/contracts/mappers/cartridge-image.hpp`、`mapper-cartridge.hpp`、`mapper-factory.hpp` | 卡带映像（规范化描述）、由插件创建的卡带、匹配与创建 |
| `src/infra/mappers/manifest.hpp`、`manifest.cpp` | 清单的数据结构与检查（上限：存储区 16、窗口 64、寄存器 64、状态 64、查表 16、匹配项 32） |
| `src/infra/mappers/mapper-chip.hpp`、`mapper-chip.cpp` | 卡带实现：窗口译码表、声明式寄存器、查表、磁带计时、回调脚本 |
| `src/infra/mappers/script-runtime.hpp`、`script-runtime.cpp` | QuickJS 沙箱与 `ctx` 对象 |
| `src/infra/mappers/plugin-registry.cpp` | 插件登记；安装与读取时用空卡带试创建一次（会执行脚本顶层代码），预装 250 多个包时要核对这一步对启动的影响 |
| `src/logic/cartridges/ines-parser.cpp` | NES 文件头解析（目前只有 mapper、变体、PRG/CHR 大小、镜像、电池、训练器标志、CRC） |
| `src/logic/hardware/mapper-chips/mmc3/` | 已有的 MMC3 芯片（`Mmc3`，A12 滤波、IRQ 计数） |
| `test/mappers/`、`test/support/plugin-test-support.hpp` | 插件、清单、脚本、卡带的测试与测试工具（`RecordingIrq`、`FakeTapeLine`、`officialPluginPackage` 等） |
| `plugins/nrom`、`plugins/qls`、`plugins/plq` | 现有三个官方插件，写法的样板 |
| `tools/build/pack-plugins.mjs` | 打包：`plugins/<插件>/` 只能有 `manifest.json`、`main.js`、`icon.png` |
| `tools/third-party/third-party.mjs` | 第三方清单：读取 `core/third_party/*/third-party.json` |

Mesen 的代码（`D:\GitCode\MesenCE\Core\NES\`）：

| 文件 | 内容 |
| --- | --- |
| `BaseMapper.h`、`BaseMapper.cpp`（1197 行） | 所有 mapper 的基类：页映射、寄存器、RAM、名称表、钩子；第 4 节是摘要 |
| `MapperFactory.cpp`（657 行） | 号到类的映射（台账的来源） |
| `Loaders/iNesLoader.cpp`、`NesHeader.cpp`、`RomData.h`、`GameDatabase.cpp` | 文件头解析、RAM 大小、数据库修正；数据库文件是 `UI/Dependencies/MesenNesDB.txt` |
| `Mappers/<厂商>/<类>.h` | 每个 mapper 的实现，台账的“文件”列给出路径 |
| `Mappers/A12Watcher.h`、`Mappers/Audio/*.h` | A12 监视器与各扩展声音芯片 |
| `Mappers/Homebrew/FlashS29.h`、`FlashSST39SF040.h`；`Utilities/` 下的 `BaseEeprom24C0X`、I2C | 闪存与 EEPROM |

## 4. Mesen 的 mapper 设计摘要

### 4.1 类结构与生命周期

- 每个 mapper 是 `BaseMapper` 的子类，必须实现 `InitMapper()`、`GetPrgPageSize()`、`GetChrPageSize()`；可选实现 `InitMapper(RomData&)`、`Reset(bool softReset)`、`WriteRegister`、`ReadRegister`、`ProcessCpuClock`、`NotifyVramAddressChange` 与一批 `Get...`、`Enable...` 虚函数（见下）。
- `Initialize` 的顺序（`BaseMapper.cpp` 第 617 行起）：确定 RAM 大小 → 登记寄存器范围 → 复制 PRG/CHR → 总线冲突 → 分配存档 RAM、工作 RAM、mapper RAM、名称表 RAM → 清空页表 → 无 CHR ROM 时建 CHR RAM 并整段映射 → 训练器 → `UpdatePageSizes` → 默认的 `$6000-$7FFF` RAM 映射 → 按文件头设镜像 → 读电池。之后才调用 `InitSpecificMapper`（先 `InitMapper()`，再 `InitMapper(RomData&)`）。所以子类的 `InitMapper` 里的初始页映射，对应 Ember 窗口的 `initial` 或脚本的 `reset`。
- `Reset(softReset)`：`false` 是上电，`true` 是软复位；对应 Ember 脚本的 `reset(ctx, 'power' | 'soft')`。
- `Serialize` 里 `SV(...)` 列出的成员就是 mapper 的全部内部状态，移植时用它核对清单的 `state` 声明有没有漏项（Ember 暂无存档，但调试服务要能读到这些状态）。

### 4.2 页映射

- 地址空间按 256 字节一页管理：CPU 侧 `_prgPages[0x100]`，PPU 侧 `_chrPages[0x40]`，每页有来源类型（`PrgRom`、`SaveRam`、`WorkRam`；`ChrRom`、`ChrRam`、`NametableRam`）、偏移与访问权限（读、写）。地址范围必须是 256 的整数倍。
- `SetCpuMemoryMapping(起, 止, 页号, 类型, 访问)`：页号按该类型的页数取模，负数从末尾数（`页数 + 页号`，页数不是 2 的幂也成立，如 mapper 191）；范围比页大时依次映射连续的页并回绕。
- 页大小由子类的 `GetPrgPageSize()`、`GetChrPageSize()` 给出，实际取 `min(页大小, ROM 大小)`；`SelectPrgPage(槽, 页)` 的起始地址是 `0x8000 + 槽 * 页大小`；`SelectPrgPage2x`、`4x`、`SelectChrPage2x`、`4x`、`8x` 是把连续的页一起映射。
- 特例：PRG 小于 32K 且页比 PRG 大时，`SelectPrgPage` 把整个 PRG 在 `$8000` 起重复映射（mapper 79 的 Pyramid 需要）。Ember 的窗口按存储区容量回绕，效果相同。
- 页的访问权限：`PrgRom` 只读，`SaveRam`、`WorkRam` 读写；`RemoveCpuMemoryMapping` 取消映射（读开放总线）；还能把任意内存块（`uint8_t*`）映射进去，例如把 mapper RAM 或 CHR 的某段映射进名称表。

### 4.3 寄存器与读写路径

- `RegisterStartAddress()`、`RegisterEndAddress()` 给出寄存器范围，默认 `$8000-$FFFF`；`AddRegisterRange`、`RemoveRegisterRange` 可按读、写分别增减；台账的“寄存器范围”列是继承链里最近的重写值，“默认”表示没有重写（有些 mapper 在 `InitMapper` 里调用 `AddRegisterRange` 增减，需读源码）。
- 写：`WriteRam(地址, 值)` 若地址是写寄存器，先处理总线冲突（`值 &= 该地址当前 PRG 字节`），再调用 `WriteRegister`；否则 `WritePrgRam`，写到映射着的 RAM 页。
- 读：地址是读寄存器且 `AllowRegisterRead()` 为真时调用 `ReadRegister`；`EnableCustomRamRead()` 为真时子类重写 `ReadRam`（读 `$6000` 以上的自定义行为）；其余读映射页，没有映射的返回开放总线（高字节）。
- `$4020-$5FFF` 也属于卡带（`GetMemoryRanges` 登记 `$4020-$FFFF`），所以很多 mapper 的寄存器在 `$4100`、`$5000` 一带。

### 4.4 钩子

- `EnableCpuClockHook()` 为真时每个 CPU 周期调用一次 `ProcessCpuClock()`：IRQ 计数器（VRC、Bandai、FME-7、Jaleco、JY 等）和声音芯片靠它。
- `EnableVramAddressHook()` 为真时 PPU 总线地址每变化一次调用 `NotifyVramAddressChange(地址)`，包括 `$2006`、`$2007` 引起的变化：MMC3 的 A12 计数、MMC2/4 的锁存、MMC5 的扫描线检测靠它。MMC3 用 `A12Watcher`：A12 为低的周期数大于 `minDelay`（默认 10 个 PPU 周期）后的上升沿才算一次。
- `EnableCustomVramRead()` 为真时子类重写 `MapperReadVram`（MMC5、JY、Rainbow、UnlDripGame、Fds、CnromProtect 用）。
- 少数 mapper 用主时钟：`MMC1` 忽略与上一次写入相距不到 2 个周期的写入（`_console->GetMasterClock()`），`MMC3.h`、`Mmc5Audio.h`、`Vrc7Audio.h`、`StudyBox.h` 也读。

### 4.5 名称表与 CHR

- 名称表 RAM 默认 2 页（四屏时 4 页，子类可用 `GetNametableCount()` 改）；`SetMirroringType` 与 `SetNametables(a, b, c, d)` 把 `$2000-$2FFF` 的四个 1K 映射到名称表页，并同时映射 `$3000-$3FFF`（不是重定向）；`SetNametable(槽, 页)` 逐个设置；也可以把 CHR ROM 或 RAM 映射进 `$2000` 起的地址（mapper 68、19、5、90 等）。
- 没有 CHR ROM 时自动建 CHR RAM（默认 8K，`GetChrRamSize()` 可改）并整段映射到 `$0000-$1FFF`；`InitializeChrRam(大小)` 可手动建。
- 页偏移在 PPU 侧也是 256 字节粒度；`ChrMemoryType::Default` 表示“有 CHR ROM 用 ROM，否则用 RAM”。

### 4.6 RAM、电池、总线冲突、上电状态

- 存档 RAM（`SaveRam`，带电池，存 `.sav`）与工作 RAM（`WorkRam`，不存）是两块：文件头或数据库没给大小（`-1`）时，带电池的卡带用 `GetSaveRamSize()`（默认 8K），不带电池的用 `GetWorkRamSize()`（默认 8K）；给了大小就用给的，`ForceSaveRamSize()`、`ForceWorkRamSize()` 为真时强制用子类的。
- `GetMapperRamSize()` 是 mapper 自己的 RAM（如 MMC5 的 ExRAM、N163 的 128 字节声音 RAM），不对应文件头。
- 总线冲突：`HasBusConflicts()` 给默认值，文件头或数据库可覆盖为“是”或“否”。
- 上电随机：`GetPowerOnByte()` 在“随机化 mapper 上电状态”开关打开时返回随机字节，否则返回默认值；RAM 初始内容也受该开关影响。
- 训练器：512 字节载入工作 RAM（没有则存档 RAM）的 `+0x1000` 处，即 `$7000-$71FF`。
- DIP 开关：`GetDipSwitchCount()` 与 `GetDipSwitches()`，取值来自用户的游戏配置。
- PRG 不足 256 字节时，加载器把它重复到至少 256 字节。

### 4.7 辅助部件

- `A12Watcher`（A12 滤波）、`Audio/` 下的 `Vrc6Audio`（`Vrc6Pulse`、`Vrc6Saw`）、`Vrc7Audio`、`Namco163Audio`、`Sunsoft5bAudio`、`Mmc5Audio`、`UnlDripGameAudio`、`RainbowAudio`、`FdsAudio`；`Epsm`（卡带上的 FM 扩展，文件头标志触发）；闪存 `FlashS29`、`FlashSST39SF040`；EEPROM `BaseEeprom24C0X`。

### 4.8 工厂、子 mapper 与数据库

- `GetMapperFromID` 的 `switch`：多数是一号一类，子 mapper 影响的只有 `4`、`34`；构造参数区分变体（如 `VRC6(VRC6a)`、`Sachen8259(Sachen8259B)`、`MMC3_ChrRam(0x08, 0x09, 2)`）。
- 数据库（`MesenNesDB.txt`，按 PRG+CHR 的 CRC）能改写文件头：mapper 号、子 mapper、RAM 大小、镜像、总线冲突、PRG/CHR 大小。Ember 不引入数据库（第 10 节第 3 项），所以对头不准的老文件会和 Mesen 不同。

### 4.9 Mesen 概念到 Ember 概念

| Mesen | Ember | 状态 |
| --- | --- | --- |
| `PrgRom`、`ChrRom` | 存储区 `rom`，`from: file.prg`、`file.chr` | 有 |
| `ChrRam`（无 CHR ROM 时） | `chr` 的 `fallback` RAM | 有（大小写死，见 G6） |
| `SaveRam`、`WorkRam` | 存储区 `ram`，`battery` | 有（大小写死，存档文件后续阶段，见 G6） |
| `SelectPrgPage(槽, 页)` | `ctx.window(id).setBank(n)` | 有 |
| 页大小可随模式变（如 MMC1 的 16K 与 32K） | 两个窗口加 `setOffset(字节偏移)` | 有 |
| `SetCpuMemoryMapping` 的访问权限 | 窗口的 `access` | 有（固定，不能运行时改，见 G4） |
| `RemoveCpuMemoryMapping` | `window.unmap()` | 有 |
| 同一地址在 ROM 与 RAM 之间切换 | 无 | 缺（G4） |
| `AddRegisterRange`、`WriteRegister`、`ReadRegister` | 清单 `registers`，回调脚本或声明 | 有 |
| `HasBusConflicts` | 无 | 缺（G5） |
| `ProcessCpuClock`、主时钟 | 无 | 缺（G1） |
| `NotifyVramAddressChange` | `MapperCircuit::onVideoAddress`（只有主板能用） | 缺（G2） |
| `SetMirroringType` | `ctx.nametable.set(mode)` | 有 |
| `SetNametable`（逐个，含映射 CHR、RAM） | 无 | 缺（G3） |
| `TriggerIrq`、`ClearIrqSource` | `ctx.irq.assert()`、`release()` | 有 |
| 扩展声音 | 无 | 缺（G7） |
| EEPROM、闪存 | 无 | 缺（G8） |
| 文件头的 RAM 大小、NES 2.0 | 描述里没有 | 缺（G6） |
| DIP 开关、`GetPowerOnByte` | `ctx.settings`、`ctx.random`（后续阶段，未实现） | 缺（G10） |
| `MapperReadVram`、`ReadRam` 重写 | 无 | 缺（G9） |
| `Serialize` 的成员 | 清单 `state` | 有 |

## 5. Ember 现状

- 卡带由插件创建：`MapperChip`（`infra/mappers`）按清单建存储区、窗口、译码表；`Cartridge` 契约只有 `cpuSide`、`ppuSide`、`ciramControl`、`reset`、`input`，创建时只收到 `IrqOutput` 与 `PadSink`，**拿不到时钟、调度器，也没有声音输出**。
- 清单已有字段：`id`、`name`、`version`、`apiVersion`、`publisher`、`description`、`host`、`devices`、`match`、`storage`、`windows`、`nametable`、`registers`、`state`、`tables`、`tape`、`settings`（只解析）。`registers` 的读写可以是回调脚本或声明（固定值、开放总线、状态、`bank`、查表）。
- 窗口：同一总线上互不重叠；大小等于地址范围；CPU 侧在 `$4020-$FFFF`；PPU 侧只在 `$0000-$1FFF`；指向 ROM 的窗口只读；一个窗口只绑一个存储区；bank 号按存储区的 bank 数取模，窗口访问越过末尾时回绕；`setOffset` 按字节偏移。
- 脚本 `ctx`：`header`、`window`、`nametable.set`、`irq`、`storage`、`state`、`log.warn`、`keys`、`pad`、`device('tape')`；`ctx.timer`、`ctx.random`、`ctx.settings` 未实现。脚本回调的开销：读约 1.9 微秒，写约 10 微秒（测试构建），所以每帧大量访问的寄存器要写成声明。
- 匹配：`mapper` 必须写，可选 `variant`、`prgSize`、`chrSize`、`crc32Prg`、`crc32All`，都是相等比较；写出条件多的优先，条件数相同报冲突。
- 文件头描述（`CartridgeDescription`）：`mapper`、`variant`、`prgSize`、`chrSize`、`mirroring`、`battery`、`trainer`、`crc32Prg`、`crc32All`；没有 RAM 大小、是否 NES 2.0、总线冲突、NES 2.0 的 PRG/CHR NVRAM 大小；训练器内容不载入；带电池的 RAM 只在内存里保持。
- 硬件模块：`MapperCircuit` 契约（寄存器、`onVideoAddress`、页号与页大小、镜像、IRQ 复位、状态列表）与 `Mmc3` 芯片；目前只有 sb2000 主板在用，清单还不能引用芯片。
- 宿主的 PPU 在 `ppuSide().read/write` 上访问卡带；`MapperCircuit::onVideoAddress` 目前只有 sb2000 主板在调用。`$2006`、`$2007` 引起的地址变化与渲染以外的访问能不能传到卡带，执行 G2 时要先读 `ppu-2c02` 与 `fc` 主机型号的装配来确认。
- 已有官方插件：`nrom`（mapper 0，带 8K 的 `$6000` PRG RAM）、`qls`（172，PRG 32K）、`plq`（172，PRG 16K）；测试插件 `test-cnrom`（mapper 3）。

## 6. 宿主缺口与拟定设计

下面每一项都是“拟定”，用户确认后实施；每项实施时同步 03 的对应章节、清单检查（`manifest.cpp`）、`manifest-test`、`mapper-chip-test` 与新增的用例，并保持只加字段、不改已有字段的含义。

### G1 时间基准与周期计数器

- 需要它的：`C` 标记的 52 个号（VRC 的 IRQ、Bandai FCG、Jaleco SS88006、Irem H3001、Sunsoft 3、4 与 FME-7、JY、Mapper 40、42、43、50、83、106 等），`MMC1` 家族（忽略连续写入）。
- 现状：卡带没有时钟，脚本不能读时间，`ctx.timer` 未实现。
- 拟定：
  - 创建卡带时多传入 `Scheduler&`（读当前时间、登记事件）与 CPU 的时钟域，按 CPU 周期计时；修改 `MapperFactory::create` 的签名与它的调用处（运行模块、装配代码、测试支持）。
  - 脚本新增 `ctx.cycle()`：返回自上电起的 CPU 周期数（整数）；MMC1 用它忽略连续写入。
  - 清单新增声明式计数器 `counters`：每项 `{ id, bits, direction: "up" | "down", clock: "cpu" | ..., enable, reload, onWrap }`，宿主用调度器事件预测溢出时刻，不逐周期运行；溢出时可拉低 IRQ、重载、置状态位。复杂的（VRC 的预分频与扫描线模式、JY 的多种计数源、FME-7 的启用位）做成命名电路（G2），简单的（Bandai FCG 的 16 位递减、Mapper 50 的 12 位递增等）用声明。
  - `ctx.timer`（`after`、`cancel`）也实现，脚本里偶发的延时用它。
- 测试：用假调度器与假 IRQ 引脚推进周期，断言溢出时刻与 IRQ 电平；与 `MMC1` 连续写入的用例。

### G2 PPU 地址监视与电路构件的清单引用

- 需要它的：`V` 标记的 78 个号，其中 MMC3 家族 62 个号；MMC2、MMC4（图案表读取到 `$FD`、`$FE` 时切换 CHR）；MMC5；JY；Rambo-1。
- 现状：`Mmc3` 芯片已有并实现 `MapperCircuit`，但只有 sb2000 主板在用；`MapperChip` 不会把 PPU 地址交给任何电路；清单不能引用芯片。
- 拟定：
  - 清单新增 `circuits`：`[{ "id": "mmc3", "type": "mmc3", "irq": true }]`，`registers[].write`/`read` 可以写 `{ "circuit": "mmc3" }` 把范围内的访问转给电路；电路的页号经 `bank(space, address)` 取出，声明 `banks`（电路的页空间与地址 → 窗口）后宿主在电路状态变化时自动 `setBank`。
  - 变体需要改页号时（外部 bank 寄存器、掩码、偏移），脚本导出 `onCircuit(ctx, id)`：电路每次写寄存器后调用，脚本读 `ctx.circuit(id).bank('program', 0x8000)` 等再自己设窗口；MMC3 的 `$A000` 镜像位、IRQ 由电路管。
  - `MapperChip::ppuSide` 的读写把地址交给声明了 PPU 监视的电路（`onVideoAddress`）；其他需要监视的芯片（MMC2/4 锁存、MMC5 扫描线）也是电路，各一个目录放 `core/src/logic/hardware/mapper-chips/<芯片>/`，实现 `MapperCircuit`，测试在 `core/test/hardware/mapper-chips/`。
  - `Mmc3` 现在用“A12 先低过 4 次显存访问”滤波，Mesen 用“A12 低够 10 个 PPU 周期”；以 Ember 现有的为准，不改；有测试 ROM（如 blargg 的 MMC3 测试）可用时再核对，没有就在汇报里写明未核对。
- 测试：每个电路的单元测试（已有 `mmc3-test.cpp` 的写法）；清单检查（引用不存在的电路、重复的电路 id 等）；MMC3 插件的端到端向量（见第 8 节）。

### G3 逐个名称表映射

- 需要它的：`N` 标记的 25 个号（mapper 68 的 Sunsoft 4、19 的 N163、5 的 MMC5、90 的 JY、80、207 的 Taito X1005、137-141 的 Sachen 8259、150、243、218、`Kaiser7037`、`TxSRom`（118）等）。
- 现状：`nametable` 只有 `header`、`horizontal`、`vertical`、`single0`、`single1`、`four` 六种；PPU 窗口只能在 `$0000-$1FFF`；`ciramControl` 只能选“主板显存的哪一页”或“卡带自己响应”。
- 拟定：
  - `nametable` 新增 `"custom"`；此时 PPU 窗口可以放在 `$2000-$2FFF`（`$3000-$3EFF` 镜像），一个窗口 1K，指向任意存储区（RAM 或 CHR ROM）；脚本用 `ctx.nametable.map(槽, 来源)` 逐个设置，来源是 `"ciram0"`、`"ciram1"`（主板显存的两页，`ciramControl` 返回 `enable` 为真和对应的 `a10`）或 `"window:<窗口 id>"`（卡带自己响应，`enable` 为假）。
  - 声明式：`registers[].write.nametable` 暂不做；这类 mapper 写入不频繁，用脚本。
- 测试：`ciramControl` 对各来源的返回；窗口读写落到正确的存储区；`$3000` 镜像。

### G4 同一地址范围在 ROM 与 RAM 之间切换

- 需要它的：ROM 与 RAM 映射到同一地址范围、运行时二选一的 mapper：`SunsoftFme7`（69）、`Kaiser202`（56、142）、`Mapper103`、`Lh32`（125）、`Kaiser7037`（307）、`Lh10`（522）、`MMC5`（5）、`JyCompany`（90、209、211）；MMC1、MMC3 家族的 PRG RAM 使能与写保护位也要它（Mesen 在使能位变化时改页的访问权限）。只把 ROM 映射到 `$6000` 的（如 `VRC2_4` 的部分号、`Mapper40`、`42`、`43`、`50`）不需要，用一个 ROM 窗口即可。
- 现状：窗口只绑一个存储区，窗口之间不能重叠，所以不能用两个窗口表达“同一地址二选一”；窗口的 `access` 写死。
- 拟定：窗口的 `storage` 可以写数组（如 `["prg", "prg-ram"]`），脚本用 `ctx.window(id).select(存储区, bank)` 切换；`size` 对所有候选存储区相同，`access` 随存储区种类（ROM 只读，RAM 读写）；初始指向数组的第一项；`setBank`、`setOffset` 作用于当前选中的存储区；`unmap` 不变；另加 `ctx.window(id).setAccess('r' | 'rw')`，RAM 的写保护用它（ROM 只能是 `r`）。清单检查：数组里的存储区必须存在。
- 测试：切换后读写落点；ROM 候选拒绝写入；写保护；调试读取。

### G5 总线冲突

- 需要它的：15 个类。固定有冲突的 11 个：`BandaiKaraoke`（188）、`OekaKids`（96）、`FaridUnrom`（324）、`BnRom`（34）、`IremLrog017`（77）、`JalecoJf16`（78）、`JalecoJf17_19`（72、92）、`CnromProtect`（185）、`Sachen_148`、`ColorDreams`（11、144）、`Mapper234`；按子 mapper 决定的 4 个：`UNROM`（2）、`CNROM`（3）、`AXROM`（7）是子 mapper 2 时有冲突，`UnRom512`（30）是子 mapper 2，或子 mapper 0 且不带电池时有冲突。
- 拟定：`registers[].write` 的声明与回调都支持新字段 `busConflict: true`：写入值先与“该地址当前读到的 ROM 字节”相与，再交给声明或脚本；该地址没有映射 ROM 时不相与。按子 mapper 决定的，脚本在 `reset` 里用 `ctx.busConflict.set(true | false)` 按 `ctx.header.variant`、`ctx.header.battery` 设置。Mesen 的数据库与文件头也能覆盖这一项，Ember 不做。
- 测试：值被相与的几种情况（相等、不等、该地址没有映射 ROM）；`ctx.busConflict.set` 的切换。

### G6 文件头里的 RAM 大小、NES 2.0 与存档

- 需要它的：`S`、`W`、`H` 标记的 76、12、22 个号（卡带的 PRG RAM、CHR RAM 大小来自文件头）。
- 现状：存储区的 `size` 写死在清单里；`CartridgeDescription` 没有 RAM 大小。
- 拟定：
  - `CartridgeDescription` 新增 `nes20`、`prgRamSize`、`prgNvramSize`、`chrRamSize`、`chrNvramSize`（NES 2.0 头的字节 10、11；旧 iNES 头里 PRG RAM 大小在字节 8，为 0 时按 8K），`ines-parser` 解析并补测试；`ctx.header` 同步。
  - 存储区的 `size` 可以写 `"header.prgRam"`、`"header.prgNvram"`、`"header.chrRam"`、`"header.chrNvram"`，并可带 `default`（头里为 0 或旧头时用的默认大小，如 8192）与 `min`、`max`；`fallback` 同理。
  - 训练器内容载入 PRG RAM 的 `+0x1000`（`$7000-$71FF`）：清单的 RAM 声明里加 `trainer: true`。
  - 电池存档的文件读写在 03 第 4.3 节标为“后续阶段”：本任务不做，带电池的 RAM 仍只在内存里保持，写进待办备忘。
- 测试：头解析（NES 2.0 与旧头各几例）；`size` 取头里的值、默认值、越界报错。

### G7 扩展声音

- 需要它的：`A` 标记的 9 个号：MMC5（5）、N163（19、210）、VRC6（24、26）、FME-7（69，Sunsoft 5B）、VRC7（85）、UnlDripGame（284）、Rainbow（682）。
- 现状：卡带没有声音输出；主机的声音经 `Machine::takeAudio` 与 `AudioOutput`。
- 拟定：先读 `apu-2a03` 与混音的实现，确定卡带音频以什么形式并入（独立的 `AudioSource` 契约，由主机型号接入 APU 的混音输入，音量比按芯片），再做：契约（`contracts/hardware/`）、各芯片的电路（放 `mapper-chips/`）、清单的引用（G2 的 `circuits`）。VRC7 要 YM2413 的 FM 合成，放在最后。
- 测试：各通道的波形与音量（与芯片规格比对），混音后的采样范围；没有声音的卡带不增加开销。

### G8 EEPROM 与闪存

- 需要它的：`E` 标记的 7 个号：Bandai FCG（16、153、157、159，24C01、24C02 的 I2C EEPROM）、UnRom512（30，闪存）、Cheapocabra（111，闪存）、Rainbow（682，闪存，归阶段 6）。
- 拟定：在 `mapper-chips/` 下各一个电路（I2C EEPROM、S29 闪存、SST39SF040 闪存），清单用 G2 的 `circuits` 引用，存储仍是清单声明的存储区（`battery: true`）；存档文件读写同 G6，后续阶段。

### G9 自定义 CPU 读与 PPU 读

- 需要它的：`K` 标记的 7 个号：MMC5（扩展 RAM 模式下的属性读）、JY（90、209、211）、Rainbow、UnlDripGame、`CnromProtect`（185）。
- 拟定：寄存器的 `bus: "ppu"` 范围已支持读寄存器，但 PPU 侧寄存器范围限制在 `$0000-$1FFF`，且窗口与寄存器的优先级需按需放宽（读声明优先于窗口，已有）；实际要的是“PPU 读某地址时用脚本或电路决定值”：先看这 7 个 mapper 的具体读法，再决定是放宽 PPU 侧寄存器范围，还是用电路。这一项放在做到阶段 3 时再定，不阻塞前面的阶段。

### G10 脚本辅助与设置项

- `ctx.window(id).bankCount`：该窗口在其存储区里的 bank 数，用来写“倒数第 n 个 bank”（Mesen 的负页号）。
- `ctx.settings.get(key)` 与设置页：DIP 开关（`D` 标记的 13 个号）用 `switch` 或 `choice` 设置项；03 第 4.7 节标为“后续阶段”；实现时同步首选项与界面（见 04、05）。
- `ctx.random(max)`：上电随机 RAM 与 mapper 状态；Ember 默认全清零，`random` 只给确实要随机的 mapper。
- 这一组里只有 `bankCount` 是阶段 2 前必须的，其余随使用它们的 mapper 的阶段再做。

### G11 匹配条件

- `34` 号按“有无 CHR ROM”选 `Nina01` 或 `BnRom`；现有匹配条件只有相等比较。
- 拟定：`match` 新增 `chr: "none" | "present"`（有无 CHR ROM）；其他按需加（如 `prgSize` 的最小值）。
- 同一个号有两个插件时，条件多的优先：`4` 的 `McAcc`（`variant: 3`）与 `MMC3`（只有 mapper 号）可以共存。

### G12 声明式 bank 表达

- 现状：声明式 `bank` 只有 `(数据 >> shift) & mask`，且只能送一个窗口；很多多合一卡的 bank 来自地址线或多个寄存器的组合。
- 结论：阶段 2 的 mapper 写入不频繁，用回调脚本（每次约 10 微秒）；不为它新增声明语法。若某个真实游戏实测（如 PLQ 的经验）显示脚本太慢，再对该 mapper 增加声明，不预先做。

## 7. 现有插件与冲突

- `nrom`（0）：已有，不改；Mesen 的 `NROM` 已完成。
- `qls`、`plq`（172）：已有，不改；Mesen 的 `Txc22211B` 也是 172，新增插件 `txc22211-b` 只按 mapper 号匹配，条件数比 `qls`（mapper 加 PRG 32K）、`plq`（mapper 加 PRG 16K）少，所以 PRG 为 32K 或 16K 的 172 号文件仍由 `qls`、`plq` 处理，其余 172 号文件走 `txc22211-b`。
- `test-cnrom`（3）：测试插件，不随程序发布；新增官方的 `cnrom` 后，测试仍用自己的包。
- 03 第 9 节“首批官方插件”表要改成指向台账（本文第 12 节的台账并入后的位置），不再逐个列。

## 8. 统一做法

### 8.1 每个 mapper 的步骤

1. 打开台账“文件”列的头文件与继承链上的头文件，读完整：页大小、`InitMapper`、`Reset`、`WriteRegister` 与 `ReadRegister` 的每个分支、`Serialize` 的成员、RAM 大小的重写、镜像、总线冲突、钩子、`RegisterStartAddress`、`RegisterEndAddress`。核对台账的特性标记。
2. 写 `plugins/<id>/manifest.json`：
   - `id` 用台账的“插件 id”；`name` 写中文名或常用名；`version` 为 `1.0.0`；`publisher` 为 `Ember`；`description` 一两句话说明卡带、寄存器、与真机相比的不同。
   - `match`：台账的号与子 mapper；同类多个号写多项。
   - `storage`：`prg`、`chr`（`fallback` 是 CHR RAM）、`prg-ram` 等，按 G6 取大小。
   - `windows`：命名统一为 `prg-8000`、`prg-a000`、`prg-c000`、`prg-e000`（8K 窗口），`prg-8000-32k`（32K 窗口）这类“用途加起始地址”；CHR 窗口为 `chr-0000`、`chr-0400` 等；PRG RAM 为 `prg-ram`；`initial` 对应 Mesen 的 `InitMapper` 里的初始页。
   - `registers`：能声明就声明；`state` 的名字与 Mesen 的成员名对应（改成 camelCase 去掉前缀 `_`），`description` 写中文。
   - `nametable`：对应 `SetMirroringType` 的初始值。
3. 需要脚本时写 `plugins/<id>/main.js`：文件头（名称、说明、作者、邮箱、日期、备注）、中文注释只写原因与约束、默认导出对象、不写 `import`、不用 `Date`、`Math.random`；寄存器逻辑中凡是 Mesen 里“先改变量、最后调用 `UpdateState`”的写法，保留成同一个函数，保证每次写入后窗口一致。
4. 写测试向量（第 8.2 节），运行 `npm run plugins:pack`，再跑相关用例。
5. 更新台账该行的“状态”，并同步台账所在文档。

### 8.2 测试向量

- 位置（拟定）：`core/test/data/mapper-vectors/<插件 id>.json`；不放进 `plugins/<id>/`，因为包里只能有 `manifest.json`、`main.js`、`icon.png`。运行器 `core/test/mappers/mapper-vectors-test.cpp` 按目录里的文件参数化，用真实打好的 `.emap` 包创建卡带。
- 格式（拟定）：

```json
{
  "plugin": "mmc1",
  "image": { "mapper": 1, "variant": 0, "prgKiB": 256, "chrKiB": 128, "mirroring": "horizontal", "battery": false },
  "steps": [
    { "write": "$8000", "value": "0x80" },
    { "expect": [{ "bus": "cpu", "address": "$8000", "storage": "prg", "offset": "0x0000" }] },
    { "cycles": 3 },
    { "expectNametable": "vertical" },
    { "expectIrq": false }
  ]
}
```

- 断言落点用 `describeAddress`（总线地址对应的存储区与偏移，不回调脚本），所以不需要标记 ROM；镜像用 `ciramControl`；IRQ 用 `RecordingIrq`；周期与 PPU 地址推进用 G1、G2 提供的假调度器与假 PPU 访问。
- 期望值逐项对照 Mesen 源码写出：每个向量文件要覆盖“上电初值、每个寄存器的每种写法、复位行为、边界页（最后一页、超出页数的回绕）、镜像与 RAM 开关”。
- 没有商业游戏 ROM（`reference/rom` 只有学习机的 ROM），所以大多数插件只有向量验证；能找到公开测试 ROM 的（如 MMC3 的）再做端到端；汇报时写明哪些只有向量验证，不说成“已验证游戏能运行”。

### 8.3 命名与登记

- 插件 id 规则（拟定）：类名转小写加连字符（`MMC3_12` 为 `mmc3-12`，`VRC2_4` 为 `vrc2-4`）；`MapperNN` 为 `mapper-NN`；少于 3 个字符的用 `mapper-<号>`；重名的加号；上限 40 个字符。台账的“插件 id”列已按这个规则生成，动手前核对有无重名与不合规。
- 目录名等于清单里的 `id`；包大小上限（`main.js` 1 MB、`manifest.json` 256 KB、整个包 2 MB）远大于需要。
- 来源与许可：新增 `core/third_party/mesen-ce/third-party.json`（`name`、`version`、`authors`、`license: GPL-3.0`、`licenseFile`、`homepage`、`usage`）与许可全文 `LICENSE`，不放源码；Ember 自己是 GPLv3，兼容。插件的 `description`、注释、文档里不写“来自 Mesen”。

## 9. 阶段与完成标准

阶段按依赖顺序排；每个阶段结束先验证再进入下一阶段，未通过先修复。阶段 1 改的是 mapper 宿主，属于底层，结束时跑一次含测试 ROM 的 `npm run check`；阶段 2 到 6 只跑相关用例（`npm run core:test -- -R <名称>`）与 `npm run plugins:pack`、`npm run boundaries`；阶段 7 跑全量。

| 阶段 | 内容 | 完成标准 |
| --- | --- | --- |
| 0 | 台账复核与向量运行器：重新统计总量并与本文对照；写 `mapper-vectors-test.cpp`、向量格式、示例向量（用现有 `nrom`、`qls`） | 运行器能跑现有插件的向量；总量与本文一致 |
| 1 | 宿主能力：按依赖顺序做 G6（头与 RAM 大小）、G10 的 `bankCount`、G5（总线冲突）、G11（匹配条件）、G4（窗口多存储区）、G3（名称表）、G1（时钟与计数器）、G2（电路引用与 PPU 监视，含 MMC3 的引用）；G7、G8、G9 在各自的阶段前做 | 每项有清单检查、宿主实现、03 文档、用例；`npm run check` 通过 |
| 2 | 149 个号、140 个类的纯 bank 切换插件，按 Mesen 目录分批（Nintendo、Unlicensed、Waixing、Sachen、Kaiser、Ntdec、Txc、Jaleco、Irem、Bandai、Codemasters、Sunsoft、Taito、Homebrew、Whirlwind）；`MMC1` 家族先做（用 G1 的 `ctx.cycle()`） | 每个类有插件与向量；台账该阶段的状态全部更新 |
| 3 | 11 个号、6 个类：`TaitoX1005`、`Sachen8259`、`Sachen74LS374N`、`MagicFloor218`、`Kaiser7037`、`CnromProtect`；定 G9 | 同上 |
| 4a | 62 个号、58 个类的 MMC3 家族：先做 `mmc3`、`mc-acc`，再按“外部 bank 寄存器”“掩码与偏移”“CHR RAM 窗口”“特殊 IRQ”几类分批 | 同上；`mmc3` 的 IRQ 有时序用例 |
| 4b | 47 个号、37 个类：`VRC2_4`、`VRC1`、`VRC3`、`Namco108` 系、`FrontFareast`、`MMC2`、`MMC4`、`Rambo1` 系、`JyCompany`（最复杂）、`Mapper35`、`Mapper91`、`Sunsoft3`、`Sunsoft4`、Jaleco、Irem、Taito TC0690 等 | 同上；各 IRQ 有时序用例 |
| 5 | 6 个号、3 个类：`BandaiFcg`（先做 G8 的 I2C EEPROM）、`UnRom512`、`Cheapocabra`（闪存） | 同上 |
| 6 | 9 个号、7 个类：G7 的声音契约与各芯片，再做 `MMC5`（最复杂）、`Namco163`、`VRC6`、`SunsoftFme7`、`VRC7`、`UnlDripGame`、`Rainbow`（网络部分不做） | 同上 |
| 7 | 全量验证：台账 284 个号全部有状态；`npm run check`；03 文档并入（第 9 节改为台账与说明，第 6.4 节更新电路构件、新增 G1-G11 的规范）；02 第 3.4 节同步；`third-party.json`；待办备忘清理；`plugins:pack` 的 250 多个包的启动时间核对；删除本文；提交消息 | 汇报逐阶段对照；复核总量 |

## 10. 要用户确认的事项

下面是我的建议，开始前请用户确认（按“用户确认规范”的结构，开始时再整理成待确认列表）：

1. 粒度：一个 Mesen 类一个插件（同类的多个号写多项 `match`，脚本按 `ctx.header.mapper` 分支）。原因：与 Mesen 的设计一致，同类的寄存器逻辑只写一份，约 251 个插件；备选是每个号一个插件（约 284 个，重复多）。
2. 不做的项（第 1.3 节）：尤其是 mapper 171、VS System，以及外设部分（条码机、麦克风、手写板、自行车）。
3. 不引入游戏数据库：头不准的老文件会与 Mesen 不同，记入待办备忘。
4. 插件 id 规则（第 8.3 节）与台账里生成的 id。
5. 预装全部插件：安装包带 250 多个包；“mapper 插件”页（05 文档）列表很长，是否要加搜索与按厂商分组，阶段 7 核对界面后再提。
6. 宿主能力 G1 到 G12 的设计（第 6 节），尤其是 G1 的 `counters` 声明与 G2 的 `circuits` 字段、G3 的 `custom` 名称表、G4 的多存储区窗口。
7. 测试向量的位置与格式（第 8.2 节）。
8. 第三方登记的位置与名称（第 8.3 节）。

## 11. 已知风险与注意

- 脚本速度：声明式之外的寄存器读写每次几微秒到十几微秒；MMC1、各类多合一卡每帧只写几次，可用脚本；MMC3 的每次 A12 变化、VRC 的每个周期都不能进脚本，一律走宿主电路与声明。
- MMC3 的 A12 滤波与 Mesen 不同（第 6 节 G2），MMC3 家族以 Ember 现有的 `Mmc3` 为准；`Mmc3` 的程序页号是 6 位，页号线数、窗口基址由使用它的板子决定，卡带插件里要自己取模。
- 同一个 `RegisterStartAddress` 也可能在 `InitMapper` 里再用 `AddRegisterRange`、`RemoveRegisterRange` 改；台账的“寄存器范围”只是继承链里的重写值，必须读源码。
- `$4020-$5FFF` 的读：没有声明读的地址，Ember 返回开放总线；Mesen 在 `AllowRegisterRead` 为假时也一样，核对时看 `ReadRegister` 的每个分支有没有返回值。
- Mesen 读到未映射的 PRG 返回 `地址高字节`（假开放总线）；Ember 的开放总线由总线保存，读到最近的值，两者在这类边角上可能不同，不为它做特殊处理。
- 上电时 Mesen 的 RAM 内容可能随机（开关控制）；Ember 的不带电池 RAM 上电清零，不改。
- 预装包多时，`plugin-registry` 在读取预装目录时对每个包都用空卡带试创建一次，会执行脚本顶层代码；阶段 7 要实测启动时间，必要时改成惰性。
- 清单上限：存储区 16、窗口 64、寄存器 64、状态 64、查表 16、匹配项 32；`MMC5`、`JyCompany` 最接近上限，先数一遍再写。
- 做完某个类后要回头核对：同一个类的所有号（如 `VRC2_4` 的六个号，地址线接法不同）都有向量。
- 本文的特性标记来自正则扫描，可能把继承来的特性算到子类，或漏掉写在 `.cpp` 里的（`Rainbow.cpp`）；每个 mapper 以读源码为准。
- 统计脚本（解析 `MapperFactory.cpp`、沿继承链收集特性、生成台账）保存在仓库笔记 `ember-mesen-ledger-scripts.md`，做完阶段 0 的复核后可以删。

## 12. 台账

列说明：

- 号：iNES mapper 号；`4`、`34` 有两行。
- Mesen 类：构造参数写在括号里（已去掉命名空间前缀）。
- 文件：`Core/NES/Mappers/` 下的路径。
- 行数：类自己的文件（含同名 `.cpp`）的行数，不含基类。
- 特性：第 2 节的标记字母；`-` 表示扫描没有发现。
- 页大小：`GetPrgPageSize()`、`GetChrPageSize()` 在继承链里最近的重写值，`?` 表示没有找到。
- 寄存器范围：`RegisterStartAddress()`、`RegisterEndAddress()` 的重写值，“默认”是 `$8000-$FFFF`。
- 插件 id：拟定的插件 id；`nrom` 已有。
- 阶段：第 9 节的阶段，`-` 是不做。
- 状态：`待做`、`已有 ...`、`不做：原因`；做完一项改一项。

| 号 | Mesen 类 | 文件 | 行数 | 特性 | 页大小 PRG/CHR | 寄存器范围 | 插件 id | 阶段 | 状态 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | NROM | Nintendo/NROM.h | 18 | - | 0x4000/0x2000 | 默认 | nrom | 2 | 已有 nrom |
| 1 | MMC1 | Nintendo/MMC1.h | 237 | - | 0x4000/0x1000 | 默认 | mmc1 | 2 | 待做 |
| 2 | UNROM | Nintendo/UNROM.h | 26 | - | 0x4000/0x2000 | 默认 | unrom | 2 | 待做 |
| 3 | CNROM | Nintendo/CNROM.h | 23 | - | 0x8000/0x2000 | 默认 | cnrom | 2 | 待做 |
| 4 | MMC3 | Nintendo/MMC3.h | 353 | VIS | 0x2000/0x0400 | 默认 | mmc3 | 4a | 待做 |
| 4 | McAcc | Mmc3Variants/McAcc.h | 55 | VIS | 0x2000/0x0400 | 默认 | mc-acc | 4a | 待做 |
| 5 | MMC5 | Nintendo/MMC5.h | 798 | CIANRKSWM | 0x2000/0x400 | 0x5000-0x5206 | mmc5 | 6 | 待做 |
| 6 | FrontFareast | Unlicensed/FrontFareast.h | 138 | CIH | 0x2000/0x400 | 0x42FE-0x4517 | front-fareast | 4b | 待做 |
| 7 | AXROM | Nintendo/AXROM.h | 26 | - | 0x8000/0x2000 | 默认 | axrom | 2 | 待做 |
| 8 | FrontFareast | Unlicensed/FrontFareast.h | 138 | CIH | 0x2000/0x400 | 0x42FE-0x4517 | front-fareast | 4b | 待做 |
| 9 | MMC2 | Nintendo/MMC2.h | 140 | V | 0x2000/0x1000 | 默认 | mmc2 | 4b | 待做 |
| 10 | MMC4 | Nintendo/MMC4.h | 48 | V | 0x4000/0x1000 | 默认 | mmc4 | 4b | 待做 |
| 11 | ColorDreams | Unlicensed/ColorDreams.h | 29 | B | 0x8000/0x2000 | 默认 | color-dreams | 2 | 待做 |
| 12 | MMC3_12 | Mmc3Variants/MMC3_12.h | 47 | VIS | 0x2000/0x0400 | 默认 | mmc3-12 | 4a | 待做 |
| 13 | CpRom | Nintendo/CpRom.h | 25 | H | 0x8000/0x1000 | 默认 | cp-rom | 2 | 待做 |
| 14 | MMC3_14 | Mmc3Variants/MMC3_14.h | 91 | VIS | 0x2000/0x0400 | 默认 | mmc3-14 | 4a | 待做 |
| 15 | Mapper15 | Unlicensed/Mapper15.h | 59 | - | 0x2000/0x2000 | 默认 | mapper-15 | 2 | 待做 |
| 16 | BandaiFcg | Bandai/BandaiFcg.h | 248 | CIRES | 0x4000/0x400 | 0x6000-0xFFFF | bandai-fcg | 5 | 待做 |
| 17 | FrontFareast | Unlicensed/FrontFareast.h | 138 | CIH | 0x2000/0x400 | 0x42FE-0x4517 | front-fareast | 4b | 待做 |
| 18 | JalecoSs88006 | Jaleco/JalecoSs88006.h | 201 | CI | 0x2000/0x0400 | 默认 | jaleco-ss88006 | 4b | 待做 |
| 19 | Namco163 | Namco/Namco163.h | 296 | CIANRM | 0x2000/0x400 | 默认 | namco163 | 6 | 待做 |
| 21 | VRC2_4 | Konami/VRC2_4.h | 339 | CR | 0x2000/0x0400 | 默认 | vrc2-4 | 4b | 待做 |
| 22 | VRC2_4 | Konami/VRC2_4.h | 339 | CR | 0x2000/0x0400 | 默认 | vrc2-4 | 4b | 待做 |
| 23 | VRC2_4 | Konami/VRC2_4.h | 339 | CR | 0x2000/0x0400 | 默认 | vrc2-4 | 4b | 待做 |
| 24 | VRC6(VRC6a) | Konami/VRC6.h | 303 | CAN | 0x2000/0x0400 | 默认 | vrc6 | 6 | 待做 |
| 25 | VRC2_4 | Konami/VRC2_4.h | 339 | CR | 0x2000/0x0400 | 默认 | vrc2-4 | 4b | 待做 |
| 26 | VRC6(VRC6b) | Konami/VRC6.h | 303 | CAN | 0x2000/0x0400 | 默认 | vrc6 | 6 | 待做 |
| 27 | VRC2_4 | Konami/VRC2_4.h | 339 | CR | 0x2000/0x0400 | 默认 | vrc2-4 | 4b | 待做 |
| 28 | Action53 | Homebrew/Action53.h | 93 | - | 0x4000/0x2000 | 默认 | action53 | 2 | 待做 |
| 29 | SealieComputing | Homebrew/SealieComputing.h | 31 | WH | 0x4000/0x2000 | 0x8000-0xFFFF | sealie-computing | 2 | 待做 |
| 30 | UnRom512 | Homebrew/UnRom512.h | 133 | NRESWH | 0x4000/0x2000 | 0x8000-0xFFFF | un-rom512 | 5 | 待做 |
| 31 | NsfCart31 | Homebrew/NsfCart31.h | 26 | - | 0x1000/0x2000 | 0x5000-0x5FFF | nsf-cart31 | 2 | 待做 |
| 32 | IremG101 | Irem/IremG101.h | 77 | - | 0x2000/0x0400 | 默认 | irem-g101 | 2 | 待做 |
| 33 | TaitoTc0190 | Taito/TaitoTc0190.h | 43 | - | 0x2000/0x0400 | 默认 | taito-tc0190 | 2 | 待做 |
| 34 | Nina01 | Unlicensed/Nina01.h | 28 | - | 0x8000/0x1000 | 0x7FFD-0x7FFF | nina01 | 2 | 待做 |
| 34 | BnRom | Irem/BnRom.h | 23 | B | 0x8000/0x2000 | 默认 | bn-rom | 2 | 待做 |
| 35 | Mapper35 | JyCompany/Mapper35.h | 85 | VI | 0x2000/0x0400 | 默认 | mapper-35 | 4b | 待做 |
| 36 | Txc22000 | Txc/Txc22000.h | 61 | R | 0x8000/0x2000 | 0x8000-0xFFFF | txc22000 | 2 | 待做 |
| 37 | MMC3_37 | Nintendo/MMC3_37.h | 64 | VIS | 0x2000/0x0400 | 0x6000-0xFFFF | mmc3-37 | 4a | 待做 |
| 38 | UnlPci556 | Unlicensed/UnlPci556.h | 24 | - | 0x8000/0x2000 | 0x7000-0x7FFF | unl-pci556 | 2 | 待做 |
| 39 | Mapper39 | Unlicensed/Mapper39.h | 28 | - | 0x8000/0x2000 | 默认 | mapper-39 | 2 | 待做 |
| 40 | Mapper40 | Whirlwind/Mapper40.h | 62 | CI | 0x2000/0x2000 | 默认 | mapper-40 | 4b | 待做 |
| 41 | Caltron41 | Ntdec/Caltron41.h | 53 | - | 0x8000/0x2000 | 0x8000-0xFFFF | caltron41 | 2 | 待做 |
| 42 | Mapper42 | Unlicensed/Mapper42.h | 97 | CI | 0x2000/0x2000 | 默认 | mapper-42 | 4b | 待做 |
| 43 | Mapper43 | Unlicensed/Mapper43.h | 92 | CI | 0x2000/0x2000 | 0x4020-0xFFFF | mapper-43 | 4b | 待做 |
| 44 | MMC3_44 | Mmc3Variants/MMC3_44.h | 49 | VIS | 0x2000/0x0400 | 默认 | mmc3-44 | 4a | 待做 |
| 45 | MMC3_45 | Mmc3Variants/MMC3_45.h | 81 | VIS | 0x2000/0x0400 | 0x8000-0xFFFF | mmc3-45 | 4a | 待做 |
| 46 | ColorDreams46 | Unlicensed/ColorDreams46.h | 50 | - | 0x8000/0x2000 | 0x6000-0xFFFF | color-dreams46 | 2 | 待做 |
| 47 | MMC3_47 | Nintendo/MMC3_47.h | 57 | VIS | 0x2000/0x0400 | 0x6000-0xFFFF | mmc3-47 | 4a | 待做 |
| 48 | TaitoTc0690 | Taito/TaitoTc0690.h | 97 | CVIS | 0x2000/0x0400 | 默认 | taito-tc0690 | 4a | 待做 |
| 49 | MMC3_49 | Mmc3Variants/MMC3_49.h | 63 | VIS | 0x2000/0x0400 | 0x6000-0xFFFF | mmc3-49 | 4a | 待做 |
| 50 | Mapper50 | Unlicensed/Mapper50.h | 71 | CI | 0x2000/0x2000 | 0x4020-0x5FFF | mapper-50 | 4b | 待做 |
| 51 | Bmc51 | Unlicensed/Bmc51.h | 60 | - | 0x2000/0x2000 | 0x6000-0xFFFF | bmc51 | 2 | 待做 |
| 52 | MMC3_52 | Mmc3Variants/MMC3_52.h | 65 | VIS | 0x2000/0x0400 | 0x6000-0xFFFF | mmc3-52 | 4a | 待做 |
| 53 | Supervision | Unlicensed/Supervision.h | 63 | - | 0x2000/0x2000 | 0x6000-0xFFFF | supervision | 2 | 待做 |
| 54 | NovelDiamond | Unlicensed/NovelDiamond.h | 22 | - | 0x8000/0x2000 | 默认 | novel-diamond | 2 | 待做 |
| 56 | Kaiser202 | Kaiser/Kaiser202.h | 120 | CI | 0x2000/0x0400 | 默认 | kaiser202 | 4b | 待做 |
| 57 | Mapper57 | Unlicensed/Mapper57.h | 54 | - | 0x4000/0x2000 | 默认 | mapper-57 | 2 | 待做 |
| 58 | Mapper58 | Unlicensed/Mapper58.h | 32 | - | 0x4000/0x2000 | 默认 | mapper-58 | 2 | 待做 |
| 59 | UnlD1038 | Unlicensed/UnlD1038.h | 49 | RD | 0x4000/0x2000 | 默认 | unl-d1038 | 2 | 待做 |
| 60 | Mapper60 | Unlicensed/Mapper60.h | 39 | - | 0x4000/0x2000 | 默认 | mapper-60 | 2 | 待做 |
| 61 | Mapper61 | Txc/Mapper61.h | 32 | - | 0x4000/0x2000 | 默认 | mapper-61 | 2 | 待做 |
| 62 | Mapper62 | Unlicensed/Mapper62.h | 44 | - | 0x4000/0x2000 | 默认 | mapper-62 | 2 | 待做 |
| 63 | Bmc63 | Ntdec/Bmc63.h | 48 | - | 0x2000/0x2000 | 默认 | bmc63 | 2 | 待做 |
| 64 | Rambo1 | Tengen/Rambo1.h | 246 | CVI | 0x2000/0x400 | 默认 | rambo1 | 4b | 待做 |
| 65 | IremH3001 | Irem/IremH3001.h | 82 | CI | 0x2000/0x400 | 默认 | irem-h3001 | 4b | 待做 |
| 66 | GxRom | Nintendo/GxRom.h | 22 | - | 0x8000/0x2000 | 默认 | gx-rom | 2 | 待做 |
| 67 | Sunsoft3 | Sunsoft/Sunsoft3.h | 74 | CI | 0x4000/0x800 | 默认 | sunsoft3 | 4b | 待做 |
| 68 | Sunsoft4 | Sunsoft/Sunsoft4.h | 148 | CN | 0x4000/0x800 | 默认 | sunsoft4 | 4b | 待做 |
| 69 | SunsoftFme7 | Sunsoft/SunsoftFme7.h | 201 | CIASW | 0x2000/0x400 | 默认 | sunsoft-fme7 | 6 | 待做 |
| 70 | Bandai74161_7432(false) | Bandai/Bandai74161_7432.h | 54 | - | 0x4000/0x2000 | 默认 | bandai74161-7432 | 2 | 待做 |
| 71 | BF909x | Codemasters/BF909x.h | 47 | - | 0x4000/0x2000 | 默认 | bf909x | 2 | 待做 |
| 72 | JalecoJf17_19(false) | Jaleco/JalecoJf17_19.h | 55 | B | 0x4000/0x2000 | 默认 | jaleco-jf17-19 | 2 | 待做 |
| 73 | VRC3 | Konami/VRC3.h | 83 | CI | 0x4000/0x2000 | 默认 | vrc3 | 4b | 待做 |
| 74 | MMC3_ChrRam(0x08,0x09,2) | Mmc3Variants/MMC3_ChrRam.h | 39 | VISH | 0x2000/0x0400 | 默认 | mmc3-chr-ram | 4a | 待做 |
| 75 | VRC1 | Konami/VRC1.h | 65 | - | 0x2000/0x1000 | 默认 | vrc1 | 2 | 待做 |
| 76 | Namco108_76 | Namco/Namco108_76.h | 17 | VIS | 0x2000/0x0800 | 默认 | namco108-76 | 4a | 待做 |
| 77 | IremLrog017 | Irem/IremLrog017.h | 30 | BH | 0x8000/0x0800 | 默认 | irem-lrog017 | 2 | 待做 |
| 78 | JalecoJf16 | Jaleco/JalecoJf16.h | 31 | B | 0x4000/0x2000 | 默认 | jaleco-jf16 | 2 | 待做 |
| 79 | Nina03_06(false) | Unlicensed/Nina03_06.h | 41 | - | 0x8000/0x2000 | 0x4100-0x5FFF | nina03-06 | 2 | 待做 |
| 80 | TaitoX1005(false) | Taito/TaitoX1005.h | 120 | NSW | 0x2000/0x0400 | 0x7EF0-0x7EFF | taito-x1005 | 3 | 待做 |
| 82 | TaitoX1017 | Taito/TaitoX1017.h | 122 | S | 0x2000/0x0400 | 0x7EF0-0x7EFF | taito-x1017 | 2 | 待做 |
| 83 | Mapper83 | Unlicensed/Mapper83.h | 161 | CIRD | 0x2000/0x400 | 默认 | mapper-83 | 4b | 待做 |
| 85 | VRC7 | Konami/VRC7.h | 116 | CA | 0x2000/0x0400 | 默认 | vrc7 | 6 | 待做 |
| 86 | JalecoJf13 | Jaleco/JalecoJf13.h | 31 | - | 0x8000/0x2000 | 0x6000-0x7FFF | jaleco-jf13 | 2 | 待做 |
| 87 | JalecoJfxx(false) | Jaleco/JalecoJfxx.h | 37 | - | 0x8000/0x2000 | 0x6000-0x7FFF | jaleco-jfxx | 2 | 待做 |
| 88 | Namco108_88 | Namco/Namco108_88.h | 20 | VIS | 0x2000/0x0400 | 默认 | namco108-88 | 4a | 待做 |
| 89 | Sunsoft89 | Sunsoft/Sunsoft89.h | 22 | - | 0x4000/0x2000 | 默认 | sunsoft89 | 2 | 待做 |
| 90 | JyCompany | JyCompany/JyCompany.h | 495 | CVINRK | 0x2000/0x0400 | 默认 | jy-company | 4b | 待做 |
| 91 | Mapper91 | JyCompany/Mapper91.h | 44 | VIS | 0x2000/0x800 | 0x6000-0x7FFF | mapper-91 | 4a | 待做 |
| 92 | JalecoJf17_19(true) | Jaleco/JalecoJf17_19.h | 55 | B | 0x4000/0x2000 | 默认 | jaleco-jf17-19 | 2 | 待做 |
| 93 | Sunsoft93 | Sunsoft/Sunsoft93.h | 25 | - | 0x4000/0x2000 | 默认 | sunsoft93 | 2 | 待做 |
| 94 | UnRom_94 | Nintendo/UnRom_94.h | 24 | - | 0x4000/0x2000 | 默认 | un-rom-94 | 2 | 待做 |
| 95 | Namco108_95 | Namco/Namco108_95.h | 19 | VINS | 0x2000/0x0400 | 默认 | namco108-95 | 4a | 待做 |
| 96 | OekaKids | Bandai/OekaKids.h | 58 | VB | 0x8000/0x1000 | 默认 | oeka-kids | 4b | 待做 |
| 97 | IremTamS1 | Irem/IremTamS1.h | 29 | - | 0x4000/0x2000 | 默认 | irem-tam-s1 | 2 | 待做 |
| 99 | VsSystem | VsSystem/VsSystem.h | 100 | CW | 0x2000/0x2000 | 默认 | - | - | 不做：VS System 街机主板，不是 FC 卡带 |
| 101 | JalecoJfxx(true) | Jaleco/JalecoJfxx.h | 37 | - | 0x8000/0x2000 | 0x6000-0x7FFF | jaleco-jfxx | 2 | 待做 |
| 103 | Mapper103 | Unlicensed/Mapper103.h | 81 | W | 0x2000/0x2000 | 0x6000-? | mapper-103 | 2 | 待做 |
| 104 | GoldenFive | Codemasters/GoldenFive.h | 39 | - | 0x4000/0x2000 | 默认 | golden-five | 2 | 待做 |
| 105 | MMC1_105 | Nintendo/MMC1_105.h | 91 | CID | 0x4000/0x1000 | 默认 | mmc1-105 | 4b | 待做 |
| 106 | Mapper106 | Unlicensed/Mapper106.h | 95 | CI | 0x2000/0x400 | 默认 | mapper-106 | 4b | 待做 |
| 107 | Mapper107 | Unlicensed/Mapper107.h | 22 | - | 0x8000/0x2000 | 默认 | mapper-107 | 2 | 待做 |
| 108 | Bb | Unlicensed/Bb.h | 51 | - | 0x2000/0x2000 | 默认 | mapper-108 | 2 | 待做 |
| 111 | Cheapocabra | Homebrew/Cheapocabra.h | 101 | NRESWH | 0x8000/0x2000 | 0x5000-0x5FFF | cheapocabra | 5 | 待做 |
| 112 | Mapper112 | Ntdec/Mapper112.h | 64 | - | 0x2000/0x400 | 0x8000-0xFFFF | mapper-112 | 2 | 待做 |
| 113 | Nina03_06(true) | Unlicensed/Nina03_06.h | 41 | - | 0x8000/0x2000 | 0x4100-0x5FFF | nina03-06 | 2 | 待做 |
| 114 | MMC3_114 | Mmc3Variants/MMC3_114.h | 78 | VIS | 0x2000/0x0400 | 0x5000-? | mmc3-114 | 4a | 待做 |
| 115 | MMC3_115 | Mmc3Variants/MMC3_115.h | 74 | VIRS | 0x2000/0x0400 | 默认 | mmc3-115 | 4a | 待做 |
| 116 | Mapper116 | Unlicensed/Mapper116.h | 323 | VI | 0x2000/0x400 | 0x4100-0xFFFF | mapper-116 | 4b | 待做 |
| 117 | Mapper117 | Unlicensed/Mapper117.h | 90 | VI | 0x2000/0x400 | 默认 | mapper-117 | 4b | 待做 |
| 118 | TxSRom | Nintendo/TxSRom.h | 42 | VINS | 0x2000/0x0400 | 默认 | tx-srom | 4a | 待做 |
| 119 | MMC3_ChrRam(0x40,0x7F,8) | Mmc3Variants/MMC3_ChrRam.h | 39 | VISH | 0x2000/0x0400 | 默认 | mmc3-chr-ram | 4a | 待做 |
| 120 | Mapper120 | Unlicensed/Mapper120.h | 44 | - | 0x2000/0x2000 | 0x41FF-0x41FF | mapper-120 | 2 | 待做 |
| 121 | MMC3_121 | Mmc3Variants/MMC3_121.h | 146 | VIRS | 0x2000/0x0400 | 默认 | mmc3-121 | 4a | 待做 |
| 123 | MMC3_123 | Mmc3Variants/MMC3_123.h | 59 | VIS | 0x2000/0x0400 | 默认 | mmc3-123 | 4a | 待做 |
| 125 | Lh32 | Whirlwind/Lh32.h | 49 | - | 0x2000/0x2000 | 0x6000-0x6000 | lh32 | 2 | 待做 |
| 126 | MMC3_126 | Mmc3Variants/MMC3_126.h | 87 | VIS | 0x2000/0x0400 | 默认 | mmc3-126 | 4a | 待做 |
| 132 | Txc22211A | Txc/Txc22211A.h | 54 | R | 0x8000/0x2000 | 0x8000-0xFFFF | txc22211-a | 2 | 待做 |
| 133 | Sachen_133 | Sachen/Sachen_133.h | 26 | - | 0x8000/0x2000 | 0x4100-0xFFFF | sachen-133 | 2 | 待做 |
| 134 | MMC3_134 | Mmc3Variants/MMC3_134.h | 53 | VIS | 0x2000/0x0400 | 默认 | mmc3-134 | 4a | 待做 |
| 136 | Sachen_136 | Sachen/Sachen_136.h | 58 | R | 0x8000/0x2000 | 0x8000-0xFFFF | sachen-136 | 2 | 待做 |
| 137 | Sachen8259(Sachen8259D) | Sachen/Sachen8259.h | 121 | N | 0x8000/_variant == Sachen8259Variant::Sachen8259D ? 0x400 : 0x800 | 0x4100-0x7FFF | sachen8259 | 3 | 待做 |
| 138 | Sachen8259(Sachen8259B) | Sachen/Sachen8259.h | 121 | N | 0x8000/_variant == Sachen8259Variant::Sachen8259D ? 0x400 : 0x800 | 0x4100-0x7FFF | sachen8259 | 3 | 待做 |
| 139 | Sachen8259(Sachen8259C) | Sachen/Sachen8259.h | 121 | N | 0x8000/_variant == Sachen8259Variant::Sachen8259D ? 0x400 : 0x800 | 0x4100-0x7FFF | sachen8259 | 3 | 待做 |
| 140 | JalecoJf11_14 | Jaleco/JalecoJf11_14.h | 25 | - | 0x8000/0x2000 | 0x6000-0x7FFF | jaleco-jf11-14 | 2 | 待做 |
| 141 | Sachen8259(Sachen8259A) | Sachen/Sachen8259.h | 121 | N | 0x8000/_variant == Sachen8259Variant::Sachen8259D ? 0x400 : 0x800 | 0x4100-0x7FFF | sachen8259 | 3 | 待做 |
| 142 | Kaiser202 | Kaiser/Kaiser202.h | 120 | CI | 0x2000/0x0400 | 默认 | kaiser202 | 4b | 待做 |
| 143 | Sachen_143 | Sachen/Sachen_143.h | 26 | R | 0x4000/0x2000 | 0x4100-0x5FFF | sachen-143 | 2 | 待做 |
| 144 | ColorDreams | Unlicensed/ColorDreams.h | 29 | B | 0x8000/0x2000 | 默认 | color-dreams | 2 | 待做 |
| 145 | Sachen_145 | Sachen/Sachen_145.h | 24 | - | 0x8000/0x2000 | 0x4100-0x7FFF | sachen-145 | 2 | 待做 |
| 146 | Nina03_06(false) | Unlicensed/Nina03_06.h | 41 | - | 0x8000/0x2000 | 0x4100-0x5FFF | nina03-06 | 2 | 待做 |
| 147 | Sachen_147 | Sachen/Sachen_147.h | 61 | R | 0x8000/0x2000 | 0x8000-0xFFFF | sachen-147 | 2 | 待做 |
| 148 | Sachen_148 | Sachen/Sachen_148.h | 22 | B | 0x8000/0x2000 | 默认 | sachen-148 | 2 | 待做 |
| 149 | Sachen_149 | Sachen/Sachen_149.h | 20 | - | 0x8000/0x2000 | 默认 | sachen-149 | 2 | 待做 |
| 150 | Sachen74LS374N | Sachen/Sachen74LS374N.h | 85 | NRD | 0x8000/0x2000 | 0x4100-0x7FFF | sachen74-ls374-n | 3 | 待做 |
| 151 | VRC1 | Konami/VRC1.h | 65 | - | 0x2000/0x1000 | 默认 | vrc1 | 2 | 待做 |
| 152 | Bandai74161_7432(true) | Bandai/Bandai74161_7432.h | 54 | - | 0x4000/0x2000 | 默认 | bandai74161-7432 | 2 | 待做 |
| 153 | BandaiFcg | Bandai/BandaiFcg.h | 248 | CIRES | 0x4000/0x400 | 0x6000-0xFFFF | bandai-fcg | 5 | 待做 |
| 154 | Namco108_154 | Namco/Namco108_154.h | 14 | VIS | 0x2000/0x0400 | 默认 | namco108-154 | 4a | 待做 |
| 155 | MMC1_155 | Nintendo/MMC1_155.h | 14 | - | 0x4000/0x1000 | 默认 | mmc1-155 | 2 | 待做 |
| 156 | DaouInfosys | Unlicensed/DaouInfosys.h | 57 | - | 0x4000/0x400 | 0xC000-0xC014 | daou-infosys | 2 | 待做 |
| 157 | BandaiFcg | Bandai/BandaiFcg.h | 248 | CIRES | 0x4000/0x400 | 0x6000-0xFFFF | bandai-fcg | 5 | 待做 |
| 158 | Rambo1_158 | Tengen/Rambo1_158.h | 39 | CVIN | 0x2000/0x400 | 默认 | rambo1-158 | 4b | 待做 |
| 159 | BandaiFcg | Bandai/BandaiFcg.h | 248 | CIRES | 0x4000/0x400 | 0x6000-0xFFFF | bandai-fcg | 5 | 待做 |
| 162 | Waixing162 | Waixing/Waixing162.h | 47 | - | 0x8000/0x2000 | 0x5000-0x5FFF | waixing162 | 2 | 待做 |
| 163 | Nanjing | Unlicensed/Nanjing.h | 120 | VR | 0x8000/0x1000 | 0x5000-0x5FFF | nanjing | 4b | 待做 |
| 164 | Waixing164 | Waixing/Waixing164.h | 42 | - | 0x8000/0x2000 | 0x5000-0x5FFF | waixing164 | 2 | 待做 |
| 165 | MMC3_165 | Mmc3Variants/MMC3_165.h | 56 | VISH | 0x2000/0x1000 | 默认 | mmc3-165 | 4a | 待做 |
| 166 | Subor166 | Unlicensed/Subor166.h | 55 | - | 0x4000/0x2000 | 默认 | subor166 | 2 | 待做 |
| 167 | Subor166 | Unlicensed/Subor166.h | 55 | - | 0x4000/0x2000 | 默认 | subor166 | 2 | 待做 |
| 168 | Racermate | Unlicensed/Racermate.h | 83 | CISH | 0x4000/0x1000 | 默认 | racermate | 4b | 待做 |
| 170 | Mapper170 | Unlicensed/Mapper170.h | 50 | R | 0x8000/0x2000 | 0x7000-0x7001 | mapper-170 | 2 | 待做 |
| 171 | Kaiser7058 | Kaiser/Kaiser7058.h | 25 | - | 0x8000/0x1000 | 0xF000-0xFFFF | - | - | 不做：学习机 BIOS 的 mapper 号，保持 mappers.noMatch 的约定 |
| 172 | Txc22211B | Txc/Txc22211B.h | 62 | R | 0x8000/0x2000 | 0x8000-0xFFFF | txc22211-b | 2 | 已有 qls、plq；本项只按 mapper 号匹配 |
| 173 | Txc22211C | Txc/Txc22211C.h | 21 | R | 0x8000/0x2000 | 0x8000-0xFFFF | txc22211-c | 2 | 待做 |
| 174 | Mapper174 | Ntdec/Mapper174.h | 31 | - | 0x4000/0x2000 | 默认 | mapper-174 | 2 | 待做 |
| 175 | Kaiser7022 | Kaiser/Kaiser7022.h | 53 | R | 0x4000/0x2000 | 0x8000-0xFFFF | kaiser7022 | 2 | 待做 |
| 176 | Fk23C | Waixing/Fk23C.h | 408 | CVIWH | 0x2000/0x0400 | 默认 | fk23-c | 4b | 待做 |
| 177 | Henggedianzi177 | Unlicensed/Henggedianzi177.h | 24 | - | 0x8000/0x2000 | 0x8000-0xFFFF | henggedianzi177 | 2 | 待做 |
| 178 | Waixing178 | Waixing/Waixing178.h | 62 | W | 0x4000/0x2000 | 0x4800-0x4FFF | waixing178 | 2 | 待做 |
| 179 | Henggedianzi179 | Unlicensed/Henggedianzi179.h | 28 | - | 0x8000/0x2000 | 0x8000-0xFFFF | henggedianzi179 | 2 | 待做 |
| 180 | UnRom_180 | Nintendo/UnRom_180.h | 22 | - | 0x4000/0x2000 | 默认 | un-rom-180 | 2 | 待做 |
| 182 | MMC3_182 | Mmc3Variants/MMC3_182.h | 39 | VIS | 0x2000/0x0400 | 默认 | mmc3-182 | 4a | 待做 |
| 183 | VRC2_4 | Konami/VRC2_4.h | 339 | CR | 0x2000/0x0400 | 默认 | vrc2-4 | 4b | 待做 |
| 184 | Sunsoft184 | Sunsoft/Sunsoft184.h | 26 | - | 0x8000/0x1000 | 0x6000-0x7FFF | sunsoft184 | 2 | 待做 |
| 185 | CnromProtect | Nintendo/CnromProtect.h | 60 | KB | 0x8000/0x2000 | 默认 | cnrom-protect | 3 | 待做 |
| 187 | MMC3_187 | Mmc3Variants/MMC3_187.h | 95 | VIRS | 0x2000/0x0400 | 默认 | mmc3-187 | 4a | 待做 |
| 188 | BandaiKaraoke | Bandai/BandaiKaraoke.h | 55 | RB | 0x4000/0x2000 | 默认 | bandai-karaoke | 2 | 待做 |
| 189 | MMC3_189 | Txc/MMC3_189.h | 41 | VIS | 0x2000/0x0400 | 0x4120-? | mmc3-189 | 4a | 待做 |
| 190 | MagicKidGooGoo | Unlicensed/MagicKidGooGoo.h | 29 | - | 0x4000/0x800 | 默认 | magic-kid-goo-goo | 2 | 待做 |
| 191 | MMC3_ChrRam(0x80,0xFF,2) | Mmc3Variants/MMC3_ChrRam.h | 39 | VISH | 0x2000/0x0400 | 默认 | mmc3-chr-ram | 4a | 待做 |
| 192 | MMC3_ChrRam(0x08,0x0B,4) | Mmc3Variants/MMC3_ChrRam.h | 39 | VISH | 0x2000/0x0400 | 默认 | mmc3-chr-ram | 4a | 待做 |
| 193 | NtdecTc112 | Ntdec/NtdecTc112.h | 43 | - | 0x2000/0x0800 | 0x6000-0x7FFF | ntdec-tc112 | 2 | 待做 |
| 194 | MMC3_ChrRam(0x00,0x01,2) | Mmc3Variants/MMC3_ChrRam.h | 39 | VISH | 0x2000/0x0400 | 默认 | mmc3-chr-ram | 4a | 待做 |
| 195 | MMC3_ChrRam(0x00,0x03,4) | Mmc3Variants/MMC3_ChrRam.h | 39 | VISH | 0x2000/0x0400 | 默认 | mmc3-chr-ram | 4a | 待做 |
| 196 | MMC3_196 | Mmc3Variants/MMC3_196.h | 49 | VIS | 0x2000/0x0400 | 默认 | mmc3-196 | 4a | 待做 |
| 197 | MMC3_197 | Mmc3Variants/MMC3_197.h | 22 | VIS | 0x2000/0x0400 | 默认 | mmc3-197 | 4a | 待做 |
| 198 | MMC3_198 | Mmc3Variants/MMC3_198.h | 64 | VISW | 0x2000/0x0400 | 默认 | mmc3-198 | 4a | 待做 |
| 199 | MMC3_199 | Mmc3Variants/MMC3_199.h | 68 | VISH | 0x2000/0x0400 | 默认 | mmc3-199 | 4a | 待做 |
| 200 | Mapper200 | Unlicensed/Mapper200.h | 28 | - | 0x4000/0x2000 | 默认 | mapper-200 | 2 | 待做 |
| 201 | NovelDiamond | Unlicensed/NovelDiamond.h | 22 | - | 0x8000/0x2000 | 默认 | novel-diamond | 2 | 待做 |
| 202 | Mapper202 | Unlicensed/Mapper202.h | 42 | - | 0x4000/0x2000 | 默认 | mapper-202 | 2 | 待做 |
| 203 | Mapper203 | Unlicensed/Mapper203.h | 24 | - | 0x4000/0x2000 | 默认 | mapper-203 | 2 | 待做 |
| 204 | Mapper204 | Unlicensed/Mapper204.h | 25 | - | 0x4000/0x2000 | 默认 | mapper-204 | 2 | 待做 |
| 205 | MMC3_205 | Mmc3Variants/MMC3_205.h | 50 | VIS | 0x2000/0x0400 | 0x6000-0xFFFF | mmc3-205 | 4a | 待做 |
| 206 | Namco108 | Namco/Namco108.h | 93 | VIS | 0x2000/0x0400 | 默认 | namco108 | 4a | 待做 |
| 207 | TaitoX1005(true) | Taito/TaitoX1005.h | 120 | NSW | 0x2000/0x0400 | 0x7EF0-0x7EFF | taito-x1005 | 3 | 待做 |
| 208 | MMC3_208 | Mmc3Variants/MMC3_208.h | 78 | VIRS | 0x2000/0x0400 | 0x8000-0xFFFF | mmc3-208 | 4a | 待做 |
| 209 | JyCompany | JyCompany/JyCompany.h | 495 | CVINRK | 0x2000/0x0400 | 默认 | jy-company | 4b | 待做 |
| 210 | Namco163 | Namco/Namco163.h | 296 | CIANRM | 0x2000/0x400 | 默认 | namco163 | 6 | 待做 |
| 211 | JyCompany | JyCompany/JyCompany.h | 495 | CVINRK | 0x2000/0x0400 | 默认 | jy-company | 4b | 待做 |
| 212 | Mapper212 | Unlicensed/Mapper212.h | 40 | R | 0x4000/0x2000 | 默认 | mapper-212 | 2 | 待做 |
| 213 | Mapper213 | Unlicensed/Mapper213.h | 21 | - | 0x8000/0x2000 | 默认 | mapper-213 | 2 | 待做 |
| 214 | Mapper214 | Unlicensed/Mapper214.h | 22 | - | 0x4000/0x2000 | 默认 | mapper-214 | 2 | 待做 |
| 215 | MMC3_215 | Mmc3Variants/MMC3_215.h | 122 | VIS | 0x2000/0x0400 | 0x5000-0xFFFF | mmc3-215 | 4a | 待做 |
| 216 | Mapper216 | Unlicensed/Mapper216.h | 30 | R | 0x8000/0x2000 | 默认 | mapper-216 | 2 | 待做 |
| 217 | MMC3_217 | Mmc3Variants/MMC3_217.h | 130 | VIS | 0x2000/0x0400 | 默认 | mmc3-217 | 4a | 待做 |
| 218 | MagicFloor218 | Homebrew/MagicFloor218.h | 32 | N | 0x8000/0x2000 | 默认 | magic-floor218 | 3 | 待做 |
| 219 | MMC3_219 | Mmc3Variants/MMC3_219.h | 85 | VIS | 0x2000/0x0400 | 默认 | mmc3-219 | 4a | 待做 |
| 221 | Mapper221 | Ntdec/Mapper221.h | 64 | - | 0x4000/0x2000 | 默认 | mapper-221 | 2 | 待做 |
| 222 | Mapper222 | Unlicensed/Mapper222.h | 66 | VI | 0x2000/0x400 | 默认 | mapper-222 | 4b | 待做 |
| 224 | MMC3_224 | Mmc3Variants/MMC3_224.h | 55 | VIS | 0x2000/0x0400 | 默认 | mmc3-224 | 4a | 待做 |
| 225 | Mapper225 | Unlicensed/Mapper225.h | 35 | - | 0x4000/0x2000 | 默认 | mapper-225 | 2 | 待做 |
| 226 | Mapper226 | Unlicensed/Mapper226.h | 71 | - | 0x4000/0x2000 | 默认 | mapper-226 | 2 | 待做 |
| 227 | Mapper227 | Unlicensed/Mapper227.h | 51 | - | 0x4000/0x2000 | 默认 | mapper-227 | 2 | 待做 |
| 228 | ActionEnterprises | Unlicensed/ActionEnterprises.h | 43 | - | 0x4000/0x2000 | 默认 | action-enterprises | 2 | 待做 |
| 229 | Mapper229 | Unlicensed/Mapper229.h | 27 | - | 0x4000/0x2000 | 默认 | mapper-229 | 2 | 待做 |
| 230 | Mapper230 | Unlicensed/Mapper230.h | 58 | - | 0x4000/0x2000 | 默认 | mapper-230 | 2 | 待做 |
| 231 | Mapper231 | Unlicensed/Mapper231.h | 32 | - | 0x4000/0x2000 | 默认 | mapper-231 | 2 | 待做 |
| 232 | BF9096 | Codemasters/BF9096.h | 51 | - | 0x4000/0x2000 | 默认 | bf9096 | 2 | 待做 |
| 233 | Mapper233 | Unlicensed/Mapper233.h | 34 | - | 0x4000/0x2000 | 默认 | mapper-233 | 2 | 待做 |
| 234 | Mapper234 | Unlicensed/Mapper234.h | 74 | RB | 0x8000/0x2000 | 0xFF80-0xFF9F | mapper-234 | 2 | 待做 |
| 235 | Bmc235 | Unlicensed/Bmc235.h | 73 | - | 0x4000/0x2000 | 默认 | bmc235 | 2 | 待做 |
| 236 | Bmc70in1 | Unlicensed/Bmc70in1.h | 101 | RD | 0x4000/0x2000 | 默认 | bmc70in1 | 2 | 待做 |
| 238 | MMC3_238 | Mmc3Variants/MMC3_238.h | 41 | VIRS | 0x2000/0x0400 | 默认 | mmc3-238 | 4a | 待做 |
| 240 | Mapper240 | Unlicensed/Mapper240.h | 24 | - | 0x8000/0x2000 | 0x4020-0x5FFF | mapper-240 | 2 | 待做 |
| 241 | Mapper241 | Unlicensed/Mapper241.h | 21 | - | 0x8000/0x2000 | 默认 | mapper-241 | 2 | 待做 |
| 242 | Mapper242 | Waixing/Mapper242.h | 28 | - | 0x8000/0x2000 | 默认 | mapper-242 | 2 | 待做 |
| 243 | Sachen74LS374N | Sachen/Sachen74LS374N.h | 85 | NRD | 0x8000/0x2000 | 0x4100-0x7FFF | sachen74-ls374-n | 3 | 待做 |
| 244 | Mapper244 | Unlicensed/Mapper244.h | 44 | - | 0x8000/0x2000 | 默认 | mapper-244 | 2 | 待做 |
| 245 | MMC3_245 | Mmc3Variants/MMC3_245.h | 42 | VIS | 0x2000/0x0400 | 默认 | mmc3-245 | 4a | 待做 |
| 246 | Mapper246 | Unlicensed/Mapper246.h | 32 | - | 0x2000/0x0800 | 0x6000-0x67FF | mapper-246 | 2 | 待做 |
| 249 | MMC3_249 | Mmc3Variants/MMC3_249.h | 57 | VIS | 0x2000/0x0400 | 默认 | mmc3-249 | 4a | 待做 |
| 250 | MMC3_250 | Mmc3Variants/MMC3_250.h | 12 | VIS | 0x2000/0x0400 | 默认 | mmc3-250 | 4a | 待做 |
| 252 | Waixing252 | Waixing/Waixing252.h | 72 | C | 0x2000/0x400 | 默认 | waixing252 | 4b | 待做 |
| 253 | Mapper253 | Waixing/Mapper253.h | 133 | CIH | 0x2000/0x400 | 默认 | mapper-253 | 4b | 待做 |
| 254 | MMC3_254 | Mmc3Variants/MMC3_254.h | 45 | VIRS | 0x2000/0x0400 | 默认 | mmc3-254 | 4a | 待做 |
| 255 | Bmc255 | Unlicensed/Bmc255.h | 26 | - | 0x4000/0x2000 | 默认 | bmc255 | 2 | 待做 |
| 258 | Unl158B | Mmc3Variants/Unl158B.h | 68 | VIRS | 0x2000/0x0400 | 默认 | unl158-b | 4a | 待做 |
| 259 | MMC3_BmcF15 | Mmc3Variants/MMC3_BmcF15.h | 44 | VIS | 0x2000/0x0400 | 默认 | mmc3-bmc-f15 | 4a | 待做 |
| 260 | BmcHpxx | Mmc3Variants/BmcHpxx.h | 117 | VIRSD | 0x2000/0x0400 | 默认 | bmc-hpxx | 4a | 待做 |
| 261 | Bmc810544CA1 | Unlicensed/Bmc810544CA1.h | 33 | - | 0x4000/0x2000 | 默认 | bmc810544-ca1 | 2 | 待做 |
| 262 | MMC3_StreetHeroes | Mmc3Variants/MMC3_StreetHeroes.h | 84 | VIRSH | 0x2000/0x0400 | 默认 | mmc3-street-heroes | 4a | 待做 |
| 263 | MMC3_Kof97 | Mmc3Variants/MMC3_Kof97.h | 21 | VIS | 0x2000/0x0400 | 默认 | mmc3-kof97 | 4a | 待做 |
| 264 | Yoko | Unlicensed/Yoko.h | 171 | CIRD | 0x2000/0x800 | 0x5000-0x5FFF | yoko | 4b | 待做 |
| 265 | T262 | Unlicensed/T262.h | 48 | - | 0x4000/0x2000 | 默认 | t262 | 2 | 待做 |
| 266 | CityFighter | Unlicensed/CityFighter.h | 128 | CI | 0x2000/0x400 | 默认 | city-fighter | 4b | 待做 |
| 268 | MMC3_Coolboy | Mmc3Variants/MMC3_Coolboy.h | 117 | VISH | 0x2000/0x0400 | 0x6000-? | mmc3-coolboy | 4a | 待做 |
| 274 | Bmc80013B | Unlicensed/Bmc80013B.h | 57 | - | 0x4000/0x2000 | 默认 | bmc80013-b | 2 | 待做 |
| 283 | Gs2004 | Unlicensed/Gs2004.h | 26 | - | 0x2000/0x2000 | 默认 | gs2004 | 2 | 待做 |
| 284 | UnlDripGame | Homebrew/UnlDripGame.h | 188 | CIARKD | 0x4000/0x800 | 0x8000-0xFFFF | unl-drip-game | 6 | 待做 |
| 285 | A65AS | Unlicensed/A65AS.h | 32 | - | 0x4000/0x2000 | 默认 | a65-as | 2 | 待做 |
| 286 | Bs5 | Waixing/Bs5.h | 35 | D | 0x2000/0x800 | 默认 | bs5 | 2 | 待做 |
| 287 | MMC3_Bmc411120C | Mmc3Variants/MMC3_Bmc411120C.h | 50 | VISD | 0x2000/0x0400 | 默认 | mmc3-bmc411120-c | 4a | 待做 |
| 288 | Gkcx1 | Unlicensed/Gkcx1.h | 22 | - | 0x8000/0x2000 | 默认 | gkcx1 | 2 | 待做 |
| 289 | Bmc60311C | Unlicensed/Bmc60311C.h | 85 | - | 0x4000/0x2000 | 0x6000-0xFFFF | bmc60311-c | 2 | 待做 |
| 290 | BmcNtd03 | Ntdec/BmcNtd03.h | 35 | - | 0x4000/0x2000 | 默认 | bmc-ntd03 | 2 | 待做 |
| 292 | DragonFighter | Mmc3Variants/DragonFighter.h | 74 | VIRS | 0x2000/0x0400 | 默认 | dragon-fighter | 4a | 待做 |
| 298 | Tf1201 | Ntdec/Tf1201.h | 135 | CI | 0x2000/0x400 | 默认 | tf1201 | 4b | 待做 |
| 299 | Bmc11160 | Txc/Bmc11160.h | 28 | - | 0x8000/0x2000 | 默认 | bmc11160 | 2 | 待做 |
| 300 | Bmc190in1 | Unlicensed/Bmc190in1.h | 23 | - | 0x4000/0x2000 | 默认 | bmc190in1 | 2 | 待做 |
| 301 | Bmc8157 | Unlicensed/Bmc8157.h | 63 | D | 0x4000/0x2000 | 默认 | bmc8157 | 2 | 待做 |
| 302 | Kaiser7057 | Kaiser/Kaiser7057.h | 73 | - | 0x800/0x2000 | 默认 | kaiser7057 | 2 | 待做 |
| 303 | Kaiser7017 | Kaiser/Kaiser7017.h | 90 | CIR | 0x4000/0x2000 | 0x4020-0x5FFF | kaiser7017 | 4b | 待做 |
| 304 | Smb2j | Whirlwind/Smb2j.h | 69 | CI | 0x1000/0x2000 | 0x4122-0x4122 | smb2j | 4b | 待做 |
| 305 | Kaiser7031 | Kaiser/Kaiser7031.h | 47 | - | 0x800/0x2000 | 默认 | kaiser7031 | 2 | 待做 |
| 306 | Kaiser7016 | Kaiser/Kaiser7016.h | 65 | - | 0x2000/0x2000 | 默认 | kaiser7016 | 2 | 待做 |
| 307 | Kaiser7037 | Kaiser/Kaiser7037.h | 60 | N | 0x1000/0x2000 | 默认 | kaiser7037 | 3 | 待做 |
| 309 | Lh51 | Whirlwind/Lh51.h | 33 | - | 0x2000/0x2000 | 默认 | lh51 | 2 | 待做 |
| 312 | Kaiser7013B | Kaiser/Kaiser7013B.h | 29 | - | 0x4000/0x2000 | 0x6000-0xFFFF | kaiser7013-b | 2 | 待做 |
| 313 | ResetTxrom | Mmc3Variants/ResetTxrom.h | 39 | VIS | 0x2000/0x0400 | 默认 | reset-txrom | 4a | 待做 |
| 314 | Bmc64in1NoRepeat | Unlicensed/Bmc64in1NoRepeat.h | 63 | - | 0x4000/0x2000 | 默认 | bmc64in1-no-repeat | 2 | 待做 |
| 319 | Hp898f | Unlicensed/Hp898f.h | 46 | - | 0x4000/0x2000 | 0x6000-0xFFFF | hp898f | 2 | 待做 |
| 320 | Bmc830425C4391T | Unlicensed/Bmc830425C4391T.h | 57 | - | 0x4000/0x2000 | 默认 | bmc830425-c4391-t | 2 | 待做 |
| 323 | FaridSlrom | Homebrew/FaridSlrom.h | 56 | - | 0x4000/0x1000 | 默认 | farid-slrom | 2 | 待做 |
| 324 | FaridUnrom | Homebrew/FaridUnrom.h | 54 | B | 0x4000/0x2000 | 默认 | farid-unrom | 2 | 待做 |
| 325 | MMC3_MaliSB | Mmc3Variants/MMC3_MaliSB.h | 27 | VIS | 0x2000/0x0400 | 默认 | mmc3-mali-sb | 4a | 待做 |
| 328 | Rt01 | Unlicensed/Rt01.h | 30 | R | 0x4000/0x800 | 默认 | rt01 | 2 | 待做 |
| 329 | Edu2000 | Unlicensed/Edu2000.h | 43 | W | 0x8000/0x2000 | 默认 | edu2000 | 2 | 待做 |
| 331 | Bmc12in1 | Unlicensed/Bmc12in1.h | 63 | - | 0x4000/0x1000 | 默认 | bmc12in1 | 2 | 待做 |
| 332 | Super40in1Ws | Unlicensed/Super40in1Ws.h | 42 | - | 0x4000/0x2000 | 0x6000-0x6FFF | super40in1-ws | 2 | 待做 |
| 333 | Bmc8in1 | Mmc3Variants/Bmc8in1.h | 46 | VIS | 0x2000/0x0400 | 默认 | bmc8in1 | 4a | 待做 |
| 336 | BmcK3046 | Unlicensed/BmcK3046.h | 27 | - | 0x4000/0x2000 | 默认 | bmc-k3046 | 2 | 待做 |
| 346 | Kaiser7012 | Kaiser/Kaiser7012.h | 24 | - | 0x8000/0x2000 | 默认 | kaiser7012 | 2 | 待做 |
| 348 | Bmc830118C | Mmc3Variants/Bmc830118C.h | 59 | VIS | 0x2000/0x0400 | 默认 | bmc830118-c | 4a | 待做 |
| 349 | BmcG146 | Unlicensed/BmcG146.h | 38 | - | 0x4000/0x2000 | 默认 | bmc-g146 | 2 | 待做 |
| 366 | BmcGn45 | Mmc3Variants/BmcGn45.h | 65 | VIS | 0x2000/0x0400 | 0x6000-0xFFFF | bmc-gn45 | 4a | 待做 |
| 487 | Mapper487 | Unlicensed/Mapper487.h | 83 | - | 0x8000/0x2000 | 0x4100-0x5FFF | mapper-487 | 2 | 待做 |
| 513 | Sachen9602 | Sachen/Sachen9602.h | 51 | VISH | 0x2000/0x0400 | 默认 | sachen9602 | 4a | 待做 |
| 518 | Dance2000 | Unlicensed/Dance2000.h | 84 | VR | 0x4000/0x1000 | 默认 | dance2000 | 4b | 待做 |
| 519 | Eh8813A | Unlicensed/Eh8813A.h | 56 | RD | 0x4000/0x2000 | 默认 | eh8813-a | 2 | 待做 |
| 521 | DreamTech01 | Unlicensed/DreamTech01.h | 24 | - | 0x4000/0x2000 | 0x5020-0x5020 | dream-tech01 | 2 | 待做 |
| 522 | Lh10 | Whirlwind/Lh10.h | 59 | - | 0x2000/0x2000 | 默认 | lh10 | 2 | 待做 |
| 529 | T230 | Unlicensed/T230.h | 122 | C | 0x2000/0x0400 | 默认 | t230 | 4b | 待做 |
| 530 | Ax5705 | Unlicensed/Ax5705.h | 64 | - | 0x2000/0x400 | 默认 | ax5705 | 2 | 待做 |
| 552 | TaitoX1017 | Taito/TaitoX1017.h | 122 | S | 0x2000/0x0400 | 0x7EF0-0x7EFF | taito-x1017 | 2 | 待做 |
| 682 | Rainbow | Homebrew/Rainbow.h | 1294 | CIANRKEM | 0x1000/0x200 | 0x4100-0x4785 | rainbow | 6 | 待做 |
