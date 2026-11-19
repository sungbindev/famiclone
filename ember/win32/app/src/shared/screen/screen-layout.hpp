/*
 * ------------------------------------------------------------------------
 * 名称：screen-layout.hpp
 * 说明：画面区域的版面计算：画面比例与缩放方式的取值，以及在画布里按比例居中放置画面的矩形。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-19
 * 备注：单位是设备像素；矩形的原点在画布左上角（Direct3D 视口的约定），
 *       位置与网页版（原点在左下角）一一对应。画面比例 4:3 表示整帧的显示宽高比为 4:3，8:7 表示像素宽高比为 8:7；
 *       整数倍按垂直方向取整数倍（画面高度是帧高度的整数倍），水平方向随比例而定。画面只影响显示，不影响模拟。
 * ------------------------------------------------------------------------
 */

#pragma once

namespace ember::win32::shared {

/** 画面比例：4:3 为整帧的显示宽高比，8:7 为像素宽高比。 */
enum class ScreenAspect { Ratio4x3, Ratio8x7 };

/** 缩放方式：适应窗口（尽量放大）或整数倍（放不下时取最大的整数倍）。 */
enum class ScreenScaling { Fit, Integer };

/** 默认的画面比例：像素宽高比 8:7，画面不变形。 */
inline constexpr ScreenAspect kDefaultScreenAspect = ScreenAspect::Ratio8x7;

/** 默认的缩放方式。 */
inline constexpr ScreenScaling kDefaultScreenScaling = ScreenScaling::Fit;

/** 标准帧（FC 的整帧）的尺寸：画面框的显示比例按它算，不随当前这一帧的实际尺寸变化。 */
inline constexpr int kNominalFrameWidth = 256;
inline constexpr int kNominalFrameHeight = 240;

/** “显示全部 240 行”关闭时，画面上下各裁掉的行数。 */
inline constexpr int kCroppedLines = 8;

/** 画面在画布里的矩形，原点在左上角，单位是设备像素。 */
struct ScreenRect {
  int x{0};
  int y{0};
  int width{0};
  int height{0};

  [[nodiscard]] bool operator==(const ScreenRect& other) const = default;
};

/** “显示全部 240 行”对应的上下裁剪行数。 */
[[nodiscard]] int cropLinesFor(bool showAllLines);

/** 画面的显示宽高比（宽除以高）。 */
[[nodiscard]] double displayAspectRatio(ScreenAspect aspect, double frameWidth, double frameHeight);

/** 标准帧在指定画面比例下的显示宽高比；cropLines 是上下各裁掉的行数。 */
[[nodiscard]] double nominalScreenRatio(ScreenAspect aspect, int cropLines = 0);

/**
 * 计算画面在画布里的矩形：按显示比例居中，周围留给黑底。
 * @param canvasWidth 画布宽度（设备像素）。
 * @param canvasHeight 画布高度。
 * @param frameWidth 帧宽度（像素）。
 * @param frameHeight 帧高度（像素）。
 * @param aspect 画面比例。
 * @param scaling 缩放方式；整数倍连一倍都放不下时退回适应窗口。
 * @returns 矩形；画布或帧的尺寸无效时宽高为 0。
 */
[[nodiscard]] ScreenRect computeScreenRect(double canvasWidth, double canvasHeight, double frameWidth, double frameHeight, ScreenAspect aspect, ScreenScaling scaling);

}  // namespace ember::win32::shared
