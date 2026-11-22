---
layout: home
title: 首页

hero:
  name: Ember
  text: 帮助文档
  tagline: 面向 NES 和学习机应用开发者，提供运行控制、内存与状态检查、断点、单步、跟踪和自动化调试能力，并附带 fcdev 工具链、硬件资料与开发手册。
  actions:
    - theme: alt
      text: 使用说明
      link: /manual/
    - theme: alt
      text: 插件开发
      link: /plugin/
    - theme: alt
      text: 调试接口
      link: /debug/
    - theme: brand
      text: 开始开发
      link: /fcdev/
    - theme: alt
      text: GitHub 仓库
      link: https://github.com/sungbindev/famiclone

features:
  - title: 使用说明
    details: 打开 NES 卡带，添加学习机 ROM，载入软盘与磁带，设置键盘与手柄，调整画面和声音，并解决日常使用问题。
    link: /manual/
  - title: 插件开发手册
    details: 为 NES 卡带编写 mapper 插件，用调试服务验证存储区映射、寄存器、状态与脚本行为，快速完成测试和排错。
    link: /plugin/
  - title: 调试接口手册
    details: 通过 WebSocket JSON-RPC 控制运行，读写内存与寄存器，设置断点和监视项，记录指令轨迹、生成截图，并用 fcdev 扩展的调试命令或 Copilot 工具自动化调试。
    link: /debug/
  - title: fcdev 开发手册
    details: 用 C 或汇编开发 NES 与 SB2000 程序，完成构建、打包、运行和调试；手册包含命令、系统调用、硬件行为、接口、符号与示例源码。
    link: /fcdev/
---
