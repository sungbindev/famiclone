# famiclone 仓库说明

面向 FC 兼容机和小霸王、步步高、裕兴等 6502 学习机的综合开发仓库，提供基于 cc65 的 C 与汇编工具链、`fcdev` 命令行和项目模板、VS Code 扩展、Ember 模拟器与调试工具，以及 SB2000、步步高等机型的 BIOS、系统程序、示例程序和硬件资料。

## 目录说明

| 目录 | 内容 |
| --- | --- |
| `packages/fcdev` | 命令行 `fcdev`、各机型目标与调试 SDK（TypeScript 源码在 `debug-sdk/`，构建到 `dist/debug/`；终端客户端是 `fcdev debug`）（npm 包） |
| `packages/create-fcdev` | `npm create fcdev` 的创建包与项目模板（npm 包） |
| `packages/fcdev-cc65` | cc65 工具链包，由 `tools/build-cc65.mjs` 生成，不入库（npm 包） |
| `packages/fcdev-ember` | 随包的 Ember 模拟器（Windows 64 位），由 `tools/build-ember.mjs` 从 Ember 的发布产物组装，只入库 `package.json`（npm 包） |
| `ember` | 模拟器 Ember：Win32 原生外壳加 C++ 核心，独立的 npm 项目，不发布到 npm |
| `vscode` | VS Code 扩展 fcdev（独立的 npm 项目，生成 `.vsix`） |
| `cc65` | cc65 源码，在本仓库维护，zlib 许可，许可文件在 `cc65/LICENSE` |
| `apps/sb2000` | 用 fcdev 开发的 SB-DOS 程序：`asm`、`edit`、`runnes`、`memtest`、`fcprobe`、`hwprobe`、`hwtest`、`fbasic`，另有 `disk`（软盘镜像脚本）与 `examples`（示例）；每个程序是独立的 npm 项目 |
| `machines/sb2000` | 小霸王学习机 SB2000：`bios`（BIOS 源码工程）、`docs`（研究文档） |
| `machines/bbg` | 步步高学习机的 BIOS 与 DOS 原厂源码、汇编器与分析工具 |
| `machines/sb97` | SB97 的 BIOS 静态分析 |
| `reference` | 真机参考资料：ROM、软盘镜像、磁带、游戏；`machines` 与测试会用到，不随 npm 包发布 |
| `tools` | 构建、检查与发布脚本 |
| `test` | 仓库级测试（工具链、SB2000 接口生成物、软盘构建、结构检查） |
| `docs` | 开发文档，清单见 [01-软件架构设计.md](docs/01-软件架构设计.md) 第 9 节 |
| `build` | 构建产物（cc65、Ember），不入库 |

`apps/bbg` 目前是空目录。仓库根的 npm 工作区只包含 `packages/*`；`ember`、`vscode`、`apps`、`machines` 各自安装依赖。

## 构建命令

### 仓库根（packages）

```bash
npm install
node tools/build-cc65.mjs   # 用 cc65/ 构建工具链包 packages/fcdev-cc65（需要 mingw32-make 与 gcc）
node tools/build-ember.mjs  # 用 Ember 的发布产物组装模拟器包 packages/fcdev-ember（没有发布产物时加 --package，先运行 Ember 的 npm run package:win）
npm run build:libs          # 构建各目标的运行库
npm test                    # 全部测试（会先运行 build:libs）
npm run check               # 依赖边界检查加测试
npm run release:check       # 发布前检查（只检查）
npm run release:verify      # 打包后在临时目录里创建、安装并构建四个模板
```

### 调试 SDK（packages/fcdev/debug-sdk）

```bash
npm run debug-sdk:check -w packages/fcdev   # 生成接口类型、类型检查、oxlint、构建、测试
npm run debug-sdk:build -w packages/fcdev   # 只构建 dist/debug
```

### ember（在 `ember/` 目录下）

```powershell
.\build-win32.ps1            # Win32 原生外壳：安装依赖、编译 Release 版、边界与导入表检查，输出 build\ember\win32\app\Release\ember-win.exe
npm run win32:start          # 以开发方式启动原生外壳
npm run win32:test           # 原生外壳的 C++ 测试（核心、控件库、应用）
npm run package:win          # 发布打包：生成 build\ember\release\Ember-<版本>.exe（升级用）与 Ember-<版本>-学习机开发模拟器.zip（发给用户）
npm run core:test            # C++ 测试；-- -R <名称> 只跑匹配的用例
npm run check                # 全部检查与测试
.\debug-win32.ps1             # 启动 Win32 原生外壳的开发版
```

发布包输出到 `build/ember/release/`。

### vscode（在 `vscode/` 目录下）

```bash
npm install
npm run check      # 类型检查、oxlint、测试、构建
npm run package    # 生成 .vsix
```

### apps/sb2000（每个程序在自己的目录下）

```bash
npm install
npm run build      # fcdev build src，产物在 build/
npm run disk       # 放进磁盘镜像
```

软盘镜像：先在 `edit`、`asm`、`runnes` 下运行 `npm run build`，再运行 `node apps/sb2000/disk/build-disk.mjs`。

### machines

```bash
# machines/sb2000/bios
npm run build          # 构建 BIOS
npm run build:fast     # 构建 fastboot 变体
npm test

# machines/bbg、machines/sb97
npm test
```

## npm 包发布

四个包用同一个版本号，发布顺序固定：`fcdev-cc65`、`fcdev-ember`、`fcdev`、`create-fcdev`。命令都在仓库根目录运行，说明见 [docs/05-发布.md](docs/05-发布.md)。

1. 在 `packages/fcdev/CHANGELOG.md` 写好新版本一节，标题为 `## <版本号>（日期）`。
2. 升版本号（改三个包的 `version`、`fcdev` 的精确依赖，重新构建工具链包，刷新锁文件），然后自己提交：

   ```bash
   npm run release:prepare -- <版本号>
   ```

3. 演练：跑全部测试、SDK 检查、`release:check`、`release:verify` 与各包的 `npm publish --dry-run`，不登录、不发布：

   ```bash
   npm run release:publish -- --dry-run
   ```

4. 登录并正式发布（工作区必须已提交，输入 `yes` 确认，验证码在终端里输入）：

   ```bash
   npm login
   npm run release:publish
   ```

不用脚本时，按顺序手动发布：

```bash
npm publish -w packages/fcdev-cc65
npm publish -w packages/fcdev-ember
npm publish -w packages/fcdev
npm publish -w packages/create-fcdev
```

## 模拟器构建与上传

升级文件放在阿里云 OSS 的 `famiclone` Bucket 下的 `updates/ember/windows/`，地址写在 `ember/win32/app/src/bootstrap/application.cpp` 的 `kUpdateFeedUrl`。命令在 `ember/` 目录下运行，上传需要先安装 `ossutil` 并运行 `ossutil config`（密钥只放本机配置）。升级清单要用签名私钥（文档仓库 `01-文档/密钥/ember-update-signing-key.pem`，与本仓库并列，不入本仓库）签名，第一次发布前运行 `npm run update:keygen` 生成。

1. 把 `ember/package.json` 的 `version` 升到新版本，在 `ember/help/manual/changelog.md` 写好该版本一节。
2. 生成发布包（单个 `Ember.exe`，帮助文档、预装插件与许可全文都嵌在里面）：

   ```powershell
   npm run package:win
   ```

3. 核对程序里的公钥与私钥一致，生成并校验签名清单 `latest.json`，打印要上传的文件，不上传：

   ```powershell
   npm run update:publish -- --dry-run
   ```

4. 上传，顺序是 `Ember-<版本号>.exe`、`latest.json`：

   ```powershell
   npm run update:publish
   ```

发给用户的是 `build/ember/release/Ember-<版本号>-学习机开发模拟器.zip`（里面是 `Ember.exe`、`LICENSE.txt`、`THIRD_PARTY_NOTICES.txt`），解压即用，不需要安装。

## 许可证

仓库与 `fcdev` 自己的代码为 Apache-2.0，见 [LICENSE](LICENSE)；`ember` 为 GPL-3.0-only；`cc65/` 为 zlib 许可。
