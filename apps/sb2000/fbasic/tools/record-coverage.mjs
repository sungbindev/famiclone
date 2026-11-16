// ------------------------------------------------------------------------
// 名称：record-coverage.mjs
// 说明：在 Ember 的 FC 模式里让 Family BASIC 依次运行 analysis/coverage/ 里的每个脚本，记录 CDL 并合并成 analysis/prg.cdl；也可以换成别的 ROM 重跑同一批脚本，把截图存到别的目录来比较。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-16
// 备注：用法 node tools/record-coverage.mjs [脚本名片段...] [--rom <nes 文件> --shots <目录>]；
//       脚本每行是一条要键入的文字（自动回车），# 开头是注释，@wait 帧数 空跑若干帧，@key 键位 [帧数] 按一下某个键，@shot 名字 截图。
//       默认用原版 ROM 并合并 CDL；给了 --rom 就不合并 CDL，只在 --shots 目录里留下每个脚本结束时的截图（用于重排 ROM 的行为比较）。
//       每个脚本从头开机，各自记一份 CDL，最后按位或合并；CDL 位定义同 Mesen（1 代码、2 数据、4 跳转目标、8 子程序入口）。
// ------------------------------------------------------------------------
import { copyFileSync, existsSync, mkdirSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { ROM, bootFamilyBasic, frames, screenshot, startEmber, typeText } from './lib/ember-fc.mjs';

const ROOT = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const SCRIPTS = join(ROOT, 'analysis', 'coverage');
const MERGED = join(ROOT, 'analysis', 'prg.cdl');

const args = process.argv.slice(2);
const optionValue = (name) => { const index = args.indexOf(name); return index >= 0 ? args[index + 1] : null; };
const romOption = optionValue('--rom');
const SHOTS = resolve(optionValue('--shots') ?? join(ROOT, 'build', 'shots'));
const filters = args.filter((arg, index) => !arg.startsWith('--') && !['--rom', '--shots'].includes(args[index - 1]));
mkdirSync(SHOTS, { recursive: true });

/**
 * 运行一个脚本并返回这一次的 CDL。
 * @param {object} client 调试客户端
 * @param {string} name 脚本名
 * @param {string} file 脚本路径
 * @returns {Promise<Buffer|null>} CDL 内容；换 ROM 重跑时不记 CDL，返回 null
 */
async function runScript(client, name, file) {
  await client.api.preferences.set({ id: 'input.familyKeyboard', value: true });
  await client.api.cartridges.load({ path: romOption ? resolve(romOption) : ROM, powerOn: true, resume: false });
  if (!romOption) await client.api.inspector.cdlStart();
  await frames(client, 200);
  let typed = 0;
  for (const raw of readFileSync(file, 'utf8').split(/\r?\n/)) {
    const line = raw.trimEnd();
    if (line === '' || line.startsWith('#')) continue;
    if (line.startsWith('@wait ')) { await frames(client, Number(line.slice(6))); continue; }
    if (line.startsWith('@key ')) {
      const [code, count = '6'] = line.slice(5).split(/\s+/);
      await client.api.input.key({ code, down: true });
      await frames(client, Number(count));
      await client.api.input.key({ code, down: false });
      await frames(client, 6);
      continue;
    }
    if (line.startsWith('@shot ')) { await screenshot(client, join(SHOTS, `${name}-${line.slice(6)}.png`)); continue; }
    await typeText(client, `${line}\n`);
    typed += 1;
  }
  await frames(client, 60);
  await screenshot(client, join(SHOTS, `${name}-end.png`));
  if (romOption) { console.log(`${name}: 键入 ${typed} 行`); return null; }
  const saved = await client.api.inspector.cdlSave({ file: `${name}.cdl` });
  console.log(`${name}: 键入 ${typed} 行，代码字节 ${saved.codeBytes}，数据字节 ${saved.dataBytes}`);
  const copy = join(ROOT, 'build', `${name}.cdl`);
  copyFileSync(saved.file, copy);
  return readFileSync(copy);
}

const files = readdirSync(SCRIPTS).filter((name) => name.endsWith('.txt') && (filters.length === 0 || filters.some((part) => name.includes(part)))).sort();
const session = await startEmber();
try {
  const merged = existsSync(MERGED) && filters.length > 0 ? Buffer.from(readFileSync(MERGED)) : Buffer.alloc(32768);
  for (const name of files) {
    const cdl = await runScript(session.client, name.replace(/\.txt$/, ''), join(SCRIPTS, name));
    if (cdl) for (let index = 0; index < merged.length; index += 1) merged[index] |= cdl[index];
  }
  if (!romOption) {
    writeFileSync(MERGED, merged);
    let code = 0;
    let data = 0;
    for (const value of merged) { if (value & 1) code += 1; if (value & 2) data += 1; }
    console.log(`合并后：代码字节 ${code}，数据字节 ${data}，写入 ${MERGED}`);
  }
} finally {
  session.close();
}
