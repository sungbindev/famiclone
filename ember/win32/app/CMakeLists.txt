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

# “关于”对话框里的编译时间：取配置工程的时刻（本机时间），每次重新配置会更新；
# 只加在用到它的 application.cpp 上，加在整个目标上会让每次配置后全部文件重新编译。
string(TIMESTAMP EMBER_BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP EMBER_BUILD_CLOCK "%H:%M:%S")

add_library(ember_app STATIC
  src/bootstrap/application.cpp
  src/bootstrap/dialog-host.cpp
  src/bootstrap/generated/third-party-notices.generated.cpp
  src/bootstrap/update-apply.cpp
  src/bootstrap/user-data-move.cpp
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
  src/infra/dialogs/message-box-confirmation.cpp
  src/infra/dialogs/remembering-file-dialog.cpp
  src/infra/directories/explorer-directory.cpp
  src/infra/display/screen-host-adapter.cpp
  src/infra/dropped-files/memory-dropped-files-service.cpp
  src/infra/help/local-help-viewer.cpp
  src/infra/last-directories/file-last-directories-service.cpp
  src/infra/links/shell-external-links.cpp
  src/infra/plugins/bundled-plugins.cpp
  src/infra/resources/zip-archive.cpp
  src/infra/recent-cartridges/file-recent-cartridges-service.cpp
  src/infra/shell-state/file-shell-state-service.cpp
  src/infra/updates/engine-app-updater.cpp
  src/infra/updates/signed-exe-update-engine.cpp
  src/infra/updates/winhttp-update-transport.cpp
  src/infra/input/system-gamepad-reader.cpp
  src/infra/user-data/user-data-location-adapter.cpp
  src/logic/debug/debug-controller.cpp
  src/logic/debug/debug-module.cpp
  src/logic/debug/debug-rules.cpp
  src/logic/input/gamepad-settings-controller.cpp
  src/logic/input/input-controller.cpp
  src/logic/input/input-module.cpp
  src/logic/input/pad-rules.cpp
  src/logic/machines/rom-controller.cpp
  src/logic/machines/machines-module.cpp
  src/logic/machines/machines-rules.cpp
  src/logic/mappers/mappers-controller.cpp
  src/logic/mappers/mappers-module.cpp
  src/logic/mappers/mappers-rules.cpp
  src/logic/media/media-controller.cpp
  src/logic/media/media-module.cpp
  src/logic/media/media-rules.cpp
  src/logic/preferences/preferences-controller.cpp
  src/logic/preferences/preferences-module.cpp
  src/logic/preferences/preferences-rules.cpp
  src/logic/preferences/user-data-controller.cpp
  src/logic/runtime/runtime-controller.cpp
  src/logic/runtime/runtime-module.cpp
  src/logic/runtime/runtime-rules.cpp
  src/logic/shell/shell-controller.cpp
  src/logic/shell/shell-module.cpp
  src/logic/shell/shell-registry-builder.cpp
  src/logic/system/about-controller.cpp
  src/logic/system/auto-update-check.cpp
  src/logic/system/release-notes-prompt.cpp
  src/logic/system/system-module.cpp
  src/logic/system/update-controller.cpp
  src/platform/dialogs/message-box.cpp
  src/platform/dialogs/system-dialogs.cpp
  src/platform/display/screen-shaders.generated.cpp
  src/platform/display/screen-window.cpp
  src/platform/crypto/signature-verifier.cpp
  src/platform/files/directory-access.cpp
  src/platform/files/json-file-reader.cpp
  src/platform/files/json-file-store.cpp
  src/platform/files/known-folders.cpp
  src/platform/files/user-data-location.cpp
  src/platform/input/gamepads.cpp
  src/platform/input/joystick-gamepads.cpp
  src/platform/input/raw-input.cpp
  src/platform/input/xinput-gamepads.cpp
  src/platform/logging/file-error-logger.cpp
  src/platform/network/https-client.cpp
  src/platform/network/local-http-server.cpp
  src/platform/resources/embedded-archive.cpp
  src/platform/process/environment.cpp
  src/platform/process/process-control.cpp
  src/platform/process/single-instance.cpp
  src/platform/shell/directory-opener.cpp
  src/platform/shell/url-opener.cpp
  src/platform/text/wide-text.cpp
  src/platform/theme/system-theme.cpp
  src/platform/threading/ui-thread.cpp
  src/platform/window/app-icon.cpp
  src/platform/window/dialog-window.cpp
  src/platform/window/frameless-frame.cpp
  src/platform/window/resize-border.cpp
  src/platform/window/window-modes.cpp
  src/platform/window/window-placement.cpp
  src/platform/window/window-shadow.cpp
  src/shared/input/scan-code-table.cpp
  src/shared/help/help-request.cpp
  src/shared/last-directories/last-directories.cpp
  src/shared/launch/launch-args.cpp
  src/shared/preferences/allow-multiple-instances.cpp
  src/shared/recent-cartridges/recent-cartridges.cpp
  src/shared/screen/pointer-mapping.cpp
  src/shared/screen/screen-layout.cpp
  src/shared/shell-state/shell-state.cpp
  src/shared/shortcut/shortcut.cpp
  src/shared/update/update-manifest.cpp
  src/shared/window/window-shadow-image.cpp
  src/ui/media/media-panel-content.cpp
  src/ui/input/gamepad-settings-content.cpp
  src/ui/input/pad-layout.cpp
  src/ui/machines/machines-content.cpp
  src/ui/mappers/mappers-content.cpp
  src/ui/preferences/preferences-content.cpp
  src/ui/runtime/runtime-panel-content.cpp
  src/ui/runtime/screen-overlay-view.cpp
  src/ui/runtime/screen-widget.cpp
  src/ui/shell/app-logo.cpp
  src/ui/shell/main-window-view.cpp
  src/ui/shell/shell-view.cpp
  src/ui/system/about-content.cpp)
# 升级只在发布配置里开启（npm run package:win 传 -DEMBER_ENABLE_UPDATES=ON），开发与测试的构建不升级，不会覆盖构建目录里的程序。
option(EMBER_ENABLE_UPDATES "开启应用升级（发布用）" OFF)
if(EMBER_ENABLE_UPDATES)
  target_compile_definitions(ember_app PRIVATE EMBER_APP_UPDATES=1)
endif()
# 应用自己的头文件目录放在前面：核心里也有 platform、shared 等同名目录，应用的文件不能与它们重名。
target_include_directories(ember_app PUBLIC src)
target_compile_definitions(ember_app PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX PRIVATE EMBER_APP_VERSION="${EMBER_CORE_VERSION}")
# 单独的宏定义与预编译头的编译参数不一致，这个文件不使用预编译头。
set_source_files_properties(src/bootstrap/application.cpp PROPERTIES
  COMPILE_DEFINITIONS "EMBER_APP_BUILD_DATE=\"${EMBER_BUILD_DATE}\";EMBER_APP_BUILD_CLOCK=\"${EMBER_BUILD_CLOCK}\""
  SKIP_PRECOMPILE_HEADERS ON)
target_compile_options(ember_app PUBLIC /utf-8 /W4 /permissive- /EHsc /MP)
target_precompile_headers(ember_app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch/ember-app-pch.hpp)
target_link_libraries(ember_app PUBLIC ember_core rujian_ui d3d11 dxgi ole32 shell32 uuid advapi32 user32 gdi32 shlwapi ws2_32 winhttp bcrypt winmm)

# 可执行文件：资源（图标、清单、版本信息）里的版本号取自 package.json
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" EMBER_VERSION_MATCH "${EMBER_CORE_VERSION}")
set(EMBER_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(EMBER_VERSION_MINOR "${CMAKE_MATCH_2}")
set(EMBER_VERSION_PATCH "${CMAKE_MATCH_3}")
set(EMBER_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/icon.ico")
# 随程序分发的资源包（预装插件、帮助文档、许可全文，由 tools/build/pack-resources.mjs 打成 zip）嵌进 ember-win.exe；为空或不存在时不嵌。
# 资源包的 SHA-256 写进生成的 .rc 里，内容变了 .rc 才会被重新编译。
set(EMBER_RESOURCE_ARCHIVE "" CACHE FILEPATH "嵌进 ember-win.exe 的资源包")
set(EMBER_RESOURCE_ARCHIVE_LINE "")
set(EMBER_RESOURCE_ARCHIVE_HASH "none")
if(EMBER_RESOURCE_ARCHIVE AND EXISTS "${EMBER_RESOURCE_ARCHIVE}")
  file(TO_CMAKE_PATH "${EMBER_RESOURCE_ARCHIVE}" EMBER_RESOURCE_ARCHIVE_PATH)
  file(SHA256 "${EMBER_RESOURCE_ARCHIVE_PATH}" EMBER_RESOURCE_ARCHIVE_HASH)
  set(EMBER_RESOURCE_ARCHIVE_LINE "EMBER_RESOURCES RCDATA \"${EMBER_RESOURCE_ARCHIVE_PATH}\"")
endif()
configure_file(resources/ember-win.rc.in ${CMAKE_CURRENT_BINARY_DIR}/ember-win.rc @ONLY)
add_executable(ember_win WIN32 src/bootstrap/main.cpp ${CMAKE_CURRENT_BINARY_DIR}/ember-win.rc resources/ember-win.manifest)
set_target_properties(ember_win PROPERTIES OUTPUT_NAME "ember-win")
target_link_libraries(ember_win PRIVATE ember_app)

if(EMBER_BUILD_TESTS)
  add_executable(ember_app_tests
    test/about-content-test.cpp
    test/debug-logic-test.cpp
    test/dialog-test.cpp
    test/display-test.cpp
    test/gamepad-settings-content-test.cpp
    test/gamepad-settings-logic-test.cpp
    test/help-test.cpp
    test/infra-test.cpp
    test/launch-args-test.cpp
    test/machines-content-test.cpp
    test/machines-logic-test.cpp
    test/main-request-test.cpp
    test/mappers-content-test.cpp
    test/mappers-logic-test.cpp
    test/media-logic-test.cpp
    test/media-panel-test.cpp
    test/platform-test.cpp
    test/preferences-logic-test.cpp
    test/preferences-content-test.cpp
    test/resources-test.cpp
    test/runtime-logic-test.cpp
    test/screen-overlay-test.cpp
    test/screen-test.cpp
    test/shared-rules-test.cpp
    test/shell-logic-test.cpp
    test/shell-test.cpp
    test/system-logic-test.cpp
    test/update-test.cpp)
  target_include_directories(ember_app_tests PRIVATE test)
  target_link_libraries(ember_app_tests PRIVATE ember_app GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(ember_app_tests DISCOVERY_MODE PRE_TEST)
endif()
