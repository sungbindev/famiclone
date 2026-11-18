# ------------------------------------------------------------------------
# 名称：CMakeLists.txt
# 说明：平台探针 ember-probe.exe：在目标系统上检查原生外壳依赖的系统能力（图形、手柄、网络、声音、核心）。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-19
# 备注：只依赖 Windows 系统与核心静态库；用于确认刚装好的 Windows 7 SP1 能运行，随后保留为每次改动工具链或依赖后的回归工具。
# ------------------------------------------------------------------------

add_executable(ember_probe
  src/main.cpp
  src/probe-support.cpp
  src/check-system.cpp
  src/check-direct2d.cpp
  src/check-direct3d.cpp
  src/check-xinput.cpp
  src/check-winhttp.cpp
  src/check-audio.cpp
  src/check-core.cpp
  ember-probe.manifest)
set_target_properties(ember_probe PROPERTIES OUTPUT_NAME "ember-probe")
target_compile_definitions(ember_probe PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(ember_probe PRIVATE /utf-8 /W4 /permissive- /EHsc)
target_link_libraries(ember_probe PRIVATE ember_core d2d1 dwrite d3d11 dxgi winhttp user32)
