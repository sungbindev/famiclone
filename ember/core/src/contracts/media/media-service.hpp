/*
 * ------------------------------------------------------------------------
 * 名称：media-service.hpp
 * 说明：media 模块的服务接口：软驱盘片的插入、取出、保存、状态与故障注入。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：数据类型由 schema/media.schema.json 生成到 generated/；请求层只经本接口使用 media 模块。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/media/generated/media.hpp"

namespace ember::contracts::media {

/** 首选项：打印机输出的 PNG 图片保存到哪个目录（文字，空表示用户数据目录下的 printer-output）；所属分组为 printer。 */
inline constexpr const char* SETTING_PRINTER_OUTPUT_DIRECTORY = "printer.outputDirectory";
/** 打印机的首选项分组。 */
inline constexpr const char* SETTING_PRINTER_GROUP = "printer";

/** 介质服务。 */
class MediaService {
 public:
  virtual ~MediaService() = default;

  /**
   * 读取镜像文件放进驱动器，替换已有的盘片；成功后发 media.changed。
   * @throws shared::CoreError media.* 的错误，见 schema/media.schema.json。
   */
  virtual MediaFloppyInsertResult floppyInsert(const MediaFloppyInsertParams& params) = 0;

  /** 取出盘片并丢弃镜像；没有盘片时没有效果。@throws shared::CoreError media.noMachine、media.noDrive。 */
  virtual MediaFloppyEjectResult floppyEject(const MediaFloppyEjectParams& params) = 0;

  /** 把镜像写回放入时的文件。@throws shared::CoreError media.noMachine、media.noDrive、media.noDisk、media.saveFailed。 */
  virtual MediaFloppySaveResult floppySave(const MediaFloppySaveParams& params) = 0;

  /** 设置或取消盘片的写保护；成功后发 media.changed。@throws shared::CoreError media.noMachine、media.noDrive。 */
  virtual MediaFloppyWriteProtectResult floppyWriteProtect(const MediaFloppyWriteProtectParams& params) = 0;

  /** 全部软驱的状态；没有主机或主机没有软驱时为空。 */
  [[nodiscard]] virtual MediaFloppyStatusResult floppyStatus() = 0;

  /** 故障注入。@throws shared::CoreError media.noMachine、media.noDrive、media.sectorRequired。 */
  virtual MediaFloppyFaultResult floppyFault(const MediaFloppyFaultParams& params) = 0;

  /** 打印机的状态；没有主机或主机没有打印机时为空闲。 */
  [[nodiscard]] virtual MediaPrinterStatusResult printerStatus() = 0;

  /** 立即输出当前页。@throws shared::CoreError media.noMachine、media.noPrinter、media.printerEmpty。 */
  virtual MediaPrinterFlushResult printerFlush() = 0;
};

}  // namespace ember::contracts::media
