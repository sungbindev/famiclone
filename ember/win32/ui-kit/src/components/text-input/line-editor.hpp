/*
 * ------------------------------------------------------------------------
 * 名称：line-editor.hpp
 * 说明：单行文字编辑的纯逻辑：文字、光标与选区、插入与删除、按字符与按词移动、词选择、撤销与重做。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：位置都是 UTF-16 单元序号，不会落在代理对或带组合符号的字符中间；不依赖绘制与系统接口，可以直接测试。
 *       撤销规则：连续输入合并成一次，连续向后删除合并成一次，连续向前删除合并成一次；移动光标、改变选区或做其他编辑后重新开始；
 *       重做栈在发生新的编辑时清空。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace rujian::ui {

/** 单行文字编辑器。 */
class LineEditor {
 public:
  /** 取全部文字。 */
  [[nodiscard]] const std::u16string& text() const { return text_; }
  /** 光标位置（选区活动端）。 */
  [[nodiscard]] std::size_t caret() const { return caret_; }
  /** 选区固定端；没有选区时等于光标位置。 */
  [[nodiscard]] std::size_t anchor() const { return anchor_; }
  [[nodiscard]] bool hasSelection() const { return caret_ != anchor_; }
  [[nodiscard]] std::size_t selectionStart() const { return caret_ < anchor_ ? caret_ : anchor_; }
  [[nodiscard]] std::size_t selectionEnd() const { return caret_ < anchor_ ? anchor_ : caret_; }
  [[nodiscard]] std::u16string selectedText() const { return text_.substr(selectionStart(), selectionEnd() - selectionStart()); }

  /**
   * 整体换成新文字（来自外部的值变化）：光标移到末尾，不进撤销栈，并清空撤销与重做历史。
   * @return 文字与原来不同时为 true。
   */
  bool setText(std::u16string text);

  /** 用 text 替换选区（没有选区就在光标处插入）；返回文字是否变化。控制字符会被去掉，换行与制表符换成空格。 */
  bool insert(std::u16string_view text);
  /** 向前删一个字符，有选区时删选区。 */
  bool deleteBackward();
  /** 向后删一个字符，有选区时删选区。 */
  bool deleteForward();
  /** 向前删到上一个词的开头。 */
  bool deleteWordBackward();
  /** 向后删到下一个词的开头。 */
  bool deleteWordForward();
  /** 删除选区；没有选区时什么也不做。 */
  bool deleteSelection();

  /** 向左移一个字符或一个词；extend 为真时扩展选区。有选区且不扩展时光标落到选区左端。 */
  void moveLeft(bool extend, bool byWord);
  /** 向右移一个字符或一个词；规则同 moveLeft。 */
  void moveRight(bool extend, bool byWord);
  void moveToStart(bool extend);
  void moveToEnd(bool extend);
  /** 把光标移到指定位置（会对齐到字符边界）；extend 为真时保留固定端。 */
  void moveTo(std::size_t position, bool extend);
  void selectAll();
  /** 选中 position 处的词（连续的同类字符；汉字与假名每个字算一个词）。 */
  void selectWordAt(std::size_t position);

  [[nodiscard]] bool canUndo() const { return !undoStack_.empty(); }
  [[nodiscard]] bool canRedo() const { return !redoStack_.empty(); }
  bool undo();
  bool redo();

 private:
  /** 一次编辑前的状态。 */
  struct State {
    std::u16string text;
    std::size_t caret;
    std::size_t anchor;
  };
  /** 上一次编辑的类型，用于合并撤销。 */
  enum class Kind { None, Typing, DeleteBackward, DeleteForward, Other };

  [[nodiscard]] std::size_t previousBoundary(std::size_t position) const;
  [[nodiscard]] std::size_t nextBoundary(std::size_t position) const;
  [[nodiscard]] std::size_t previousWord(std::size_t position) const;
  [[nodiscard]] std::size_t nextWord(std::size_t position) const;
  /** 记录撤销快照；kind 与上一次相同且可合并时不记录。 */
  void record(Kind kind);
  /** 把 [start, end) 换成 replacement，光标落在替换内容之后。 */
  void replace(std::size_t start, std::size_t end, std::u16string_view replacement);
  void clamp();

  std::u16string text_;
  std::size_t caret_{0};
  std::size_t anchor_{0};
  Kind lastKind_{Kind::None};
  std::vector<State> undoStack_;
  std::vector<State> redoStack_;
};

}  // namespace rujian::ui
