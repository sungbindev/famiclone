# create-fcdev

创建 fcdev 项目：在空目录生成 6502 兼容机程序的开发结构与示例程序。

## 使用

```bash
npm create fcdev@latest my-game
npm create fcdev@latest . -- --target sb2000 --template c
```

| 选项 | 说明 |
| --- | --- |
| `[目录]` | 默认当前目录，必须为空（允许已有 `.git`）；不存在时会创建 |
| `--target <名>` | 目标：`nes` 或 `sb2000`；缺省时询问 |
| `--template <语言>` | `c` 或 `asm`；缺省时询问 |
| `--no-install` | 不运行 `npm install` 与 `npm run sync` |

不能交互（例如在脚本里）时必须同时给出 `--target` 与 `--template`。

生成的项目包含 `src/hello.c`（或 `hello.s`）、`fcdev.json`、`package.json`、`README.md` 和 `.gitignore`，依赖与本包同版本的 [fcdev](https://www.npmjs.com/package/fcdev)。之后用 `npm run build` 构建，`npm start` 在模拟器里运行。

模板内置在本包里，不会从网络下载；已有文件不会被覆盖。

## 许可证

Apache-2.0
