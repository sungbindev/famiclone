/*
 * ------------------------------------------------------------------------
 * 名称：svg-path-test.cpp
 * 说明：SVG 路径解析的测试，并检查生成的全部图标都能解析。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：覆盖相对与绝对坐标、H/V、S/T 的控制点反射、弧标志位紧贴数字、紧凑数字写法与各种语法错误。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "base/icon-data.hpp"
#include "base/svg-path.hpp"

using rujian::ui::parseSvgPath;
using rujian::ui::PathSegment;

TEST(SvgPath, AbsoluteAndRelativeLines) {
  const auto path = parseSvgPath("M2 3L5 3l0 4h2V20z");
  ASSERT_TRUE(path);
  ASSERT_EQ(path->size(), 6u);
  EXPECT_EQ((*path)[0].kind, PathSegment::Kind::Move);
  EXPECT_DOUBLE_EQ((*path)[1].end.x, 5);
  EXPECT_DOUBLE_EQ((*path)[2].end.y, 7);
  EXPECT_DOUBLE_EQ((*path)[3].end.x, 7);
  EXPECT_DOUBLE_EQ((*path)[3].end.y, 7);
  EXPECT_DOUBLE_EQ((*path)[4].end.y, 20);
  EXPECT_EQ((*path)[5].kind, PathSegment::Kind::Close);
}

TEST(SvgPath, ImplicitLineAfterMoveAndRepeatedArguments) {
  const auto path = parseSvgPath("m1 1 2 2 3 3");
  ASSERT_TRUE(path);
  ASSERT_EQ(path->size(), 3u);
  EXPECT_EQ((*path)[1].kind, PathSegment::Kind::Line);
  EXPECT_DOUBLE_EQ((*path)[1].end.x, 3);
  EXPECT_DOUBLE_EQ((*path)[2].end.x, 6);
}

TEST(SvgPath, CompactNumbers) {
  const auto path = parseSvgPath("M.5.5L1-2 3.5.25");
  ASSERT_TRUE(path);
  ASSERT_EQ(path->size(), 3u);
  EXPECT_DOUBLE_EQ((*path)[0].end.x, 0.5);
  EXPECT_DOUBLE_EQ((*path)[0].end.y, 0.5);
  EXPECT_DOUBLE_EQ((*path)[1].end.y, -2);
  EXPECT_DOUBLE_EQ((*path)[2].end.x, 3.5);
  EXPECT_DOUBLE_EQ((*path)[2].end.y, 0.25);
}

TEST(SvgPath, CubicAndSmoothCubicReflectsControlPoint) {
  const auto path = parseSvgPath("M0 0C1 1 2 1 3 0S5 -1 6 0");
  ASSERT_TRUE(path);
  ASSERT_EQ(path->size(), 3u);
  const PathSegment& smooth = (*path)[2];
  EXPECT_EQ(smooth.kind, PathSegment::Kind::Cubic);
  // 上一段第二控制点 (2,1) 关于当前点 (3,0) 的反射是 (4,-1)
  EXPECT_DOUBLE_EQ(smooth.control1.x, 4);
  EXPECT_DOUBLE_EQ(smooth.control1.y, -1);
  EXPECT_DOUBLE_EQ(smooth.end.x, 6);
}

TEST(SvgPath, SmoothCubicWithoutPreviousCurveUsesCurrentPoint) {
  const auto path = parseSvgPath("M1 1S2 2 3 3");
  ASSERT_TRUE(path);
  EXPECT_DOUBLE_EQ((*path)[1].control1.x, 1);
  EXPECT_DOUBLE_EQ((*path)[1].control1.y, 1);
}

TEST(SvgPath, QuadAndSmoothQuad) {
  const auto path = parseSvgPath("M0 0Q2 2 4 0T8 0");
  ASSERT_TRUE(path);
  ASSERT_EQ(path->size(), 3u);
  EXPECT_EQ((*path)[2].kind, PathSegment::Kind::Quad);
  EXPECT_DOUBLE_EQ((*path)[2].control1.x, 6);
  EXPECT_DOUBLE_EQ((*path)[2].control1.y, -2);
}

TEST(SvgPath, ArcFlagsMayTouchFollowingNumbers) {
  const auto path = parseSvgPath("M0 0a1 1 0 011 1");
  ASSERT_TRUE(path);
  const PathSegment& arc = (*path)[1];
  EXPECT_EQ(arc.kind, PathSegment::Kind::Arc);
  EXPECT_FALSE(arc.largeArc);
  EXPECT_TRUE(arc.sweep);
  EXPECT_DOUBLE_EQ(arc.end.x, 1);
  EXPECT_DOUBLE_EQ(arc.end.y, 1);
}

TEST(SvgPath, ArcWithSpacedFlagsAndRotation) {
  const auto path = parseSvgPath("M2 2A3 4 30 1 0 8 9");
  ASSERT_TRUE(path);
  const PathSegment& arc = (*path)[1];
  EXPECT_DOUBLE_EQ(arc.radiusX, 3);
  EXPECT_DOUBLE_EQ(arc.radiusY, 4);
  EXPECT_DOUBLE_EQ(arc.rotation, 30);
  EXPECT_TRUE(arc.largeArc);
  EXPECT_FALSE(arc.sweep);
}

TEST(SvgPath, CloseReturnsCurrentPointToSubpathStart) {
  const auto path = parseSvgPath("M5 5l2 0zl1 1");
  ASSERT_TRUE(path);
  ASSERT_EQ(path->size(), 4u);
  EXPECT_DOUBLE_EQ((*path)[3].end.x, 6);
  EXPECT_DOUBLE_EQ((*path)[3].end.y, 6);
}

TEST(SvgPath, RejectsInvalidInput) {
  EXPECT_FALSE(parseSvgPath(""));
  EXPECT_FALSE(parseSvgPath("   "));
  EXPECT_FALSE(parseSvgPath("L1 1"));
  EXPECT_FALSE(parseSvgPath("M1"));
  EXPECT_FALSE(parseSvgPath("M1 1 X"));
  EXPECT_FALSE(parseSvgPath("M1 1 L2"));
  EXPECT_FALSE(parseSvgPath("M1 1 a1 1 0 2 0 1 1"));
  EXPECT_FALSE(parseSvgPath("z"));
  EXPECT_FALSE(parseSvgPath("M1 1 z 5 5"));
  EXPECT_FALSE(parseSvgPath("M1 1 L a b"));
}

TEST(SvgPath, EveryGeneratedIconPathParses) {
  using rujian::ui::IconId;
  for (std::size_t i = 0; i < static_cast<std::size_t>(IconId::Count); ++i) {
    const auto& data = rujian::ui::iconData(static_cast<IconId>(i));
    ASSERT_GT(data.count, 0u) << "图标 " << i << " 没有路径";
    for (std::size_t p = 0; p < data.count; ++p) {
      EXPECT_TRUE(parseSvgPath(data.paths[p].d)) << "图标 " << i << " 的第 " << p << " 条路径解析失败";
    }
  }
}
