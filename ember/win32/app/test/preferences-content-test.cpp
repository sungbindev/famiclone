/*
 * ------------------------------------------------------------------------
 * 名称：preferences-content-test.cpp
 * 说明：首选项对话框内容的测试：左边的分组导航与右边的设置行（默认选中第一个分组）、选择与开关立即提交、输入框回车或失焦时提交并显示校验说明、恢复默认按钮随取值是否与默认不同出现、选择文件夹按钮、
 *       模型变化时原地更新且不冲掉别的输入框里正在输入的内容、“应用”分组里的用户数据目录、销毁后取消订阅。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-20
 * 备注：用控件库的离屏根容器测试，模型是假的；点击与按键用根容器的事件，缩放 1.0。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <map>
#include <memory>

#include "contracts/preferences/preferences-model.hpp"
#include "rujian/ui/root.hpp"
#include "ui/preferences/preferences-content.hpp"

namespace ember::win32 {
namespace {

constexpr int kVirtualKeyReturn = 0x0D;

/** 假的首选项模型：记下操作，内容由测试直接改。 */
class FakePreferences final : public contracts::PreferencesModel {
 public:
  contracts::PreferencesView current;
  std::vector<std::string> calls;
  std::string message;
  std::map<int, std::function<void()>> handlers;

  [[nodiscard]] contracts::PreferencesView view() const override { return current; }
  void setChoice(const std::string& id, const std::string& value) override { calls.push_back("choice " + id + " " + value); }
  void setSwitch(const std::string& id, bool value) override { calls.push_back("switch " + id + (value ? " on" : " off")); }
  std::string submitText(const std::string& id, const std::string& text) override {
    calls.push_back("text " + id + " " + text);
    return message;
  }
  void reset(const std::string& id) override { calls.push_back("reset " + id); }
  void pickDirectory(const std::string& id) override { calls.push_back("pick " + id); }
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

/** 假的用户数据目录模型：记下操作。 */
class FakeUserData final : public contracts::UserDataModel {
 public:
  contracts::UserDataView current{"C:\\Users\\me\\AppData\\Roaming\\Ember", "改用软件所在目录", ""};
  std::vector<std::string> calls;

  [[nodiscard]] contracts::UserDataView view() const override { return current; }
  void openDirectory() override { calls.push_back("open"); }
  void switchPlace() override { calls.push_back("switch"); }
};

contracts::PreferenceRow choiceRow() {
  contracts::PreferenceRow row;
  row.id = "appearance.theme";
  row.label = "主题";
  row.description = "跟随系统时，系统切换深色或浅色后立即生效。";
  row.control = contracts::PreferenceControl::Choice;
  row.options = {{"system", "跟随系统"}, {"dark", "深色"}, {"light", "浅色"}};
  row.text = "system";
  return row;
}

contracts::PreferenceRow switchRow() {
  contracts::PreferenceRow row;
  row.id = "sound.startMuted";
  row.label = "启动时静音";
  row.control = contracts::PreferenceControl::Switch;
  return row;
}

contracts::PreferenceRow integerRow() {
  contracts::PreferenceRow row;
  row.id = "sound.fade";
  row.label = "淡入毫秒";
  row.control = contracts::PreferenceControl::Integer;
  row.text = "10";
  return row;
}

contracts::PreferenceRow textRow() {
  contracts::PreferenceRow row;
  row.id = "sound.note";
  row.label = "备注";
  row.control = contracts::PreferenceControl::Text;
  row.text = "abc";
  row.changed = true;
  return row;
}

contracts::PreferenceRow folderRow() {
  contracts::PreferenceRow row;
  row.id = "printer.outputDirectory";
  row.label = "图片保存位置";
  row.control = contracts::PreferenceControl::Text;
  row.directoryPicker = true;
  return row;
}

struct Scene {
  // 模型要比根容器活得久：控件销毁时还要取消订阅
  FakePreferences preferences;
  FakeUserData userData;
  rujian::ui::Root root{rujian::ui::ThemeName::Dark, 1.0};
  ui::PreferencesContent* content{nullptr};

  Scene() {
    contracts::PreferenceGroupView appearance{"appearance", "外观", {choiceRow()}};
    contracts::PreferenceGroupView sound{"sound", "声音", {switchRow(), integerRow(), textRow()}};
    contracts::PreferenceGroupView printer{"printer", "打印机", {folderRow()}};
    contracts::PreferenceGroupView app{"app", "应用", {switchRow()}};
    preferences.current.load = contracts::PreferencesLoad::Ready;
    preferences.current.groups = {appearance, sound, printer, app};
  }

  void open() {
    auto created = std::make_unique<ui::PreferencesContent>(preferences, userData);
    content = created.get();
    root.setContent(std::move(created));
    root.layout({700, 480});
  }

  void click(const rujian::ui::Widget& widget) {
    const auto& box = widget.bounds();
    const rujian::ui::Point at{box.x + box.width / 2, box.y + box.height / 2};
    root.mouseDown(at, rujian::ui::MouseButton::Left, 0, 0);
    root.mouseUp(at, rujian::ui::MouseButton::Left, 0);
  }

  /** 点左边导航里的第 index 个分组。 */
  void selectGroup(std::size_t index) {
    const auto& box = content->tree().bounds();
    const double rowHeight = box.height / static_cast<double>(preferences.current.groups.size());
    const rujian::ui::Point at{box.x + box.width / 2, box.y + rowHeight * (static_cast<double>(index) + 0.5)};
    root.mouseDown(at, rujian::ui::MouseButton::Left, 0, 0);
    root.mouseUp(at, rujian::ui::MouseButton::Left, 0);
  }

  void type(const std::u32string& text) {
    for (const char32_t c : text) root.character(c);
  }

  [[nodiscard]] ui::PreferencesContent::RowWidgets* row(const std::string& id) const {
    for (const auto& item : content->rows()) {
      if (item->id == id) return item.get();
    }
    return nullptr;
  }
};

TEST(PreferencesContent, ListsGroupsAndShowsTheFirstGroupsRows) {
  Scene s;
  s.open();
  EXPECT_EQ(s.content->selectedGroupId(), "appearance");
  EXPECT_EQ(s.content->tree().selectedId(), "appearance");
  ASSERT_EQ(s.content->rows().size(), 1U);
  EXPECT_NE(s.row("appearance.theme")->select, nullptr);
  EXPECT_EQ(s.row("appearance.theme")->select->value(), "system");
  EXPECT_FALSE(s.row("appearance.theme")->reset->visible());
  // 左栏默认宽度就是最小宽度
  EXPECT_DOUBLE_EQ(s.content->split().sideWidth(), 140);
}

TEST(PreferencesContent, ClickingAnotherGroupSwitchesTheRows) {
  Scene s;
  s.open();
  s.selectGroup(1);
  EXPECT_EQ(s.content->selectedGroupId(), "sound");
  ASSERT_EQ(s.content->rows().size(), 3U);
  EXPECT_NE(s.row("sound.startMuted")->checkbox, nullptr);
  EXPECT_NE(s.row("sound.fade")->input, nullptr);
  EXPECT_EQ(s.row("sound.note")->input->value(), "abc");
  EXPECT_EQ(s.row("appearance.theme"), nullptr);
}

TEST(PreferencesContent, ChoiceAndSwitchSubmitAtOnce) {
  Scene s;
  s.open();
  s.click(*s.row("appearance.theme")->select);
  ASSERT_TRUE(s.row("appearance.theme")->select->open());
  const auto option = s.row("appearance.theme")->select->optionRect(1);
  ASSERT_TRUE(option.has_value());
  const rujian::ui::Point at{option->x + option->width / 2, option->y + option->height / 2};
  s.root.mouseDown(at, rujian::ui::MouseButton::Left, 0, 0);
  s.root.mouseUp(at, rujian::ui::MouseButton::Left, 0);
  EXPECT_EQ(s.preferences.calls, (std::vector<std::string>{"choice appearance.theme dark"}));
  s.preferences.calls.clear();
  s.selectGroup(1);
  s.click(*s.row("sound.startMuted")->checkbox);
  EXPECT_EQ(s.preferences.calls, (std::vector<std::string>{"switch sound.startMuted on"}));
}

TEST(PreferencesContent, ResetButtonShowsOnlyWhenTheValueDiffersFromTheDefault) {
  Scene s;
  s.open();
  s.selectGroup(1);
  EXPECT_FALSE(s.row("sound.fade")->reset->visible());
  ASSERT_TRUE(s.row("sound.note")->reset->visible());
  s.click(*s.row("sound.note")->reset);
  EXPECT_EQ(s.preferences.calls, (std::vector<std::string>{"reset sound.note"}));
  // 模型里取值变了：按钮随之显示与消失
  s.preferences.current.groups[1].rows[2].changed = false;
  s.preferences.current.groups[1].rows[1].changed = true;
  s.preferences.notify();
  EXPECT_FALSE(s.row("sound.note")->reset->visible());
  EXPECT_TRUE(s.row("sound.fade")->reset->visible());
}

TEST(PreferencesContent, EnterSubmitsTheTypedTextAndBlurDoesToo) {
  Scene s;
  s.open();
  s.selectGroup(1);
  s.click(*s.row("sound.fade")->input);
  s.row("sound.fade")->input->setValue("");
  s.type(U"30");
  EXPECT_TRUE(s.row("sound.fade")->dirty);
  s.root.keyDown(kVirtualKeyReturn, 0, false);
  EXPECT_EQ(s.preferences.calls, (std::vector<std::string>{"text sound.fade 30"}));
  // 提交后没有再改过的输入框不再提交（失焦时也一样）
  s.preferences.calls.clear();
  s.click(*s.row("sound.note")->input);
  EXPECT_TRUE(s.preferences.calls.empty());
  // 再改另一个，不按回车直接点别处：失焦时提交
  s.type(U"x");
  s.click(*s.row("sound.fade")->input);
  EXPECT_EQ(s.preferences.calls, (std::vector<std::string>{"text sound.note abcx"}));
}

TEST(PreferencesContent, ValidationMessageMarksTheInputAndIsClearedByTyping) {
  Scene s;
  s.open();
  s.selectGroup(1);
  s.preferences.message = "请输入整数。";
  s.click(*s.row("sound.fade")->input);
  s.type(U"x");
  s.root.keyDown(kVirtualKeyReturn, 0, false);
  EXPECT_EQ(s.row("sound.fade")->error, "请输入整数。");
  EXPECT_TRUE(s.row("sound.fade")->dirty);
  s.type(U"1");
  EXPECT_EQ(s.row("sound.fade")->error, "");
}

TEST(PreferencesContent, SubmittedInputGoesBackToTheModelValueWhenTheModelNotifies) {
  Scene s;
  s.open();
  s.selectGroup(1);
  s.click(*s.row("sound.note")->input);
  s.type(U"zz");
  s.root.keyDown(kVirtualKeyReturn, 0, false);
  ASSERT_EQ(s.preferences.calls.size(), 1U);
  // 保存失败：取值没变，输入框恢复成原值
  s.preferences.notify();
  EXPECT_EQ(s.row("sound.note")->input->value(), "abc");
  // 保存成功：显示新值
  s.click(*s.row("sound.note")->input);
  s.type(U"q");
  s.root.keyDown(kVirtualKeyReturn, 0, false);
  s.preferences.current.groups[1].rows[2].text = "abcq";
  s.preferences.notify();
  EXPECT_EQ(s.row("sound.note")->input->value(), "abcq");
}

TEST(PreferencesContent, UnrelatedChangesDoNotWipeWhatIsBeingTyped) {
  Scene s;
  s.open();
  s.selectGroup(1);
  s.click(*s.row("sound.note")->input);
  s.type(U"123");
  s.preferences.current.groups[1].rows[0].checked = true;
  s.preferences.notify();
  EXPECT_EQ(s.row("sound.note")->input->value(), "abc123");
  EXPECT_TRUE(s.row("sound.startMuted")->checkbox->checked());
  // 同一行的取值在别处变了：输入框跟着变
  s.preferences.current.groups[1].rows[2].text = "别处改的";
  s.preferences.notify();
  EXPECT_EQ(s.row("sound.note")->input->value(), "别处改的");
}

TEST(PreferencesContent, DirectoryPickerButtonOnlyOnRowsThatDeclareIt) {
  Scene s;
  s.open();
  s.selectGroup(1);
  EXPECT_EQ(s.row("sound.note")->pick, nullptr);
  s.selectGroup(2);
  ASSERT_NE(s.row("printer.outputDirectory")->pick, nullptr);
  s.click(*s.row("printer.outputDirectory")->pick);
  EXPECT_EQ(s.preferences.calls, (std::vector<std::string>{"pick printer.outputDirectory"}));
}

TEST(PreferencesContent, RegenerateRowHasAReplaceButtonAndNeverShowsReset) {
  Scene s;
  contracts::PreferenceRow token;
  token.id = "debug.token";
  token.label = "令牌";
  token.control = contracts::PreferenceControl::Text;
  token.text = "abc123";
  token.regenerates = true;
  s.preferences.current.groups[1].rows.push_back(token);
  s.open();
  s.selectGroup(1);
  EXPECT_EQ(s.row("sound.note")->regenerate, nullptr);
  ASSERT_NE(s.row("debug.token")->regenerate, nullptr);
  EXPECT_FALSE(s.row("debug.token")->reset->visible());
  s.click(*s.row("debug.token")->regenerate);
  EXPECT_EQ(s.preferences.calls, (std::vector<std::string>{"reset debug.token"}));
  // 取值与默认值不同（令牌总是有值）也不显示“恢复默认”
  s.preferences.current.groups[1].rows.back().text = "def456";
  s.preferences.notify();
  EXPECT_FALSE(s.row("debug.token")->reset->visible());
}

TEST(PreferencesContent, AppGroupEndsWithTheUserDataRowAndOtherGroupsHaveNone) {
  Scene s;
  s.open();
  EXPECT_EQ(s.content->userDataLink(), nullptr);
  s.selectGroup(3);
  EXPECT_EQ(s.content->selectedGroupId(), "app");
  ASSERT_EQ(s.content->rows().size(), 1U);
  ASSERT_NE(s.content->userDataLink(), nullptr);
  ASSERT_NE(s.content->userDataSwitch(), nullptr);
  EXPECT_TRUE(s.content->userDataSwitch()->enabled());
  s.selectGroup(0);
  EXPECT_EQ(s.content->userDataLink(), nullptr);
}

TEST(PreferencesContent, UserDataLinkOpensTheDirectoryAndTheButtonSwitches) {
  Scene s;
  s.open();
  s.selectGroup(3);
  s.click(*s.content->userDataLink());
  s.click(*s.content->userDataSwitch());
  EXPECT_EQ(s.userData.calls, (std::vector<std::string>{"open", "switch"}));
}

TEST(PreferencesContent, UserDataLinkAndButtonAreOnTwoLinesAlignedAtTheLeft) {
  Scene s;
  s.open();
  s.selectGroup(3);
  const auto& link = s.content->userDataLink()->bounds();
  const auto& button = s.content->userDataSwitch()->bounds();
  EXPECT_GE(button.y, link.bottom());
  EXPECT_DOUBLE_EQ(button.x, link.x);
}

TEST(PreferencesContent, UserDataSwitchIsDisabledWhenTheModelGivesAReason) {
  Scene s;
  s.userData.current.unavailableReason = "软件所在的目录不能写入";
  s.open();
  s.selectGroup(3);
  EXPECT_FALSE(s.content->userDataSwitch()->enabled());
}

TEST(PreferencesContent, NothingToShowWhileLoading) {
  Scene s;
  s.preferences.current = contracts::PreferencesView{};
  s.open();
  EXPECT_EQ(s.content->selectedGroupId(), "");
  EXPECT_TRUE(s.content->rows().empty());
  // 读到之后出现分组，默认选中第一个
  s.preferences.current.load = contracts::PreferencesLoad::Ready;
  s.preferences.current.groups = {{"appearance", "外观", {choiceRow()}}};
  s.preferences.notify();
  EXPECT_EQ(s.content->selectedGroupId(), "appearance");
  EXPECT_EQ(s.content->rows().size(), 1U);
}

TEST(PreferencesContent, SelectedGroupFallsBackToTheFirstWhenItDisappears) {
  Scene s;
  s.open();
  s.selectGroup(2);
  ASSERT_EQ(s.content->selectedGroupId(), "printer");
  s.preferences.current.groups.erase(s.preferences.current.groups.begin() + 2);
  s.preferences.notify();
  EXPECT_EQ(s.content->selectedGroupId(), "appearance");
}

TEST(PreferencesContent, DestroyingUnsubscribesFromTheModel) {
  Scene s;
  s.open();
  EXPECT_EQ(s.preferences.handlers.size(), 1U);
  s.root.setContent(nullptr);
  EXPECT_TRUE(s.preferences.handlers.empty());
}

TEST(PreferencesContent, RowsStayBesideTheTextInANarrowWindow) {
  Scene s;
  s.open();
  s.root.layout({600, 480});
  const auto* theme = s.row("appearance.theme");
  const auto& rowBox = theme->row->bounds();
  const auto& select = theme->select->bounds();
  // 控件靠行的右边，和名称在同一行，不上下排列
  EXPECT_NEAR(select.right(), rowBox.right(), 0.01);
  EXPECT_GT(select.x, rowBox.x + rowBox.width / 2);
  EXPECT_LT(select.y, rowBox.bottom());
  EXPECT_LT(select.bottom(), rowBox.bottom());
}

TEST(PreferencesContent, MinimumSizeKeepsEveryRowsControlsOnOneLine) {
  Scene s;
  s.open();
  const rujian::ui::Size minimum = s.content->minimumSize();
  EXPECT_GT(minimum.width, s.content->split().sideWidth());
  EXPECT_GT(minimum.height, 0);
  s.root.layout({minimum.width, minimum.height});
  for (std::size_t group = 0; group < s.preferences.current.groups.size(); ++group) {
    s.selectGroup(group);
    for (const auto& state : s.content->rows()) {
      // 行里显示出来的控件竖直方向的中心都在同一条线上（换行了就不在）
      double center = -1;
      for (const rujian::ui::Widget* control : {static_cast<const rujian::ui::Widget*>(state->select), static_cast<const rujian::ui::Widget*>(state->checkbox), static_cast<const rujian::ui::Widget*>(state->input),
                                                  static_cast<const rujian::ui::Widget*>(state->pick), static_cast<const rujian::ui::Widget*>(state->reset)}) {
        if (control == nullptr || !control->visible()) continue;
        const double middle = control->bounds().y + control->bounds().height / 2.0;
        if (center < 0) center = middle;
        EXPECT_NEAR(middle, center, 0.5) << state->id;
      }
      // 行没有被挤成上下两部分：控件在行的右边
      EXPECT_GT(state->row->bounds().right(), state->row->bounds().x + 100) << state->id;
    }
  }
}

TEST(PreferencesContent, MinimumSizeCoversThePageWithTheResetButtonShown) {
  Scene s;
  s.open();
  const double without = s.content->minimumSize().width;
  // 某一页的行显示“恢复默认”后更宽：最小宽度按它们都显示算，已经包含
  s.preferences.current.groups[0].rows[0].changed = true;
  s.preferences.notify();
  EXPECT_DOUBLE_EQ(s.content->minimumSize().width, without);
}

}  // namespace
}  // namespace ember::win32
