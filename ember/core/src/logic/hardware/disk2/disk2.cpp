/*
 * ------------------------------------------------------------------------
 * 名称：disk2.cpp
 * 说明：5.25 英寸软驱控制器模块的实现。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-25
 * 备注：无
 * ------------------------------------------------------------------------
 */

#include "logic/hardware/disk2/disk2.hpp"

#include <algorithm>
#include <span>

namespace ember::logic::hardware {

namespace hw = contracts::hardware;

namespace {

constexpr unsigned REGISTER_MOTOR_OFF = 8;
constexpr unsigned REGISTER_MOTOR_ON = 9;
constexpr unsigned REGISTER_DRIVE_1 = 10;
constexpr unsigned REGISTER_DRIVE_2 = 11;
constexpr unsigned REGISTER_Q6_LOW = 12;
constexpr unsigned REGISTER_Q6_HIGH = 13;
constexpr unsigned REGISTER_Q7_LOW = 14;
constexpr unsigned REGISTER_Q7_HIGH = 15;
constexpr std::uint8_t WRITE_PROTECT_BIT = 0x80;
/** 一个数据寄存器移出的位数。 */
constexpr unsigned BITS_PER_BYTE = 8;

}  // namespace

Disk2::Disk2(hw::Scheduler& scheduler, double ticksPerBit, hw::MasterTime motorOffDelay) : scheduler_(scheduler), ticksPerBit_(ticksPerBit), motorOffDelay_(motorOffDelay) {}

void Disk2::attachDrive(hw::FloppyDrive* drive) {
  drive_ = drive;
  imageData_ = nullptr;
  imageSize_ = 0;
  imageInserted_ = false;
  dropCaches();
}

void Disk2::reset(bool power) {
  if (motorEvent_ != hw::NO_EVENT) scheduler_.cancel(motorEvent_);
  motorEvent_ = hw::NO_EVENT;
  magnets_ = 0;
  motorOn_ = false;
  selected_ = 0;
  q6_ = false;
  q7_ = false;
  latch_ = 0;
  cursor_ = 0;
  dropCaches();
  if (power) halfTrack_ = 0;
  updateMotor();
  if (drive_ != nullptr) drive_->setCylinder(track());
}

std::optional<std::uint8_t> Disk2::read(unsigned registerNumber) { return access(registerNumber & 0x0FU, false, 0); }

void Disk2::write(unsigned registerNumber, std::uint8_t value) { access(registerNumber & 0x0FU, true, value); }

std::optional<std::uint8_t> Disk2::access(unsigned registerNumber, bool isWrite, std::uint8_t value) {
  refreshMedia();
  advance();
  switch (registerNumber) {
    case REGISTER_MOTOR_OFF:
      // 停转要晚一段时间：这段时间里再开马达，盘片还在转。
      if (motorOn_ && motorEvent_ == hw::NO_EVENT) motorEvent_ = scheduler_.scheduleAt(scheduler_.now() + motorOffDelay_, *this, 0);
      break;
    case REGISTER_MOTOR_ON:
      if (motorEvent_ != hw::NO_EVENT) scheduler_.cancel(motorEvent_);
      motorEvent_ = hw::NO_EVENT;
      motorOn_ = true;
      updateMotor();
      break;
    case REGISTER_DRIVE_1:
    case REGISTER_DRIVE_2:
      selected_ = registerNumber == REGISTER_DRIVE_1 ? 0 : 1;
      updateMotor();
      break;
    case REGISTER_Q6_LOW:
      q6_ = false;
      // 写模式下切回移位态：数据寄存器的 8 位从下一个位开始移出。
      if (q7_) {
        shiftReg_ = latch_;
        shiftLeft_ = BITS_PER_BYTE;
      }
      break;
    case REGISTER_Q6_HIGH:
      q6_ = true;
      if (q7_ && isWrite) latch_ = value;
      break;
    case REGISTER_Q7_LOW:
      if (q7_) finishWrite();
      q7_ = false;
      readShift_ = 0;
      readReady_ = false;
      break;
    case REGISTER_Q7_HIGH:
      if (!q7_) {
        q7_ = true;
        shiftLeft_ = 0;
        sessionDirty_ = false;
      }
      if (q6_ && isWrite) latch_ = value;
      break;
    default:
      setPhase(registerNumber >> 1, (registerNumber & 1U) != 0);
      break;
  }
  if ((registerNumber & 1U) != 0) return std::nullopt;
  if (q7_) return latch_;
  if (q6_) return static_cast<std::uint8_t>(drive_ != nullptr && drive_->exists() && drive_->inserted() && drive_->writeProtected() ? WRITE_PROTECT_BIT : 0);
  return readData();
}

void Disk2::setPhase(unsigned phase, bool on) {
  const auto bit = static_cast<std::uint8_t>(1U << phase);
  magnets_ = static_cast<std::uint8_t>(on ? (magnets_ | bit) : (magnets_ & ~bit));
  if (selected_ != 0) return;
  // 读头所在位置对着的磁铁不动；相邻的磁铁吸引读头往那个方向走半个磁道，两边都吸时不动。
  int direction = 0;
  if ((magnets_ & (1U << ((halfTrack_ + 1) & 3U))) != 0) ++direction;
  if ((magnets_ & (1U << ((halfTrack_ + 3) & 3U))) != 0) --direction;
  if (direction == 0) return;
  const int next = std::clamp(static_cast<int>(halfTrack_) + direction, 0, static_cast<int>(MAX_HALF_TRACK));
  if (static_cast<unsigned>(next) == halfTrack_) return;
  halfTrack_ = static_cast<unsigned>(next);
  readShift_ = 0;
  readReady_ = false;
  if (drive_ != nullptr) drive_->setCylinder(track());
}

void Disk2::updateMotor() {
  if (drive_ != nullptr) drive_->setMotor(motorOn_ && selected_ == 0);
}

void Disk2::refreshMedia() {
  if (drive_ == nullptr) return;
  const std::vector<std::uint8_t>& image = drive_->image();
  const bool inserted = drive_->inserted();
  if (image.data() == imageData_ && image.size() == imageSize_ && inserted == imageInserted_) return;
  imageData_ = image.data();
  imageSize_ = image.size();
  imageInserted_ = inserted;
  dropCaches();
}

void Disk2::dropCaches() {
  for (auto& ring : rings_) ring.clear();
  readShift_ = 0;
  readReady_ = false;
  shiftLeft_ = 0;
  sessionDirty_ = false;
}

bool Disk2::spinning() const { return motorOn_ && selected_ == 0 && drive_ != nullptr && drive_->exists(); }

bool Disk2::hasDisk() const { return drive_ != nullptr && drive_->inserted() && drive_->image().size() == disk2::IMAGE_SIZE; }

std::vector<std::uint8_t>& Disk2::trackBits(unsigned track) {
  std::vector<std::uint8_t>& ring = rings_[std::min<unsigned>(track, disk2::TRACK_COUNT)];
  if (!ring.empty()) return ring;
  if (!hasDisk()) {
    // 没有盘片：读头读到杂波，每个磁道一圈固定的随机位。
    std::uint32_t state = 0x9E3779B9U * (track + 1U);
    ring.resize(disk2::TRACK_BITS);
    for (auto& bit : ring) {
      state ^= state << 13;
      state ^= state >> 17;
      state ^= state << 5;
      bit = static_cast<std::uint8_t>(state & 1U);
    }
  } else if (track < disk2::TRACK_COUNT) {
    const std::span<const std::uint8_t> sectors(drive_->image());
    ring = disk2::encodeTrack(track, sectors.subspan(track * disk2::TRACK_BYTES, disk2::TRACK_BYTES));
  } else {
    // 镜像之外的磁道（格式化会写到这里）：什么都没有。
    ring.assign(disk2::TRACK_BITS, 0);
  }
  return ring;
}

void Disk2::advance() {
  const auto target = static_cast<std::int64_t>(static_cast<double>(scheduler_.now()) / ticksPerBit_);
  if (!spinning()) {
    // 盘片没转：位置不重要，读写状态清掉。
    cursor_ = target;
    readShift_ = 0;
    readReady_ = false;
    shiftLeft_ = 0;
    return;
  }
  if (target <= cursor_) return;
  // 读模式下隔了超过一圈没有访问：只需要转最后一圈。
  if (!q7_ && target - cursor_ > static_cast<std::int64_t>(disk2::TRACK_BITS)) {
    cursor_ = target - static_cast<std::int64_t>(disk2::TRACK_BITS);
    readShift_ = 0;
  }
  std::vector<std::uint8_t>& ring = trackBits(track());
  const bool writable = q7_ && hasDisk() && !drive_->writeProtected();
  for (; cursor_ < target; ++cursor_) {
    const auto position = static_cast<std::size_t>(cursor_ % static_cast<std::int64_t>(disk2::TRACK_BITS));
    if (q7_) {
      std::uint8_t out = 0;
      if (shiftLeft_ > 0) {
        out = static_cast<std::uint8_t>((shiftReg_ >> 7) & 1U);
        shiftReg_ = static_cast<std::uint8_t>(shiftReg_ << 1);
        --shiftLeft_;
      }
      if (!writable) continue;
      ring[position] = out;
      if (!sessionDirty_) {
        sessionDirty_ = true;
        sessionTrack_ = track();
      }
      continue;
    }
    // 读：数据寄存器从第一个 1 位起收满 8 位，读完的半字节最高位是 1。
    const unsigned bit = ring[position] != 0 ? 1U : 0U;
    if (readShift_ == 0 && bit == 0) continue;
    readShift_ = (readShift_ << 1) | bit;
    if ((readShift_ & 0x80U) != 0) {
      readLatch_ = static_cast<std::uint8_t>(readShift_);
      readReady_ = true;
      readShift_ = 0;
    }
  }
}

std::uint8_t Disk2::readData() {
  if (!spinning() || !readReady_) return 0;
  readReady_ = false;
  return readLatch_;
}

void Disk2::finishWrite() {
  if (!sessionDirty_) return;
  sessionDirty_ = false;
  if (sessionTrack_ >= disk2::TRACK_COUNT || drive_ == nullptr || !drive_->inserted() || drive_->image().size() != disk2::IMAGE_SIZE) return;
  for (const disk2::DecodedSector& sector : disk2::decodeTrack(rings_[sessionTrack_])) {
    if (sector.track != sessionTrack_) continue;
    const std::size_t base = (static_cast<std::size_t>(sessionTrack_) * disk2::SECTORS_PER_TRACK + sector.logical) * disk2::SECTOR_SIZE;
    for (std::size_t index = 0; index < disk2::SECTOR_SIZE; ++index) {
      if (drive_->readByte(base + index) != sector.data[index]) drive_->writeByte(base + index, sector.data[index]);
    }
  }
}

void Disk2::onEvent(hw::MasterTime /*when*/, std::uint32_t /*tag*/) {
  motorEvent_ = hw::NO_EVENT;
  advance();
  motorOn_ = false;
  updateMotor();
  // 停转时丢掉编好的磁道：下次转起来从镜像重编，镜像在这期间被外部改动也能看到。
  dropCaches();
}

}  // namespace ember::logic::hardware
