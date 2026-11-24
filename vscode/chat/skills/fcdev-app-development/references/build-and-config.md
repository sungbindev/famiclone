# 配置与构建

新建项目、写配置、构建与排查构建问题时使用。内容取自 FC Dev 的使用文档《构建》《新建项目与模板》。

## 新建项目

工具 `fcdev_new_project`（参数 `directory`、`target`、`language`，调用前会请用户确认），或命令“FCDEV：新建项目”（选目标与语言、选空目录、询问在当前窗口还是新窗口打开）。

- 目录必须为空（允许已有 `.git`），不会覆盖已有文件；目录不存在时工具会创建。
- 目标与语言必须由用户确定，不要猜。
- 生成的文件：`fcdev.json`、`.gitignore`、`README.md`、`.vscode/tasks.json`（构建、运行任务）、`src/hello.c` 或 `src/hello.s`；带磁盘能力的目标（`sb2000`、`sb97`、`yuxing`）还有 `disk/README.TXT`（构建磁盘镜像时放进镜像的使用说明）。程序名固定为 `hello`，要改名同时改 `fcdev.json` 的 `name`；`bbg` 模板没有 `disk` 目录，运行前要在 `fcdev.json` 里加 `firmware` 与 `diskImage`。
- 七个模板：`nes-c`、`nes-asm`、`sb2000-c`、`sb2000-asm`、`sb97-asm`、`yuxing-asm`、`bbg-asm`（SB97、裕兴与步步高没有 C 运行库，只有汇编）。

## 任务、命令与工具

任务类型是 `fcdev`，在 `.vscode/tasks.json` 里这样写：

```json
{ "label": "FCDEV：构建", "type": "fcdev", "command": "build", "path": "src", "problemMatcher": "$fcdev", "group": { "kind": "build", "isDefault": true } }
```

| 字段 | 说明 |
| --- | --- |
| `command` | `build`（带磁盘能力的目标配置了 `diskName` 时同时生成磁盘镜像）、`run`（构建后在 Ember 里运行） |
| `path` | 构建路径，相对工作区文件夹，缺省为文件夹本身 |
| `target` | 覆盖 `fcdev.json` 的 `target` |
| `debug` | `run` 以调试模式启动 |

扩展也为每个含 `fcdev.json` 的项目自动提供构建、运行两个任务。任务输出就是工具的原始输出，错误出现在“问题”面板并可跳转到源码行。智能体用工具 `fcdev_build`、`fcdev_run`，参数含义相同。

## 构建单位

| 构建路径 | 含义 |
| --- | --- |
| 工作区文件夹本身（`path` 缺省） | 以这个文件夹为项目 |
| 目录，如 `src` | 目录下递归收集的全部 `.c`、`.s`、`.asm` 链接成一个程序，程序名取 `fcdev.json` 的 `name`，没有时取目录名 |
| 文件，如 `src/a.c` | 只编译链接这一个文件，程序名取文件名 |

- 收集目录时跳过 `build`、`node_modules`、以 `.` 开头的目录和输出目录；目录里没有源文件时报错。
- 目标取自构建路径向上查找到的最近一个 `fcdev.json`，`target` 可覆盖；两者都没有时报错，不默认猜。
- 目录里有多个独立程序时不能一次构建目录，只能逐个指定文件。

## `fcdev.json`

| 项 | 默认值 | 说明 |
| --- | --- | --- |
| `target` | 必填 | `nes`、`sb2000`、`sb97`、`yuxing`、`bbg` 或 `rom` |
| `name` | 目录名 | 目录构建时的程序名（SB2000 必须是 8.3 名字） |
| `out` | `build` | 输出目录，相对项目根 |
| `optimize` | `true` | 给 `cc65` 加 `-Oirs` |
| `diskName` | 无 | 构建时生成的磁盘镜像文件名（只对 `sb2000`、`sb97`、`yuxing`、`bbg` 有效，缺 `.img` 时自动补上，不能含路径分隔符）：写了才生成，不写不生成；镜像里有程序与项目 `disk` 目录下的全部文件，文件名须是 8.3 名字，不能有子目录；`bbg` 的镜像建在 `diskImage` 的系统盘上，没有 `diskImage` 时不生成 |
| `define` | 空 | 预定义宏，值为字符串或数字，覆盖目标自带的同名宏 |
| `include` | 空 | 额外头文件目录，相对项目根 |
| `firmware` | 无 | 固件（BIOS）文件；SB2000、SB97、裕兴、步步高运行时必填；`rom` 目标把它当底固件，构建时把链接输出覆盖进去 |
| `diskImage` | 无 | 磁盘镜像：`rom` 目标运行时放进软驱 0；`bbg` 把它当系统盘，构建磁盘镜像时在其上添加文件（系统盘不改，要有足够的剩余空间） |
| `linkConfig` | 无 | 自定义 `ld65` 链接配置，相对项目根；替换目标自带的链接配置，原样使用，不检查产物大小上限；不能与大于 0 的 `banks` 同用；以目标自带的 `link.cfg` 为底稿改，启动代码与库仍按目标链接 |
| `targetOptions` | 空 | 目标专用：NES 的 `font`（布尔，默认 `true`），SB2000 的 `banks`（0 到 113，默认 0），`rom` 的 `machine`（机型，运行时必填）、`offset`（链接输出在整块固件里的偏移）、`size`（整块固件必须正好等于的大小） |

- 未知项、类型不对、`define` 值不是字符串或数字时报错并列出可用项；`targetOptions` 里目标没定义的选项也报错。`toolchain` 与 `emulator` 不是配置项：工具链随扩展发布，Ember 的位置在扩展设置里指定。
- 路径类的相对路径都以 `fcdev.json` 所在目录为基准。

SB2000 示例：

```json
{
  "target": "sb2000",
  "name": "hello",
  "firmware": "D:/roms/SB2K_BIOS.NES"
}
```

## 构建流程与产物

- 产物：`<项目根>/<out>/<目标>/<程序名><扩展名>`；中间文件：`<out>/obj/<目标>/<程序名>/`，含 `.o`、`.d`、`.s`、链接产物 `.bin`、`.map`、`.dbg`，多页程序另有 `<程序名>.cfg`，以及指纹 `stamp.json`。
- 流程：`cc65`（.c → .s）→ `ca65`（.s → .o）→ `ld65`（按目标链接配置链接启动代码、用户目标文件、目标库）→ 打包器（`.bin` → `.nes` 或 `.EXE`）。
- 增量：源文件与依赖没变就不重编；扩展版本、工具链版本、目标定义、目标选项、`optimize`、`define`、`include`、源文件列表任一变化，会清掉中间目录全部重编。
- `.map` 用来看各段大小，`.dbg` 用来查符号地址（调试时设断点用，见 [run-and-verify.md](run-and-verify.md) 的“定位符号地址”）。
- 工具输出原样透传，不改写。

## 工具链与 Ember 的位置

- 工具链在扩展目录的 `toolchain/` 里，随扩展升级；缺少 `cc65`、`ca65`、`ld65` 任一个时报错。不使用 `PATH` 里的 cc65，也不接受其他位置的工具链。
- Ember：用户设置 `fcdev.emulator.path`（只能在用户设置里指定，工作区设置无效）；没有设置时“运行”报错，构建不受影响。调试端口是设置 `fcdev.emulator.debugPort`，默认 5699。扩展不提供 Ember。
- 项目信息（目标、工具链目录、头文件目录、汇编引入目录）用工具 `fcdev_info` 或命令“FCDEV：显示项目信息”查看。

## 编辑器配置

扩展自动生成 `.vscode/c_cpp_properties.json`（只写或更新名为 `fcdev` 的配置）与根目录的 `compile_flags.txt`（第一行 `-DFCDEV_GENERATED`，不是这个标记的同名文件视为用户自己的，不覆盖）。两者含本机路径，已被 `.gitignore` 忽略。`__fastcall__`、`__cdecl__` 在编辑器配置里是空宏，不参与构建。打开项目、`fcdev.json` 变化时自动同步，也可以运行命令“FCDEV：同步 C/C++ 智能感知配置”。

## 排查构建问题

- 先看第一条错误；`ld65` 的“内存区放不下”按目标限制处理（NES 16K PRG，SB2000 常驻 15K，见各目标文档）。
- 缺少或重复 `main`：目录里混入了别的程序的源文件，或汇编没有 `.export _main`。
- 怀疑增量缓存不一致：删掉 `build/` 重建。
- 错误信息中的“hint/建议”部分就是扩展给出的解决办法，照着做，不要绕过（例如不要把 PATH 里的 cc65 拷进来）。
