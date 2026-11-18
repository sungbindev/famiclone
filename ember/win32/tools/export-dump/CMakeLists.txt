# ------------------------------------------------------------------------
# 名称：CMakeLists.txt
# 说明：导出表转储工具 ember-export-dump.exe：在 Windows 7 SP1 上导出系统 DLL 的函数名，作为兼容检查的基线。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-19
# 备注：只依赖 Windows 系统，静态链接运行库，可以直接拷到干净的 Windows 7 上运行。
# ------------------------------------------------------------------------

add_executable(ember_export_dump src/main.cpp)
set_target_properties(ember_export_dump PROPERTIES OUTPUT_NAME "ember-export-dump")
target_compile_definitions(ember_export_dump PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(ember_export_dump PRIVATE /utf-8 /W4 /permissive- /EHsc)
