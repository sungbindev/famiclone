/*
 * ------------------------------------------------------------------------
 * 名称：icon-data.generated.cpp
 * 说明：图标的路径数据：每个图标是若干条 SVG 路径，按图标编号的顺序排列。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：由 tools/gen-icons.mjs 生成，不要手改；图标图形的来源与许可见 third_party。
 * ------------------------------------------------------------------------
 */

#include "base/icon-data.hpp"

namespace rujian::ui {

namespace {

constexpr IconPath AdjustmentsPaths[] = {
    {"M4 10a2 2 0 1 0 4 0a2 2 0 0 0 -4 0", "", "", 1},
    {"M6 4v4", "", "", 1},
    {"M6 12v8", "", "", 1},
    {"M10 16a2 2 0 1 0 4 0a2 2 0 0 0 -4 0", "", "", 1},
    {"M12 4v10", "", "", 1},
    {"M12 18v2", "", "", 1},
    {"M16 7a2 2 0 1 0 4 0a2 2 0 0 0 -4 0", "", "", 1},
    {"M18 4v1", "", "", 1},
    {"M18 9v11", "", "", 1},
};

constexpr IconPath AlertCirclePaths[] = {
    {"M3 12a9 9 0 1 0 18 0a9 9 0 0 0 -18 0", "", "", 1},
    {"M12 8v4", "", "", 1},
    {"M12 16h.01", "", "", 1},
};

constexpr IconPath AlertTrianglePaths[] = {
    {"M12 9v4", "", "", 1},
    {"M10.363 3.591l-8.106 13.534a1.914 1.914 0 0 0 1.636 2.871h16.214a1.914 1.914 0 0 0 1.636 -2.87l-8.106 -13.536a1.914 1.914 0 0 0 -3.274 0", "", "", 1},
    {"M12 16h.01", "", "", 1},
};

constexpr IconPath AppWindowPaths[] = {
    {"M3 7a2 2 0 0 1 2 -2h14a2 2 0 0 1 2 2v10a2 2 0 0 1 -2 2h-14a2 2 0 0 1 -2 -2v-10", "", "", 1},
    {"M6 8h.01", "", "", 1},
    {"M9 8h.01", "", "", 1},
};

constexpr IconPath ApplePaths[] = {
    {"M4 11.319c0 3.102 .444 5.319 2.222 7.978c1.351 1.797 3.156 2.247 5.08 .988c.426 -.268 .97 -.268 1.397 0c1.923 1.26 3.728 .809 5.079 -.988c1.778 -2.66 2.222 -4.876 2.222 -7.977c0 -2.661 -1.99 -5.32 -4.444 -5.32c-1.267 0 -2.41 .693 -3.22 1.44a.5 .5 0 0 1 -.672 0c-.809 -.746 -1.953 -1.44 -3.22 -1.44c-2.454 0 -4.444 2.66 -4.444 5.319", "", "", 1},
    {"M7 12c0 -1.47 .454 -2.34 1.5 -3", "", "", 1},
    {"M12 7c0 -1.2 .867 -4 3 -4", "", "", 1},
};

constexpr IconPath ArrowBackUpPaths[] = {
    {"M9 14l-4 -4l4 -4", "", "", 1},
    {"M5 10h11a4 4 0 1 1 0 8h-1", "", "", 1},
};

constexpr IconPath ArrowDownPaths[] = {
    {"M12 5l0 14", "", "", 1},
    {"M18 13l-6 6", "", "", 1},
    {"M6 13l6 6", "", "", 1},
};

constexpr IconPath ArrowLeftPaths[] = {
    {"M5 12l14 0", "", "", 1},
    {"M5 12l6 6", "", "", 1},
    {"M5 12l6 -6", "", "", 1},
};

constexpr IconPath ArrowRightPaths[] = {
    {"M5 12l14 0", "", "", 1},
    {"M13 18l6 -6", "", "", 1},
    {"M13 6l6 6", "", "", 1},
};

constexpr IconPath ArrowUpPaths[] = {
    {"M12 5l0 14", "", "", 1},
    {"M18 11l-6 -6", "", "", 1},
    {"M6 11l6 -6", "", "", 1},
};

constexpr IconPath ArrowsDiffPaths[] = {
    {"M11 16h10", "", "", 1},
    {"M11 16l4 4", "", "", 1},
    {"M11 16l4 -4", "", "", 1},
    {"M13 8h-10", "", "", 1},
    {"M13 8l-4 4", "", "", 1},
    {"M13 8l-4 -4", "", "", 1},
};

constexpr IconPath BellPaths[] = {
    {"M10 5a2 2 0 1 1 4 0a7 7 0 0 1 4 6v3a4 4 0 0 0 2 3h-16a4 4 0 0 0 2 -3v-3a7 7 0 0 1 4 -6", "", "", 1},
    {"M9 17v1a3 3 0 0 0 6 0v-1", "", "", 1},
};

constexpr IconPath BinaryPaths[] = {
    {"M11 10v-5h-1m8 14v-5h-1", "", "", 1},
    {"M15 5.5a.5 .5 0 0 1 .5 -.5h2a.5 .5 0 0 1 .5 .5v4a.5 .5 0 0 1 -.5 .5h-2a.5 .5 0 0 1 -.5 -.5l0 -4", "", "", 1},
    {"M10 14.5a.5 .5 0 0 1 .5 -.5h2a.5 .5 0 0 1 .5 .5v4a.5 .5 0 0 1 -.5 .5h-2a.5 .5 0 0 1 -.5 -.5l0 -4", "", "", 1},
    {"M6 10h.01m-.01 9h.01", "", "", 1},
};

constexpr IconPath CategoryPaths[] = {
    {"M4 4h6v6h-6l0 -6", "", "", 1},
    {"M14 4h6v6h-6l0 -6", "", "", 1},
    {"M4 14h6v6h-6l0 -6", "", "", 1},
    {"M14 17a3 3 0 1 0 6 0a3 3 0 1 0 -6 0", "", "", 1},
};

constexpr IconPath CheckPaths[] = {
    {"M5 12l5 5l10 -10", "", "", 1},
};

constexpr IconPath ChevronDownPaths[] = {
    {"M6 9l6 6l6 -6", "", "", 1},
};

constexpr IconPath ChevronRightPaths[] = {
    {"M9 6l6 6l-6 6", "", "", 1},
};

constexpr IconPath ChevronUpPaths[] = {
    {"M6 15l6 -6l6 6", "", "", 1},
};

constexpr IconPath CircleCheckPaths[] = {
    {"M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0", "", "", 1},
    {"M9 12l2 2l4 -4", "", "", 1},
};

constexpr IconPath CircleXPaths[] = {
    {"M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0", "", "", 1},
    {"M10 10l4 4m0 -4l-4 4", "", "", 1},
};

constexpr IconPath ClipboardPaths[] = {
    {"M9 5h-2a2 2 0 0 0 -2 2v12a2 2 0 0 0 2 2h10a2 2 0 0 0 2 -2v-12a2 2 0 0 0 -2 -2h-2", "", "", 1},
    {"M9 5a2 2 0 0 1 2 -2h2a2 2 0 0 1 2 2a2 2 0 0 1 -2 2h-2a2 2 0 0 1 -2 -2", "", "", 1},
};

constexpr IconPath ClockPaths[] = {
    {"M3 12a9 9 0 1 0 18 0a9 9 0 0 0 -18 0", "", "", 1},
    {"M12 7v5l3 3", "", "", 1},
};

constexpr IconPath CopyPaths[] = {
    {"M7 9.667a2.667 2.667 0 0 1 2.667 -2.667h8.666a2.667 2.667 0 0 1 2.667 2.667v8.666a2.667 2.667 0 0 1 -2.667 2.667h-8.666a2.667 2.667 0 0 1 -2.667 -2.667l0 -8.666", "", "", 1},
    {"M4.012 16.737a2.005 2.005 0 0 1 -1.012 -1.737v-10c0 -1.1 .9 -2 2 -2h10c.75 0 1.158 .385 1.5 1", "", "", 1},
};

constexpr IconPath CutPaths[] = {
    {"M4 17a3 3 0 1 0 6 0a3 3 0 1 0 -6 0", "", "", 1},
    {"M14 17a3 3 0 1 0 6 0a3 3 0 1 0 -6 0", "", "", 1},
    {"M9.15 14.85l8.85 -10.85", "", "", 1},
    {"M6 4l8.85 10.85", "", "", 1},
};

constexpr IconPath DeviceAudioTapePaths[] = {
    {"M3 7a2 2 0 0 1 2 -2h14a2 2 0 0 1 2 2v10a2 2 0 0 1 -2 2h-14a2 2 0 0 1 -2 -2v-10", "", "", 1},
    {"M3 17l4 -3h10l4 3", "", "", 1},
    {"M7 9.5a.5 .5 0 1 0 1 0a.5 .5 0 1 0 -1 0", "currentColor", "", 1},
    {"M16 9.5a.5 .5 0 1 0 1 0a.5 .5 0 1 0 -1 0", "currentColor", "", 1},
};

constexpr IconPath DeviceFloppyPaths[] = {
    {"M6 4h10l4 4v10a2 2 0 0 1 -2 2h-12a2 2 0 0 1 -2 -2v-12a2 2 0 0 1 2 -2", "", "", 1},
    {"M10 14a2 2 0 1 0 4 0a2 2 0 1 0 -4 0", "", "", 1},
    {"M14 4l0 4l-6 0l0 -4", "", "", 1},
};

constexpr IconPath EraserPaths[] = {
    {"M19 20h-10.5l-4.21 -4.3a1 1 0 0 1 0 -1.41l10 -10a1 1 0 0 1 1.41 0l5 5a1 1 0 0 1 0 1.41l-9.2 9.3", "", "", 1},
    {"M18 13.3l-6.3 -6.3", "", "", 1},
};

constexpr IconPath EyePaths[] = {
    {"M10 12a2 2 0 1 0 4 0a2 2 0 0 0 -4 0", "", "", 1},
    {"M21 12c-2.4 4 -5.4 6 -9 6c-3.6 0 -6.6 -2 -9 -6c2.4 -4 5.4 -6 9 -6c3.6 0 6.6 2 9 6", "", "", 1},
};

constexpr IconPath EyeOffPaths[] = {
    {"M10.585 10.587a2 2 0 0 0 2.829 2.828", "", "", 1},
    {"M16.681 16.673a8.717 8.717 0 0 1 -4.681 1.327c-3.6 0 -6.6 -2 -9 -6c1.272 -2.12 2.712 -3.678 4.32 -4.674m2.86 -1.146a9.055 9.055 0 0 1 1.82 -.18c3.6 0 6.6 2 9 6c-.666 1.11 -1.379 2.067 -2.138 2.87", "", "", 1},
    {"M3 3l18 18", "", "", 1},
};

constexpr IconPath FilePaths[] = {
    {"M14 3v4a1 1 0 0 0 1 1h4", "", "", 1},
    {"M17 21h-10a2 2 0 0 1 -2 -2v-14a2 2 0 0 1 2 -2h7l5 5v11a2 2 0 0 1 -2 2", "", "", 1},
};

constexpr IconPath FileCodePaths[] = {
    {"M14 3v4a1 1 0 0 0 1 1h4", "", "", 1},
    {"M17 21h-10a2 2 0 0 1 -2 -2v-14a2 2 0 0 1 2 -2h7l5 5v11a2 2 0 0 1 -2 2", "", "", 1},
    {"M10 13l-1 2l1 2", "", "", 1},
    {"M14 13l1 2l-1 2", "", "", 1},
};

constexpr IconPath FileExportPaths[] = {
    {"M14 3v4a1 1 0 0 0 1 1h4", "", "", 1},
    {"M11.5 21h-4.5a2 2 0 0 1 -2 -2v-14a2 2 0 0 1 2 -2h7l5 5v5m-5 6h7m-3 -3l3 3l-3 3", "", "", 1},
};

constexpr IconPath FileTextPaths[] = {
    {"M14 3v4a1 1 0 0 0 1 1h4", "", "", 1},
    {"M17 21h-10a2 2 0 0 1 -2 -2v-14a2 2 0 0 1 2 -2h7l5 5v11a2 2 0 0 1 -2 2", "", "", 1},
    {"M9 9l1 0", "", "", 1},
    {"M9 13l6 0", "", "", 1},
    {"M9 17l6 0", "", "", 1},
};

constexpr IconPath FileUnknownPaths[] = {
    {"M14 3v4a1 1 0 0 0 1 1h4", "", "", 1},
    {"M17 21h-10a2 2 0 0 1 -2 -2v-14a2 2 0 0 1 2 -2h7l5 5v11a2 2 0 0 1 -2 2", "", "", 1},
    {"M12 17v.01", "", "", 1},
    {"M12 14a1.5 1.5 0 1 0 -1.14 -2.474", "", "", 1},
};

constexpr IconPath FileUploadPaths[] = {
    {"M14 3v4a1 1 0 0 0 1 1h4", "", "", 1},
    {"M17 21h-10a2 2 0 0 1 -2 -2v-14a2 2 0 0 1 2 -2h7l5 5v11a2 2 0 0 1 -2 2", "", "", 1},
    {"M12 11v6", "", "", 1},
    {"M9.5 13.5l2.5 -2.5l2.5 2.5", "", "", 1},
};

constexpr IconPath FileZipPaths[] = {
    {"M6 20.735a2 2 0 0 1 -1 -1.735v-14a2 2 0 0 1 2 -2h7l5 5v11a2 2 0 0 1 -2 2h-1", "", "", 1},
    {"M11 17a2 2 0 0 1 2 2v2a1 1 0 0 1 -1 1h-2a1 1 0 0 1 -1 -1v-2a2 2 0 0 1 2 -2", "", "", 1},
    {"M11 5l-1 0", "", "", 1},
    {"M13 7l-1 0", "", "", 1},
    {"M11 9l-1 0", "", "", 1},
    {"M13 11l-1 0", "", "", 1},
    {"M11 13l-1 0", "", "", 1},
    {"M13 15l-1 0", "", "", 1},
};

constexpr IconPath FilterPaths[] = {
    {"M4 4h16v2.172a2 2 0 0 1 -.586 1.414l-4.414 4.414v7l-6 2v-8.5l-4.48 -4.928a2 2 0 0 1 -.52 -1.345v-2.227", "", "", 1},
};

constexpr IconPath FolderPaths[] = {
    {"M5 4h4l3 3h7a2 2 0 0 1 2 2v8a2 2 0 0 1 -2 2h-14a2 2 0 0 1 -2 -2v-11a2 2 0 0 1 2 -2", "", "", 1},
};

constexpr IconPath FolderOpenPaths[] = {
    {"M5 19l2.757 -7.351a1 1 0 0 1 .936 -.649h12.307a1 1 0 0 1 .986 1.164l-.996 5.211a2 2 0 0 1 -1.964 1.625h-14.026a2 2 0 0 1 -2 -2v-11a2 2 0 0 1 2 -2h4l3 3h7a2 2 0 0 1 2 2v2", "", "", 1},
};

constexpr IconPath FolderPlusPaths[] = {
    {"M12 19h-7a2 2 0 0 1 -2 -2v-11a2 2 0 0 1 2 -2h4l3 3h7a2 2 0 0 1 2 2v3.5", "", "", 1},
    {"M16 19h6", "", "", 1},
    {"M19 16v6", "", "", 1},
};

constexpr IconPath HandClickPaths[] = {
    {"M8 13v-8.5a1.5 1.5 0 0 1 3 0v7.5", "", "", 1},
    {"M11 11.5v-2a1.5 1.5 0 0 1 3 0v2.5", "", "", 1},
    {"M14 10.5a1.5 1.5 0 0 1 3 0v1.5", "", "", 1},
    {"M17 11.5a1.5 1.5 0 0 1 3 0v4.5a6 6 0 0 1 -6 6h-2h.208a6 6 0 0 1 -5.012 -2.7l-.196 -.3c-.312 -.479 -1.407 -2.388 -3.286 -5.728a1.5 1.5 0 0 1 .536 -2.022a1.867 1.867 0 0 1 2.28 .28l1.47 1.47", "", "", 1},
    {"M5 3l-1 -1", "", "", 1},
    {"M4 7h-1", "", "", 1},
    {"M14 3l1 -1", "", "", 1},
    {"M15 6h1", "", "", 1},
};

constexpr IconPath HardDiskOnePaths[] = {
    {"M19.5 3H4.5C3.67158 3 3 3.67158 3 4.5V19.5C3 20.32845 3.67158 21 4.5 21H19.5C20.32845 21 21 20.32845 21 19.5V4.5C21 3.67158 20.32845 3 19.5 3Z", "", "", 1},
    {"M16 3H8V8H16V3Z", "accent", "", 1},
    {"M8.5 18H15.5", "", "", 1},
};

constexpr IconPath InfoCirclePaths[] = {
    {"M3 12a9 9 0 1 0 18 0a9 9 0 0 0 -18 0", "", "", 1},
    {"M12 9h.01", "", "", 1},
    {"M11 12h1v4h1", "", "", 1},
};

constexpr IconPath LayoutBottombarCollapsePaths[] = {
    {"M20 6v12a2 2 0 0 1 -2 2h-12a2 2 0 0 1 -2 -2v-12a2 2 0 0 1 2 -2h12a2 2 0 0 1 2 2", "", "", 1},
    {"M20 15h-16", "", "", 1},
    {"M14 8l-2 2l-2 -2", "", "", 1},
};

constexpr IconPath LayoutSidebarRightCollapsePaths[] = {
    {"M4 6a2 2 0 0 1 2 -2h12a2 2 0 0 1 2 2v12a2 2 0 0 1 -2 2h-12a2 2 0 0 1 -2 -2l0 -12", "", "", 1},
    {"M15 4v16", "", "", 1},
    {"M9 10l2 2l-2 2", "", "", 1},
};

constexpr IconPath LayoutSidebarRightExpandPaths[] = {
    {"M4 6a2 2 0 0 1 2 -2h12a2 2 0 0 1 2 2v12a2 2 0 0 1 -2 2h-12a2 2 0 0 1 -2 -2l0 -12", "", "", 1},
    {"M15 4v16", "", "", 1},
    {"M10 10l-2 2l2 2", "", "", 1},
};

constexpr IconPath LockPaths[] = {
    {"M5 13a2 2 0 0 1 2 -2h10a2 2 0 0 1 2 2v6a2 2 0 0 1 -2 2h-10a2 2 0 0 1 -2 -2v-6", "", "", 1},
    {"M11 16a1 1 0 1 0 2 0a1 1 0 0 0 -2 0", "", "", 1},
    {"M8 11v-4a4 4 0 1 1 8 0v4", "", "", 1},
};

constexpr IconPath LockOpenPaths[] = {
    {"M5 13a2 2 0 0 1 2 -2h10a2 2 0 0 1 2 2v6a2 2 0 0 1 -2 2h-10a2 2 0 0 1 -2 -2l0 -6", "", "", 1},
    {"M11 16a1 1 0 1 0 2 0a1 1 0 1 0 -2 0", "", "", 1},
    {"M8 11v-5a4 4 0 0 1 8 0", "", "", 1},
};

constexpr IconPath PausePaths[] = {
    {"M8 3V21", "", "", 1},
    {"M16 3V21", "", "", 1},
};

constexpr IconPath PencilPaths[] = {
    {"M4 20h4l10.5 -10.5a2.828 2.828 0 1 0 -4 -4l-10.5 10.5v4", "", "", 1},
    {"M13.5 6.5l4 4", "", "", 1},
};

constexpr IconPath PhotoPaths[] = {
    {"M15 8h.01", "", "", 1},
    {"M3 6a3 3 0 0 1 3 -3h12a3 3 0 0 1 3 3v12a3 3 0 0 1 -3 3h-12a3 3 0 0 1 -3 -3v-12", "", "", 1},
    {"M3 16l5 -5c.928 -.893 2.072 -.893 3 0l5 5", "", "", 1},
    {"M14 14l1 -1c.928 -.893 2.072 -.893 3 0l3 3", "", "", 1},
};

constexpr IconPath PlayOnePaths[] = {
    {"M5 12V3L12.79425 7.5L20.58845 12L12.79425 16.5L5 21V12Z", "accent", "", 1},
};

constexpr IconPath PlayerEjectPaths[] = {
    {"M5 12h14l-7 -8l-7 8", "", "", 1},
    {"M5 17a1 1 0 0 1 1 -1h12a1 1 0 0 1 1 1v2a1 1 0 0 1 -1 1h-12a1 1 0 0 1 -1 -1l0 -2", "", "", 1},
};

constexpr IconPath PlayerPausePaths[] = {
    {"M6 6a1 1 0 0 1 1 -1h2a1 1 0 0 1 1 1v12a1 1 0 0 1 -1 1h-2a1 1 0 0 1 -1 -1l0 -12", "", "", 1},
    {"M14 6a1 1 0 0 1 1 -1h2a1 1 0 0 1 1 1v12a1 1 0 0 1 -1 1h-2a1 1 0 0 1 -1 -1l0 -12", "", "", 1},
};

constexpr IconPath PlayerPlayPaths[] = {
    {"M7 4v16l13 -8l-13 -8", "", "", 1},
};

constexpr IconPath PlayerRecordPaths[] = {
    {"M5 12a7 7 0 1 0 14 0a7 7 0 1 0 -14 0", "", "", 1},
};

constexpr IconPath PlayerStopPaths[] = {
    {"M5 7a2 2 0 0 1 2 -2h10a2 2 0 0 1 2 2v10a2 2 0 0 1 -2 2h-10a2 2 0 0 1 -2 -2l0 -10", "", "", 1},
};

constexpr IconPath PlusPaths[] = {
    {"M12 5l0 14", "", "", 1},
    {"M5 12l14 0", "", "", 1},
};

constexpr IconPath PowerPaths[] = {
    {"M7 6a7.75 7.75 0 1 0 10 0", "", "", 1},
    {"M12 4l0 8", "", "", 1},
};

constexpr IconPath PrinterPaths[] = {
    {"M17 17h2a2 2 0 0 0 2 -2v-4a2 2 0 0 0 -2 -2h-14a2 2 0 0 0 -2 2v4a2 2 0 0 0 2 2h2", "", "", 1},
    {"M17 9v-4a2 2 0 0 0 -2 -2h-6a2 2 0 0 0 -2 2v4", "", "", 1},
    {"M7 15a2 2 0 0 1 2 -2h6a2 2 0 0 1 2 2v4a2 2 0 0 1 -2 2h-6a2 2 0 0 1 -2 -2l0 -4", "", "", 1},
};

constexpr IconPath RefreshPaths[] = {
    {"M20 11a8.1 8.1 0 0 0 -15.5 -2m-.5 -4v4h4", "", "", 1},
    {"M4 13a8.1 8.1 0 0 0 15.5 2m.5 4v-4h-4", "", "", 1},
};

constexpr IconPath ReloadPaths[] = {
    {"M19.933 13.041a8 8 0 1 1 -9.925 -8.788c3.899 -1 7.935 1.007 9.425 4.747", "", "", 1},
    {"M20 4v5h-5", "", "", 1},
};

constexpr IconPath ReplacePaths[] = {
    {"M3 4a1 1 0 0 1 1 -1h4a1 1 0 0 1 1 1v4a1 1 0 0 1 -1 1h-4a1 1 0 0 1 -1 -1l0 -4", "", "", 1},
    {"M15 16a1 1 0 0 1 1 -1h4a1 1 0 0 1 1 1v4a1 1 0 0 1 -1 1h-4a1 1 0 0 1 -1 -1l0 -4", "", "", 1},
    {"M21 11v-3a2 2 0 0 0 -2 -2h-6l3 3m0 -6l-3 3", "", "", 1},
    {"M3 13v3a2 2 0 0 0 2 2h6l-3 -3m0 6l3 -3", "", "", 1},
};

constexpr IconPath RotatePaths[] = {
    {"M19.95 11a8 8 0 1 0 -.5 4m.5 5v-5h-5", "", "", 1},
};

constexpr IconPath SelectAllPaths[] = {
    {"M8 9a1 1 0 0 1 1 -1h6a1 1 0 0 1 1 1v6a1 1 0 0 1 -1 1h-6a1 1 0 0 1 -1 -1l0 -6", "", "", 1},
    {"M12 20v.01", "", "", 1},
    {"M16 20v.01", "", "", 1},
    {"M8 20v.01", "", "", 1},
    {"M4 20v.01", "", "", 1},
    {"M4 16v.01", "", "", 1},
    {"M4 12v.01", "", "", 1},
    {"M4 8v.01", "", "", 1},
    {"M4 4v.01", "", "", 1},
    {"M8 4v.01", "", "", 1},
    {"M12 4v.01", "", "", 1},
    {"M16 4v.01", "", "", 1},
    {"M20 4v.01", "", "", 1},
    {"M20 8v.01", "", "", 1},
    {"M20 12v.01", "", "", 1},
    {"M20 16v.01", "", "", 1},
    {"M20 20v.01", "", "", 1},
};

constexpr IconPath SendPaths[] = {
    {"M10 14l11 -11", "", "", 1},
    {"M21 3l-6.5 18a.55 .55 0 0 1 -1 0l-3.5 -7l-7 -3.5a.55 .55 0 0 1 0 -1l18 -6.5", "", "", 1},
};

constexpr IconPath SettingsPaths[] = {
    {"M10.325 4.317c.426 -1.756 2.924 -1.756 3.35 0a1.724 1.724 0 0 0 2.573 1.066c1.543 -.94 3.31 .826 2.37 2.37a1.724 1.724 0 0 0 1.065 2.572c1.756 .426 1.756 2.924 0 3.35a1.724 1.724 0 0 0 -1.066 2.573c.94 1.543 -.826 3.31 -2.37 2.37a1.724 1.724 0 0 0 -2.572 1.065c-.426 1.756 -2.924 1.756 -3.35 0a1.724 1.724 0 0 0 -2.573 -1.066c-1.543 .94 -3.31 -.826 -2.37 -2.37a1.724 1.724 0 0 0 -1.065 -2.572c-1.756 -.426 -1.756 -2.924 0 -3.35a1.724 1.724 0 0 0 1.066 -2.573c-.94 -1.543 .826 -3.31 2.37 -2.37c1 .608 2.296 .07 2.572 -1.065", "", "", 1},
    {"M9 12a3 3 0 1 0 6 0a3 3 0 0 0 -6 0", "", "", 1},
};

constexpr IconPath SparklesPaths[] = {
    {"M16 18a2 2 0 0 1 2 2a2 2 0 0 1 2 -2a2 2 0 0 1 -2 -2a2 2 0 0 1 -2 2m0 -12a2 2 0 0 1 2 2a2 2 0 0 1 2 -2a2 2 0 0 1 -2 -2a2 2 0 0 1 -2 2m-7 12a6 6 0 0 1 6 -6a6 6 0 0 1 -6 -6a6 6 0 0 1 -6 6a6 6 0 0 1 6 6", "", "", 1},
};

constexpr IconPath SquarePaths[] = {
    {"M3 5a2 2 0 0 1 2 -2h14a2 2 0 0 1 2 2v14a2 2 0 0 1 -2 2h-14a2 2 0 0 1 -2 -2v-14", "", "", 1},
};

constexpr IconPath SquareMinusPaths[] = {
    {"M9 12h6", "", "", 1},
    {"M3 5a2 2 0 0 1 2 -2h14a2 2 0 0 1 2 2v14a2 2 0 0 1 -2 2h-14a2 2 0 0 1 -2 -2v-14", "", "", 1},
};

constexpr IconPath TrashPaths[] = {
    {"M4 7l16 0", "", "", 1},
    {"M10 11l0 6", "", "", 1},
    {"M14 11l0 6", "", "", 1},
    {"M5 7l1 12a2 2 0 0 0 2 2h8a2 2 0 0 0 2 -2l1 -12", "", "", 1},
    {"M9 7v-3a1 1 0 0 1 1 -1h4a1 1 0 0 1 1 1v3", "", "", 1},
};

constexpr IconPath UnlinkPaths[] = {
    {"M17 22v-2", "", "", 1},
    {"M9 15l6 -6", "", "", 1},
    {"M11 6l.463 -.536a5 5 0 0 1 7.071 7.072l-.534 .464", "", "", 1},
    {"M13 18l-.397 .534a5.068 5.068 0 0 1 -7.127 0a4.972 4.972 0 0 1 0 -7.071l.524 -.463", "", "", 1},
    {"M20 17h2", "", "", 1},
    {"M2 7h2", "", "", 1},
    {"M7 2v2", "", "", 1},
};

constexpr IconPath UploadPaths[] = {
    {"M4 17v2a2 2 0 0 0 2 2h12a2 2 0 0 0 2 -2v-2", "", "", 1},
    {"M7 9l5 -5l5 5", "", "", 1},
    {"M12 4l0 12", "", "", 1},
};

constexpr IconPath VolumePaths[] = {
    {"M15 8a5 5 0 0 1 0 8", "", "", 1},
    {"M17.7 5a9 9 0 0 1 0 14", "", "", 1},
    {"M6 15h-2a1 1 0 0 1 -1 -1v-4a1 1 0 0 1 1 -1h2l3.5 -4.5a.8 .8 0 0 1 1.5 .5v14a.8 .8 0 0 1 -1.5 .5l-3.5 -4.5", "", "", 1},
};

constexpr IconPath VolumeOffPaths[] = {
    {"M15 8a5 5 0 0 1 1.912 4.934m-1.377 2.602a5 5 0 0 1 -.535 .464", "", "", 1},
    {"M17.7 5a9 9 0 0 1 2.362 11.086m-1.676 2.299a9 9 0 0 1 -.686 .615", "", "", 1},
    {"M9.069 5.054l.431 -.554a.8 .8 0 0 1 1.5 .5v2m0 4v8a.8 .8 0 0 1 -1.5 .5l-3.5 -4.5h-2a1 1 0 0 1 -1 -1v-4a1 1 0 0 1 1 -1h2l1.294 -1.664", "", "", 1},
    {"M3 3l18 18", "", "", 1},
};

constexpr IconPath XPaths[] = {
    {"M18 6l-12 12", "", "", 1},
    {"M6 6l12 12", "", "", 1},
};

constexpr IconData kIcons[] = {
    {AdjustmentsPaths, sizeof(AdjustmentsPaths) / sizeof(IconPath)},
    {AlertCirclePaths, sizeof(AlertCirclePaths) / sizeof(IconPath)},
    {AlertTrianglePaths, sizeof(AlertTrianglePaths) / sizeof(IconPath)},
    {AppWindowPaths, sizeof(AppWindowPaths) / sizeof(IconPath)},
    {ApplePaths, sizeof(ApplePaths) / sizeof(IconPath)},
    {ArrowBackUpPaths, sizeof(ArrowBackUpPaths) / sizeof(IconPath)},
    {ArrowDownPaths, sizeof(ArrowDownPaths) / sizeof(IconPath)},
    {ArrowLeftPaths, sizeof(ArrowLeftPaths) / sizeof(IconPath)},
    {ArrowRightPaths, sizeof(ArrowRightPaths) / sizeof(IconPath)},
    {ArrowUpPaths, sizeof(ArrowUpPaths) / sizeof(IconPath)},
    {ArrowsDiffPaths, sizeof(ArrowsDiffPaths) / sizeof(IconPath)},
    {BellPaths, sizeof(BellPaths) / sizeof(IconPath)},
    {BinaryPaths, sizeof(BinaryPaths) / sizeof(IconPath)},
    {CategoryPaths, sizeof(CategoryPaths) / sizeof(IconPath)},
    {CheckPaths, sizeof(CheckPaths) / sizeof(IconPath)},
    {ChevronDownPaths, sizeof(ChevronDownPaths) / sizeof(IconPath)},
    {ChevronRightPaths, sizeof(ChevronRightPaths) / sizeof(IconPath)},
    {ChevronUpPaths, sizeof(ChevronUpPaths) / sizeof(IconPath)},
    {CircleCheckPaths, sizeof(CircleCheckPaths) / sizeof(IconPath)},
    {CircleXPaths, sizeof(CircleXPaths) / sizeof(IconPath)},
    {ClipboardPaths, sizeof(ClipboardPaths) / sizeof(IconPath)},
    {ClockPaths, sizeof(ClockPaths) / sizeof(IconPath)},
    {CopyPaths, sizeof(CopyPaths) / sizeof(IconPath)},
    {CutPaths, sizeof(CutPaths) / sizeof(IconPath)},
    {DeviceAudioTapePaths, sizeof(DeviceAudioTapePaths) / sizeof(IconPath)},
    {DeviceFloppyPaths, sizeof(DeviceFloppyPaths) / sizeof(IconPath)},
    {EraserPaths, sizeof(EraserPaths) / sizeof(IconPath)},
    {EyePaths, sizeof(EyePaths) / sizeof(IconPath)},
    {EyeOffPaths, sizeof(EyeOffPaths) / sizeof(IconPath)},
    {FilePaths, sizeof(FilePaths) / sizeof(IconPath)},
    {FileCodePaths, sizeof(FileCodePaths) / sizeof(IconPath)},
    {FileExportPaths, sizeof(FileExportPaths) / sizeof(IconPath)},
    {FileTextPaths, sizeof(FileTextPaths) / sizeof(IconPath)},
    {FileUnknownPaths, sizeof(FileUnknownPaths) / sizeof(IconPath)},
    {FileUploadPaths, sizeof(FileUploadPaths) / sizeof(IconPath)},
    {FileZipPaths, sizeof(FileZipPaths) / sizeof(IconPath)},
    {FilterPaths, sizeof(FilterPaths) / sizeof(IconPath)},
    {FolderPaths, sizeof(FolderPaths) / sizeof(IconPath)},
    {FolderOpenPaths, sizeof(FolderOpenPaths) / sizeof(IconPath)},
    {FolderPlusPaths, sizeof(FolderPlusPaths) / sizeof(IconPath)},
    {HandClickPaths, sizeof(HandClickPaths) / sizeof(IconPath)},
    {HardDiskOnePaths, sizeof(HardDiskOnePaths) / sizeof(IconPath)},
    {InfoCirclePaths, sizeof(InfoCirclePaths) / sizeof(IconPath)},
    {LayoutBottombarCollapsePaths, sizeof(LayoutBottombarCollapsePaths) / sizeof(IconPath)},
    {LayoutSidebarRightCollapsePaths, sizeof(LayoutSidebarRightCollapsePaths) / sizeof(IconPath)},
    {LayoutSidebarRightExpandPaths, sizeof(LayoutSidebarRightExpandPaths) / sizeof(IconPath)},
    {LockPaths, sizeof(LockPaths) / sizeof(IconPath)},
    {LockOpenPaths, sizeof(LockOpenPaths) / sizeof(IconPath)},
    {PausePaths, sizeof(PausePaths) / sizeof(IconPath)},
    {PencilPaths, sizeof(PencilPaths) / sizeof(IconPath)},
    {PhotoPaths, sizeof(PhotoPaths) / sizeof(IconPath)},
    {PlayOnePaths, sizeof(PlayOnePaths) / sizeof(IconPath)},
    {PlayerEjectPaths, sizeof(PlayerEjectPaths) / sizeof(IconPath)},
    {PlayerPausePaths, sizeof(PlayerPausePaths) / sizeof(IconPath)},
    {PlayerPlayPaths, sizeof(PlayerPlayPaths) / sizeof(IconPath)},
    {PlayerRecordPaths, sizeof(PlayerRecordPaths) / sizeof(IconPath)},
    {PlayerStopPaths, sizeof(PlayerStopPaths) / sizeof(IconPath)},
    {PlusPaths, sizeof(PlusPaths) / sizeof(IconPath)},
    {PowerPaths, sizeof(PowerPaths) / sizeof(IconPath)},
    {PrinterPaths, sizeof(PrinterPaths) / sizeof(IconPath)},
    {RefreshPaths, sizeof(RefreshPaths) / sizeof(IconPath)},
    {ReloadPaths, sizeof(ReloadPaths) / sizeof(IconPath)},
    {ReplacePaths, sizeof(ReplacePaths) / sizeof(IconPath)},
    {RotatePaths, sizeof(RotatePaths) / sizeof(IconPath)},
    {SelectAllPaths, sizeof(SelectAllPaths) / sizeof(IconPath)},
    {SendPaths, sizeof(SendPaths) / sizeof(IconPath)},
    {SettingsPaths, sizeof(SettingsPaths) / sizeof(IconPath)},
    {SparklesPaths, sizeof(SparklesPaths) / sizeof(IconPath)},
    {SquarePaths, sizeof(SquarePaths) / sizeof(IconPath)},
    {SquareMinusPaths, sizeof(SquareMinusPaths) / sizeof(IconPath)},
    {TrashPaths, sizeof(TrashPaths) / sizeof(IconPath)},
    {UnlinkPaths, sizeof(UnlinkPaths) / sizeof(IconPath)},
    {UploadPaths, sizeof(UploadPaths) / sizeof(IconPath)},
    {VolumePaths, sizeof(VolumePaths) / sizeof(IconPath)},
    {VolumeOffPaths, sizeof(VolumeOffPaths) / sizeof(IconPath)},
    {XPaths, sizeof(XPaths) / sizeof(IconPath)},
};

}  // namespace

const IconData& iconData(IconId id) { return kIcons[static_cast<std::size_t>(id)]; }

}  // namespace rujian::ui
