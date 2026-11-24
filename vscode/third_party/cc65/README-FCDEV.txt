本目录是 cc65 的删减版，不是上游的完整源码；导入后由本仓库维护，可以直接修改。

- 上游：https://github.com/cc65/cc65
- 提交：357f64e4e3c733d54b82bc155afc841fd8d9ee8d（2025-08-24）
- 许可证：zlib，见 LICENSE
- 只保留 FC Dev 构建 cc65、ca65、ld65、ar65、da65，以及 none、nes 两个目标的运行库所需的文件；
  其余机型的库、文档、示例与测试都已删除。
