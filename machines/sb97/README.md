# SB97

小霸王 97 学习机（SB97）的 BIOS 反汇编工程。本目录是 famiclone 仓库里的一个机型目录（`machines/sb97/`），分析的对象是 [reference/rom/小霸王/SB97.nes](../../reference/rom/小霸王/SB97.nes)。

## 目录

| 目录 | 内容 | 状态 |
| --- | --- | --- |
| [disassembly/](disassembly/) | 静态分析工具、单元测试和分析结论 | 进行中（阶段 A） |

## 环境

- Windows，Node.js 22 及以上（脚本无第三方依赖）。
- 工具引用 `machines/sb2000/disassembly/tools/disasm.mjs` 与 `machines/sb2000/bios/tools/opcodes.mjs` 的 6502 解码，不改动它们。

## 快速开始

```powershell
cd machines/sb97/disassembly     # 在仓库根目录运行
npm test                         # 单元测试（含用真实 BIOS 核对的结论）
node tools/sb97-scan.mjs pages   # 每页的判别结果与向量
node tools/sb97-scan.mjs io      # 对 $2000-$7FFF 的访问统计
node tools/sb97-scan.mjs bank    # 写 $5000 前装入的页号立即数
node tools/sb97-scan.mjs listing 0 C0C2 FFE0   # 生成第 0 页的清单，后面是额外入口
```

分析结论、已知与待核对的事项见 [disassembly/docs/README.md](disassembly/docs/README.md)。
