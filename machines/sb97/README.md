# SB97

小霸王 97 学习机（SB97）的 BIOS 静态分析工程。分析的对象是 [reference/rom/小霸王/SB97.nes](../../reference/rom/小霸王/SB97.nes)，按这台机器的内存映射找出代码、统计对整机寄存器的访问、生成各页的汇编清单。

## 环境

- Windows，Node.js 22 及以上（脚本无第三方依赖）。
- 6502 指令解码借用 `machines/sb2000/disassembly/tools/disasm.mjs`，只引用，不改动。

## 使用

在本目录运行：

```powershell
npm test                      # 单元测试
npm run info                  # 文件头、程序区大小与校验和
npm run pages                 # 每页的用法与代码字节数
npm run io                    # 对 $2000-$5FFF 的访问统计与写入的立即数
npm run banks                 # 写 $5000 的立即数
npm run listing               # 重新生成各页清单到 listing/
```

## 限制

- 只做静态分析：从向量、功能表、跳转桩与内嵌跳转表出发顺着控制流找代码，找不到入口的页（多半是字库、图形与压缩数据，或经运行时才填的功能表进入的代码）按数据输出。
- 清单只用来阅读，不保证能重新汇编出同一份 ROM。
- 分析结论、已查明与待核对的事项见 [docs/README.md](docs/README.md)。

## 目录结构

| 目录 | 内容 |
| --- | --- |
| `tools/` | 文件读取与校验、地址视图、控制流分析、各页分类、清单生成、命令行 |
| `test/` | 单元测试，含用真实 BIOS 核对的结论 |
| `docs/` | 分析结论 |
| `listing/` | 各页的汇编清单（`page-00.asm` 到 `page-63.asm`），已提交，分析规则或工具变了要重新生成 |
