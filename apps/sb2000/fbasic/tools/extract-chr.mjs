// 把 ROM 里的 CHR 另存成 reference/fbasic-chr.bin（8192 字节，原生版用 DMA 送进扩展显存）。用法 node tools/extract-chr.mjs
import { writeFileSync } from 'node:fs';
import { join } from 'node:path';
import { ROOT, readRom } from './lib/rom.mjs';

const { chr } = readRom();
writeFileSync(join(ROOT, 'reference', 'fbasic-chr.bin'), chr);
console.log(`写入 reference/fbasic-chr.bin：${chr.length} 字节，非零 ${chr.filter((value) => value !== 0).length} 字节`);