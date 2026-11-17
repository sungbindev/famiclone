# {{name}}

用 fcdev 开发的小霸王学习机 SB2000（SB-DOS）程序，C 语言。

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

源文件用 UTF-8 保存，字符串里可以直接写汉字：构建时 fcdev 自动把它转成学习机用的 GB2312。汉字要写在 `.c` 文件里（头文件不转换），不在 GB2312 里的字符会报错。

## 文件

| 文件 | 说明 |
| --- | --- |
| `src/hello.c` | 示例程序：显示一行英文和一行中文，等待按键后返回 DOS |
| `fcdev.json` | fcdev 配置：目标 sb2000，程序名 hello |

## 查阅接口

运行库把 DOS 的系统调用封装成了 C 函数（文字、键盘手柄鼠标、文件目录、软驱、显示、声音等），头文件是 `sb2000.h`。随时查询：

```bash
npx fcdev api               # 列出全部模块
npx fcdev api file          # 看模块里的函数
npx fcdev api sb_file_open  # 看一个函数的完整文档
npx fcdev api '$7B'         # 看用到某个系统调用的函数
```

## 程序超过 16K

在 `fcdev.json` 里加 `"targetOptions": { "banks": 库页个数 }`，把部分函数用 `SB_BANK_BEGIN(n)` 与 `SB_BANK_END()` 放进库页，声明处用 `SB_BANK_CALLS_BEGIN(n)` 与 `SB_BANK_CALLS_END()` 包起来，调用时自动换页。函数的用法用 `npx fcdev api bank` 查，写法与限制见 `node_modules/fcdev/targets/sb2000/include/sb2000-bank.h` 文件头的说明。

## 文档

使用文档随 fcdev 安装在 `node_modules/fcdev/docs/`。运行 `npx fcdev docs` 列出全部主题，`npx fcdev docs sb2000` 查开发指南，`npx fcdev docs syscall` 查系统调用手册。