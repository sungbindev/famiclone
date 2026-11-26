# Apple DOS 3.3 命令

中华学习机 CEC-I 兼容苹果 II，软驱可以启动 Apple DOS 3.3 系统盘。本文记录 DOS 3.3 的命令格式与作用，以及启动时屏幕上的提示。

## 启动画面

从 DOS 3.3 System Master 盘启动时，屏幕依次显示：

| 屏幕文字 | 含义 |
| --- | --- |
| `DOS VERSION 3.3` | DOS 版本 3.3 |
| `08/25/80` | DOS 3.3 系统盘的日期 |
| `APPLE II PLUS OR ROMCARD SYSTEM MASTER` | 系统母盘，适用于 Apple II Plus 或带 ROMCARD 的机器 |
| `(LOADING INTEGER INTO LANGUAGE CARD)` | 正在把 INTEGER BASIC 装入语言卡 |
| `]` | 命令提示符；`]` 是 Applesoft 的提示符，INTEGER BASIC 的提示符是 `>` |

启动画面的文字与提示符是按苹果 DOS 3.3 的常规行为写的，没有在 Ember 的 `cec1` 机型里逐字核对。

## 命令格式

```text
命令 文件名 [,选项]
```

- 文件名最长 30 个字符，可以包含空格。
- 文件名前加 `D` 与驱动器号、加 `S` 与槽号、加 `V` 与卷号，都可以在命令后用逗号指定，如 `CATALOG,S6,D1`。
- 命令与文件名之间的空格可有可无，命令用大写输入。

## 命令一览

### 磁盘与目录

| 命令 | 格式 | 作用 |
| --- | --- | --- |
| `INIT` | `INIT 程序名[,S槽,D驱动器,V卷号]` | 格式化磁盘，并把程序装入后写为 DOS 盘；会清除盘上的全部内容 |
| `CATALOG` | `CATALOG[,S槽,D驱动器]` | 列出磁盘目录，每个文件一行，前面的字母是文件类型 |
| `VERIFY` | `VERIFY 文件名` | 检查文件能否读完，不装入内存 |
| `MAXFILES` | `MAXFILES 文件数` | 设置同时打开的文件数上限（1 到 16），默认 3 |
| `LOCK` | `LOCK 文件名` | 锁定文件，不能删除或改写 |
| `UNLOCK` | `UNLOCK 文件名` | 解除锁定 |
| `RENAME` | `RENAME 旧名,新名` | 改名 |
| `DELETE` | `DELETE 文件名` | 删除文件 |

### 程序的保存与装入

| 命令 | 格式 | 作用 |
| --- | --- | --- |
| `SAVE` | `SAVE 文件名` | 把内存里的 BASIC 程序存成文件 |
| `LOAD` | `LOAD 文件名` | 把 BASIC 程序装入内存，覆盖原有程序 |
| `RUN` | `RUN 文件名` | 装入 BASIC 程序并运行；不带文件名时运行内存里的程序 |
| `BSAVE` | `BSAVE 文件名,Aaddr,Llen` | 把内存区间存成二进制文件，`addr` 为起始地址，`len` 为长度 |
| `BLOAD` | `BLOAD 文件名[,Aaddr]` | 装入二进制文件，不给地址时装到文件里记录的地址 |
| `BRUN` | `BRUN 文件名[,Aaddr]` | 装入二进制文件并跳到装入地址执行 |
| `EXEC` | `EXEC 文件名` | 把文本文件里的内容当作键盘输入逐行执行 |

### 文本文件（随机存取）

| 命令 | 格式 | 作用 |
| --- | --- | --- |
| `OPEN` | `OPEN 文件名[,Lrec]` | 打开或创建文本文件，`rec` 为记录长度 |
| `APPEND` | `APPEND 文件名` | 打开文本文件并定位到末尾，之后的输出追加到文件 |
| `POSITION` | `POSITION 文件名,Rrec` | 把文件指针移到指定记录 |
| `READ` | `READ 文件名[,Rrec][,Bbyte]` | 之后的输入从文件读取 |
| `WRITE` | `WRITE 文件名[,Rrec][,Bbyte]` | 之后的输出写入文件 |
| `CLOSE` | `CLOSE [文件名]` | 关闭文件，不带文件名时关闭全部文件 |

文本文件的操作要写在 BASIC 程序里，直接输入时不能用 `POSITION`、`READ`、`WRITE`。

### 输入输出

| 命令 | 格式 | 作用 |
| --- | --- | --- |
| `PR#` | `PR# n` | 把输出送到槽号 `n` 的接口；`PR#0` 回到屏幕 |
| `IN#` | `IN# n` | 把输入取自槽号 `n` 的接口；`IN#0` 回到键盘 |
| `MON` | `MON[,C][,I][,O]` | 打开命令、输入、输出的回显（`C` 命令、`I` 输入、`O` 输出） |
| `NOMON` | `NOMON[,C][,I][,O]` | 关闭对应的回显 |

### 其他

| 命令 | 格式 | 作用 |
| --- | --- | --- |
| `FP` | `FP` | 切换到浮点 BASIC（Applesoft） |
| `INT` | `INT` | 切换到整数 BASIC（INTEGER BASIC），提示符变为 `>` |

## 文件类型

`CATALOG` 列出的第一个字母表示文件类型：

| 字母 | 类型 |
| --- | --- |
| `T` | 文本文件 |
| `I` | 整数 BASIC 程序 |
| `A` | Applesoft BASIC 程序 |
| `B` | 二进制文件 |
| `*` | 文件已锁定 |

## 验证

- 本文的命令表依据苹果 DOS 3.3 的通用资料写定，没有在 Ember 的 `cec1` 机型里逐条运行核对。
- 需要核对时，先复制一份 `DOS3.3` 镜像再载入软驱，然后逐条输入命令观察输出。
