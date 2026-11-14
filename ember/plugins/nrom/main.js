// NROM 没有寄存器，脚本只负责键位：把键盘按键转成手柄 1 的按键，程序里没有任何默认键位。
const KEYS = new Map([
  ['KeyW', 'up'],
  ['KeyS', 'down'],
  ['KeyA', 'left'],
  ['KeyD', 'right'],
  ['Numpad1', 'b'],
  ['Numpad2', 'a'],
  ['Space', 'select'],
  ['Enter', 'start']
]);

export default {
  onKey(ctx, event) {
    const button = KEYS.get(event.code);
    if (button !== undefined) ctx.pad(0).set(button, event.down);
  }
};
