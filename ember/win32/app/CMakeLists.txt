# ------------------------------------------------------------------------
# 名称：CMakeLists.txt
# 说明：Ember 原生应用 ember-win.exe：应用库（八层的全部源码，除程序入口外）、可执行文件与单元测试。
# 作者：sungbin
# 邮箱：sungbin@3578.cn
# 日期：2026-11-19
# 备注：应用库链接核心静态库 ember_core 与控件库 rujian_ui；程序入口 src/bootstrap/main.cpp 单独放在可执行文件里，测试链接应用库；
#       src/contracts/core-gateway/generated 里的请求目录由 tools/schema-gen 生成（不提交），配置前由 win32-cmake.mjs 先生成。
# ------------------------------------------------------------------------

# 与核心库、GoogleTest 保持一致：Debug 版关闭标准库的迭代器检查，否则链接时报不匹配。
add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)

add_library(ember_app STATIC
  src/bootstrap/application.cpp
  src/bootstrap/resource-locations.cpp
  src/entry/cartridges/cartridge-handlers.cpp
  src/entry/machines/machine-handlers.cpp
  src/entry/main-requests/dropped-path.cpp
  src/entry/main-requests/main-request-router.cpp
  src/entry/main-requests/param-reader.cpp
  src/entry/mappers/mapper-handlers.cpp
  src/entry/media/media-handlers.cpp
  src/entry/preferences/preference-handlers.cpp
  src/entry/preferences/theme-sync.cpp
  src/entry/system/system-handlers.cpp
  src/infra/core-client/in-process-core-gateway.cpp
  src/infra/core-client/latest-frame-slot.cpp
  src/infra/dialogs/remembering-file-dialog.cpp
  src/infra/directories/explorer-directory.cpp
  src/infra/display/screen-host-adapter.cpp
  src/infra/dropped-files/memory-dropped-files-service.cpp
  src/infra/last-directories/file-last-directories-service.cpp
  src/infra/recent-cartridges/file-recent-cartridges-service.cpp
  src/infra/shell-state/file-shell-state-service.cpp
  src/logic/debug/debug-controller.cpp
  src/logic/debug/debug-module.cpp
  src/logic/debug/debug-rules.cpp
  src/logic/input/input-controller.cpp
  src/logic/media/media-controller.cpp
  src/logic/media/media-module.cpp
  src/logic/media/media-rules.cpp
  src/logic/runtime/runtime-controller.cpp
  src/logic/runtime/runtime-module.cpp
  src/logic/runtime/runtime-rules.cpp
  src/logic/shell/shell-controller.cpp
  src/logic/shell/shell-module.cpp
  src/logic/shell/shell-registry-builder.cpp
  src/platform/dialogs/message-box.cpp
  src/platform/dialogs/system-dialogs.cpp
  src/platform/display/screen-shaders.generated.cpp
  src/platform/display/screen-window.cpp
  src/platform/files/json-file-reader.cpp
  src/platform/files/json-file-store.cpp
  src/platform/files/known-folders.cpp
  src/platform/input/raw-input.cpp
  src/platform/input/xinput-gamepads.cpp
  src/platform/logging/file-error-logger.cpp
  src/platform/process/environment.cpp
  src/platform/process/single-instance.cpp
  src/platform/shell/directory-opener.cpp
  src/platform/text/wide-text.cpp
  src/platform/theme/system-theme.cpp
  src/platform/threading/ui-thread.cpp
  src/platform/window/frameless-frame.cpp
  src/platform/window/window-modes.cpp
  src/platform/window/window-placement.cpp
  src/shared/input/scan-code-table.cpp
  src/shared/last-directories/last-directories.cpp
  src/shared/launch/launch-args.cpp
  src/shared/preferences/allow-multiple-instances.cpp
  src/shared/recent-cartridges/recent-cartridges.cpp
  src/shared/screen/pointer-mapping.cpp
  src/shared/screen/screen-layout.cpp
  src/shared/shell-state/shell-state.cpp
  src/shared/shortcut/shortcut.cpp
  src/ui/media/media-panel-content.cpp
  src/ui/runtime/runtime-panel-content.cpp
  src/ui/runtime/screen-overlay-view.cpp
  src/ui/runtime/screen-widget.cpp
  src/ui/shell/main-window-view.cpp
  src/ui/shell/shell-view.cpp)
# 应用自己的头文件目录放在前面：核心里也有 platform、shared 等同名目录，应用的文件不能与它们重名。
target_include_directories(ember_app PUBLIC src)
target_compile_definitions(ember_app PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX PRIVATE EMBER_APP_VERSION="${EMBER_CORE_VERSION}")
target_compile_options(ember_app PUBLIC /utf-8 /W4 /permissive- /EHsc)
target_link_libraries(ember_app PUBLIC ember_core rujian_ui d3d11 dxgi ole32 shell32 uuid advapi32 user32 shlwapi)

# 可执行文件：资源（图标、清单、版本信息）里的版本号取自 package.json
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" EMBER_VERSION_MATCH "${EMBER_CORE_VERSION}")
set(EMBER_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(EMBER_VERSION_MINOR "${CMAKE_MATCH_2}")
set(EMBER_VERSION_PATCH "${CMAKE_MATCH_3}")
set(EMBER_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/icon.ico")
configure_file(resources/ember-win.rc.in ${CMAKE_CURRENT_BINARY_DIR}/ember-win.rc @ONLY)
add_executable(ember_win WIN32 src/bootstrap/main.cpp ${CMAKE_CURRENT_BINARY_DIR}/ember-win.rc resources/ember-win.manifest)
set_target_properties(ember_win PROPERTIES OUTPUT_NAME "ember-win")
target_link_libraries(ember_win PRIVATE ember_app)

if(EMBER_BUILD_TESTS)
  add_executable(ember_app_tests
    test/debug-logic-test.cpp
    test/display-test.cpp
    test/infra-test.cpp
    test/launch-args-test.cpp
    test/main-request-test.cpp
    test/media-logic-test.cpp
    test/media-panel-test.cpp
    test/platform-test.cpp
    test/runtime-logic-test.cpp
    test/screen-test.cpp
    test/shared-rules-test.cpp
    test/shell-logic-test.cpp
    test/shell-test.cpp)
  target_include_directories(ember_app_tests PRIVATE test)
  target_link_libraries(ember_app_tests PRIVATE ember_app GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(ember_app_tests DISCOVERY_MODE PRE_TEST)
endif()
