/*
 * ------------------------------------------------------------------------
 * 名称：cartridge-loader-test.cpp
 * 说明：卡带装载的测试：解析 NES 文件、匹配插件、创建卡带的整条路径，以及各步骤的错误经装载器原样返回。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-14
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../hardware/fc-test-support.hpp"
#include "../support/plugin-test-support.hpp"
#include "contracts/cartridges/cartridge-error.hpp"
#include "contracts/mappers/mapper-error.hpp"
#include "infra/mappers/plugin-registry.hpp"
#include "logic/cartridges/ines-cartridge-loader.hpp"
#include "shared/errors/core-error.hpp"

namespace {

using ember::logic::cartridges::InesCartridgeLoader;

/** 一个 16K PRG、没有 CHR 的 iNES 文件，PRG 全是 0x6B，mapper 号由参数给出。 */
std::vector<std::uint8_t> makeFile(std::uint8_t mapper) {
  std::vector<std::uint8_t> file = {'N', 'E', 'S', 0x1A, 1, 0, static_cast<std::uint8_t>((mapper & 0x0F) << 4), static_cast<std::uint8_t>(mapper & 0xF0), 0, 0, 0, 0, 0, 0, 0, 0};
  file.insert(file.end(), 16384, 0x6B);
  return file;
}

template <typename Call>
void expectError(Call&& call, std::string_view code) {
  try {
    call();
    FAIL() << "应当抛出异常：" << code;
  } catch (const ember::shared::CoreError& error) {
    EXPECT_EQ(error.code(), code) << error.what();
  }
}

TEST(CartridgeLoader, PlansAndAssemblesACartridgeFromAnInesFileAndAnInstalledPlugin) {
  auto registry = std::make_shared<ember::infra::mappers::PluginRegistry>(ember::test::fcRegistryOptions());
  registry->installPackage(ember::test::officialPluginPackage("nrom"));
  const InesCartridgeLoader loader(registry);

  const auto plan = loader.plan(makeFile(0));
  EXPECT_EQ(plan.image.description.mapper, 0U);
  EXPECT_EQ(plan.image.prg.size(), 16384U);
  EXPECT_EQ(plan.match.pluginId, "nrom");
  EXPECT_EQ(plan.match.host, "fc");

  ember::bootstrap::FcMachine machine;
  const auto cartridge = loader.assemble(plan, machine.cartridgeIrq());
  EXPECT_EQ(cartridge->pluginId(), "nrom");
  machine.cartridgeSlot().insert(*cartridge);
  EXPECT_TRUE(machine.cartridgeSlot().occupied());
  EXPECT_EQ(cartridge->cpuSide().read(0xC000).value, 0x6B);
  EXPECT_EQ(cartridge->storage("prg")->size(), 16384U);
}

TEST(CartridgeLoader, ParseAndMatchErrorsComeBackWithTheirOwnCodes) {
  auto registry = std::make_shared<ember::infra::mappers::PluginRegistry>(ember::test::fcRegistryOptions());
  registry->installPackage(ember::test::officialPluginPackage("nrom"));
  const InesCartridgeLoader loader(registry);

  auto notInes = makeFile(0);
  notInes[0] = 'X';
  expectError([&] { static_cast<void>(loader.plan(notInes)); }, ember::contracts::cartridges::ERROR_NOT_INES);
  auto truncated = makeFile(0);
  truncated.resize(100);
  expectError([&] { static_cast<void>(loader.plan(truncated)); }, ember::contracts::cartridges::ERROR_TRUNCATED);
  expectError([&] { static_cast<void>(loader.plan(makeFile(5))); }, ember::contracts::mappers::ERROR_NO_MATCH);
}

TEST(CartridgeLoader, AnAssembledCartridgeRunsOnAMachine) {
  // 整条路径：iNES 解析、插件匹配、创建卡带、插入主机，CPU 从复位向量取到程序并执行。
  ember::test::Assembler main(0x8000);
  main.ldaImm(0x3C).staAbs(0x0010).staAbs(0x6000).jmpSelf();
  ember::test::FcRig rig(ember::test::makeNromFile(main));
  rig.machine.run(12 * 100);
  EXPECT_EQ(rig.machine.internalRam().read(0x10), 0x3C);
  EXPECT_EQ(rig.prgRam()[0], 0x3C);
}

}  // namespace
