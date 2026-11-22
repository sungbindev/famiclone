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

#include <algorithm>

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
constexpr const char* kHomepage = "主页地址";
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
/** 应用名的字号与字重。 */
constexpr double kNameFontSize = 28;
constexpr int kNameFontWeight = 600;
/** 标志、应用名、右侧信息之间的间距，以及头部右边的留白（不贴着右边）。 */
constexpr double kHeaderGap = 16;
constexpr double kHeaderRightPadding = 14;
/** 右侧两行（版本、编译时间）之间的间距。 */
constexpr double kInfoGap = 4;
/** 内容右边的留白（滚动条贴着对话框的右边，内容自己空出来，要比滚动条的轨道宽一些，网址才不会被挡住）。 */
constexpr double kContentRightPadding = 24;
/** 上面的固定部分与下面表格之间的间距，以及表格区至少要露出的高度（表头加两行多一点）。 */
constexpr double kTableGap = 10;
constexpr double kTableMinHeight = 110;
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
  auto top = std::make_unique<Stack>();
  top->setPadding(Insets{0, kContentRightPadding, 0, 0});

  // 头部：标志、应用名（大字，和标志垂直居中），右边靠右对齐两行：版本与核心版本、编译时间
  auto title = std::make_unique<Row>();
  title->setGap(kHeaderGap);
  title->setPadding(Insets{0, kHeaderRightPadding, 0, 0});
  if (logo) title->addChild(std::make_unique<FixedSize>(std::move(logo), kLogoSize));
  auto name = std::make_unique<Label>(view.appName);
  name->setFontSize(kNameFontSize);
  name->setFontWeight(kNameFontWeight);
  name->setWrap(false);
  title->addChild(std::move(name));
  auto info = std::make_unique<Stack>();
  info->setGap(kInfoGap);
  auto versions = std::make_unique<DescriptionRow>();
  versions->setText(versionsOf(view));
  versions->setTextAlign(TextAlign::End);
  shownVersions_ = versions->text();
  versions_ = static_cast<DescriptionRow*>(info->addChild(std::move(versions)));
  auto buildTime = std::make_unique<DescriptionRow>();
  buildTime->setText(std::string(kBuildTime) + " " + view.buildTime);
  buildTime->setTextAlign(TextAlign::End);
  buildTime_ = static_cast<DescriptionRow*>(info->addChild(std::move(buildTime)));
  Widget* infoRaw = title->addChild(std::move(info));
  title->setGrow(infoRaw, 1);
  header_ = top->addChild(std::move(title));

  auto facts = std::make_unique<Facts>();
  facts->addFact(kAuthor, view.author);
  facts->addFact(kEmail, view.email);
  facts->addFact(kQq, view.qq);
  auto homepage = std::make_unique<Button>(view.homepageUrl);
  homepage->setVariant(ButtonVariant::Link);
  homepage->setWrap(false);
  homepage->setOnClick([this] { model_.openHomepage(); });
  homepageLink_ = homepage.get();
  facts->addFact(kHomepage, std::move(homepage));
  auto link = std::make_unique<Button>(view.projectUrl);
  link->setVariant(ButtonVariant::Link);
  link->setWrap(false);
  link->setOnClick([this] { model_.openProject(); });
  projectLink_ = link.get();
  facts->addFact(kProject, std::move(link));
  facts->addFact(kLicense, view.license);
  facts_ = top->addChild(std::move(facts));

  top->addChild(std::make_unique<Heading>(kThirdParty));
  auto hint = std::make_unique<DescriptionRow>();
  hint->setText(kThirdPartyHint);
  top->addChild(std::move(hint));
  top_ = addChild(std::move(top));

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
  // 只有表格滚动：上面的头部、信息与说明固定，滚动条贴着对话框的右边，表格自己空出右边的留白
  auto tableBox = std::make_unique<Stack>();
  tableBox->setPadding(Insets{0, kContentRightPadding, 0, 0});
  table_ = static_cast<Table*>(tableBox->addChild(std::move(table)));

  auto scroll = std::make_unique<ScrollArea>();
  scroll->setContent(std::move(tableBox));
  scroll_ = static_cast<ScrollArea*>(addChild(std::move(scroll)));

  unsubscribe_ = model_.subscribe([this] { refresh(); });
}

AboutContent::~AboutContent() {
  if (unsubscribe_) unsubscribe_();
}

Size AboutContent::measure(Size available) {
  const Size limit{available.width, 0};
  return Size{available.width > 0 ? available.width : top_->measure(limit).width, top_->measure(limit).height + kTableGap + scroll_->measure(limit).height};
}

Size AboutContent::minimumSize() {
  if (host() == nullptr) return Size{};
  const double widest = std::max({header_->measure(Size{}).width, facts_->measure(Size{}).width, table_->minimumSize().width});
  const double width = widest + kContentRightPadding;
  return Size{width, top_->measure(Size{width, 0}).height + kTableGap + kTableMinHeight};
}

void AboutContent::arrange(const Rect& bounds) {
  setBounds(bounds);
  const double topHeight = top_->measure(Size{bounds.width, 0}).height;
  top_->arrange(Rect{bounds.x, bounds.y, bounds.width, topHeight});
  const double tableTop = bounds.y + topHeight + kTableGap;
  scroll_->arrange(Rect{bounds.x, tableTop, bounds.width, std::max(0.0, bounds.bottom() - tableTop)});
}

void AboutContent::refresh() {
  const std::string text = versionsOf(model_.view());
  if (text == shownVersions_) return;
  shownVersions_ = text;
  versions_->setText(text);
  requestLayout();
}

}  // namespace ember::win32::ui
