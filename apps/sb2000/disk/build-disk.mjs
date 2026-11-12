/*
 * ------------------------------------------------------------------------
 * 名称：build-disk.mjs
 * 说明：把 content/ 目录里的全部文件做成一张 1.44MB 的软盘镜像。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-12
 * 备注：先在 edit、asm、runnes 目录执行 npm run build。运行时先把 EDIT、ASM、RUNNES 的 EXE、
 *       示例源码、SB2000.INC 和按模板生成的 README.TXT 放进 content/（同名文件会被覆盖），
 *       再把 content/ 里的所有文件做成 SB2000汇编开发盘.img；MARIO.NES 等其他文件自己放进
 *       content/ 即可一起打包。源码在仓库里是 UTF-8，放进盘里时转成 GB2312 并统一成 CR LF。
 * ------------------------------------------------------------------------
 */
import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { buildFat12Image } from '../../../machines/sb2000/disassembly/tools/fat12-image.mjs';
import { readDirectoryFiles } from '../../../machines/sb2000/disassembly/tools/make-floppy.mjs';

const SCRIPT_DIR = path.dirname(fileURLToPath(import.meta.url));

/** 放进软盘的文件所在的目录，脚本运行时创建，不入库 */
const CONTENT_DIR = path.join(SCRIPT_DIR, 'content');

/** 生成的镜像文件 */
const OUTPUT_PATH = path.join(SCRIPT_DIR, 'SB2000汇编开发盘.img');

/** 软盘卷标 */
const DISK_LABEL = 'SB2KAPPS';

/** 盘里说明文件的文件名 */
const README_NAME = 'README.TXT';

/**
 * 说明文件的模板，字符都要在 GB2312 里。
 * {{生成日期}} 换成生成镜像的日期，{{文件列表}} 换成 content/ 里除说明文件外的全部文件。
 */
const README_TEMPLATE = `SB2000 汇编开发盘
生成日期：{{生成日期}}

在 SB-DOS 下写 6502 汇编、汇编并运行。

盘内文件：
{{文件列表}}

试一试：
1. 输入 EDIT HELLO.ASM 打开示例。
2. 按 F5 保存并汇编，生成 HELLO.EXE。
   也可以直接输入 ASM HELLO 汇编。
3. 输入 HELLO 运行。

注意：
- EDIT.EXE 与 ASM.EXE 要在同一张盘里。
- 源文件用 .INCLUDE 引入 SB2000.INC，
  汇编时它要和源文件在同一张盘里。
- 输入 RUNNES 游戏名.NES 运行 NES 游戏，
  游戏文件名要符合 DOS 8.3 规则。
`;

/** 盘内文件的说明，文件列表里用；不在表里的文件只列名字 */
const FILE_DESCRIPTIONS = {
  'EDIT.EXE': '文本编辑器',
  'ASM.EXE': '6502 汇编器',
  'RUNNES.EXE': '运行 NES 游戏',
  'HELLO.ASM': '示例：输出“你好，小霸王！”',
  'SUM.ASM': '示例：10 个数求和',
  'SB2000.INC': '系统接口的符号文件',
};

/**
 * 由脚本放进 content/ 的文件：盘内文件名、来源路径、是否是要转成 GB2312 的 UTF-8 文本。
 * 来源里的 EXE 需要先构建。
 */
const MANAGED_FILES = [
  { name: 'EDIT.EXE', source: path.join(SCRIPT_DIR, '../edit/build/sb2000/EDIT.EXE') },
  { name: 'ASM.EXE', source: path.join(SCRIPT_DIR, '../asm/build/sb2000/ASM.EXE') },
  { name: 'RUNNES.EXE', source: path.join(SCRIPT_DIR, '../runnes/build/sb2000/RUNNES.EXE') },
  { name: 'HELLO.ASM', source: path.join(SCRIPT_DIR, '../asm/examples/hello.asm'), utf8Text: true },
  { name: 'SUM.ASM', source: path.join(SCRIPT_DIR, '../asm/examples/sum.asm'), utf8Text: true },
  { name: 'SB2000.INC', source: path.join(SCRIPT_DIR, '../asm/examples/SB2000.INC'), utf8Text: true },
];

/** GB2312 双字节区的字符到内码的对照表，用 GBK 解码器逐个解出来 */
function buildGb2312Table() {
  const decoder = new TextDecoder('gbk');
  const table = new Map();
  for (let lead = 0xa1; lead <= 0xf7; lead++) {
    for (let trail = 0xa1; trail <= 0xfe; trail++) {
      const text = decoder.decode(Uint8Array.of(lead, trail));
      if (text.length === 1 && text !== '\ufffd') {
        table.set(text, [lead, trail]);
      }
    }
  }
  return table;
}

/**
 * UTF-8 文本转 GB2312 字节，换行统一成 CR LF。
 * @param {string} text 文本
 * @returns {Buffer} GB2312 字节
 * @throws 文本里有 GB2312 没有的字符时抛出错误
 */
export function utf8ToGb2312(text) {
  const table = buildGb2312Table();
  const bytes = [];
  for (const char of text.replace(/\r?\n/g, '\r\n')) {
    const code = char.codePointAt(0);
    if (code < 0x80) {
      bytes.push(code);
    } else if (table.has(char)) {
      bytes.push(...table.get(char));
    } else {
      throw new Error(`GB2312 里没有字符 ${char}（U+${code.toString(16)}）`);
    }
  }
  return Buffer.from(bytes);
}

/**
 * 按模板生成说明文件的文本。
 * @param {string} template 模板，含 {{生成日期}} 与 {{文件列表}}
 * @param {string[]} fileNames 盘内文件名（不含说明文件）
 * @param {Date} date 生成日期
 * @returns {string} 说明文件的文本
 */
export function renderReadme(template, fileNames, date) {
  const list = fileNames
    .map((name) => `  ${name.padEnd(12)}${FILE_DESCRIPTIONS[name] ?? ''}`.trimEnd())
    .join('\n');
  // 用函数做替换，避免说明里的 $ 被当成替换符号
  return template
    .replace('{{生成日期}}', () => date.toLocaleDateString('sv-SE'))
    .replace('{{文件列表}}', () => list);
}

/**
 * 生成软盘镜像：先把受管文件和说明文件放进 content 目录，再把目录里的全部文件做成镜像。
 * @param {object} options 选项
 * @param {string} options.contentDir 放进软盘的文件所在的目录，不存在时创建
 * @param {string} options.outputPath 镜像输出路径
 * @param {{name: string, source: string, utf8Text?: boolean}[]} options.managedFiles
 *   由脚本放进目录的文件
 * @param {string} [options.readmeTemplate] 说明文件的模板
 * @param {Date} [options.date] 说明文件里的生成日期
 * @returns {{outputPath: string, fileNames: string[]}} 输出路径与盘内全部文件名
 * @throws 来源文件不存在、文件名不符合 8.3、软盘放不下时抛出错误
 */
export function buildDisk({ contentDir, outputPath, managedFiles, readmeTemplate = README_TEMPLATE, date = new Date() }) {
  mkdirSync(contentDir, { recursive: true });

  // 受管文件覆盖写入；文本转成 GB2312，EXE 原样复制
  for (const file of managedFiles) {
    if (!existsSync(file.source)) {
      throw new Error(`找不到 ${file.source}，EXE 需要先在对应的程序目录执行 npm run build`);
    }
    const data = readFileSync(file.source);
    writeFileSync(path.join(contentDir, file.name), file.utf8Text ? utf8ToGb2312(data.toString('utf8')) : data);
  }

  // 文件列表取目录里除说明文件外的全部文件，其中包括用户自己放进去的文件
  const otherNames = readDirectoryFiles(contentDir)
    .map((file) => file.name)
    .filter((name) => name.toUpperCase() !== README_NAME);
  writeFileSync(path.join(contentDir, README_NAME), utf8ToGb2312(renderReadme(readmeTemplate, otherNames, date)));

  const files = readDirectoryFiles(contentDir);
  writeFileSync(outputPath, buildFat12Image(files, { label: DISK_LABEL }));
  return { outputPath, fileNames: files.map((file) => file.name) };
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const result = buildDisk({ contentDir: CONTENT_DIR, outputPath: OUTPUT_PATH, managedFiles: MANAGED_FILES });
  console.log(`已生成 ${result.outputPath}，共 ${result.fileNames.length} 个文件：${result.fileNames.join(' ')}`);
}
