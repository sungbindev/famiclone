/*
 * ------------------------------------------------------------------------
 * 名称：mapper-vectors-test.cpp
 * 说明：官方 mapper 插件的测试向量运行器：读取 core/test/data/mapper-vectors/ 下的每个 JSON 文件，用打好的 .emap 包匹配并创建卡带，依次执行写入、复位、读取与断言。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-17
 * 备注：向量文件是 { plugin, image, steps }：plugin 等于文件名（同一插件的多个向量用 <标识>.<后缀>.json）；image 给 mapper、variant、prgKiB（或 prgSize）、prgFill（PRG 的填充字节，默认 0）、chrKiB（或 chrSize）、mirroring、battery；
 *       steps 的每一步可含 reset（power、soft）、write 加 value（bus 默认 cpu）、read 加可选的 expect（不写表示没有驱动总线）、
 *       expectMap（地址落到哪块存储区的哪个偏移，或 unmapped）、expectState、expectNametable（地址、a10、enable）、expectIrq。
 *       地址与值可以写整数、"$8000" 或 "0x80"。断言地址落点用 describeAddress（不回调脚本、不依赖 ROM 内容），所以映像的 PRG、CHR 内容全为 0，只有大小有意义。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "../support/plugin-test-support.hpp"
#include "contracts/mappers/mapper-cartridge.hpp"
#include "infra/mappers/plugin-registry.hpp"

namespace {

using nlohmann::json;
namespace fs = std::filesystem;
namespace mappers = ember::contracts::mappers;

/** 向量文件所在目录。 */
fs::path vectorDirectory() { return fs::path(EMBER_TEST_DATA_DIR) / "mapper-vectors"; }

/** 目录里的全部向量文件，按文件名排序。 */
std::vector<fs::path> vectorFiles() {
  std::vector<fs::path> files;
  std::error_code error;
  for (fs::directory_iterator it(vectorDirectory(), error), end; !error && it != end; it.increment(error)) {
    if (it->path().extension() == ".json") files.push_back(it->path());
  }
  std::sort(files.begin(), files.end());
  return files;
}

/** 数字：整数、"$XXXX" 或 "0xXX"。 */
std::uint64_t number(const json& value) {
  if (value.is_number_integer()) return value.get<std::uint64_t>();
  const std::string text = value.get<std::string>();
  if (text.size() > 1 && text[0] == '$') return std::stoull(text.substr(1), nullptr, 16);
  return std::stoull(text, nullptr, 0);
}

mappers::Mirroring mirroringOf(const std::string& name) {
  if (name == "vertical") return mappers::Mirroring::Vertical;
  if (name == "four") return mappers::Mirroring::FourScreen;
  return mappers::Mirroring::Horizontal;
}

/** 由向量里的 image 段构造卡带映像。 */
mappers::CartridgeImage makeImage(const json& spec) {
  mappers::CartridgeImage image;
  const std::uint64_t prg = spec.contains("prgSize") ? number(spec["prgSize"]) : number(spec.value("prgKiB", 16)) * 1024;
  const std::uint64_t chr = spec.contains("chrSize") ? number(spec["chrSize"]) : number(spec.value("chrKiB", 0)) * 1024;
  image.prg.assign(static_cast<std::size_t>(prg), static_cast<std::uint8_t>(number(spec.value("prgFill", 0))));
  image.chr.assign(static_cast<std::size_t>(chr), 0);
  image.description.mapper = static_cast<std::uint32_t>(number(spec.at("mapper")));
  image.description.variant = static_cast<std::uint32_t>(number(spec.value("variant", 0)));
  image.description.prgSize = prg;
  image.description.chrSize = chr;
  image.description.mirroring = mirroringOf(spec.value("mirroring", "horizontal"));
  image.description.battery = spec.value("battery", false);
  return image;
}

class MapperVectors : public ::testing::TestWithParam<fs::path> {};

TEST_P(MapperVectors, FollowTheirSteps) {
  const json vector = json::parse(ember::test::readTextFile(GetParam().string()));
  const std::string pluginId = vector.at("plugin").get<std::string>();
  const std::string stem = GetParam().stem().string();
  ASSERT_TRUE(stem == pluginId || stem.rfind(pluginId + ".", 0) == 0) << "向量文件名必须是 <插件标识>.json 或 <插件标识>.<后缀>.json";

  auto registry = std::make_shared<ember::infra::mappers::PluginRegistry>(ember::test::fcRegistryOptions());
  registry->installPackage(ember::test::officialPluginPackage(pluginId));
  const mappers::CartridgeImage image = makeImage(vector.at("image"));
  const mappers::MapperMatch match = registry->match(image.description);
  ASSERT_EQ(match.pluginId, pluginId);

  ember::test::RecordingIrq irq;
  ember::test::RecordingPads pads;
  const std::unique_ptr<mappers::MapperCartridge> cartridge = registry->create(match, image, irq, pads);
  cartridge->reset(true);

  int stepNumber = 0;
  for (const json& step : vector.at("steps")) {
    ++stepNumber;
    SCOPED_TRACE("第 " + std::to_string(stepNumber) + " 步：" + step.dump());
    if (step.contains("reset")) cartridge->reset(step["reset"].get<std::string>() == "power");
    if (step.contains("write")) {
      const bool ppu = step.value("bus", "cpu") == "ppu";
      (ppu ? cartridge->ppuSide() : cartridge->cpuSide()).write(static_cast<std::uint16_t>(number(step["write"])), static_cast<std::uint8_t>(number(step.at("value"))));
    }
    if (step.contains("read")) {
      const bool ppu = step.value("bus", "cpu") == "ppu";
      const auto result = (ppu ? cartridge->ppuSide() : cartridge->cpuSide()).read(static_cast<std::uint16_t>(number(step["read"])));
      if (step.contains("expect")) {
        EXPECT_EQ(result.drivenMask, 0xFF);
        EXPECT_EQ(result.value, number(step["expect"]));
      } else {
        EXPECT_EQ(result.drivenMask, 0) << "应当没有驱动总线";
      }
    }
    for (const json& item : step.value("expectMap", json::array())) {
      const bool ppu = item.value("bus", "cpu") == "ppu";
      const auto info = cartridge->describeAddress(ppu, static_cast<std::uint16_t>(number(item.at("address"))));
      if (item.value("unmapped", false)) {
        EXPECT_TRUE(info.kind == mappers::AddressInfo::Kind::None || !info.mapped) << "应当没有映射";
        continue;
      }
      EXPECT_EQ(info.kind, mappers::AddressInfo::Kind::Storage);
      EXPECT_EQ(info.storage, item.at("storage").get<std::string>());
      EXPECT_EQ(info.offset, number(item.at("offset")));
    }
    const json expectedStates = step.value("expectState", json::object());
    for (const auto& [name, value] : expectedStates.items()) {
      EXPECT_EQ(cartridge->state(name), static_cast<std::uint32_t>(number(value))) << name;
    }
    for (const json& item : step.value("expectNametable", json::array())) {
      const auto control = cartridge->ciramControl(static_cast<std::uint16_t>(number(item.at("address"))));
      EXPECT_EQ(control.enable, item.value("enable", true));
      if (control.enable) EXPECT_EQ(control.a10, item.at("a10").get<bool>());
    }
    if (step.contains("expectIrq")) EXPECT_EQ(irq.active(), step["expectIrq"].get<bool>());
  }
  EXPECT_FALSE(cartridge->failure().has_value()) << (cartridge->failure() ? cartridge->failure()->message : "");
}

INSTANTIATE_TEST_SUITE_P(Plugins, MapperVectors, ::testing::ValuesIn(vectorFiles()), [](const ::testing::TestParamInfo<fs::path>& info) {
  std::string name = info.param.stem().string();
  std::replace(name.begin(), name.end(), '-', '_');
  std::replace(name.begin(), name.end(), '.', '_');
  return name;
});

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(MapperVectors);

}  // namespace
