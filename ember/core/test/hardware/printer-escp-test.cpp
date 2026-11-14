/*
 * ------------------------------------------------------------------------
 * 名称：printer-escp-test.cpp
 * 说明：虚拟针式打印机的测试：ESC @ 开始一页、9 针与 24 针位图的画点位置、ESC L、行距与进纸、回车与打印头回左端、各种输出时机（FF、复位线、复位、超时、写满一页、立即输出）、空白页不输出、未知命令被忽略、状态线。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：输出方用假的，记下收到的每一页；页是 1 位每像素的位图。
 * ------------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "logic/hardware/printer-escp/printer-escp.hpp"
#include "logic/hardware/scheduler/master-scheduler.hpp"

namespace {

namespace hw = ember::contracts::hardware;
using ember::logic::hardware::MasterScheduler;
using ember::logic::hardware::PrinterEscp;

constexpr hw::MasterTime TIMEOUT = 1000000;

class RecordingOutput final : public hw::PrinterOutput {
 public:
  void outputPage(hw::PrintedPage page) override { pages.push_back(std::move(page)); }
  std::vector<hw::PrintedPage> pages;
};

/** 页上的一个点是不是黑的。 */
bool ink(const hw::PrintedPage& page, int x, int y) {
  const int rowBytes = (static_cast<int>(page.width) + 7) / 8;
  return (page.rows[static_cast<std::size_t>(y) * rowBytes + static_cast<std::size_t>(x / 8)] & (0x80 >> (x % 8))) != 0;
}

/** 页上黑点的个数。 */
int inkCount(const hw::PrintedPage& page) {
  int count = 0;
  for (const std::uint8_t byte : page.rows) {
    for (int bit = 0; bit < 8; ++bit) count += (byte >> bit) & 1;
  }
  return count;
}

struct Bench {
  Bench() : printer(scheduler, output, TIMEOUT) {}

  void send(std::initializer_list<std::uint8_t> bytes) {
    for (const std::uint8_t byte : bytes) printer.writeData(byte);
  }

  void reset() { send({0x1B, '@'}); }

  MasterScheduler scheduler;
  RecordingOutput output;
  PrinterEscp printer;
};

constexpr int LEFT = PrinterEscp::LEFT_MARGIN;
constexpr int TOP = PrinterEscp::TOP_MARGIN;

TEST(PrinterEscp, StatusLinesShowAnOnlinePrinterWithPaper) {
  Bench bench;
  EXPECT_TRUE(bench.printer.ack());
  EXPECT_FALSE(bench.printer.paperOut());
  EXPECT_TRUE(bench.printer.select());
  EXPECT_FALSE(bench.printer.error());
  EXPECT_TRUE(bench.printer.busyN());
}

TEST(PrinterEscp, ABlankPageIsNotOutput) {
  Bench bench;
  bench.reset();
  EXPECT_TRUE(bench.printer.pageOpen());
  bench.send({0x0C});
  EXPECT_FALSE(bench.printer.pageOpen());
  EXPECT_TRUE(bench.output.pages.empty());
  EXPECT_EQ(bench.printer.pagesOutput(), 0U);
}

TEST(PrinterEscp, NineNeedleBitmapDrawsColumnsAtTheMarginsAndIsOutputOnFormFeed) {
  Bench bench;
  bench.reset();
  // ESC * 0 n=2：60 点/英寸 9 针，两列，每列一个字节，最高位在上。
  bench.send({0x1B, '*', 0, 2, 0, 0x81, 0x40});
  EXPECT_TRUE(bench.printer.pageHasContent());
  EXPECT_TRUE(bench.output.pages.empty()) << "没有收到换页之前不输出";
  bench.send({0x0C});
  ASSERT_EQ(bench.output.pages.size(), 1U);
  const hw::PrintedPage& page = bench.output.pages[0];
  EXPECT_EQ(page.width, 1660U);
  EXPECT_EQ(page.height, 2340U);
  EXPECT_EQ(inkCount(page), 3);
  EXPECT_TRUE(ink(page, LEFT + 0, TOP + 0));
  EXPECT_TRUE(ink(page, LEFT + 0, TOP + 7));
  EXPECT_TRUE(ink(page, LEFT + 1, TOP + 1));
  EXPECT_EQ(bench.printer.pagesOutput(), 1U);
  EXPECT_FALSE(bench.printer.pageHasContent());
}

TEST(PrinterEscp, TwentyFourNeedleBitmapUsesThreeBytesPerColumn) {
  Bench bench;
  bench.reset();
  // ESC * 32 n=1：一列 3 个字节，从上到下 24 个点。
  bench.send({0x1B, '*', 32, 1, 0, 0x80, 0x01, 0x80});
  bench.send({0x0C});
  const hw::PrintedPage& page = bench.output.pages.at(0);
  EXPECT_EQ(inkCount(page), 3);
  EXPECT_TRUE(ink(page, LEFT, TOP + 0));
  EXPECT_TRUE(ink(page, LEFT, TOP + 8 + 7));
  EXPECT_TRUE(ink(page, LEFT, TOP + 16));
}

TEST(PrinterEscp, AfterABitmapLineThePrintHeadReturnsToTheLeft) {
  Bench bench;
  bench.reset();
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.send({0x0C});
  const hw::PrintedPage& page = bench.output.pages.at(0);
  EXPECT_EQ(inkCount(page), 1) << "两次都画在最左边的同一个点上";
}

TEST(PrinterEscp, LineFeedMovesDownByTheLineSpacing) {
  Bench bench;
  bench.reset();
  bench.send({0x1B, '*', 0, 1, 0, 0x80});  // 密度 0：纵向 72 点/英寸
  bench.send({0x0A});                      // 默认行距 30/216 英寸 = 10 个点
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.send({0x1B, '3', 36});             // 行距 36/216 英寸 = 12 个点
  bench.send({0x0A});
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.send({0x0C});
  const hw::PrintedPage& page = bench.output.pages.at(0);
  EXPECT_TRUE(ink(page, LEFT, TOP + 0));
  EXPECT_TRUE(ink(page, LEFT, TOP + 10));
  EXPECT_TRUE(ink(page, LEFT, TOP + 22));
}

TEST(PrinterEscp, AdvanceAndCarriageReturnPositionThePen) {
  Bench bench;
  bench.reset();
  bench.send({0x1B, '*', 0, 1, 0, 0x00});  // 设置密度 0，不画点
  bench.send({0x1B, 'J', 108});            // 进纸 108/216 英寸 = 36 个点
  bench.send({0x1B, '*', 0, 2, 0, 0x80, 0x80});
  bench.send({0x0C});
  const hw::PrintedPage& page = bench.output.pages.at(0);
  EXPECT_TRUE(ink(page, LEFT, TOP + 36));
  EXPECT_TRUE(ink(page, LEFT + 1, TOP + 36));
}

TEST(PrinterEscp, HeadLeftCommandMovesToTheLeftEdge) {
  Bench bench;
  bench.reset();
  bench.send({0x1B, '*', 0, 1, 0, 0x00});
  // ESC L 图形模式结束后打印头不回左端，所以连续画两段会并排。
  bench.send({0x1B, 'L', 1, 0, 0x80});
  bench.send({0x1B, 'L', 1, 0, 0x80});
  bench.send({0x0D});
  bench.send({0x1B, 'L', 1, 0, 0x80});
  bench.send({0x0C});
  const hw::PrintedPage& page = bench.output.pages.at(0);
  // 120 点/英寸位图的竖直比例是 2：位置与点都加倍（先乘 2 再取页内位置）。
  EXPECT_TRUE(ink(page, LEFT + 0, TOP * 2 + 0));
  EXPECT_TRUE(ink(page, LEFT + 0, TOP * 2 + 1));
  EXPECT_TRUE(ink(page, LEFT + 1, TOP * 2 + 0));
  EXPECT_FALSE(ink(page, LEFT + 2, TOP * 2 + 0)) << "回车之后第三段又从最左边开始";
  EXPECT_EQ(inkCount(page), 2 * 2) << "两个不同的位置，每个点占 1 列 2 行；第三段与第一段重合";
}

TEST(PrinterEscp, UnknownEscCommandsAreIgnoredAndDoNotSwallowLaterBytes) {
  Bench bench;
  bench.reset();
  bench.send({0x1B, 'Z', 'A', 'B'});
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.send({0x0C});
  ASSERT_EQ(bench.output.pages.size(), 1U);
  EXPECT_TRUE(ink(bench.output.pages[0], LEFT, TOP));
}

TEST(PrinterEscp, ResetCommandWhileAPageIsOpenKeepsThePage) {
  Bench bench;
  bench.reset();
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.reset();
  EXPECT_TRUE(bench.printer.pageHasContent()) << "ESC @ 不丢弃已经画的内容";
  bench.send({0x0C});
  EXPECT_EQ(bench.output.pages.size(), 1U);
}

TEST(PrinterEscp, DrawingWithoutAPageIsIgnored) {
  Bench bench;
  bench.send({0x1B, '*', 0, 1, 0, 0xFF});
  bench.send({0x0C});
  EXPECT_TRUE(bench.output.pages.empty());
}

TEST(PrinterEscp, TheTimeoutOutputsTheCurrentPage) {
  Bench bench;
  bench.reset();
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.scheduler.advance(TIMEOUT - 1);
  EXPECT_TRUE(bench.output.pages.empty());
  bench.scheduler.advance(2);
  EXPECT_EQ(bench.output.pages.size(), 1U);
  EXPECT_FALSE(bench.printer.pageOpen());
}

TEST(PrinterEscp, EveryByteRestartsTheTimeout) {
  Bench bench;
  bench.reset();
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.scheduler.advance(TIMEOUT - 10);
  bench.send({0x0D});
  bench.scheduler.advance(TIMEOUT - 10);
  EXPECT_TRUE(bench.output.pages.empty());
  bench.scheduler.advance(20);
  EXPECT_EQ(bench.output.pages.size(), 1U);
}

TEST(PrinterEscp, TheResetLineOutputsThePageOnItsRisingEdge) {
  Bench bench;
  bench.reset();
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.printer.setReset(true);
  EXPECT_EQ(bench.output.pages.size(), 1U);
  bench.printer.setReset(true);
  bench.printer.setReset(false);
  EXPECT_EQ(bench.output.pages.size(), 1U) << "只有上升沿起作用";
}

TEST(PrinterEscp, ResettingThePrinterOutputsTheOpenPage) {
  Bench bench;
  bench.reset();
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.printer.reset();
  EXPECT_EQ(bench.output.pages.size(), 1U);
  EXPECT_FALSE(bench.printer.pageOpen());
}

TEST(PrinterEscp, FlushPageOutputsOnlyWhenThereIsContent) {
  Bench bench;
  bench.reset();
  bench.printer.flushPage();
  EXPECT_TRUE(bench.printer.pageOpen()) << "没有内容时没有效果";
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.printer.flushPage();
  EXPECT_EQ(bench.output.pages.size(), 1U);
  EXPECT_EQ(bench.printer.pagesOutput(), 1U);
}

TEST(PrinterEscp, AFullPageIsOutputBeforeANewPageStarts) {
  Bench bench;
  bench.reset();
  bench.send({0x1B, '*', 0, 1, 0, 0x80});
  bench.send({0x1B, '3', 216});  // 行距 1 英寸 = 72 个点
  for (int line = 0; line < 40; ++line) {
    bench.send({0x0A});
    bench.send({0x1B, '*', 0, 1, 0, 0x80});
  }
  EXPECT_EQ(bench.output.pages.size(), 1U) << "页写满时先输出";
  EXPECT_TRUE(bench.printer.pageOpen());
  EXPECT_TRUE(bench.printer.pageHasContent()) << "新页上有后面的内容";
  const hw::PrintedPage& first = bench.output.pages[0];
  EXPECT_TRUE(ink(first, LEFT, TOP));
  EXPECT_TRUE(ink(first, LEFT, TOP + 72 * 30));
}

}  // namespace
