/*
 * ------------------------------------------------------------------------
 * 名称：about-content-test.cpp
 * 说明：“关于 Ember”对话框内容的测试：版本一行在读取中、读取失败与读到时的文字，模型变化后跟着更新，点项目地址链接调用模型，两个网址链接在最小宽度下不折行，没有标志时照常显示，销毁后取消订阅。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：用控件库的离屏根容器测试，模型是假的。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <map>
#include <memory>

#include "contracts/system/about-model.hpp"
#include "rujian/ui/root.hpp"
#include "ui/system/about-content.hpp"

namespace ember::win32 {
namespace {

/** 假的关于模型：记下操作，内容由测试直接改。 */
class FakeAbout final : public contracts::AboutModel {
 public:
  contracts::AboutView current;
  int projectOpens{0};
  int homepageOpens{0};
  std::map<int, std::function<void()>> handlers;

  [[nodiscard]] contracts::AboutView view() const override { return current; }
  void openHomepage() override { ++homepageOpens; }
  void openProject() override { ++projectOpens; }
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

struct Scene {
  // 模型要比根容器活得久：控件销毁时还要取消订阅
  FakeAbout model;
  rujian::ui::Root root{rujian::ui::ThemeName::Dark, 1.0};
  ui::AboutContent* content{nullptr};

  explicit Scene(bool withLogo = true) {
    model.current.appName = "Ember";
    model.current.buildTime = "2026-11-21 10:00:00";
    model.current.author = "sungbin";
    model.current.email = "sungbin@3578.cn";
    model.current.qq = "46427991";
    model.current.homepageUrl = "https://fcdev.cn";
    model.current.projectUrl = "https://github.com/sungbindev/famiclone/tree/master/ember";
    model.current.license = "GPL-3.0";
    model.current.notices = {{"Zlib", "1.0", "A、B", "MIT", "https://z", "压缩"}, {"Tabler Icons", "3.0", "Paweł", "MIT", "https://t", "图标"}};
    auto created = std::make_unique<ui::AboutContent>(model, withLogo ? std::make_unique<rujian::ui::Widget>() : nullptr);
    content = created.get();
    root.setContent(std::move(created));
    root.layout({520, 560});
  }

  void click(const rujian::ui::Widget& widget) {
    const auto& box = widget.bounds();
    const rujian::ui::Point at{box.x + box.width / 2, box.y + box.height / 2};
    root.mouseDown(at, rujian::ui::MouseButton::Left, 0, 0);
    root.mouseUp(at, rujian::ui::MouseButton::Left, 0);
  }
};

TEST(AboutContent, VersionsShowLoadingFailedAndReadStates) {
  Scene s;
  EXPECT_EQ(s.content->versionsText(), "版本 正在读取…　核心版本 正在读取…");
  s.model.current.versionLoad = contracts::AboutVersionLoad::Failed;
  s.model.notify();
  EXPECT_EQ(s.content->versionsText(), "版本 读取失败　核心版本 读取失败");
  s.model.current.versionLoad = contracts::AboutVersionLoad::Ready;
  s.model.current.appVersion = "0.3.0";
  s.model.current.coreVersion = "0.3.1";
  s.model.notify();
  EXPECT_EQ(s.content->versionsText(), "版本 0.3.0　核心版本 0.3.1");
}

TEST(AboutContent, BuildTimeSitsUnderTheVersionsAtTheTopRight) {
  Scene s;
  EXPECT_EQ(s.content->buildTimeText(), "编译时间 2026-11-21 10:00:00");
}

TEST(AboutContent, ProjectLinkOpensTheProjectThroughTheModel) {
  Scene s;
  s.click(s.content->projectLink());
  EXPECT_EQ(s.model.projectOpens, 1);
}

TEST(AboutContent, HomepageLinkOpensTheHomepageAndSitsAboveTheProjectLink) {
  Scene s;
  EXPECT_LT(s.content->homepageLink().bounds().y, s.content->projectLink().bounds().y);
  s.click(s.content->homepageLink());
  EXPECT_EQ(s.model.homepageOpens, 1);
  EXPECT_EQ(s.model.projectOpens, 0);
}

TEST(AboutContent, ProjectLinkIsAsWideAsItsTextAndNothingIsFocusedAtFirst) {
  Scene s;
  const auto& link = s.content->projectLink();
  // 链接的焦点框与点击区域贴着文字，不拉到一行的右边
  EXPECT_LT(link.bounds().right(), 500.0);
  EXPECT_FALSE(link.focused());
  EXPECT_EQ(s.root.focusedWidget(), nullptr);
}

TEST(AboutContent, WorksWithoutALogoAndUnsubscribesWhenDestroyed) {
  Scene s(false);
  EXPECT_EQ(s.model.handlers.size(), 1U);
  EXPECT_GT(s.content->table().bounds().height, 0);
  s.root.setContent(std::make_unique<rujian::ui::Widget>());
  EXPECT_TRUE(s.model.handlers.empty());
}

TEST(AboutContent, MinimumWidthFitsTheHeaderAndTheFactsAndHasNoHeightRequirement) {
  Scene s;
  const rujian::ui::Size minimum = s.content->minimumSize();
  EXPECT_GT(minimum.width, 200);
  EXPECT_DOUBLE_EQ(minimum.height, 0);
  s.root.layout({minimum.width, 560});
  EXPECT_LE(s.content->projectLink().bounds().right(), minimum.width);
}

TEST(AboutContent, LinksStayOnOneLineAtTheMinimumWidth) {
  Scene s;
  const double lineHeight = s.content->homepageLink().bounds().height;
  EXPECT_GT(lineHeight, 0);
  s.root.layout({s.content->minimumSize().width, 560});
  EXPECT_DOUBLE_EQ(s.content->homepageLink().bounds().height, lineHeight);
  EXPECT_DOUBLE_EQ(s.content->projectLink().bounds().height, lineHeight);
}

}  // namespace
}  // namespace ember::win32
