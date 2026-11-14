/*
 * ------------------------------------------------------------------------
 * 名称：debug-text-file-writer.hpp
 * 说明：调试输出文件写入的实现：写到调试输出目录，先写临时文件再改名，所以不会留下写了一半的文件。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "contracts/output/debug-file-writer.hpp"

namespace ember::infra::output {

/** 调试输出文件写入。 */
class DebugTextFileWriter final : public contracts::output::DebugFileWriter {
 public:
  /** @param outputDirectory 取调试输出目录的函数，每次写入时调用。 */
  explicit DebugTextFileWriter(std::function<std::filesystem::path()> outputDirectory);

  void validateName(const std::string& name) const override;
  std::string writeLines(const std::string& fileName, const std::vector<std::string>& lines) override;

 private:
  std::function<std::filesystem::path()> outputDirectory_;
};

}  // namespace ember::infra::output
