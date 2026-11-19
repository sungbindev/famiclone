# 如见控件库（Win32）

`win32/ui-kit` 是 Ember 原生外壳用的 C++ 控件库：静态库 `rujian_ui`，用 Direct2D 1.0、DirectWrite 1.0 与 WIC 绘制，外观、尺寸、颜色与 [rujian-ui-kit](../../../../rujian-ui-kit)（网页版组件库）一致。目标系统是 Windows 7 SP1 到 Windows 11 的 64 位系统。本库不依赖 Ember 的任何代码，计划与决策见 [docs/08-Win32原生外壳.md](../../docs/08-Win32原生外壳.md)。

## 目录

| 目录 | 内容 |
| --- | --- |
| `include/rujian/ui/` | 公共头文件：颜色、几何、文字样式、画布接口、控件基类、宿主、根容器、窗口宿主、离屏绘制、各控件 |
| `src/base/` | 绘制（Direct2D 画布）、文字排版（DirectWrite）、图标几何、控件树与输入分发、窗口、离屏绘制；不引用控件 |
| `src/components/<控件>/` | 一个控件或一族控件一个目录；只引用公共头文件、`base/theme-tokens.hpp` 与自己目录里的文件，不直接用系统图形头文件 |
| `src/generated/` | 令牌与图标生成的代码，不手写、不手改，已提交 |
| `tools/` | 令牌与图标生成器、与网页版对照的工具（`reference`） |
| `gallery/` | 预览程序 `rujian-ui-gallery`，窗口模式用来目测，`--render` 模式离屏绘制成 PNG |
| `test/` | GoogleTest 单元测试（构建时开启 `EMBER_BUILD_TESTS`） |

## 常用命令

在 `ember/` 目录下运行：

| 命令 | 用途 |
| --- | --- |
| `node win32/ui-kit/tools/gen-tokens.mjs` | 从 rujian-ui-kit 的令牌与主题生成 `src/generated/tokens.generated.hpp`；`--check` 只检查一致 |
| `node win32/ui-kit/tools/gen-icons.mjs` | 生成 `include/rujian/ui/icon-id.hpp` 与 `src/generated/icon-data.generated.cpp`；`--check` 只检查一致 |
| `node win32/ui-kit/tools/gen-fonts.mjs` | 把 rujian-ui-kit 的 Quicksand（WOFF2）解码成 TrueType，生成 `src/generated/font-data.generated.cpp`；`--check` 只检查一致 |
| `npm run win32:test` | 构建并运行测试；用 `-- -R ^Root\.` 只运行名称匹配的用例 |
| `npm run win32:check` | 结构检查与 Windows 7 导入表审计 |
| `npm run win32:reference` | 与网页版逐像素对照（需先 `win32:test` 构建画廊） |

组件库源码的位置默认是 `../../rujian-ui-kit`，用环境变量 `EMBER_UI_KIT_SOURCE` 修改。

## 使用方式

```cpp
#include "rujian/ui/button.hpp"
#include "rujian/ui/window.hpp"

rujian::ui::HostWindow window({.title = L"示例", .clientSize = {440, 60}});
auto button = std::make_unique<rujian::ui::Button>("确定");
button->setOnClick([] { /* 点击 */ });
window.root().setContent(std::move(button));
window.show();
return rujian::ui::HostWindow::runMessageLoop();
```

- 控件树挂到 `Root` 上之后才有宿主，才能度量文字；`Root` 负责布局、绘制、悬停、按下、焦点遍历（Tab）与输入事件分发。
- 所有长度都是逻辑像素（与 CSS 像素相同），绘制时由画布乘以窗口的缩放比例并对齐到设备像素。
- 颜色在绘制时按当前主题从生成的令牌里现取，不缓存；`Root::setTheme` 切换主题。
- 离屏绘制：`OffscreenSurface` 把控件树画到内存位图，可取像素或存成 PNG，用于自动测试与对照。
- 弹层（下拉选项等）经 `Host::showLayer` 显示在内容之上，由打开它的控件拥有；在弹层之外按下鼠标或窗口大小变化时，宿主通知它自己关闭。
- 定时器与动画：`Host::startTimer` 是一次性定时器，到时间调用控件的 `onTimer`；`Host::animationsEnabled` 为假时（离屏对照）控件不播放渐变。测试里用 `Root::fireTimer` 手动触发。
- 悬停提示：控件用 `Widget::setTooltip`（纯文字）或 `setRichTooltip`（标题行加分条说明）设置提示；应用在每个 `Root` 上创建一个 `Tooltip`（先于 `Root` 销毁），延迟与收起的时机由 `Root` 处理。
- 通知：应用在每个 `Root` 上创建一个 `NotificationCenter`（铺满窗口的弹层），用 `notify` 发通知，`NotificationButton` 放在状态栏右侧（`StatusBar::addEndWidget`），“通知详情”经 `setOnShowDetail` 由应用用对话框显示。
- 标题栏自绘系统按钮：窗口的 `WM_NCHITTEST` 调用 `TitleBar::hitTestWindow`，把结果转成 `HTMINBUTTON`、`HTMAXBUTTON`、`HTCLOSE`、`HTCAPTION` 或客户区。

## 已有控件

| 控件 | 头文件 | 说明 |
| --- | --- | --- |
| 布局 | `layout.hpp` | `Stack`、`Row`、`Wrap`、`Actions`（弹性布局） |
| 按钮 | `button.hpp` | 次要、主要、危险、无底色、链接，紧凑、纯图标、切换、文字自动匹配图标 |
| 文字输入框 | `text-input.hpp` | 自绘编辑（光标、选区、复制粘贴、撤销、鼠标选择、输入法）、占位文字、校验失败、搜索类型的清除按钮 |
| 复选框 | `checkbox.hpp` | 点击或空格切换 |
| 分段单选 | `radio-group.hpp` | 选项等宽排成一行，方向键在可用选项间移动并选中 |
| 下拉框 | `select.hpp` | 触发器加弹层选项列表，键盘与鼠标操作，下方放不下时翻到上方 |
| 滑块 | `slider.hpp` | 轨道加圆形滑块，拖动、点击轨道跳转、键盘增减 |
| 滚动区域 | `scroll-area.hpp` | 纵向滚动，滚动条盖在内容上，鼠标移入或滚动时显示、停下后渐隐，点击轨道、拖动、滚轮、键盘、焦点控件滚到可见 |
| 文字 | `label.hpp` | `Label`（可换行的文字：字号、字重、行高倍数、颜色，空文字不占高度）与 `Heading`（小节标题） |
| 面板 | `panel.hpp` | 卡片外观、标题行（标题居左、aside 居右）、可滚动的内容区；紧凑标题栏、平面外观 |
| 键值清单 | `facts.hpp` | 名称列加内容列，窗口宽度不超过 420 时上下排列 |
| 字段 | `field.hpp` | 标签、说明、控件纵向排列；没有标签时说明在控件下方 |
| 设置行 | `setting-row.hpp` | 左边名称与说明、右边控件，控件下方可带错误文字；窗口宽度不超过 720 时上下排列 |
| 提示块 | `callout.hpp` | 浅色底加同色细边框的说明，下方可放别的控件；提示与错误两种 |
| 说明块 | `state.hpp` | 没有内容或出错时的一段说明，出错时带图标、用错误色 |
| 状态标签 | `status-label.hpp` | 图标加文字，成功与已停止两种 |
| 分栏视图 | `split-view.hpp` | 左栏（可滚动）加主区，分隔条可拖动、方向键调整、限制最小与最大宽度 |
| 页签 | `tabs.hpp` | 页签栏加各页签的内容，鼠标与方向键切换，只显示选中页签的内容 |
| 分隔线列表 | `list.hpp` | `List`（行之间有分割线）与 `ListRow`（`SettingRow` 是它的一种）；放进面板后贴边的行去掉内边距 |
| 信息行列表 | `row-list.hpp` | `RowList` 与 `RowListItem`：前导、主文字、信息列、行尾操作，悬停与按下底色 |
| 工具栏 | `toolbar.hpp` | 前导控件、说明、右侧操作，放不下时操作换行 |
| 树 | `tree.hpp` | 扁平条目，鼠标与键盘（上下、Home、End 移焦点，回车与空格选中），焦点行滚入可见范围 |
| 表格 | `table.hpp` | `Table`（紧凑、平铺：列宽自动分配、行选中、行详情）与 `TableMainCell`（主副文字） |
| 菜单 | `menu.hpp` | `MenuBar`（菜单名按钮、点击打开、悬停切换、左右方向键切换）与 `MenuPopup`（勾选、单选、图标、快捷键、提示文字、分隔线、子菜单、助记符、键盘导航） |
| 标题栏 | `title-bar.hpp` | 图标、应用名与副名（Quicksand、字间距）、菜单栏、操作区、折叠按钮、自绘的系统按钮与命中测试 |
| 状态栏 | `status-bar.hpp` | `StatusBar`（普通与大尺寸）与 `StatusBarItem`（状态点、图标、等级、指示灯、按钮状态项、放不下时按权重收缩） |
| 悬停提示 | `tooltip.hpp` | `Tooltip`（纯文字与版式提示、角标、定位），延迟等时机在 `Root`；内容类型在 `tooltip-content.hpp` |
| 通知 | `notification.hpp` | `NotificationCenter`（弹出的通知、通知面板、历史、未读）与 `NotificationButton` |
| 对话框框架 | `dialog-frame.hpp` | 独立窗口对话框的内容：标题栏（最大化、关闭）、正文、按钮区，Esc 与回车处理、初始焦点、标题栏空白处的命中测试 |
| 窗口框架 | `app-frame.hpp` | 标题栏、页面区、底部面板、状态栏自上而下排布，失焦状态与通知让出状态栏 |

## 与网页版对照

`win32:reference` 用 Electron 渲染 rujian-ui-kit 的真实组件作基线，画廊离屏绘制同样的页面，按“主题 × 缩放比例（100%、125%、150%、200%）× 状态”逐项比较控件位置、非文字区域的像素与文字（图标）的外框。判定标准与实测得到的对齐规则见 08 的 4.1 与 4.4。

## 许可

Apache-2.0，见 [LICENSE](LICENSE)；图标图形来自 Tabler Icons（MIT），标题栏应用名的字体是 Quicksand（SIL OFL 1.1），许可与清单见 `third_party/tabler-icons`、`third_party/quicksand`。
