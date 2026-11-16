// ------------------------------------------------------------------------
// 名称：lookup.mjs
// 说明：把 CPU 地址（和当前库页号）解析成 FBASIC.EXE 里的标签名：读 ca65 的调试信息，常驻页按地址找，库页按“库页号 + 窗口 2 的地址”找。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：只用于调试输出；标签是离该地址最近的、不超过它的那个。
// ------------------------------------------------------------------------
import { readFileSync } from 'node:fs';

/**
 * 读调试信息，返回解析函数。
 * @param {string} dbgFile fbasic.dbg 的路径
 * @returns {(address: number, bank: number) => string} 按地址与当前库页号取标签名
 */
export function createLookup(dbgFile) {
  const dbg = readFileSync(dbgFile, 'latin1');
  const segments = new Map();
  for (const match of dbg.matchAll(/^seg\s+id=(\d+),name="([^"]+)"/gm)) segments.set(Number(match[1]), match[2]);
  const labels = [];
  for (const match of dbg.matchAll(/^sym\s+id=\d+,name="([^"]+)",[^\n]*?val=0x([0-9A-Fa-f]+),seg=(\d+),type=lab/gm)) {
    labels.push({ name: match[1], address: Number.parseInt(match[2], 16), segment: segments.get(Number(match[3])) ?? '' });
  }
  const pick = (predicate, address) => {
    let best = null;
    for (const label of labels) if (predicate(label) && label.address <= address && (best === null || label.address > best.address)) best = label;
    return best === null ? '' : `${best.name}+${address - best.address}`;
  };
  return (address, bank) => {
    if (address >= 0xa000 && address < 0xb000) return `${pick((label) => label.segment === `BANK${bank}`, address)}（库页 ${bank}）`;
    return pick((label) => !label.segment.startsWith('BANK') && label.address >= 0x8000 && label.address < 0xa000, address);
  };
}
