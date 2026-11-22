# 迁移到 VS Code 扩展

把 `packages/` 下 `fcdev`、`create-fcdev`、`fcdev-cc65` 三个包的全部功能移入 VS Code 扩展（`vscode/`），模拟器由使用者在扩展设置里指定位置，不随扩展分发；迁移完成后删除 `packages/`（含 `fcdev-ember`），仓库只维护这一个扩展。

本文是迁移计划：第 1 至 5 节写目标与方案，第 6 节写分阶段计划，第 7、8 节写外部引用清单与完成判据，第 9 节记录进度，每个阶段完成后更新。第 3.2 节的三项已由用户在 2026-11-22 确认按建议执行。

## 1. 目标与范围

用户要求：

- 三个包的功能全部由扩展提供，用 VS Code 的方式给出能力。
- 模拟器 Ember 不进扩展，使用者在扩展设置里手动设置模拟器位置。
- 全部功能转入扩展后删除 `fcdev`，以后只维护这一个扩展。

保持不变的对外标识：

- 扩展 ID `sungbin.fcdev`、命令前缀 `fcdev.`、现有 30 个语言模型工具的名字。
- 项目配置文件名 `fcdev.json`、反汇编工程文件 `disasm.json`。

范围外：

- Ember 的源码与功能，只更新它与 `fcdev` 相关的文档、脚本与生成模板。
- `cc65/` 源码与 `machines/` 下的资料，只更新引用。

## 2. 现状与总量

统计方法：文件数用 `git ls-files`，行数是文本文件的换行数，引用数用 `git grep -I -E` 加第 2.4 节的正则；迁移结束后用同一方法复核。

### 2.1 npm 包

| 包 | 入库文件数 | 内容 | 去向 |
| --- | --- | --- | --- |
| `fcdev` | 491 | `bin` 1、根文件 5、`debug-sdk` 35、`docs` 8、`src` 123、`targets` 263、`test` 56 | 全部移入扩展 |
| `create-fcdev` | 24 | `src` 6、`templates` 13、`bin` 1、`test` 1、其余 3 | 移入扩展 |
| `fcdev-cc65` | 1 | 只入库 `package.json`；工具与运行库由 `tools/build-cc65.mjs` 生成 | 工具链随扩展发布 |
| `fcdev-ember` | 1 | 只入库 `package.json`；`Ember.exe` 由 `tools/build-ember.mjs` 组装 | 取消 |
| 合计 | 517 | | |

### 2.2 `fcdev` 源码与测试

| 部分 | 文件数 | 行数 | 去向 |
| --- | --- | --- | --- |
| `src/shared` | 6 | 430 | 移入 |
| `src/config` | 2 | 118 | 移入 |
| `src/targets` | 3 | 231 | 移入 |
| `src/toolchain` | 2 | 84 | 移入 |
| `src/packagers` | 5 | 339 | 移入 |
| `src/build` | 9 | 627 | 移入 |
| `src/api` | 2 | 173 | 移入 |
| `src/docs` | 3 | 108 | 移入 |
| `src/emulator` | 14 | 1,120 | 移入，取消随包 Ember 的安装与定位 |
| `src/disasm` | 65 | 5,487 | 移入 |
| `src/cli` | 12 | 625 | 取消，由入口层的命令、任务与工具取代 |
| `src` 合计 | 123 | 9,342 | |
| `debug-sdk/src` | 18 | 1,255 | 移入 |
| `debug-sdk/scripts` | 3 | 193 | 移入或取消，见 Q2 |
| `targets/nes` | 9 | 340 | 移入 |
| `targets/sb2000` | 254 | 10,947 | 移入（含运行库源码 226 个文件） |

测试文件：

| 位置 | 测试文件数 | 其他文件 |
| --- | --- | --- |
| `packages/fcdev/test` | 31 | 固定数据 23、辅助 2 |
| `packages/create-fcdev/test` | 1 | 无 |
| `packages/fcdev/debug-sdk/test` | 7 | 辅助 1 |
| 仓库根 `test` | 8 | 无 |
| 合计 | 47 | |

### 2.3 扩展现状

- 入库 94 个文件：`src` 50 个（`bootstrap` 1、`contracts` 10、`entry` 11、`infra` 9、`logic` 3、`platform` 2、`shared` 14），`test` 23 个，`chat` 5 个。
- 功能目录：`assembly`、`c-intellisense`、`disassembly`、`project`、`errors`。
- 经外部 `fcdev` 工作的位置：`infra/project` 的 `fcdev-cli-runner.ts`、`fcdev-project-info-provider.ts`（运行命令行 `info`、`sync`），`infra/disassembly` 的 `fcdev-module-runner.ts`、`workspace-disasm-runner.ts`、`workspace-emulator-runner.ts`（动态加载 `node_modules/fcdev`），`fcdev-build-comparer.ts`（运行 `fcdev build`）。这些位置随迁移全部删除。

### 2.4 外部引用的基线

正则：`packages/fcdev|fcdev-cc65|fcdev-ember|create-fcdev|npx fcdev|fcdev (build|run|disk|debug|sync|info|docs|api|disasm)|FCDEV_[A-Z_]+|node_modules/fcdev`。

| 仓库 | 目录 | 命中文件数 | 命中行数 |
| --- | --- | --- | --- |
| famiclone | `packages`（随迁移删除） | 78 | 279 |
| famiclone | `apps` | 37 | 80 |
| famiclone | `docs` | 11 | 133 |
| famiclone | `ember` | 20 | 74 |
| famiclone | `tools` | 9 | 57 |
| famiclone | `vscode` | 23 | 52 |
| famiclone | `machines` | 3 | 5 |
| famiclone | `test` | 2 | 2 |
| famiclone | 根文件（`.gitignore`、`README.md`、`package-lock.json`） | 3 | 48 |
| famiclone | 合计 | 186 | 730 |
| copilot-plugins | 全部 | 8 | 63 |
| docs | 全部 | 1 | 7 |

## 3. 决策

### 3.1 已定

| 编号 | 决策 | 依据 |
| --- | --- | --- |
| M01 | 功能用 TypeScript 按扩展现有的分层重写并移入，扩展不加载外部的 `fcdev` | 用户要求；扩展按 `contracts`、`shared`、`logic`、`infra`、`entry`、`platform`、`bootstrap` 分层 |
| M02 | 工具链、目标、项目模板与使用文档随扩展发布，放在扩展目录下 | 用户要求全部功能转入扩展 |
| M03 | 模拟器位置由设置 `fcdev.emulator.path` 给出，作用域为 `machine`（工作区设置不能指定要运行的程序）；没有设置时运行类操作报错，并给出打开设置的按钮 | 用户要求手动设置模拟器位置 |
| M04 | 取消 `fcdev.json` 的 `emulator` 项、环境变量 `FCDEV_EMULATOR`、随包 Ember 的复制与定位、`fcdev-ember` 包和 `tools/build-ember.mjs` | 位置只有设置这一种途径 |
| M05 | `run --debug` 的端口由设置 `fcdev.emulator.debugPort` 给出，默认 5699；取消环境变量 `FCDEV_DEBUG_PORT` | 与 M03 同理 |
| M06 | 命令行取消，能力经命令、任务、设置与语言模型工具提供，对照见第 4.3 节 | 用户要求用 VS Code 的方式提供能力 |
| M07 | 平台与现状相同，只支持 Windows x64：工具链是 Windows 可执行文件，打包用 `vsce package --target win32-x64` | `tools/build-cc65.mjs` 只产出 `.exe` |
| M08 | 47 个测试文件迁成 vitest 用例，放在 `vscode/test/` 的同名功能目录；真实 Ember 的用例经测试专用环境变量 `FCDEV_TEST_EMULATOR` 取得位置，只在测试里读取 | 扩展已用 vitest；设置只在 VS Code 里存在 |
| M09 | 使用文档放 `vscode/docs/`，主题登记表是 `vscode/docs/topics.json`，扩展与 Ember 帮助站点的导入脚本读同一份 | Ember 帮助站点的“fcdev 开发手册”以使用文档为来源，导入脚本不应依赖扩展的编译产物 |
| M10 | 迁移后用真实 cc65 构建全部样例项目与四个模板，确认都能构建成功 | 删除 `fcdev` 之后没有可对照的实现 |
| M11 | 扩展的 `engines.vscode` 在阶段 4 核对：调试客户端用全局 `WebSocket`，宿主的 Node 要达到 22 以上，按官方文档确定最低版本 | [09-AI反汇编工具](09-AI反汇编工具.md) 第 2 节已定“实施时核对”；2026-11-22 已核对：VS Code 1.101 起扩展宿主的 Node 是 22.15.1（Electron 35，见 1.101 版本说明），故 `engines.vscode` 写 `^1.101.0` |
| M12 | 头文件目录写进 `.vscode/c_cpp_properties.json` 与 `compile_flags.txt`，路径在扩展目录里，随扩展升级而变；激活时比较路径，不同就重新同步 | 现有自动同步在激活时执行，只需把路径比较纳入 |

### 3.2 已确认（2026-11-22）

| 编号 | 问题 | 结论 |
| --- | --- | --- |
| Q1 | 取消命令行后，仓库里的样例项目与 Node 脚本如何继续构建、运行 | A：样例项目用 VS Code 任务；仓库内的 Node 脚本直接引用扩展的编译产物 |
| Q2 | `fcdev debug` 终端客户端与可被外部引用的调试 SDK 如何处理 | B：客户端功能做成扩展命令，SDK 只作扩展内部功能 |
| Q3 | 是否保留自备 cc65 的途径（`fcdev.json` 的 `toolchain`、环境变量 `FCDEV_CC65_HOME`） | 取消，只用随扩展的工具链 |

**Q1 的做法：**

- A（已定）：
  - `apps/` 下每个样例项目加 `.vscode/tasks.json`（`fcdev` 类型的构建、运行、磁盘镜像任务），删除它们的 `package.json`、`package-lock.json` 与对 `fcdev` 的依赖。
  - `apps/sb2000/fbasic/tools`（5 个脚本）、`machines/bbg/tools/probe-fc.mjs`、`tools/*.mjs`、Ember 的开发脚本直接 `import` `vscode/out/` 里编译好的模块。这些模块属于 `contracts`、`shared`、`logic`、`infra`，结构测试规定基础设施层只有登记的两个文件引用 `vscode`；阶段 0 已验证 `.mjs` 能按命名导出引用编译后的模块。
- B：扩展向集成终端的 `PATH` 注入 `fcdev.cmd`，样例项目的 npm 脚本保持不变。不选的原因：这是为扩展之外的终端另留一条途径，与“用 VS Code 的方式提供能力”相悖，外部终端与 CI 仍然要维护命令行。
- C：把核心代码复制一份给脚本用。不选的原因：一份能力两份实现。

**Q2 的做法：**

- A：终端客户端随扩展发布（`debug/cli.js`），Ember 帮助文档里的示例写成 `node "<扩展目录>/debug/cli.js" call …`。不选的原因：命令行里要写扩展安装目录，路径随扩展版本变化。
- B（已定）：
  - 终端客户端的 `endpoint`、`methods`、`describe`、`call`、`screenshot`、`watch` 做成命令：调用调试请求（选请求、填参数）、查看请求说明、保存截图、监视事件，结果写入“fcdev 调试”输出通道。
  - Copilot 已有 11 个 `emu_*` 工具，不新增工具。
  - SDK 源码并入扩展作内部功能，不对外提供 `import` 入口，也不生成对外的 API 文档；调试协议是开放的，第三方可以按协议自行连接。
  - Ember 的开发脚本按 Q1 的 A 引用 `vscode/out/` 里的客户端。
  - Ember 帮助文档里每个请求的 `fcdev debug call …` 示例，由 `ember/tools/schema-gen` 与 `debug-sdk/scripts/docs.mjs` 的模板生成，模板只写请求名与参数。
- C：终端客户端与 SDK 留在 Ember 仓库独立维护。不选的原因：与“只维护这一个扩展”不符。

**Q3 的做法：**

- 取消：运行库（`.lib`）与增量构建的指纹都绑定随扩展的 cc65 版本，[03-cc65工具链](03-cc65工具链.md) 已规定不使用其他位置的 cc65，避免版本不一致；取消后工具链只有一个来源，错误信息也最简单。
- 保留：新增设置 `fcdev.toolchain.path`。不选的原因：运行库与自备 cc65 版本不一致时，错误难以定位。

## 4. 目标形态

### 4.1 扩展目录

```text
vscode/
  package.json         唯一的 package.json，脚本含构建工具链、运行库、文档与打包检查
  src/                 分层源码，见第 4.2 节
  test/                vitest 测试，按功能分目录
  toolchain/           cc65 的 bin、include、asminc、cfg、lib（生成物，不入库，随 .vsix 发布）
  targets/             nes、sb2000：目标定义、头文件、启动代码、运行库源码、链接配置、接口参考；运行库 lib/ 是生成物
  templates/           四个项目模板与公共文件
  docs/                使用文档与 topics.json
  chat/  syntaxes/  resources/    已有
```

### 4.2 源码分层

`fcdev` 的目录并入扩展的分层，层内按功能分目录；依赖方向、文件头与注释规则沿用扩展现有的约定与 `test/structure.test.ts`。

| `fcdev` 的部分 | 扩展中的位置 |
| --- | --- |
| `shared`（错误类型、文件与进程小工具、参数检查、目标描述） | 错误类型并入 `shared/errors`；进程与文件小工具放 `platform/process`；目标描述放 `shared/disassembly` |
| `config` | `contracts/project` 的类型、`shared/project` 的解析与校验、`infra/project` 的读取 |
| `targets` | `contracts/targets`、`shared/targets`（名称规则、字段校验）、`infra/targets`（读取 `targets/<名>/target.json`） |
| `toolchain` | `contracts/toolchain`、`infra/toolchain`（定位扩展目录里的 cc65，运行工具） |
| `packagers` | `shared/packaging`（映像与 FAT12 的字节处理）、`infra/packaging`（写出文件） |
| `build` | `contracts/build`、`logic/build`（构建计划、增量、流程）、`infra/build`（源文件收集、依赖文件读取、转码） |
| `api` | `shared/c-api`（头文件文档注释解析）、`logic/c-api`（查询） |
| `docs` | `contracts/usage-docs`、`logic/usage-docs`、`infra/usage-docs` |
| `create-fcdev` | `contracts/scaffold`、`logic/scaffold`、`infra/scaffold`；模板在 `templates/` |
| `emulator` | `contracts/emulator`、`logic/emulator`（操作、会话、开机步骤）、`infra/emulator`（启动进程、端点文件） |
| `debug-sdk` | `contracts/debug-client`、`logic/debug-client`（帧编解码、会话、请求目录）、`infra/debug-client`（WebSocket 传输）；Ember 生成的类型放 `contracts/debug-client/generated/` |
| `disasm` | `shared/disassembly`（指令表、解码、映像格式）、`logic/disassembly`（分析、工程模型、生成、回读、视图、查询、操作）、`infra/disassembly`（工程与映像的读写） |
| `cli` | 取消 |

- “反汇编核心里除映像格式目录外不出现机型名”的检查（[07-反汇编设计](07-反汇编设计.md) 第 5 节）并入扩展的结构测试。
- `entry` 层新增功能目录：`build`（任务提供者、命令、问题匹配器）、`scaffold`、`usage-docs`、`emulator`、`debug-client`；组装根 `bootstrap/extension.ts` 创建全部实现。

### 4.3 能力对照

| 现有能力 | 扩展中的提供方式 |
| --- | --- |
| `npm create fcdev` | 命令“fcdev：新建项目”（选目标、语言、空目录）、工具 `fcdev_new_project` |
| `fcdev build [路径]` | 任务类型 `fcdev`（`command` 为 `build`，可给 `path`）、命令“构建项目”“构建当前文件”“构建当前目录”、工具 `fcdev_build`；问题匹配器 `$fcdev` 识别 `文件(行): Error: 信息` |
| `fcdev disk [路径]` | 任务 `command` 为 `disk`、命令“构建磁盘镜像”、工具 `fcdev_build` 的 `disk` 参数 |
| `fcdev run [路径]`、`--debug`、`--port` | 任务 `command` 为 `run`（`debug` 参数）、命令“运行”“调试运行”、工具 `fcdev_run`；端口来自设置 |
| `fcdev debug …` | 按 Q2 |
| `fcdev disasm <文件>` | 命令“反汇编映像文件”（构建、追踪、生成、回读一次完成）；已有 19 个 `disasm_*` 工具直接调用核心 |
| `fcdev sync` | 已有命令与自动同步，直接调用核心 |
| `fcdev info [--json]` | 命令“显示项目信息”（写入输出通道）、工具 `fcdev_info`；智能感知直接使用核心 |
| `fcdev api [词]` | 命令“查询 C 接口”（快速选择）、工具 `fcdev_api` |
| `fcdev docs [词]` | 命令“打开使用文档”（快速选择后用 Markdown 预览打开）、工具 `fcdev_docs`（返回主题清单或文档正文） |

- 新增 6 个语言模型工具：`fcdev_new_project`、`fcdev_build`、`fcdev_run`、`fcdev_info`、`fcdev_api`、`fcdev_docs`，工具总数由 30 个变为 36 个；工具名与说明只写在 `package.json` 的 `languageModelTools`。
- 11 个 `emu_*` 工具的必填参数 `projectDir` 只为定位工作区里的 `fcdev` 而设，位置由设置给出后取消，运行类工具只在需要项目时才要路径。
- 自定义智能体“6502 反汇编工程师”与 `copilot-plugins` 四个智能体的工具清单同步加入新工具。

### 4.4 设置项

| 设置 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `fcdev.emulator.path` | 字符串，作用域 `machine` | 空 | Ember 可执行文件的路径 |
| `fcdev.emulator.debugPort` | 数字，作用域 `machine` | 5699 | 调试运行用的端口 |
| `fcdev.intelliSense.autoSync`、`fcdev.intelliSense.promptConfigurationSelection` | 布尔 | 开启 | 已有，保持不变 |

`fcdev.json` 的配置项除 `emulator`（M04）和按 Q3 取消的 `toolchain` 外保持不变。

### 4.5 构建与打包

- `vscode/package.json` 的脚本顺序：`build`（`tsc`）、`build:toolchain`（运行 `tools/build-cc65.mjs`，输出到 `vscode/toolchain/`）、`build:libs`（各目标的运行库）、`gen:docs`（接口参考、符号参考、示例源码三份生成文档）、`check:package`（核对 `.vsix` 清单）、`package`（`vsce package --target win32-x64`）；`vscode:prepublish` 串联它们。
- `tools/build-cc65.mjs` 把工具与运行库放进 `vscode/toolchain/` 并写出校验清单；`tools/build-libs.mjs`、三个文档生成脚本与 `check-sb2000-api.mjs` 引用 `vscode/out/` 里的模块。
- `check:package` 取代 `release-check` 与 `verify-install`：核对 `.vsix` 含工具链（文件与清单的 SHA-256 一致）、各目标的运行库、模板、使用文档，不含测试、`node_modules`、密钥与 `.tgz`。
- `.vscodeignore` 排除源码与测试，保留 `toolchain`、`targets`、`templates`、`docs`。

## 5. 风险与技术前提

- 样例项目与脚本引用 `vscode/out/` 的前提是编译产物导出稳定：阶段 0 用一个 `.mjs` 脚本验证命名导出可被引用，验证不过时先调整导出方式，再继续。
- 工具链与目标资源在扩展目录下，路径含扩展版本号；所有读取资源的代码经 `contracts` 的“扩展资源位置”抽象取得路径，不直接用 `__dirname`，测试时注入临时目录。
- 构建增量指纹使用扩展版本，扩展升级后样例项目首次构建会全部重编。
- 迁移期间不保留 `fcdev` 与扩展并存的过渡实现；阶段之间按依赖顺序推进，被取代的位置当阶段删除。

## 6. 阶段计划

每个阶段都同步本阶段影响的开发文档与扩展的 README，阶段结束先验证再进入下一阶段。

### 阶段 0：骨架

范围：

- 迁移前的构建产物没有保留为对照数据；构建正确性靠真实 cc65 构建全部样例与四个模板（`apps/sb2000` 下 19 个入库的项目、`apps/disasm-example`、四个模板）来保证，`apps/sb97/runnes` 未入库，不在范围内。
- 建立结构测试规则：基础设施层只有登记的文件引用 `vscode`（脚本才能直接加载编译产物），新增功能目录沿用层间依赖与功能目录隔离规则；“反汇编核心没有机型名”在阶段 3 随核心加入。
- 验证 `.mjs` 脚本能引用 `vscode/out/` 里的命名导出。
- `package` 脚本加 `--target win32-x64`；`.vscodeignore` 不排除 `toolchain`、`targets`、`templates`、`docs`，无需修改；其余脚本在对应文件出现的阶段加入。

完成标准：结构测试与引用验证通过；`npm run check` 通过。

### 阶段 1：构建核心

范围：

- `src/shared`、`config`、`targets`、`toolchain`、`packagers`、`build` 共 27 个文件、1,830 行。
- `targets/` 263 个文件移入 `vscode/targets/`；`tools/build-cc65.mjs`、`tools/build-libs.mjs` 输出到 `vscode/toolchain/` 与各目标的 `lib/`。
- 对应测试（参数解析的测试随 `cli` 取消）。

完成标准：用真实 cc65 构建全部样例项目都能成功，学习机项目能生成磁盘镜像；增量、源文件转码（GB2312）、多页链接、程序名规则的用例通过；Q3 取消的途径已删除。

### 阶段 2：项目入口、新建项目与文档查询

范围：

- 任务提供者、问题匹配器 `$fcdev`、构建与磁盘镜像命令、显示项目信息命令。
- `create-fcdev` 的 24 个文件移成 `scaffold` 功能与 `templates/`；命令与工具 `fcdev_new_project`。
- `src/api` 2 个文件、`src/docs` 3 个文件；使用文档 8 篇与三份生成文档移入 `vscode/docs/` 与 `vscode/targets/`；`topics.json`；命令与工具 `fcdev_api`、`fcdev_docs`。
- 工具 `fcdev_build`、`fcdev_info`。
- `infra/project` 的命令行运行器与项目信息提供者、`fcdev-build-comparer.ts` 直接调用构建核心，替换后删除原文件。
- 智能感知的项目信息与 `sync` 直接使用核心，落实 M12。

完成标准：

- 四个模板由命令创建后能构建。
- 工具声明与代码一一对应的测试通过，使用文档主题与目录一致的测试通过。
- 任务报错跳转到源码行，在真实 VS Code 里手动核对，由用户确认。

### 阶段 3：反汇编核心

范围：

- `src/disasm` 65 个文件、5,487 行移入 `shared`、`logic`、`infra` 的 `disassembly`。
- 删除 `fcdev-module-runner.ts`、`workspace-disasm-runner.ts` 与 `disasm-runner` 契约里面向动态加载的部分；19 个 `disasm_*` 工具直接调用核心。
- 命令“反汇编映像文件”。
- 对应测试与 `apps/disasm-example` 的工作流测试。

完成标准：NES 与 SB2000 样例的反汇编回读通过；`disasm-workflow` 的八步断言通过；19 个工具的用例通过。

### 阶段 4：模拟器与调试

范围：

- `src/emulator` 14 个文件、1,120 行；`debug-sdk/src` 18 个文件、1,255 行及其测试；设置 `fcdev.emulator.path`、`fcdev.emulator.debugPort`（M03 至 M05）。
- 运行与调试运行的任务、命令、工具 `fcdev_run`；11 个 `emu_*` 工具直接调用核心并取消必填的 `projectDir`；删除 `workspace-emulator-runner.ts`、随包 Ember 的安装与定位逻辑。
- Q2 对应的命令与 Ember 侧的改动：`ember/tools/schema-gen` 的输出位置、`debug-sdk:check` 与 `help:build` 里生成接口文档的脚本、`ember/tools/dev/win32-dev-support.mjs` 的客户端引用。
- 核对并写入 `engines.vscode`（M11）。

完成标准：真实 Ember 用例通过（NES 调试载入、60 帧截图、CDL 记录到反汇编的闭环、SB2000 开机运行）；未设置模拟器位置时的报错用例通过；调试客户端的用例通过。

### 阶段 5：外部引用更新

范围：第 7 节清单中除 `packages/` 以外的全部位置。

完成标准：按第 2.4 节的正则统计，除 `packages/` 外的命中清零；允许保留的只有：扩展 `CHANGELOG.md` 与 Ember 更新记录里已发布版本的条目、代码里的常量名与标记（`FCDEV_CONFIGURATION_NAME`、`FCDEV_ERROR`、`HELP_FCDEV_DIRECTORY`、`FCDEV_GENERATORS`、`-DFCDEV_GENERATED`）、测试专用的环境变量（`FCDEV_TEST_EMULATOR`、`FCDEV_TEST_FIRMWARE`、`FCDEV_TEST_NES_ROM`、`FCDEV_SKIP_GUI_TESTS`）、调试用户数据目录名 `fcdev-ember`、缺陷记录 B001 的历史说明；本文自身是迁移记录，写旧命令行的地方是历史说明，不计入统计。

### 阶段 6：删除与收尾

范围：

- 删除 `packages/`、根 `package.json` 与 `workspaces`、根 `test/`（已迁完）、`tools/build-ember.mjs`、`tools/release.mjs`、`tools/release-check.mjs`、`tools/verify-install.mjs`、`tools/check-structure.mjs`（并入扩展的结构测试）。
- 清理 `.gitignore` 与根 `package-lock.json`。
- 生成 `.vsix`，运行 `check:package`。

完成标准：第 8 节全部通过。

## 7. 外部引用更新清单

| 位置 | 内容 |
| --- | --- |
| `apps/sb2000` 下 7 个有 `package.json` 的项目 | 删除 `package.json`、`package-lock.json`，加 `.vscode/tasks.json`，更新 README 与 `.gitignore` 里对 `fcdev sync` 的说明 |
| `apps/sb2000/examples` 12 个示例、`apps/sb2000/fbasic/exe` | 没有 `package.json`，补 `.vscode/tasks.json` 或并入上级项目的任务 |
| `apps/disasm-example` | 同上；`README.md` 的走法写成扩展命令 |
| `apps/sb2000/fbasic/tools` | 5 个脚本引用 `vscode/out/`，替换 `packages/fcdev/src/*` 的引用 |
| `machines/bbg/tools/probe-fc.mjs` 与两份机型文档 | 同上 |
| `ember/docs`（01、02、04、05、06、07、08、待办备忘） | 调试客户端、帮助站点导入、模拟器随包的描述 |
| `ember/tools/build`（`build-help.mjs`、`import-fcdev-docs.mjs`、`import-fcdev-docs.d.mts`） | 文档来源是 `vscode/docs/topics.json` 与 `vscode/docs/`；三个文档生成脚本的位置 |
| `ember/tools/schema-gen`、`ember/package.json`、`ember/tools/dev` | 调试 SDK 生成物的位置、`debug-sdk:check` 脚本、客户端引用 |
| `ember/help` 与 `ember/win32`、`ember/core` 里的说明性文字 | `fcdev run`、`fcdev debug` 等命令行写法替换为扩展的写法 |
| `docs`（01 至 09、待办备忘、缺陷记录） | 包与依赖、目录、发布、测试、反汇编与 AI 工具的设计文档按新结构重写；`05-发布` 重写为扩展的打包与发布；`04-测试与验证` 重写为 vitest 的分层与环境变量 |
| `vscode` 的 `README.md`、`chat/`、`test/`、`src/` | 说明、指令、skill、智能体的工具清单；删除对 `fcdev` 外部包的说明 |
| 仓库根 `README.md` | 使用说明 |
| `copilot-plugins` | 四个智能体的工具清单、`README.md` 按扩展的命令与工具重写；`skills/fcdev-app-development`（`SKILL.md` 与 6 个参考文件）重写后迁入扩展的 `vscode/chat/skills/`，不再放在 `copilot-plugins` |
| `docs` 仓库 | `.github/instructions/ember.instructions.md` 里关于 `packages/fcdev`、`fcdev-ember`、公共调试函数路径的说明 |

## 8. 完成判据

1. 第 2.4 节的统计：famiclone 除允许项外清零，`copilot-plugins` 与 `docs` 清零。
2. 仓库里没有 `packages/` 目录与根 `package.json`。
3. 在 `vscode/` 运行 `npm run check` 通过；`check:package` 通过。
4. 第 2.2 节的 47 个测试文件全部迁入并通过；真实 Ember 与真实 cc65 的用例在环境具备时通过。
5. 全部样例项目与四个模板用真实 cc65 构建成功。
6. 以下手动核对项由用户在真实 VS Code 里确认：Copilot 发现并调用 36 个工具、任务的问题匹配与跳转、新建项目、在设置的 Ember 里运行 NES 与 SB2000 程序。

## 9. 进度

| 阶段 | 状态 | 完成日期 |
| --- | --- | --- |
| 阶段 0：骨架 | 已完成 | 2026-11-22 |
| 阶段 1：构建核心 | 已完成 | 2026-11-22 |
| 阶段 2：项目入口、新建项目与文档查询 | 已完成（任务报错跳转待用户手动核对） | 2026-11-22 |
| 阶段 3：反汇编核心 | 已完成 | 2026-11-22 |
| 阶段 4：模拟器与调试 | 已完成 | 2026-11-22 |
| 阶段 5：外部引用更新 | 已完成 | 2026-11-22 |
| 阶段 6：删除与收尾 | 已完成 | 2026-11-22 |

计划状态：已完成；第 8 节第 6 项的手动核对待用户确认。

阶段 2 的测试位置：根 `test/` 里测试生成脚本与工具链的 6 个文件移到 `tools/test/`，由 `npm run test:tools` 运行（已并入 `npm run check`）；`gen:docs` 重新生成四份 SB2000 生成文档。

阶段 3 的补充：反汇编测试移成 `test/shared|logic/disassembly`（需要 cc65 的用例没有工具链时跳过）；命令“fcdev：反汇编映像文件”（`fcdev.disassemble`）选映像与输出目录，平铺二进制缺加载地址或入口时再询问。

阶段 4 的补充：

- 设置 `fcdev.emulator.path`（Ember 位置）与 `fcdev.emulator.debugPort`（默认 5699），作用域都是 `machine`；没有设置位置时运行类操作报错并说明设置项，命令“运行”“调试运行”还给“打开设置”按钮。
- 新增命令“fcdev：运行”“fcdev：调试运行”（任务 `command: run`，`debug` 为真是调试运行）、工具 `fcdev_run`；调试客户端做成 5 个命令：调用调试请求、查看调试请求说明、保存调试截图、监视调试事件、显示调试服务地址，结果写到“fcdev 调试”输出通道。
- 调试协议的接口文档与类型由 Ember 的 `npm run schema` 写到 `vscode/debug/openrpc.json` 与 `vscode/src/contracts/debug-client/generated/api.ts`；`ember` 的 `vscode:check` 取代 `debug-sdk:check`；帮助文档的协议参考由 `vscode/tools/gen-debug-api-docs.mjs` 生成；开发脚本的调试请求经 `ember/tools/dev/debug-call.mjs` 引用 `vscode/out`。
- 真实 Ember 的用例在 `test/real-emulator`，用环境变量 `FCDEV_TEST_EMULATOR` 指向 Ember 的可执行文件（固件用 `FCDEV_TEST_FIRMWARE`，没有时用 `ember/roms/sb2000_bios.bin`），缺 Ember、cc65 或固件时跳过，`FCDEV_SKIP_GUI_TESTS=1` 也跳过。

阶段 5 的补充：

- 仓库里的 Node 脚本（`apps/sb2000/fbasic/tools`、`machines/bbg/tools/probe-fc.mjs`、`tools`）经 `tools/lib/extension.mjs` 引用 `vscode/out/`；Ember 位置用环境变量 `EMBER_EXE`（只用于开发脚本）。`tools/build-cc65.mjs` 的 make 程序环境变量改名为 `MAKE_PROGRAM`。
- `apps/sb2000` 下 7 个有 `package.json` 的项目删除了 `package.json` 与 `package-lock.json`，各加 `.vscode/tasks.json`（构建、构建磁盘镜像、运行、调试运行）；示例与 `fbasic/exe` 没有另加，扩展会为每个含 `fcdev.json` 的项目自动提供任务。
- Ember 的帮助站点导入脚本改读 `vscode/docs/topics.json`（字段 `root: 'extension'` 指文件相对扩展目录，参数 `extensionRoot`）；帮助文档里的调试客户端、`docs/07-调试客户端工具` 改写为扩展的命令与内部客户端。
- 开发文档 01 至 09、待办备忘、缺陷记录与扩展的使用文档、README 按新结构重写；`05-发布` 是扩展的打包与发布，`04-测试与验证` 是 vitest 的分层与环境变量。
- `copilot-plugins` 的 `fcdev-app-development` skill 改写为扩展的命令与工具（`references/cli-and-build.md` 改名 `build-and-config.md`），并迁入 `vscode/chat/skills/`，经 `chatSkills` 声明、随扩展发布，使用扩展的人不需要再装 `copilot-plugins`；四个智能体的工具清单加入 6 个 fcdev 工具；`docs` 仓库的 `ember.instructions.md` 同步。

阶段 6 的补充：

- 已删除 `packages/`、根 `package.json`、根 `package-lock.json`、根 `test/`（结构检查已并入扩展的结构测试）、`tools/build-ember.mjs`、`tools/release.mjs`、`tools/release-check.mjs`、`tools/verify-install.mjs`、`tools/check-structure.mjs` 与基线记录脚本 `record-baseline.mjs`、基线文件及其测试；`.gitignore` 已清理。
- 新增 `tools/check-package.mjs`（`npm run check:package`）与它的测试：核对 `.vsix` 里的编译产物、工具链（文件与清单的 SHA-256 一致）、目标与运行库、模板、使用文档与调试接口描述，且不含测试、源码、密钥、Ember、BIOS 与 ROM。
- 扩展版本升到 0.3.0，`CHANGELOG.md` 新增 0.3.0 一节；`.vscodeignore` 加入 `tools/**`。
- 核对结果：在 `vscode/` 运行 `npm run check` 通过（66 个测试文件、653 个用例通过，20 个真实 Ember 的用例因没有设置 `FCDEV_TEST_EMULATOR` 跳过）；另用删除前复制出来的 Ember 单独运行 `test/real-emulator`，2 个文件、20 个用例通过；`npm run package` 生成 `fcdev-win32-x64-0.3.0.vsix`（756 个文件，约 2.9 MB），`npm run check:package` 通过；样例项目与四个模板的构建用例通过；统计正则除允许项外清零。
- 没有运行 Ember 自己的测试与冒烟，Ember 里改动的帮助站点导入脚本只运行了一次导入（12 页）。
