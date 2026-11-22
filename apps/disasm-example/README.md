# 反汇编示例

一个很小的 NES 程序（`src/hello.s`，在屏幕上显示 HELLO, NES!），用来试一遍“6502 工程师”智能体的反汇编流程：把构建出的 `.nes` 反汇编成可重新汇编、逐字节一致的 ca65 工程，再与原映像比较。程序自己写、自己构建，没有版权问题。

## 准备

在 VS Code 里打开本仓库，运行任务“fcdev：构建”（或命令“fcdev：构建项目”，路径选 `apps/disasm-example/src`），生成 `build/nes/hello.nes`。

需要 VS Code 的 Copilot 聊天与已安装的 fcdev 扩展（自带 cc65 工具链）。想看运行效果还要 Windows 上的 Ember，并在设置 `fcdev.emulator.path` 里指向它，没有也能做完前面的步骤。

## 走一遍

1. 打开本仓库，在 Copilot 聊天的 Agent 下拉里选“6502 工程师”（由本扩展提供）。
2. 发送：

   ```text
   反汇编 apps/disasm-example/build/nes/hello.nes，工程放在 apps/disasm-example/disasm
   ```

3. 它会按八步做，每步用 `disasm_status` 对照完成标准。这个例子里可以核对：
   - 识别：`target` 为 `nes`，布局是一个 `$8000` 起的 32K 段，三个向量（复位、NMI、IRQ）都已登记为入口，符号集是 `fcnes.inc`。
   - 追踪：程序自己的代码加上 cc65 启动与 neslib 库代码，一共十几个例程，没有未解析的间接跳转。有几处没被调用到的库代码（`$8107` 等）一开始留在 `unknown` 里，要加为入口再追踪。
   - 区域分类：代码后面是 32 字节调色板（`disasm_detect` 把它连同后面的所有字节一起当作图形数据候选，长度要核实）、字符串 `HELLO, NES!`、一小段带 `$FFFF` 占位的 RAM 代码模板、`$00` 填充和向量表，分别标成 `bytes`、`string`、`bytes`（注明未确认）、`padding`、`words`。
   - 命名与注释：寄存器访问自动用 `fcnes.inc` 的名字（`PPUMASK`、`PPUADDR`、`PPUDATA`……），自动命名的符号（约六十个，含局部标号）全部换成有含义的名字，每个例程写块注释。
   - 分层：整个程序一个模块（例如 `main`）也可以，再试着把数据表归到单独的模块。
   - 生成与回读：`disasm_generate`、`disasm_verify` 得到“一致”。
4. 结果在 `apps/disasm-example/disasm/`：`disasm.json`（工程）、`src/*.s`、`include/`、`link.cfg`。

## 用模拟器补全（可选）

让它在 Ember 里运行记录一次，再导入工程：

```text
用模拟器载入 hello.nes，记录 CDL 并运行 120 帧，把记录导入工程后重新追踪
```

这个程序没有间接跳转，运行记录补不出多少新代码；换成有表驱动分发的程序，这一步才会补出新的入口。

## 自动测试

扩展里的 `vscode/test/logic/disassembly/disasm-workflow.test.ts` 用同一份源码按这些步骤走一遍（构建、打开、追踪、区域分类、命名、注释、分层、生成、回读），并检查每步的完成标准，需要 cc65。
