// ------------------------------------------------------------------------
// 名称：check-package.mjs
// 说明：核对 fcdev 扩展的 .vsix 内容：编译产物、cc65 工具链（与清单的 SHA-256 一致）、各目标的运行库、
//       项目模板、使用文档与调试接口描述都在，测试、源码、node_modules、密钥与 Ember、BIOS、ROM 文件都不在。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-22
// 备注：用法 node vscode/tools/check/check-package.mjs [.vsix 路径]；不给路径时取 build/vscode/fcdev-win32-x64-<版本>.vsix。只检查，
//       不修改任何文件；用自带的最小 zip 读取器，不依赖第三方包。
// ------------------------------------------------------------------------
import { createHash } from 'node:crypto';
import { existsSync, readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { inflateRawSync } from 'node:zlib';

/** 仓库根目录 */
const REPOSITORY_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..', '..', '..');

/** 扩展目录 */
const EXTENSION_DIRECTORY = path.join(REPOSITORY_ROOT, 'vscode');

/** .vsix 里扩展文件的前缀 */
const PREFIX = 'extension/';

/** 必须随扩展发布的目标 */
const REQUIRED_TARGETS = ['nes', 'sb2000'];

/** 必须随扩展发布的模板 */
const REQUIRED_TEMPLATES = ['nes-c', 'nes-asm', 'sb2000-c', 'sb2000-asm'];

/** 不允许出现的路径：测试、源码、依赖、密钥、安装包、模拟器与固件 */
const FORBIDDEN = [
  { pattern: /^(src|test|node_modules)\//, reason: '源码、测试与依赖不随扩展发布' },
  { pattern: /\.(tgz|vsix|pem|key|p12|pfx)$/i, reason: '密钥、证书与安装包不随扩展发布' },
  { pattern: /(^|\/)\.env/, reason: '环境变量文件不随扩展发布' },
  { pattern: /(^|\/)ember(-win)?\.exe$/i, reason: '扩展不含模拟器 Ember' },
  { pattern: /\.(nes|rom)$/i, reason: '扩展不含 ROM 文件' },
  { pattern: /(^|[/_.-])bios([/_.-]|$)/i, reason: '扩展不含 BIOS 文件' }
];

/** 中央目录结束记录的签名 */
const END_OF_CENTRAL_DIRECTORY = 0x06054b50;

/** 中央目录文件头的签名 */
const CENTRAL_DIRECTORY_ENTRY = 0x02014b50;

/**
 * 读取 zip 文件的中央目录。
 * @param {string} file zip 文件路径
 * @returns {{names: string[], has: (name: string) => boolean, read: (name: string) => Buffer}} 文件名列表与读取函数
 * @throws {Error} 不是有效的 zip 文件，或用了本读取器不支持的特性（zip64、不认识的压缩方式）
 */
function openZip(file) {
  const buffer = readFileSync(file);
  let end = -1;
  for (let index = buffer.length - 22; index >= Math.max(0, buffer.length - 65_557); index -= 1) {
    if (buffer.readUInt32LE(index) === END_OF_CENTRAL_DIRECTORY) {
      end = index;
      break;
    }
  }
  if (end < 0) throw new Error(`${file} 不是有效的 zip 文件`);

  const count = buffer.readUInt16LE(end + 10);
  let offset = buffer.readUInt32LE(end + 16);
  /** @type {Map<string, {method: number, size: number, localOffset: number}>} */
  const entries = new Map();
  for (let index = 0; index < count; index += 1) {
    if (buffer.readUInt32LE(offset) !== CENTRAL_DIRECTORY_ENTRY) throw new Error(`${file} 的中央目录已损坏`);
    const method = buffer.readUInt16LE(offset + 10);
    const size = buffer.readUInt32LE(offset + 20);
    const nameLength = buffer.readUInt16LE(offset + 28);
    const extraLength = buffer.readUInt16LE(offset + 30);
    const commentLength = buffer.readUInt16LE(offset + 32);
    const localOffset = buffer.readUInt32LE(offset + 42);
    if (size === 0xff_ff_ff_ff || localOffset === 0xff_ff_ff_ff) throw new Error(`${file} 用了 zip64，本脚本不支持`);
    const name = buffer.toString('utf8', offset + 46, offset + 46 + nameLength);
    entries.set(name, { method, size, localOffset });
    offset += 46 + nameLength + extraLength + commentLength;
  }

  return {
    names: [...entries.keys()],
    has: (name) => entries.has(name),
    read(name) {
      const entry = entries.get(name);
      if (entry === undefined) throw new Error(`${file} 里没有 ${name}`);
      const dataStart = entry.localOffset + 30 + buffer.readUInt16LE(entry.localOffset + 26) + buffer.readUInt16LE(entry.localOffset + 28);
      const data = buffer.subarray(dataStart, dataStart + entry.size);
      if (entry.method === 0) return data;
      if (entry.method === 8) return inflateRawSync(data);
      throw new Error(`${name} 用了不认识的压缩方式 ${entry.method}`);
    }
  };
}

/**
 * 解析 JSON 文本。
 * @param {Buffer} data 文件内容
 * @param {string} name 文件名，用在出错说明里
 * @returns {any} 解析结果
 * @throws {Error} 不是合法的 JSON
 */
function parseJson(data, name) {
  try {
    return JSON.parse(data.toString('utf8').replace(/^\uFEFF/, ''));
  } catch (error) {
    throw new Error(`${name} 不是合法的 JSON：${error instanceof Error ? error.message : String(error)}`);
  }
}

/**
 * 核对 .vsix 的内容。
 * @param {string} vsixFile .vsix 文件路径
 * @returns {{errors: string[], fileCount: number}} 问题清单与 .vsix 里的文件数
 */
export function checkPackage(vsixFile) {
  const zip = openZip(vsixFile);
  /** @type {string[]} */
  const errors = [];
  const files = new Set(zip.names.filter((name) => name.startsWith(PREFIX)).map((name) => name.slice(PREFIX.length)));
  /** 要求某个文件存在 */
  const mustHave = (name, why) => {
    if (!files.has(name)) errors.push(`缺少 ${name}（${why}）`);
  };

  const extensionManifest = zip.has(`${PREFIX}package.json`) ? parseJson(zip.read(`${PREFIX}package.json`), 'package.json') : undefined;
  if (extensionManifest === undefined) {
    errors.push('缺少 package.json');
  } else {
    const main = String(extensionManifest.main ?? '').replace(/^\.\//, '');
    mustHave(main, '扩展入口 main 指向的文件');
  }

  mustHave('debug/openrpc.json', '调试接口描述');
  // vsce 把这三个文件改成小写文件名，扩展名也可能不同，按不分大小写匹配
  const lowerNames = [...files].map((name) => name.toLowerCase());
  for (const [stem, reason] of [['license', '许可证'], ['readme', '说明'], ['changelog', '更新记录']]) {
    if (!lowerNames.some((name) => name === stem || name.startsWith(`${stem}.`))) errors.push(`缺少 ${stem}（${reason}）`);
  }

  // 禁止出现的路径
  for (const name of files) {
    for (const { pattern, reason } of FORBIDDEN) {
      if (pattern.test(name)) errors.push(`不应包含 ${name}：${reason}`);
    }
  }

  // 工具链：清单里的文件都在且校验值一致，没有清单之外的文件
  if (!files.has('toolchain/manifest.json')) {
    errors.push('缺少 toolchain/manifest.json（cc65 工具链的清单）');
  } else {
    const manifest = parseJson(zip.read(`${PREFIX}toolchain/manifest.json`), 'toolchain/manifest.json');
    const listed = new Set(Object.keys(manifest.files ?? {}));
    if (listed.size === 0) errors.push('toolchain/manifest.json 没有登记任何文件');
    for (const [relative, expected] of Object.entries(manifest.files ?? {})) {
      const name = `toolchain/${relative}`;
      if (!files.has(name)) {
        errors.push(`缺少 ${name}（工具链清单里的文件）`);
        continue;
      }
      const actual = createHash('sha256').update(zip.read(`${PREFIX}${name}`)).digest('hex');
      if (actual !== expected) errors.push(`${name} 的 SHA-256 与清单不一致`);
    }
    for (const name of files) {
      if (name.startsWith('toolchain/') && name !== 'toolchain/manifest.json' && !listed.has(name.slice('toolchain/'.length))) {
        errors.push(`${name} 不在工具链清单里`);
      }
    }
    for (const tool of ['cc65', 'ca65', 'ld65', 'ar65']) mustHave(`toolchain/bin/${tool}.exe`, '工具链的可执行文件');
  }

  // 目标：目标定义与运行库
  const targets = [...files].map((name) => /^targets\/([^/]+)\/target\.json$/.exec(name)?.[1]).filter((name) => name !== undefined);
  for (const target of REQUIRED_TARGETS) {
    if (!targets.includes(target)) errors.push(`缺少目标 ${target}（targets/${target}/target.json）`);
  }
  for (const target of targets) {
    const buildFile = `targets/${target}/lib-build.json`;
    if (!files.has(buildFile)) continue;
    const build = parseJson(zip.read(`${PREFIX}${buildFile}`), buildFile);
    for (const library of [...Object.keys(build.libraries ?? {}), ...Object.keys(build.objects ?? {})]) {
      mustHave(`targets/${target}/${library}`, `${target} 目标的运行库`);
    }
  }

  // 项目模板
  mustHave('templates/_common/gitignore', '模板的公共文件');
  for (const template of REQUIRED_TEMPLATES) {
    mustHave(`templates/${template}/template.json`, '模板定义');
    mustHave(`templates/${template}/README.md`, '模板说明');
    if (!files.has(`templates/${template}/src/hello.c`) && !files.has(`templates/${template}/src/hello.s`)) {
      errors.push(`缺少 templates/${template}/src/hello.c 或 hello.s（模板的示例程序）`);
    }
  }

  // 使用文档：登记表里的每份文档都在
  if (!files.has('docs/topics.json')) {
    errors.push('缺少 docs/topics.json（使用文档的主题登记表）');
  } else {
    const topics = parseJson(zip.read(`${PREFIX}docs/topics.json`), 'docs/topics.json').topics ?? [];
    for (const topic of topics) {
      mustHave(topic.root === 'extension' ? topic.file : `docs/${topic.file}`, `使用文档主题 ${topic.topic}`);
    }
  }

  return { errors, fileCount: files.size };
}

/** 命令行入口 */
function main() {
  const argument = process.argv[2];
  let vsixFile = argument === undefined ? undefined : path.resolve(argument);
  if (vsixFile === undefined) {
    const version = JSON.parse(readFileSync(path.join(EXTENSION_DIRECTORY, 'package.json'), 'utf8')).version;
    vsixFile = path.join(REPOSITORY_ROOT, 'build', 'vscode', `fcdev-win32-x64-${version}.vsix`);
  }
  if (!existsSync(vsixFile)) {
    console.error(`找不到 ${vsixFile}：先在 vscode/ 里运行 npm run package`);
    process.exit(1);
  }
  const { errors, fileCount } = checkPackage(vsixFile);
  if (errors.length > 0) {
    console.error(`${path.basename(vsixFile)} 有 ${errors.length} 处问题：`);
    for (const error of errors) console.error(`  ${error}`);
    process.exit(1);
  }
  console.log(`核对通过：${path.basename(vsixFile)}，${fileCount} 个文件`);
}

if (process.argv[1] !== undefined && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) main();
