/*
 * ------------------------------------------------------------------------
 * 名称：disk-image-opener.hpp
 * 说明：认出磁盘镜像的文件系统并打开；列出能新建的空白镜像种类，并生成空白镜像。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：识别按内容，扩展名只作顺序偏好与位流格式的判断：.nib、.woz 先认金字塔的盘（磁道上全是 0 至 13 号扇区），
 *       按金字塔格式解码后只读打开（认不出目录时标“金字塔 DOS（认不出目录）”），否则经解码器按苹果格式解成扇区镜像只读打开；
 *       苹果格式含 DOS 3.3、ProDOS 与 Apple Pascal（只读）；143360 字节的 .dsk 不是苹果格式时按金字塔 DOS 只读打开；
 *       认不出文件系统时给出说明：全是 0 的是“空白（未格式化）”，其余是“未识别”，不凭大小猜格式。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "contracts/filemanager/disk-file-system.hpp"

namespace ember::logic::filemanager {

/** 识别结果：能打开时有文件系统，认不出时只有格式说明。 */
struct IdentifiedImage {
  /** 文件系统；认不出时为空。 */
  std::unique_ptr<contracts::filemanager::DiskFileSystem> fileSystem;
  /** 格式标识：fat12、fat16、appledos33、prodos、pascal、pyramid；认不出文件系统时是 blank、unknown（位流金字塔盘认不出目录时是 pyramid）。 */
  std::string formatId;
  /** 界面显示的格式名称。 */
  std::string formatName;
};

/**
 * 认出镜像的文件系统并打开。
 * @param image 镜像字节，文件系统直接改它（位流格式除外，它们解码后只读）；必须活得比结果久。
 * @param extension 小写扩展名，不带点。
 * @param decoder 位流格式的解码器；为空时 .nib 与 .woz 认不出。
 */
[[nodiscard]] IdentifiedImage identifyImage(std::vector<std::uint8_t>& image, std::string_view extension, const contracts::filemanager::AppleBitstreamDecoder* decoder);

/** 一种能新建的空白镜像。 */
struct BlankImageKind {
  /** 标识：fat1440、fat720、fat2880、fat1200、fat360、appledos33、prodos、pyramid。 */
  std::string id;
  /** 界面显示的名称。 */
  std::string title;
  /** 新建文件的扩展名，不带点。 */
  std::string extension;
};

/** 能新建的空白镜像种类，按界面显示的顺序。 */
[[nodiscard]] const std::vector<BlankImageKind>& blankImageKinds();

/**
 * 生成空白镜像的内容（已格式化，ProDOS 的卷名是 BLANK；金字塔的是 266240 字节全 0 的 .nib，要在机器里格式化）。
 * @throws shared::CoreError filemanager.unsupported：没有这种空白镜像。
 */
[[nodiscard]] std::vector<std::uint8_t> createBlankImage(const std::string& kindId);

}  // namespace ember::logic::filemanager
