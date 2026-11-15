// 声像教学卡 QLS（mapper 172，PRG 32K）：磁带机的数据线接在卡上，右声道的每个下降沿向 CPU 发一次中断。
// 数据线的电平 = 采样不大于 -1000 时为 1，否则为 0；
// 下降沿到来时，把“距上一次下降沿的采样数”按 1764/960 折成计时累加到 tickCount，游戏在 $9000-$9FFF 读到它并撤销中断。

/** 采样数折成计时的比例（采样率 96 kHz）：计时 = 采样数 * 1764 / 960。 */
const TICK_NUMERATOR = 1764;
const TICK_DENOMINATOR = 960;

export default {
  reset(ctx) {
    // 比较器：采样 > -1000 时状态为 1，取反后数据线为 0；静音时数据线为 0。
    ctx.device('tape').setComparator(-1000, -999, true);
    ctx.state.tickCount = 0;
    ctx.state.sampleCount = 0;
    ctx.irq.release();
  },

  read(ctx, bus, addr) {
    if (addr < 0xa000) {
      // $9000-$9FFF：读计时，同时撤销中断。
      ctx.irq.release();
      return ctx.state.tickCount;
    }
    // $A000-$B7FF 读出 0；$B800 以上卡上没有器件，不驱动总线。
    return addr < 0xb800 ? 0 : undefined;
  },

  onTapeEdge(ctx, level, samples) {
    ctx.state.sampleCount = ctx.state.sampleCount + samples;
    if (level !== 0) return;
    // 下降沿。
    const tick = Math.floor((ctx.state.sampleCount * TICK_NUMERATOR) / TICK_DENOMINATOR) & 0xff;
    ctx.state.tickCount = (ctx.state.tickCount + tick) & 0xff;
    ctx.state.sampleCount = 0;
    ctx.irq.assert();
  }
};
