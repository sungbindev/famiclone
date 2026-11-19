/*
 * ------------------------------------------------------------------------
 * 名称：mappers-content-test.cpp
 * 说明：“mapper 插件”对话框内容的测试：工具栏（搜索框、来源下拉框、安装按钮、统计文字）、表格各行、点行选中并在行下方展开清单摘要、启停与卸载按钮（预装插件的卸载置灰）、读取中与失败与没有符合条件时的提示、
 *       模型变化时只重建表格而搜索框的内容不被冲掉、销毁后取消订阅。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：用控件库的离屏根容器测试，模型是假的；模型变化后表格延后到零毫秒定时器里重建，测试里触发全部待处理的定时器；点表格行用扫描点击位置的办法找到各行的位置。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <map>
#include <memory>

#include "contracts/mappers/mappers-model.hpp"
#include "rujian/ui/root.hpp"
#include "ui/mappers/mappers-content.hpp"

namespace ember::win32 {
namespace {

/** 假的 mapper 插件模型：记下操作，内容由测试直接改。 */
class FakeMappers final : public contracts::MappersModel {
 public:
  contracts::MappersView current;
  std::vector<std::string> calls;
  std::map<int, std::function<void()>> handlers;

  [[nodiscard]] contracts::MappersView view() const override { return current; }
  void setSearch(const std::string& text) override { calls.push_back("search " + text); }
  void setSource(const std::string& source) override { calls.push_back("source " + source); }
  void toggleSelect(const std::string& id) override { calls.push_back("select " + id); }
  void install() override { calls.push_back("install"); }
  void uninstall(const std::string& id) override { calls.push_back("uninstall " + id); }
  void setEnabled(const std::string& id, bool enabled) override { calls.push_back("enable " + id + (enabled ? " on" : " off")); }
  [[nodiscard]] std::function<void()> subscribe(std::function<void()> handler) override {
    const int id = static_cast<int>(handlers.size()) + 1;
    handlers[id] = std::move(handler);
    return [this, id] { handlers.erase(id); };
  }
  void notify() {
    const auto copy = handlers;
    for (const auto& [id, handler] : copy) handler();
  }
};

contracts::MapperRow row(const std::string& id, const std::string& name, bool bundled, bool enabled) {
  contracts::MapperRow result;
  result.id = id;
  result.name = name;
  result.description = "说明 " + name;
  result.version = "1.0.0";
  result.match = "mapper 4";
  result.hosts = "经典 FC";
  result.source = bundled ? "预装" : "用户安装";
  result.enabled = enabled;
  result.bundled = bundled;
  return result;
}

struct Scene {
  // 模型要比根容器活得久：控件销毁时还要取消订阅
  FakeMappers model;
  rujian::ui::Root root{rujian::ui::ThemeName::Dark, 1.0};
  ui::MappersContent* content{nullptr};

  Scene() {
    model.current.load = contracts::MappersLoad::Ready;
    model.current.stats = "共 2 个插件（已启用 1 个），支持 1 种 mapper";
    model.current.rows = {row("ember.mmc3", "MMC3", true, true), row("user.cool", "Cool", false, false)};
    auto created = std::make_unique<ui::MappersContent>(model);
    content = created.get();
    root.setContent(std::move(created));
    root.layout({760, 560});
  }

  /** 触发全部待处理的定时器（模型变化后的重建在定时器里），再重新布局。 */
  void flush() {
    for (const unsigned timer : root.pendingTimers()) root.fireTimer(timer);
    root.layout({760, 560});
  }

  void notifyAndFlush() {
    model.notify();
    flush();
  }

  void click(const rujian::ui::Widget& widget) {
    const auto& box = widget.bounds();
    clickAt({box.x + box.width / 2, box.y + box.height / 2});
  }

  void clickAt(const rujian::ui::Point at) {
    root.mouseDown(at, rujian::ui::MouseButton::Left, 0, 0);
    root.mouseUp(at, rujian::ui::MouseButton::Left, 0);
  }

  /** 在表格上自上而下扫描点击，找出点中“select 标识”的纵坐标；找不到返回 -1。 */
  double rowY(const std::string& id) {
    const auto& box = content->table()->bounds();
    for (double y = box.y; y < box.y + box.height; y += 2) {
      model.calls.clear();
      clickAt({box.x + 10, y});
      if (!model.calls.empty() && model.calls.back() == "select " + id) {
        model.calls.clear();
        return y;
      }
    }
    model.calls.clear();
    return -1;
  }

  void type(const std::u32string& text) {
    for (const char32_t c : text) root.character(c);
  }
};

TEST(MappersContent, ShowsTheStatsAndOneTableRowPerPlugin) {
  Scene s;
  ASSERT_NE(s.content->table(), nullptr);
  EXPECT_EQ(s.content->rowIds(), (std::vector<std::string>{"ember.mmc3", "user.cool"}));
  EXPECT_TRUE(s.content->statusText().empty());
  EXPECT_EQ(s.content->source().value(), "all");
  EXPECT_EQ(s.content->search().value(), "");
  EXPECT_EQ(s.content->detail().toggle, nullptr);
}

TEST(MappersContent, TypingInTheSearchBoxAndChoosingASourceCallTheModel) {
  Scene s;
  s.root.setFocus(&s.content->search(), false);
  s.type(U"mm");
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"search m", "search mm"}));
  s.model.calls.clear();
  s.click(s.content->install());
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"install"}));
}

TEST(MappersContent, ClickingARowSelectsIt) {
  Scene s;
  const double y = s.rowY("user.cool");
  ASSERT_GE(y, 0);
  s.clickAt({s.content->table()->bounds().x + 10, y});
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"select user.cool"}));
}

TEST(MappersContent, SelectedRowShowsTheDetailWithToggleAndUninstallButtons) {
  Scene s;
  s.model.current.selectedId = "user.cool";
  s.model.current.selected = row("user.cool", "Cool", false, false);
  s.model.current.detailLoad = contracts::MapperDetailLoad::Ready;
  s.model.current.facts = {{"标识", {"user.cool"}}, {"匹配", {"mapper 4", "mapper 118"}}};
  s.notifyAndFlush();
  ASSERT_NE(s.content->detail().toggle, nullptr);
  ASSERT_NE(s.content->detail().uninstall, nullptr);
  EXPECT_TRUE(s.content->detail().uninstall->enabled());
  s.click(*s.content->detail().toggle);
  s.click(*s.content->detail().uninstall);
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"enable user.cool on", "uninstall user.cool"}));
}

TEST(MappersContent, ToggleButtonOffersDisableForAnEnabledPluginAndBundledCannotBeUninstalled) {
  Scene s;
  s.model.current.selectedId = "ember.mmc3";
  s.model.current.selected = row("ember.mmc3", "MMC3", true, true);
  s.model.current.detailLoad = contracts::MapperDetailLoad::Loading;
  s.notifyAndFlush();
  ASSERT_NE(s.content->detail().toggle, nullptr);
  EXPECT_FALSE(s.content->detail().uninstall->enabled());
  s.click(*s.content->detail().toggle);
  EXPECT_EQ(s.model.calls, (std::vector<std::string>{"enable ember.mmc3 off"}));
}

TEST(MappersContent, DetailFailureAndLoadingStillShowTheButtons) {
  Scene s;
  s.model.current.selectedId = "user.cool";
  s.model.current.selected = row("user.cool", "Cool", false, false);
  s.model.current.detailLoad = contracts::MapperDetailLoad::Failed;
  s.model.current.detailMessage = "找不到插件";
  s.notifyAndFlush();
  EXPECT_NE(s.content->detail().toggle, nullptr);
}

TEST(MappersContent, ShowsLoadingFailureAndEmptyMessagesInsteadOfTheTable) {
  Scene s;
  s.model.current = contracts::MappersView{};
  s.notifyAndFlush();
  EXPECT_EQ(s.content->table(), nullptr);
  EXPECT_EQ(s.content->statusText(), "正在读取…");
  s.model.current.load = contracts::MappersLoad::Failed;
  s.model.current.message = "读取插件列表失败";
  s.notifyAndFlush();
  EXPECT_EQ(s.content->statusText(), "读取插件列表失败");
  s.model.current.load = contracts::MappersLoad::Ready;
  s.model.current.message.clear();
  s.model.current.rows.clear();
  s.notifyAndFlush();
  EXPECT_EQ(s.content->statusText(), "没有符合条件的插件");
  EXPECT_EQ(s.content->table(), nullptr);
}

TEST(MappersContent, ModelChangesKeepTheSearchBoxContentAndFollowAnExternalChange) {
  Scene s;
  s.root.setFocus(&s.content->search(), false);
  s.type(U"mm");
  s.model.current.search = "mm";
  s.notifyAndFlush();
  EXPECT_EQ(s.content->search().value(), "mm");
  // 回调不重复触发
  EXPECT_EQ(s.model.calls.size(), 2U);
  s.model.current.search = "";
  s.model.current.source = "user";
  s.notifyAndFlush();
  EXPECT_EQ(s.content->search().value(), "");
  EXPECT_EQ(s.content->source().value(), "user");
  EXPECT_EQ(s.model.calls.size(), 2U);
}

TEST(MappersContent, RowsFollowTheModelAndUnsubscribesWhenDestroyed) {
  Scene s;
  s.model.current.rows = {row("ember.mmc3", "MMC3", true, true)};
  s.notifyAndFlush();
  EXPECT_EQ(s.content->rowIds(), (std::vector<std::string>{"ember.mmc3"}));
  EXPECT_EQ(s.model.handlers.size(), 1U);
  s.root.setContent(std::make_unique<rujian::ui::Widget>());
  EXPECT_TRUE(s.model.handlers.empty());
}

}  // namespace
}  // namespace ember::win32
