/*
 * ------------------------------------------------------------------------
 * 名称：floppy-drive.hpp
 * 说明：硬件契约：软驱驱动器。持有盘片镜像、磁头位置、马达、写保护与故障；软驱控制器经它读写盘片，界面与调试服务经它插入、取出、保存盘片。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：盘片镜像是唯一的数据：学习机经控制器读写它，界面与调试服务经本接口读写它，两者看到的始终是同一份；
 *       写扇区只改内存里的镜像并置“已修改”标志与递增修改序号，由介质服务在写盘停下后自动写回文件，写回成功才清除标志；硬件模块里的驱动器对象由主机型号创建，只在模拟线程使用。
 * ------------------------------------------------------------------------
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ember::contracts::hardware {

/** 盘片的格式：由镜像的大小推断。 */
struct FloppyFormat {
  unsigned cylinders{0};
  unsigned sides{0};
  /** 每道的扇区数。 */
  unsigned sectors{0};
  /** 数据率（千位每秒）：250、300、500、1000。 */
  unsigned dataRate{0};
};

/** 注入的故障；默认全部关闭。 */
struct FloppyFaults {
  /** 没有索引脉冲：读扇区编号、读写与格式化命令一直不结束。 */
  bool noIndex{false};
  /** 磁头卡住：寻道与回零不移动。 */
  bool seekFail{false};
  /** 控制器不响应：收到命令字节后不再处理，主状态一直不就绪。 */
  bool unresponsive{false};
  /** 读时数据校验出错的逻辑扇区号。 */
  std::vector<int> badSectors;
};

/** 软驱驱动器。 */
class FloppyDrive {
 public:
  virtual ~FloppyDrive() = default;

  // ------------------------------------------------ 控制器一侧

  /** 驱动器是否存在（装了这个驱动器）。 */
  [[nodiscard]] virtual bool exists() const = 0;

  /** 里面有没有盘片。 */
  [[nodiscard]] virtual bool inserted() const = 0;

  /** 马达是否在转。 */
  [[nodiscard]] virtual bool motorRunning() const = 0;
  virtual void setMotor(bool on) = 0;

  /** 盘片是否写保护。 */
  [[nodiscard]] virtual bool writeProtected() const = 0;

  /** 盘片更换标志：取出盘片时置位，磁头换道或读标志寄存器时清除。 */
  [[nodiscard]] virtual bool diskChanged() const = 0;
  virtual void clearDiskChanged() = 0;

  /** 磁头所在的磁道。 */
  [[nodiscard]] virtual unsigned cylinder() const = 0;
  virtual void setCylinder(unsigned cylinder) = 0;

  /** 盘片的格式；没有盘片时全为 0。 */
  [[nodiscard]] virtual FloppyFormat format() const = 0;

  /** 读镜像里的一个字节；没有盘片或超出镜像时为 0。 */
  [[nodiscard]] virtual std::uint8_t readByte(std::size_t offset) const = 0;

  /** 写镜像里的一个字节，置已修改标志；没有盘片或超出镜像时丢弃。 */
  virtual void writeByte(std::size_t offset, std::uint8_t value) = 0;

  /** 注入的故障。 */
  [[nodiscard]] virtual const FloppyFaults& faults() const = 0;

  // ------------------------------------------------ 盘片与调试一侧

  /**
   * 放入盘片，替换已有的；盘片更换标志不变。
   * @param image 镜像内容。
   * @param writeProtected 是否写保护。
   * @param source 来源（文件路径），只作记录，保存时写回这里。
   */
  virtual void insertImage(std::vector<std::uint8_t> image, bool writeProtected, std::string source) = 0;

  /** 取出盘片并丢弃镜像；盘片更换标志置位。没有盘片时没有效果。 */
  virtual void ejectImage() = 0;

  /** 镜像内容；没有盘片时为空。 */
  [[nodiscard]] virtual const std::vector<std::uint8_t>& image() const = 0;

  /** 镜像的来源（放入时给出的文件路径）。 */
  [[nodiscard]] virtual const std::string& source() const = 0;

  /** 镜像自放入或上次写回以来是否有尚未写回文件的修改。 */
  [[nodiscard]] virtual bool modified() const = 0;

  /** 修改序号：每次写镜像、放入或取出盘片都加 1，用来判断写回期间镜像有没有再被改动。 */
  [[nodiscard]] virtual std::uint64_t revision() const = 0;

  /** 写回之后清除已修改标志。 */
  virtual void markSaved() = 0;

  /** 设置写保护。 */
  virtual void setWriteProtected(bool protect) = 0;

  /** 修改注入的故障。 */
  virtual FloppyFaults& mutableFaults() = 0;

  /**
   * 取出盘片但保留镜像（模拟“没有盘”），之后可以用 restoreMedia 放回；已取出时没有效果。
   * @return 取出前是否有盘片。
   */
  virtual bool suspendMedia() = 0;

  /** 把 suspendMedia 取出的盘片放回；没有保留的镜像时返回 false。 */
  virtual bool restoreMedia() = 0;

  /** 盘片是否被 suspendMedia 暂时取出（镜像还在）。 */
  [[nodiscard]] virtual bool mediaSuspended() const = 0;
};

}  // namespace ember::contracts::hardware
