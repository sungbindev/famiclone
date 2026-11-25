/*
 * ------------------------------------------------------------------------
 * 名称：diskimages-service.hpp
 * 说明：diskimages 模块的服务接口：镜像目录的设置与树、镜像里文件的列出、重命名、删除、移动、新建文件夹、导入、导出、撤销，空白镜像的新建。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：数据类型由 schema/diskimages.schema.json 生成到 generated/；请求层只经本接口使用 diskimages 模块；
 *       修改类操作立即生效（镜像插在软驱里时改软驱里的盘片，随后由介质服务写回文件，否则直接改文件）并发 diskimages.imageChanged，没有确认。
 * ------------------------------------------------------------------------
 */

#pragma once

#include "contracts/diskimages/generated/diskimages.hpp"

namespace ember::contracts::diskimages {

/** 磁盘镜像管理服务。 */
class DiskImageService {
 public:
  virtual ~DiskImageService() = default;

  /** 设置镜像目录并开始监视与扫描；path 为空表示取消。@throws shared::CoreError diskimages.invalidPath。 */
  virtual DiskimagesSetRootResult setRoot(const DiskimagesSetRootParams& params) = 0;

  /** 镜像目录里的树。 */
  [[nodiscard]] virtual DiskimagesTreeResult tree() = 0;

  /** 列出镜像里一个文件夹的内容。@throws shared::CoreError diskimages.* 的错误，见 schema/diskimages.schema.json。 */
  [[nodiscard]] virtual DiskimagesListResult list(const DiskimagesListParams& params) = 0;

  virtual DiskimagesRenameResult rename(const DiskimagesRenameParams& params) = 0;
  virtual DiskimagesRemoveResult remove(const DiskimagesRemoveParams& params) = 0;
  virtual DiskimagesMoveResult move(const DiskimagesMoveParams& params) = 0;
  virtual DiskimagesCopyResult copy(const DiskimagesCopyParams& params) = 0;
  virtual DiskimagesMakeDirectoryResult makeDirectory(const DiskimagesMakeDirectoryParams& params) = 0;
  virtual DiskimagesUndoResult undo(const DiskimagesUndoParams& params) = 0;
  [[nodiscard]] virtual DiskimagesReadEntriesResult readEntries(const DiskimagesReadEntriesParams& params) = 0;
  virtual DiskimagesImportFilesResult importFiles(const DiskimagesImportFilesParams& params) = 0;
  virtual DiskimagesCreateImageResult createImage(const DiskimagesCreateImageParams& params) = 0;
  [[nodiscard]] virtual DiskimagesBlankKindsResult blankKinds() = 0;
  [[nodiscard]] virtual DiskimagesImagePathResult imagePath(const DiskimagesImagePathParams& params) = 0;
};

}  // namespace ember::contracts::diskimages
