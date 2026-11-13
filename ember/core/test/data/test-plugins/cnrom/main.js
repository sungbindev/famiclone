// 测试用的 CNROM：写 $8000-$FFFF 选择 CHR 的 8K bank。
export default {
  reset(ctx) {
    ctx.state.chrBank = 0;
    ctx.window('chr').setBank(0);
  },
  write(ctx, bus, addr, value) {
    ctx.state.chrBank = value;
    ctx.window('chr').setBank(value);
  }
};
