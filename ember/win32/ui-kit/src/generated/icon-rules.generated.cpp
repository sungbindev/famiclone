/*
 * ------------------------------------------------------------------------
 * 名称：icon-rules.generated.cpp
 * 说明：按文字含义匹配图标的规则表，顺序与组件库 icon-rules.ts 里的内置规则相同，先匹配先得。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：由 tools/gen-icons.mjs 生成，不要手改；图标图形来自 Tabler Icons（MIT，见 third_party/tabler-icons）。
 * ------------------------------------------------------------------------
 */

#include "base/icon-rules.hpp"

namespace rujian::ui {

const std::vector<LabelIconRule>& labelIconRules() {
  static const std::vector<LabelIconRule> rules = {
      {"^上一步", IconId::ArrowLeft, false},
      {"^下一步", IconId::ArrowRight, true},
      {"^前移", IconId::ArrowLeft, false},
      {"^后移", IconId::ArrowRight, true},
      {"^上移", IconId::ArrowUp, false},
      {"^下移", IconId::ArrowDown, false},
      {"^展开", IconId::ChevronDown, false},
      {"^收起", IconId::ChevronUp, false},
      {"^重启应用", IconId::Refresh, false},
      {"^设置$", IconId::Settings, false},
      {"^保存", IconId::DeviceFloppy, false},
      {"^(确认采用|采用|选用)", IconId::CircleCheck, false},
      {"^(确定|确认|已保存|已采用|已选用)$", IconId::Check, false},
      {"^覆盖", IconId::Replace, false},
      {"^改用上传", IconId::Upload, false},
      {"^改用生成", IconId::Sparkles, false},
      {"^改用", IconId::Replace, false},
      {"^(放弃修改|恢复沿用)", IconId::ArrowBackUp, false},
      {"^继续编辑", IconId::Pencil, false},
      {"^继续", IconId::ArrowRight, true},
      {"^保留", IconId::Check, false},
      {"^取消生成", IconId::PlayerStop, false},
      {"^(取消|关闭)", IconId::X, false},
      {"^重试", IconId::Reload, false},
      {"^重新(生成|抽取)", IconId::Refresh, false},
      {"^(开始生成|AI 生成|创建并生成|生成)", IconId::Sparkles, false},
      {"生成$", IconId::Sparkles, false},
      {"^提交", IconId::Send, false},
      {"^查看", IconId::Eye, false},
      {"^对比", IconId::ArrowsDiff, false},
      {"^(播放|试听)", IconId::PlayerPlay, false},
      {"^停止", IconId::PlayerStop, false},
      {"^导出", IconId::FileExport, false},
      {"^在文件夹中显示", IconId::FolderOpen, false},
      {"^解除", IconId::Unlink, false},
      {"^(选择|再选一个)$", IconId::HandClick, false},
      {"^分类管理", IconId::Category, false},
      {"^(管理|调整)", IconId::Adjustments, false},
      {"^(添加文件|选择文件)", IconId::FileUpload, false},
      {"^全选", IconId::SelectAll, false},
      {"^仅未完成", IconId::Filter, false},
      {"^(清空|清除)", IconId::Eraser, false},
      {"^(添加|新建|创建|仅创建)", IconId::Plus, false},
      {"^(修改|编辑)", IconId::Pencil, false},
      {"^(删除|移除)", IconId::Trash, false},
  };
  return rules;
}

}  // namespace rujian::ui
