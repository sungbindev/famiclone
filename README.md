# famiclone 仓库说明

面向 FC 兼容机和小霸王、步步高、裕兴等 6502 学习机的综合开发仓库，提供基于 cc65 的 C 与汇编开发环境（VS Code 扩展 FC Dev：构建、运行、调试、新建项目、反汇编、智能感知与 Copilot 工具）、Ember 模拟器与调试工具，以及 SB2000、步步高等机型的 BIOS、系统程序、示例程序和硬件资料。

## 目录说明

| 目录 | 内容 |
| --- | --- |
| `vscode` | VS Code 扩展 FC Dev（唯一的开发环境发行形式，独立的 npm 项目，生成 `.vsix`）：任务与命令、各机型目标与运行库、cc65 工具链、项目模板、使用文档、反汇编核心、调试客户端、Copilot 工具 |
| `ember` | 模拟器 Ember：Win32 原生外壳加 C++ 核心，独立的 npm 项目，不发布到 npm，也不随扩展发布；使用者在扩展设置 `fcdev.emulator.path` 里指向它 |
| `vscode/third_party/cc65` | cc65 源码，在本仓库维护，zlib 许可，许可文件在 `vscode/third_party/cc65/LICENSE` |
| `apps/sb2000` | 用 FC Dev 开发的 SB-DOS 程序：`asm`、`edit`、`game`、`memtest`、`hwprobe`、`hwtest`、`family-basic-v3`，另有 `disk`（软盘镜像脚本）与 `examples`（示例）；每个程序是一个 FC Dev 项目，用 VS Code 任务构建 |
| `apps/sb97`、`apps/yuxing`、`apps/bbg` | 各机型 DOS 下的 `game`（运行 NES 游戏的加载程序，汇编）：SB97 与步步高运行 mapper 0 的游戏，裕兴运行 mapper 0 与 mapper 4 的游戏；每个是一个 FC Dev 项目 |
| `machines/sb2000` | 小霸王学习机 SB2000：`bios`（BIOS 源码工程）、`docs`（研究文档） |
| `machines/bbg` | 步步高学习机的 BIOS 与 DOS 原厂源码、汇编器与分析工具 |
| `machines/sb97` | SB97 的 BIOS 反汇编工程与分析结论 |
| `reference` | 真机参考资料：ROM、软盘镜像、磁带、游戏；`machines` 与测试会用到，不随扩展发布；按机型的资料索引见 [reference/README.md](reference/README.md) |
| `tools` | 各部分脚本共用的入口 `tools/extension.mjs`（引用扩展的编译产物）；扩展自己的维护脚本（工具链、运行库、SB2000 生成文档、打包检查、发布）在 `vscode/tools/` |
| `docs` | 开发文档，清单见 [01-软件架构设计.md](docs/01-软件架构设计.md) 第 9 节 |
| `build` | 构建产物（cc65、Ember），不入库 |

仓库根没有 `package.json`：`vscode`、`ember`、`machines` 各自安装依赖；`apps` 下的 FC Dev 项目不需要 Node。

## 构建命令

### vscode（在 `vscode/` 目录下）

```bash
npm install
npm run build:toolchain   # 用 ../cc65 构建工具链到 toolchain/（需要 mingw32-make 与 gcc）
npm run build             # 编译扩展（tools 里的脚本引用 out/）
npm run build:libs        # 构建各目标的运行库
npm run gen:docs          # 重新生成 SB2000 的符号头文件、接口参考、符号参考与示例源码
npm run check             # 类型检查、oxlint、测试、构建、维护脚本的测试
npm run package           # 生成 .vsix
npm run check:package     # 核对 .vsix 的内容
```

按 F5 可以在扩展开发宿主里调试。真实 Ember 的测试用环境变量 `FCDEV_TEST_EMULATOR` 指向 `Ember.exe`，没有设置时跳过，见 [docs/04-测试与验证.md](docs/04-测试与验证.md)。

### ember（在 `ember/` 目录下）

```powershell
.\build-win32.ps1            # Win32 原生外壳：安装依赖、编译 Release 版、边界与导入表检查，输出 build\ember\win32\app\Release\ember-win32.exe
npm run win32:start          # 以开发方式启动原生外壳
npm run win32:test           # 原生外壳的 C++ 测试（核心、控件库、应用）
npm run package:win          # 发布打包：生成 build\ember\release\Ember-<版本>.exe（升级用）与 Ember-<版本>-学习机开发模拟器.zip（发给用户），以及下载用的 Ember-<版本>.zip
npm run core:test            # C++ 测试；-- -R <名称> 只跑匹配的用例
npm run check                # 全部检查与测试（含 vscode:check）
.\debug-win32.ps1             # 启动 Win32 原生外壳的开发版
```

发布包输出到 `build/ember/release/`。

### apps/sb2000（FC Dev 项目）

在 VS Code 里打开仓库，用“终端 → 运行任务”选 `fcdev` 的构建、运行任务（每个项目的 `.vscode/tasks.json` 或扩展自动提供的任务），构建会同时生成软盘镜像；运行需要先在用户设置 `fcdev.emulator.path` 里指向 Ember。

### machines

```bash
# machines/sb2000（在 VS Code 中运行 FC Dev 构建，产物 build/rom/sb2000.bin）

# machines/bbg
# 汇编链接见 machines/bbg/README.md
```

## 扩展发布

扩展 `sungbin.fcdev` 只发布 Windows 64 位版，打包与检查见 [docs/05-发布.md](docs/05-发布.md)。登录市场与 `vsce publish` 由用户执行。

1. 在 `vscode/CHANGELOG.md` 顶部新增新版本一节，把 `vscode/package.json` 的 `version` 升到新版本。
2. 在 `vscode/` 里依次运行 `npm run build:toolchain`、`npm run build:libs`、`npm run check`、`npm run package`、`npm run check:package`。
3. 用 `vsce publish --target win32-x64` 发布，或在市场管理页面上传 `.vsix`。

## 模拟器构建与上传

升级文件放在仓库的 `updates/ember/windows/`，提交并推送后由 GitHub Pages 经自有域名 `fcdev.cn` 对外，地址写在 `ember/win32/app/src/bootstrap/application.cpp` 的 `kUpdateFeedUrl`。命令在 `ember/` 目录下运行。升级清单要用签名私钥（文档仓库 `01-文档/密钥/ember-update-signing-key.pem`，与本仓库并列，不入本仓库）签名，第一次发布前运行 `npm run update:keygen` 生成。

1. 把 `ember/package.json` 的 `version` 升到新版本，在 `ember/help/manual/changelog.md` 写好该版本一节。
2. 生成发布包（单个 `Ember.exe`，帮助文档、预装插件与许可全文都嵌在里面）：

   ```powershell
   npm run package:win
   ```

3. 核对程序里的公钥与私钥一致，生成并校验签名清单 `latest.json`，打印要上传的文件，不上传：

   ```powershell
   npm run update:publish -- --dry-run
   ```

4. 上传（复制到 `updates/ember/windows/`），顺序是 `Ember-<版本号>.exe`、`Ember-<版本号>.zip`、`latest.json`：

   ```powershell
   npm run update:publish
   ```

5. 发给用户的 `Ember-<版本号>-学习机开发模拟器.zip` 不随上传复制，留在 `build/ember/release/`，由你手动发出。
6. 在 `site/` 目录运行 `npm run build`，更新站点的首页与帮助页；提交并推送仓库根目录的 `index.html` 与 `site/`。

发给用户的是 `build/ember/release/Ember-<版本号>-学习机开发模拟器.zip`（里面是 `Ember.exe`、`LICENSE.txt`、`THIRD_PARTY_NOTICES.txt`，以及 `roms/` 目录下的 ROM 文件），解压即用，不需要安装。

## 许可证

仓库与扩展自己的代码为 Apache-2.0，见 [LICENSE](LICENSE)；`ember` 为 GPL-3.0-only；`vscode/third_party/cc65/` 为 zlib 许可。
