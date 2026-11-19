/*
 * ------------------------------------------------------------------------
 * 名称：about-content.cpp
 * 说明：“关于 Ember”对话框内容的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：无。
 * ------------------------------------------------------------------------
 */

#include "ui/system/about-content.hpp"

#include "rujian/ui/facts.hpp"
#include "rujian/ui/label.hpp"

namespace ember::win32::ui {

namespace {

using namespace rujian::ui;

/** 用户可见的文字。 */
constexpr const char* kVersion = "版本";
constexpr const char* kCoreVersion = "核心版本";
constexpr const char* kBuildTime = "编译时间";
constexpr const char* kAuthor = "作者";
constexpr const char* kEmail = "邮箱";
constexpr const char* kQq = "QQ";
constexpr const char* kProject = "项目地址";
constexpr const char* kLicense = "许可证";
constexpr const char* kLoading = "正在读取…";
constexpr const char* kFailed = "读取失败";
constexpr const char* kThirdParty = "第三方依赖";
constexpr const char* kThirdPartyHint = "Ember 使用了下列开源项目，各项目的许可全文随程序分发。";
constexpr const char* kColumnName = "名称与版本";
constexpr const char* kColumnAuthors = "作者";
constexpr const char* kColumnLicense = "许可";

/** 标志的边长（逻辑像素）。 */
constexpr double kLogoSize = 48;
/** 表格列的最小宽度。 */
constexpr double kNameMinWidth = 160;
constexpr double kAuthorsMinWidth = 120;

/** 固定大小的容器：子控件的大小固定。 */
class FixedSize final : public Widget {
 public:
  FixedSize(std::unique_ptr<Widget> child, double size) : size_(size) { child_ = addChild(std::move(child)); }

  [[nodiscard]] Size measure(Size) override { return {size_, size_}; }
  void arrange(const Rect& bounds) override {
    setBounds(bounds);
    child_->arrange(bounds);
  }
  [[nodiscard]] bool stretchesWidth() const override { return false; }
  [[nodiscard]] bool stretchesHeight() const override { return false; }

 private:
  double size_;
  Widget* child_{nullptr};
};

/** 版本在一行里显示的文字。 */
std::string versionsOf(const contracts::AboutView& view) {
  switch (view.versionLoad) {
    case contracts::AboutVersionLoad::Loading:
      return std::string(kVersion) + " " + kLoading + "　" + kCoreVersion + " " + kLoading;
    case contracts::AboutVersionLoad::Failed:
      return std::string(kVersion) + " " + kFailed + "　" + kCoreVersion + " " + kFailed;
    case contracts::AboutVersionLoad::Ready:
      break;
  }
  return std::string(kVersion) + " " + view.appVersion + "　" + kCoreVersion + " " + view.coreVersion;
}

}  // namespace

AboutContent::AboutContent(contracts::AboutModel& model, std::unique_ptr<Widget> logo) : model_(model) {
  const contracts::AboutView view = model_.view();
  auto stack = std::make_unique<Stack>();

  auto title = std::make_unique<Row>();
  if (logo) title->addChild(std::make_unique<FixedSize>(std::move(logo), kLogoSize));
  auto name = std::make_unique<Stack>();
  name->addChild(std::make_unique<Heading>(view.appName));
  auto versions = std::make_unique<DescriptionRow>();
  versions->setText(versionsOf(view));
  shownVersions_ = versions->text();
  versions_ = static_cast<DescriptionRow*>(name->addChild(std::move(versions)));
  title->addChild(std::move(name));
  stack->addChild(std::move(title));

  auto facts = std::make_unique<Facts>();
  facts->addFact(kBuildTime, view.buildTime);
  facts->addFact(kAuthor, view.author);
  facts->addFact(kEmail, view.email);
  facts->addFact(kQq, view.qq);
  auto link = std::make_unique<Button>(view.projectUrl);
  link->setVariant(ButtonVariant::Link);
  link->setOnClick([this] { model_.openProject(); });
  projectLink_ = link.get();
  facts->addFact(kProject, std::move(link));
  facts->addFact(kLicense, view.license);
  stack->addChild(std::move(facts));

  stack->addChild(std::make_unique<Heading>(kThirdParty));
  auto hint = std::make_unique<DescriptionRow>();
  hint->setText(kThirdPartyHint);
  stack->addChild(std::move(hint));

  auto table = std::make_unique<Table>();
  table->addColumn({kColumnName, kNameMinWidth, false, false});
  table->addColumn({kColumnAuthors, kAuthorsMinWidth, false, true});
  table->addColumn({kColumnLicense, 0, true, true});
  for (const contracts::ThirdPartyNotice& notice : view.notices) {
    std::vector<std::unique_ptr<Widget>> cells;
    auto nameCell = Table::makeText(notice.name + " " + notice.version);
    nameCell->setTooltip(notice.usage);
    cells.push_back(std::move(nameCell));
    cells.push_back(Table::makeText(notice.authors));
    cells.push_back(Table::makeText(notice.license));
    table->addRow(std::move(cells));
  }
  table_ = static_cast<Table*>(stack->addChild(std::move(table)));

  auto scroll = std::make_unique<ScrollArea>();
  scroll->setContent(std::move(stack));
  scroll_ = static_cast<ScrollArea*>(addChild(std::move(scroll)));

  unsubscribe_ = model_.subscribe([this] { refresh(); });
}

AboutContent::~AboutContent() {
  if (unsubscribe_) unsubscribe_();
}

Size AboutContent::measure(Size available) { return scroll_->measure(available); }

void AboutContent::arrange(const Rect& bounds) {
  setBounds(bounds);
  scroll_->arrange(bounds);
}

void AboutContent::refresh() {
  const std::string text = versionsOf(model_.view());
  if (text == shownVersions_) return;
  shownVersions_ = text;
  versions_->setText(text);
  requestLayout();
}

}  // namespace ember::win32::ui
