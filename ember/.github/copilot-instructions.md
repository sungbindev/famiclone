# Ember 开发指令

Ember 是 FC 与学习机模拟器：外壳使用 Electron、TypeScript 和 React，外壳以外的核心代码使用 C++。通用编码规范由 copilot-plugins 插件的规则负责，本文件只写本项目特有的约定。项目的结构与做法参照如见客户端（rujian-client）。

## 文档

| 文档 | 内容 |
| --- | --- |
| [docs/01-软件架构设计.md](../docs/01-软件架构设计.md) | 设计决策、进程划分、目录分层、依赖规则、消息约定 |
| [docs/02-硬件模块.md](../docs/02-硬件模块.md) | 主机、卡槽、卡带，总线与时钟，芯片与功能模块，存储区规则 |
| [docs/03-mapper插件规范.md](../docs/03-mapper插件规范.md) | mapper 插件的包格式、清单、脚本接口、匹配与加载 |
| [docs/04-调试服务.md](../docs/04-调试服务.md) | 调试服务的协议、运行控制、请求与事件 |
| [docs/05-界面.md](../docs/05-界面.md) | 界面布局、页面、菜单、状态栏，界面原型 |
| [docs/06-构建与模块登记.md](../docs/06-构建与模块登记.md) | 构建与测试命令、由 Schema 生成的代码、模块的登记方式、依赖检查 |
| [docs/待办备忘.md](../docs/待办备忘.md) | 已确定以后再做的事项；做完写进对应文档并从中删掉 |
| [docs/07-调试客户端工具.md](../docs/07-调试客户端工具.md) | `packages/ember-debug-sdk/`：SDK、终端客户端 `ember-debug`、MCP 服务 `ember-debug-mcp` 的设计、目录与依赖规则 |

- 开始任何开发任务前，先阅读架构设计文档与硬件模块文档。
- 【强制】每次修改代码、配置或脚本后，必须在同一次任务中同步全部受影响的文档（上表所列文档与本文件）；文档未同步不得汇报完成。汇报时逐项列出已同步的文档与位置，以及已检查但无须修改的文档。
- 文档中标为“待定”“拟定”的内容，实现前先向用户确认，不自行决定。
- 文档一个主题一份；新增模块时在对应文档中增加该模块的内容。

## 约定

- 硬件行为以真机资料与实测为准：依据 `../machines/`、`../reference/` 和实测，不以旧版 `../virtuanes/` 的代码为硬件事实；旧代码只参考逻辑，不复制。
- 每个硬件模块一个型号，模块之间只经契约层的抽象（总线、中断线、时钟、端口等）协作，实现之间不互相引用；具体实现只在组装根按主机型号创建并连线。硬件契约在 `core/src/contracts/hardware/`（只能包含标准库、自己与 `shared`），硬件模块在 `core/src/logic/hardware/<模块>/`（只能包含 `contracts/hardware`、`shared` 与 `platform` 的抽象）。
- CPU 核用 nestest 日志逐行比对验证，测试数据在 `core/test/data/nestest/`，不改动数据文件。
- 调试访问只经 `Machine::debugger()`（`MachineDebugger`）与 `BusDevice::peek`：无副作用读不得调用设备的 `read`（会清标志、回调脚本）；新增有寄存器的设备时，`peek` 保持默认（不驱动数据线），存储类设备才覆盖；检查请求由模拟线程执行（`RuntimeService::inspect`），不在别的线程直接碰主机；断点命中只经 `MachineDebugger::takeBreakpointHit` 交给运行时，总线的读写钩子在调试访问（检查、调试写内存、单步）期间必须暂停，不能触发读写断点；监视项只在模拟线程经 `RuntimeService::setObserver` 的回调里检查，回调里不得调用运行服务的方法；跟踪、调用记录的文件只经 `DebugFileWriter`（`output` 模块提供，只写调试输出目录，只收不含目录的文件名），记录与代码数据标记属于主机，不跨主机保留；代码数据标记里 CPU 地址到 PRG 偏移的换算只经卡带的 `describeAddress`，不在主机里写死地址范围。
- PPU 用公开的测试 ROM 验证（`core/test/data/` 下的 9 组，来源与结果见各目录的 `README.md`），由 `core/test/hardware/ppu-rom-test.cpp` 运行；改动 PPU、DMA、手柄或 `fc` 主机型号后必须重跑，暂时不能通过的项登记在 02 第 12 节“已知差异”，不删测试。测试用的卡带经 iNES 解析加真实的 nrom 插件装配（ppu_read_buffer 的 mapper 3 用测试插件 `test-cnrom`），核心程序和测试代码里都没有 mapper 的替身实现。
- PPU 的 CPU 相位（`fc.hpp` 的 `PPU_PHASE_TICKS`）按 vbl/NMI 时序测试调定，不随意改；PPU 的帧缓冲每像素 4 字节，内存顺序 R、G、B、A，见 02。
- 模拟器内置主机型号（含学习机整机的地址译码表与胶合逻辑），不含任何具体卡带的规则；卡带的规则只在 mapper 插件里。mapper 清单只能按名字选择主机型号，不能修改芯片内部与主板上的固定资源；插卡学习机（如裕兴，经典 FC 之后做）的卡带 mapper 在脚本里译码，经 `ctx.device` 读写主机上的软驱等功能模块，清单不声明连线（不恢复 `wires`、`uses`）。
- 学习机的 BIOS 不是卡带：不走 mapper 匹配，由用户在首选项的“机型与 BIOS”里按机型添加（复制进用户数据目录的 `bios/`），经“文件”菜单的“开机…”开机；NES 文件只走卡带路线（主机按清单的 `host` 自动装配），mapper 171 没有插件时按 `mappers.noMatch` 报错。整机机型只有 sb97、sb2000、bbk10、bbk20，各是独立的主机型号，目前只做 sb2000；`machines` 模块已实现（BIOS 目录 `<用户数据目录>/bios/<机型>/`）。sb2000 的 CPU 是 15 分频（约 1.7734 MHz），按真机 hwtest 的定时器计数定的，不要改成 16 分频；学习机的真机资料只用 famiclone 仓库里的（`reference/`、`packages/fcdev/docs/sb2000/`、模拟器源码与实测）。
- 功能模块（软驱、打印机、声音、磁带等）各机型共用一份实现，模块与总线地址无关，地址由每个机型的地址译码表在主机装配里挂接；键盘、鼠标协议、整机寄存器与胶合逻辑是机型自己的。划分只看两点：地址由谁译码（主机译码表还是卡带 mapper）、能否由用户插拔（端口上的外部设备），物理上在不在同一颗芯片里不是规则。
- 官方 mapper 插件的源码在 `plugins/<id>/`（目录名等于清单里的 `id`，只含 `manifest.json`、`main.js`、`icon.png`），用 `npm run plugins:pack` 打包到 `build/plugins/`；核心测试依赖打好的包（`npm run core:test` 会先打包）。改了插件规范或宿主的行为，同步 03，并把与设计不一致处写进 03 第 11 节。
- mapper 插件包与脚本来自外部，是不可信输入：包的读取要检查路径穿越与大小上限，脚本只能经宿主对象 `ctx` 访问外部，内存、栈、单次回调时长都有上限；C 回调里的 C++ 异常必须在边界截获，脚本出错不能让异常穿过 CPU 的总线访问（卡带记下错误，由运行模块轮询）。
- 存储按位置分三档：芯片内与主板上（含学习机整机的存储与 BIOS）的由芯片模块、主机型号写死，卡带上的由 mapper 清单声明，见硬件模块文档。
- 请求、事件与数据类型只在 `schema/<模块>.schema.json` 中定义和修改，TypeScript 与 C++ 的 `generated/` 目录由脚本生成，不手写、不手改。
- 调试服务与界面共用同一套请求；调试专用的请求在 Schema 里标 `debugOnly`，只经调试服务开放，界面不使用。调试服务（`core/src/infra/debug/`，网络在 `core/src/platform/network/`）是 WebSocket 加 JSON-RPC 2.0，只监听 127.0.0.1，令牌必须；开关、端口、令牌是首选项的 `debug.*`，启动参数 `--debug-port`、`--debug-token` 强制开启；会话逻辑（`DebugSession`）不碰网络，结果或事件数据顶层的 `data` 字段是 `nlohmann::json::binary` 时改发二进制帧；接口描述文档 `docs/api/openrpc.json` 由 `npm run schema` 生成，不手写；修改协议同步 04 文档，并运行 `npm run debug-sdk:check`（`packages/ember-debug-sdk/` 的类型与协议参考跟着 Schema 变）。
- 调试客户端工具在仓库根目录的 `packages/ember-debug-sdk/`（独立的 npm 包 `ember-debug-sdk`，有自己的 `package.json` 与测试，依赖由仓库根目录的 npm 工作区统一安装，不进入应用打包），设计见 [docs/07-调试客户端工具.md](../docs/07-调试客户端工具.md)：`generated/` 由 Ember 的 `npm run schema` 生成，不手写不提交；`src` 分 `contracts`、`shared`、`logic`、`infra`、`entry`、`cli`、`mcp`，`logic` 不引用 `infra`，库入口不引用 Node 内置模块，不引入第三方运行时依赖；自动测试只用假传输层，真实联调用 `scripts/live-check.mjs`；终端客户端与 MCP 服务的用户可见文字为中文；修改它的对外用法同步它的 `README.md`。
- 新增模块时在各层建立同名模块目录，并在 C++、主进程、渲染进程三份模块清单中登记；分层与模块依赖由 `npm run boundaries` 检查，见 [docs/06-构建与模块登记.md](../docs/06-构建与模块登记.md)。
- 业务逻辑、模拟与数据读写一律放 C++ 核心；TypeScript 部分只做外壳、界面、入口校验、进程间转接和系统能力。
- 界面的全部组件与令牌都来自 rujian-ui-kit，客户端不重复实现组件、不自定义令牌；界面不做调试窗口。
- 外壳（`src/ui/shell/`）只渲染登记表，不写死任何入口：显示区、对话框、命令与默认快捷键、菜单项、底部面板（系统栏左侧自动生成按钮）、状态项都由模块在 `src/bootstrap/renderer/modules/<模块>.ts` 登记，外壳自己的登记项在 `ui/shell/shell-module.ts`；登记表由 `ui/shell/build-registry.ts` 检查（标识前缀、重复、引用、快捷键冲突），规则见 [docs/06-构建与模块登记.md](../docs/06-构建与模块登记.md) 第 3 节“界面登记”；模块的显示区、对话框、面板与状态项只经 `useShell()` 和外壳交互（打开对话框、切换面板、发通知等）。按键已被画面处理（`defaultPrevented`）时外壳不再响应快捷键。登记项还有子菜单（`menuItems[].submenu`，项随数据变化，如“最近打开”）、可以是函数的命令名称（如“上电”与“断电”的切换）、常驻组件 `Host`（外壳启动后一直挂载，用来订阅事件，模块状态变化后调用 `shell.refreshCommands()` 让外壳重算菜单的名称、勾选与置灰）；焦点在文本输入控件里时，不带 Ctrl 或 Alt 的快捷键不触发。运行模块的状态集中在 `ui/runtime/runtime-controller.ts`（不依赖 React），运行面板、系统栏、画面提示与菜单都从它取数据，核心重启时经 `onCoreStatusChange` 清空并重新读取状态；界面层的模块之间不引用对方的契约（最近打开清单的类型取自 `EmberBridge` 的返回值）。
- 主窗口隐藏系统标题栏（系统绘制窗口按钮）；界面只能发起白名单里的窗口动作（`shell.windowAction`，见 `contracts/shell/window-control.ts`），界面状态（窗口位置、底部面板高度、已选机型）由主进程保存到用户数据目录的 `config/shell-state.json`，收到的修改整条校验（`shared/shell-state`），文件不存在或损坏时用默认值、不报错，见 01 第 2、9 节。用保存的边界恢复窗口时要用 `setExactBounds`，否则非整数倍缩放下窗口每次启动大 1 像素。
- 渲染进程开启 `contextIsolation`、`sandbox`，关闭 `nodeIntegration`；只有 `src/platform`、`src/entry`、`src/core-host`、`src/bootstrap` 可以引入 `electron`，C++ 中只有 `core/src/binding` 可以引用 Node-API。
- 先在 Windows 上开发、调试与编译；macOS 的编译与打包脚本同时写齐，但不在 Windows 上运行或测试。
- 测试分层，不每次都跑全量：开发中只跑与改动相关的用例（`npm run core:test -- -R <名称>`、`vitest run <文件>`）；小阶段做完跑 `npm run core:test -- -E ^Roms/` 加其他检查；只有改了 CPU、PPU、APU、总线、DMA、手柄、`fc` 主机型号、mapper 宿主等底层，或交付前，才跑含测试 ROM 的 `npm run check`。过滤命令的输出时只取需要的行（编译错误用区分大小写的 `error C`、`warning C`，测试结果取 `% tests passed` 与 `FAILED`），不要把用例名整批读进来。
- 硬件状态只属于模拟线程（`logic/runtime`）：电源、复位、暂停、继续、装入与取出卡带、按键都投递到它的命令队列，在帧边界执行；每帧完成后经 `platform/streams/frame-output.hpp` 的 `FrameOutput` 发布帧缓冲，把主机生成的声音采样（`Machine::takeAudio`）写进 `platform/audio/audio-output.hpp` 的 `AudioOutput`，`logic` 只能包含 `platform` 的事件、取消、错误日志、帧输出、声音输出五个抽象，硬件模块只能包含前两个。
- 声音由核心直接输出到系统（miniaudio，只在 `platform/audio/` 里使用）：模拟线程写、音频回调线程读的无锁环形缓冲，回调里不加锁、不分配内存，用缓冲水位微调重采样比（±0.5%）保持同步；暂停、断电时淡出，继续时清空过时的采样再淡入；设备打不开时降级为空输出并写错误日志；音量与静音只经 `output.setVolume` 请求，音量保存到首选项的 `sound.volume`，静音不保存。
- 经典 FC 的 APU（`apu-2a03`）按调度器的方式运行（被访问前追赶，会改变 IRQ 线与 RDY 线的时刻预测成调度器事件）；帧计数器、长度计数器与 DMC 的时序以 blargg 的测试 ROM 为准，暂时不一致的项登记在 02 第 12 节“已知差异”并由测试断言；`$4017` 同时属于手柄口与 APU，由主机装配里的 `PadApuPort` 转给两边，两个模块互不引用。
- 画面与输入不走请求，用单独的消息端口（`window.ember.streams.requestPorts()`，契约在 `src/contracts/stream-ports/`）：核心到渲染进程的帧消息（带 `publishedAt` 与 `hostDropped`）、渲染进程在画面端口上回的帧确认（`ack`）与渲染进程到核心的按键消息，格式只在 `stream-messages.ts` 里定义；核心宿主对渲染进程发来的输入与确认逐条校验，在途帧不超过 `MAX_FRAMES_IN_FLIGHT`（背压，超过就丢新帧），binding 的帧缓冲槽由 JavaScript 线程创建、不转移、不保存。
- 画面在渲染 Worker（`src/ui/runtime/render-worker.ts`）里用 WebGL 显示：Worker 收帧、校验、只保留最新一帧、由它自己的 `requestAnimationFrame` 驱动绘制并确认（每一帧恰好确认一次：画出、被顶掉、格式不符但能读出帧号）；收帧与结算逻辑写成不依赖 Worker 对象的纯类（`frame-session.ts`、`frame-receiver.ts`）以便测试。Worker 与它依赖的文件不得引用 `electron`、Node 内置模块、React、组件库、`.tsx` 与样式文件（`npm run boundaries` 检查）；内容安全策略只给 `worker-src 'self'`，不放宽 `script-src`。开发期诊断入口 `window.emberRuntimeDiagnostics` 只在 `import.meta.env.DEV` 下挂载。
- 键盘、鼠标与游戏手柄只采集原始事件（`key-capture.ts`、`pointer-capture.ts`、`gamepad-capture.ts`），界面不做键位映射，程序里也没有默认键位，由卡带的 mapper 脚本（`onKey`、`onMouse`、`onGamepad`，经 `ctx.pad` 设置手柄口）决定怎么用；键盘只在画面区获得焦点时采集，失焦、页面隐藏、卸载时松开全部按键，可编辑元素上不采集。
- 主进程自己处理的请求写在 Schema 的 `mainRequests`，经 `window.ember.main` 暴露与 `entry/ipc/core/main-request-ipc.ts` 校验；参数带磁盘路径的核心请求标 `mainOnly`，路径只来自用户在系统窗口里的选择（如 `cartridges.open`）或主进程的启动参数 `--cartridge=`，渲染进程不能传任意路径。最近打开的卡带由主进程保存（`config/recent-cartridges.json`，最多 10 个，读不到或不合格时用空清单），渲染进程只能经 `cartridges.recent`、`cartridges.openRecent` 拿到编号与显示名并按编号重新打开；打开卡带都经 `entry/ipc/cartridges/load-cartridge.ts`：先记入清单再加载，失败时移出。
- 设置项由各模块在核心里经 `PreferenceService` 登记（标识“分组.名称”、种类、出厂默认值），取值只保存在核心的 `config/preferences.json`，界面只经 `preferences.list/set/reset` 与事件 `preferences.changed` 读取和修改，不自己保存设置；新增有设置项的功能时在自己的模块里登记，并同步 04 第 4.8 节的设置项表与 05 第 4.5 节。设置项的修改立即生效，取值不符合登记时核心拒绝（`preferences.invalidValue`）；首选项页不显示操作结果，保存失败用外壳通知。主题经主进程设为系统主题来源（`nativeTheme.themeSource`），界面只监听 `prefers-color-scheme`。
- TypeScript 的 `platform` 不引用 `contracts`，用结构类型实现契约里的接口，由组装根注入。
- 使用的第三方开源代码按如见的做法登记 `third-party.json`，由脚本生成“关于”对话框的开源清单与许可全文，不手写。

## 常用命令

在 `ember/` 目录下运行，详见 [docs/06-构建与模块登记.md](../docs/06-构建与模块登记.md)。

| 命令 | 用途 |
| --- | --- |
| `npm start` | 生成 Schema 代码、打包插件、编译原生模块，再启动开发版 |
| `.\debug.ps1` | 在任意目录下启动开发版（切到项目目录后执行 `npm start`） |
| `npm run schema` | 由 `schema/` 生成 TypeScript 与 C++ 代码 |
| `npm run core:build` | 打包官方插件并编译原生模块，输出 `core/build/Release/ember_core.node`（Windows 用 Visual Studio 自带的 CMake） |
| `npm run core:test` | 打包官方插件，构建并运行 C++ 测试；`--` 之后的参数交给 CTest：`-- -R <名称>` 只跑匹配的用例，`-- -E ^Roms/` 排除测试 ROM 用例 |
| `npm run plugins:pack` | 把 `plugins/<插件>/` 打成 `build/plugins/<id>.emap` |
| `npm test` | 运行 TypeScript 测试 |
| `npm run typecheck` | 类型检查 |
| `npm run lint` | oxlint 检查 |
| `npm run boundaries` | 分层、模块依赖与模块登记检查 |
| `npm run debug-sdk:check` | `packages/ember-debug-sdk/` 的生成、类型检查、lint、构建与测试（首次前在仓库根目录 `npm install`） |
| `npm run check` | 以上全部检查与测试 |
| `node tools/dev/smoke-run.mjs` | Electron 冒烟：启动开发版加载测试 ROM，经 CDP 检查帧号、渲染 Worker 的画面帧与非空画面、键盘输入、系统栏的机器状态与运行面板、F8 快捷键、“文件”菜单的最近打开与选择机型子菜单、最小窗口下的标题栏与画面框四周间距、展开与收起面板时窗口大小不变且画面框等比缩放、点画面可以锁定指针、页面重载与核心重启（含重启后运行按钮状态刷新）（见 docs/06 第 8 节），只结束自己启动的进程 |
| `node tools/dev/latency-run.mjs` | 画面延迟与丢帧测量：开发版运行 30 秒，打印核心、收到、渲染帧率与各类丢帧、两段延迟（见 docs/06 第 9 节、docs/01 第 7 节），只结束自己启动的进程 |
| `npm run package:win` | 打包 Windows x64 版 |
| `npm run make:win` | 生成 Windows x64 安装程序 |
| `npm run package:mac` | 打包 macOS arm64 版（只在苹果电脑上运行） |
