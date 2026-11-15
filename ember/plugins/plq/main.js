// 声像教学卡 PLQ（mapper 172，PRG 16K）：磁带机的数据线接在卡上，卡上的 PAL 逻辑在 $8000-$9FFF 写入时锁存，在 $A000-$BFFF 读出。
// 逻辑方程按地址位 2、1 分成四种写法化简；磁带的电平（采样不大于 -1000 为 1）只在写入的那一刻进入逻辑。
// 输出寄存器的位 4 至 7 依次是方程的 q7、q6、q5、q4，下一次写入时又按位号当作 Q4 至 Q7 读回。

const bit = (value, index) => (value >> index) & 1;

/** 写 $8000-$9FFF 时 PAL 的新输出：reg 是原来的输出，addr 取地址的位 1、2，data 取低 4 位，tape 是磁带数据线的电平。 */
function nextPal(reg, addr, data, tape) {
  const d = data & 0x0f;
  const old = reg & 0x0f;
  let low;
  let n4;
  let n5;
  let n6;
  let n7;
  switch ((addr >> 1) & 3) {
    case 0:
      // 低 4 位直接锁存，其余保持。
      low = d;
      n4 = bit(reg, 4);
      n5 = bit(reg, 5);
      n6 = bit(reg, 6);
      n7 = bit(reg, 7);
      break;
    case 1:
      // 与原值异或。
      low = d ^ old;
      n4 = bit(low, 3);
      n5 = bit(low, 2);
      n6 = bit(low, 1);
      n7 = bit(low, 0);
      break;
    case 2:
      // 与原值相或。
      low = d | old;
      n4 = bit(reg, 4) | bit(d, 3);
      n5 = bit(reg, 5) | bit(d, 2);
      n6 = bit(reg, 6) | bit(d, 1);
      n7 = bit(reg, 7) | bit(d, 1);
      break;
    default:
      // 数据位 0 为 1 时原值保持、位 4 至 7 翻转对应位；为 0 时清掉没有对应数据位的位，位 4 取磁带电平。
      low = (d & 1) !== 0 ? old : old & d & 0x0e;
      n4 = (d & 1) !== 0 ? bit(reg, 3) ^ bit(reg, 4) : tape;
      n5 = (d & 1) & (bit(reg, 2) ^ bit(reg, 5));
      n6 = (d & 1) & (bit(reg, 1) ^ bit(reg, 6));
      n7 = (d & 1) & (bit(reg, 0) ^ bit(reg, 7));
      break;
  }
  return low | (n7 << 4) | (n6 << 5) | (n5 << 6) | (n4 << 7);
}

export default {
  reset(ctx) {
    // 比较器：采样 > -1000 时状态为 1，取反后数据线为 0；静音时数据线为 0。
    ctx.device('tape').setComparator(-1000, -999, true);
    ctx.state.pal = 0xff;
  },

  read(ctx, bus, addr) {
    // $8000-$9FFF 读出 0；$A000-$BFFF 读出 PAL 的输出。
    return addr < 0xa000 ? 0 : ctx.state.pal;
  },

  write(ctx, bus, addr, value) {
    // 只有“地址位 1、2 都为 1 且数据位 0 为 0”的写入用得到磁带电平，其余不去问磁带机，免得拖慢频繁的写入。
    const usesTape = (addr & 6) === 6 && (value & 1) === 0;
    ctx.state.pal = nextPal(ctx.state.pal, addr, value, usesTape ? ctx.device('tape').level() : 0);
  }
};
