# {{name}}

用 fcdev 开发的任天堂红白机（NES）程序，C 语言。

## 构建与运行

```bash
npm run build    # 生成 build/nes/hello.nes
npm start        # 在模拟器里运行（只支持 Windows）
```

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/hello.c` | 示例程序：在屏幕上显示 HELLO, NES! |
| `fcdev.json` | fcdev 配置：目标 nes，程序名 hello |

## 文档

开发文档随 fcdev 安装在 `node_modules/fcdev/docs/`。运行 `npx fcdev docs` 列出全部主题，`npx fcdev docs nes` 查 NES 目标的文档位置。
