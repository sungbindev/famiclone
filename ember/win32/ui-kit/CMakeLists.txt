# ------------------------------------------------------------------------
# 名称：CMakeLists.txt
# 说明：Win32 控件库 rujian_ui：静态库、预览程序（gallery）与单元测试。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-19
# 备注：只依赖 Windows 系统（Direct2D 1.0、DirectWrite 1.0、WIC），不依赖核心；
#       src/generated 里的文件由 tools 下的生成器生成并提交，npm run win32:check 检查它们与生成结果一致。
# ------------------------------------------------------------------------

# 与核心库、GoogleTest 保持一致：Debug 版关闭标准库的迭代器检查，否则链接时报不匹配。
add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)

add_library(rujian_ui STATIC
  src/base/clipboard.cpp
  src/base/color.cpp
  src/base/d2d-canvas.cpp
  src/base/graphics.cpp
  src/base/icon-rules.cpp
  src/base/offscreen.cpp
  src/base/root.cpp
  src/base/svg-path.cpp
  src/base/text-engine.cpp
  src/base/unicode.cpp
  src/base/widget.cpp
  src/base/window.cpp
  src/components/button/button.cpp
  src/components/choice/checkbox.cpp
  src/components/choice/radio-group.cpp
  src/components/layout/flex-box.cpp
  src/components/select/select.cpp
  src/components/slider/slider.cpp
  src/components/text-input/line-editor.cpp
  src/components/text-input/text-input.cpp
  src/generated/icon-data.generated.cpp
  src/generated/icon-rules.generated.cpp)
target_include_directories(rujian_ui PUBLIC include PRIVATE src)
target_compile_definitions(rujian_ui PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(rujian_ui PRIVATE /utf-8 /W4 /permissive- /EHsc)
target_link_libraries(rujian_ui PUBLIC d2d1 dwrite windowscodecs ole32 user32 gdi32 imm32)

add_executable(rujian_ui_gallery gallery/main.cpp gallery/pages.cpp)
set_target_properties(rujian_ui_gallery PROPERTIES OUTPUT_NAME "rujian-ui-gallery")
target_include_directories(rujian_ui_gallery PRIVATE src)
target_compile_definitions(rujian_ui_gallery PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(rujian_ui_gallery PRIVATE /utf-8 /W4 /permissive- /EHsc)
target_link_libraries(rujian_ui_gallery PRIVATE rujian_ui)

if(EMBER_BUILD_TESTS)
  add_executable(rujian_ui_tests
    test/button-test.cpp
    test/checkbox-test.cpp
    test/color-test.cpp
    test/layout-test.cpp
    test/line-editor-test.cpp
    test/offscreen-test.cpp
    test/radio-group-test.cpp
    test/root-test.cpp
    test/select-test.cpp
    test/slider-test.cpp
    test/svg-path-test.cpp
    test/text-input-test.cpp
    test/text-test.cpp)
  target_include_directories(rujian_ui_tests PRIVATE src)
  target_compile_definitions(rujian_ui_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_compile_options(rujian_ui_tests PRIVATE /utf-8 /W4 /permissive- /EHsc)
  target_link_libraries(rujian_ui_tests PRIVATE rujian_ui GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(rujian_ui_tests DISCOVERY_MODE PRE_TEST)
endif()
