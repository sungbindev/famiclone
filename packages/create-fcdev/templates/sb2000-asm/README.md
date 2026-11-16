# {{name}}

用 fcdev 开发的小霸王学习机 SB2000（SB-DOS）程序，ca65 汇编。

## 构建

```bash
npm run build    # 生成 build/sb2000/HELLO.EXE
npm run disk     # 生成放有 HELLO.EXE 的软盘镜像 build/sb2000/HELLO.img
```

程序名必须符合 DOS 8.3 规则（主名不超过 8 个字符），在 `fcdev.json` 的 `name` 里修改。

## 在模拟器里运行（只支持 Windows）

fcdev 不提供 BIOS。先在 `fcdev.json` 里用 `firmware` 指定 SB2000 的 BIOS 文件：

```json
{ "target": "sb2000", "name": "hello", "firmware": "D:/rom/SB2K_BIOS.NES" }
```

```bash
npm start
```

DOS 启动后按两次回车跳过日期与时间提示，在提示符下输入 `hello`，显示后按任意键返回 DOS。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/hello.s` | 示例程序：显示一行英文和一行中文，等待按键后返回 DOS，入口是导出的 `_main` |
| `fcdev.json` | fcdev 配置：目标 sb2000，程序名 hello |

## 文档

使用文档随 fcdev 安装在 `node_modules/fcdev/docs/`。运行 `npx fcdev docs` 列出全部主题，`npx fcdev docs sb2000` 查开发指南，`npx fcdev docs syscall` 查系统调用手册。
