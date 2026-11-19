/*
 * ------------------------------------------------------------------------
 * 名称：about-model.hpp
 * 说明：“关于 Ember”对话框的模型：应用名称、版本与核心版本（读取状态）、编译时间、作者、邮箱、QQ、项目地址、许可证与第三方依赖清单，以及用户的操作（打开项目地址）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-21
 * 备注：第三方依赖的清单由构建脚本从各项目的声明生成，由组装根传入；版本经核心的 system.version 读取，读不到时显示“读取失败”，不用通知；规则见 docs/05-界面.md 第 8 节。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <functional>
#include <string>
#include <vector>

namespace ember::win32::contracts {

/** 一项第三方开源代码的声明。 */
struct ThirdPartyNotice {
  std::string name;
  std::string version;
  /** 作者，多个作者已用“、”连起来。 */
  std::string authors;
  std::string license;
  std::string homepage;
  /** 用途，表格里名称一列的悬停提示。 */
  std::string usage;
  bool operator==(const ThirdPartyNotice&) const = default;
};

/** 版本的读取状态。 */
enum class AboutVersionLoad { Loading, Ready, Failed };

/** 关于对话框此刻要显示的内容。 */
struct AboutView {
  std::string appName;
  AboutVersionLoad versionLoad{AboutVersionLoad::Loading};
  std::string appVersion;
  std::string coreVersion;
  std::string buildTime;
  std::string author;
  std::string email;
  std::string qq;
  std::string projectUrl;
  std::string license;
  std::vector<ThirdPartyNotice> notices;
  bool operator==(const AboutView&) const = default;
};

/** 关于对话框的模型。 */
class AboutModel {
 public:
  virtual ~AboutModel() = default;

  [[nodiscard]] virtual AboutView view() const = 0;

  /** 用系统默认浏览器打开项目地址，打不开时用通知给出原因。 */
  virtual void openProject() = 0;

  /** 订阅内容变化；返回取消订阅的函数。 */
  [[nodiscard]] virtual std::function<void()> subscribe(std::function<void()> handler) = 0;
};

}  // namespace ember::win32::contracts
