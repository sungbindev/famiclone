/*
 * ------------------------------------------------------------------------
 * 名称：svg-path.cpp
 * 说明：SVG 路径解析的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：解析器只处理图标数据这类可信输入，但仍对语法错误返回空值，不抛异常。
 * ------------------------------------------------------------------------
 */

#include "base/svg-path.hpp"

#include <cctype>
#include <charconv>

namespace rujian::ui {

namespace {

/** 路径解析的游标：保存剩余文本与当前位置。 */
class Cursor {
 public:
  explicit Cursor(std::string_view text) : text_(text) {}

  /** 跳过空白与逗号。 */
  void skipSeparators() {
    while (pos_ < text_.size() && (std::isspace(static_cast<unsigned char>(text_[pos_])) != 0 || text_[pos_] == ',')) ++pos_;
  }

  [[nodiscard]] bool atEnd() const { return pos_ >= text_.size(); }
  [[nodiscard]] char peek() const { return text_[pos_]; }
  void advance() { ++pos_; }

  /** 读一个数字；失败时返回 false。 */
  bool readNumber(double& value) {
    skipSeparators();
    if (atEnd()) return false;
    std::size_t begin = pos_;
    if (text_[begin] == '+') ++begin;
    const char* first = text_.data() + begin;
    const char* last = text_.data() + text_.size();
    const auto result = std::from_chars(first, last, value);
    if (result.ec != std::errc{}) return false;
    pos_ = static_cast<std::size_t>(result.ptr - text_.data());
    return true;
  }

  /** 读一个标志位（单个 0 或 1 字符）。 */
  bool readFlag(bool& flag) {
    skipSeparators();
    if (atEnd() || (text_[pos_] != '0' && text_[pos_] != '1')) return false;
    flag = text_[pos_] == '1';
    ++pos_;
    return true;
  }

 private:
  std::string_view text_;
  std::size_t pos_{0};
};

/** 命令需要的参数个数；不是命令字母时返回 0。 */
int argumentCount(char command) {
  switch (std::tolower(static_cast<unsigned char>(command))) {
    case 'm':
    case 'l':
    case 't':
      return 2;
    case 'h':
    case 'v':
      return 1;
    case 'c':
      return 6;
    case 's':
    case 'q':
      return 4;
    case 'a':
      return 7;
    default:
      return 0;
  }
}

}  // namespace

std::optional<std::vector<PathSegment>> parseSvgPath(std::string_view data) {
  Cursor cursor(data);
  std::vector<PathSegment> segments;
  Point current{};
  Point start{};
  Point lastControl{};
  char command = 0;
  char previous = 0;

  while (true) {
    cursor.skipSeparators();
    if (cursor.atEnd()) break;
    const char next = cursor.peek();
    if (std::isalpha(static_cast<unsigned char>(next)) != 0) {
      cursor.advance();
      command = next;
      if (command == 'Z' || command == 'z') {
        if (segments.empty()) return std::nullopt;
        segments.push_back(PathSegment{.kind = PathSegment::Kind::Close});
        current = start;
        previous = command;
        continue;
      }
      if (argumentCount(command) == 0) return std::nullopt;
    } else if (command == 0 || command == 'Z' || command == 'z') {
      return std::nullopt;
    }
    if (segments.empty() && command != 'M' && command != 'm') return std::nullopt;

    const bool relative = std::islower(static_cast<unsigned char>(command)) != 0;
    const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(command)));
    const Point base = relative ? current : Point{};
    double v[7]{};
    for (int i = 0; i < argumentCount(command); ++i) {
      const bool isFlag = upper == 'A' && (i == 3 || i == 4);
      bool flag = false;
      if (isFlag ? !cursor.readFlag(flag) : !cursor.readNumber(v[i])) return std::nullopt;
      if (isFlag) v[i] = flag ? 1.0 : 0.0;
    }

    PathSegment segment;
    switch (upper) {
      case 'M':
        segment.kind = PathSegment::Kind::Move;
        segment.end = {base.x + v[0], base.y + v[1]};
        start = segment.end;
        // 移动命令后面多出来的坐标对按直线处理
        command = relative ? 'l' : 'L';
        break;
      case 'L':
        segment.kind = PathSegment::Kind::Line;
        segment.end = {base.x + v[0], base.y + v[1]};
        break;
      case 'H':
        segment.kind = PathSegment::Kind::Line;
        segment.end = {relative ? current.x + v[0] : v[0], current.y};
        break;
      case 'V':
        segment.kind = PathSegment::Kind::Line;
        segment.end = {current.x, relative ? current.y + v[0] : v[0]};
        break;
      case 'C':
        segment.kind = PathSegment::Kind::Cubic;
        segment.control1 = {base.x + v[0], base.y + v[1]};
        segment.control2 = {base.x + v[2], base.y + v[3]};
        segment.end = {base.x + v[4], base.y + v[5]};
        lastControl = segment.control2;
        break;
      case 'S': {
        segment.kind = PathSegment::Kind::Cubic;
        const bool smooth = previous == 'C' || previous == 'c' || previous == 'S' || previous == 's';
        segment.control1 = smooth ? Point{2 * current.x - lastControl.x, 2 * current.y - lastControl.y} : current;
        segment.control2 = {base.x + v[0], base.y + v[1]};
        segment.end = {base.x + v[2], base.y + v[3]};
        lastControl = segment.control2;
        break;
      }
      case 'Q':
        segment.kind = PathSegment::Kind::Quad;
        segment.control1 = {base.x + v[0], base.y + v[1]};
        segment.end = {base.x + v[2], base.y + v[3]};
        lastControl = segment.control1;
        break;
      case 'T': {
        segment.kind = PathSegment::Kind::Quad;
        const bool smooth = previous == 'Q' || previous == 'q' || previous == 'T' || previous == 't';
        segment.control1 = smooth ? Point{2 * current.x - lastControl.x, 2 * current.y - lastControl.y} : current;
        segment.end = {base.x + v[0], base.y + v[1]};
        lastControl = segment.control1;
        break;
      }
      case 'A':
        segment.kind = PathSegment::Kind::Arc;
        segment.radiusX = v[0];
        segment.radiusY = v[1];
        segment.rotation = v[2];
        segment.largeArc = v[3] != 0.0;
        segment.sweep = v[4] != 0.0;
        segment.end = {base.x + v[5], base.y + v[6]};
        break;
      default:
        return std::nullopt;
    }
    current = segment.end;
    previous = upper == 'M' ? 'M' : command;
    segments.push_back(segment);
  }
  if (segments.empty()) return std::nullopt;
  return segments;
}

}  // namespace rujian::ui
