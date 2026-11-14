// ------------------------------------------------------------------------
// 名称：release-check.mjs
// 说明：发布前检查：三个包的版本一致、工具链包的清单与文件相符、各包的发布内容完整且不含测试与密钥文件。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：用法 node tools/release-check.mjs；只检查，不修改任何文件，不执行 npm publish。
// ------------------------------------------------------------------------
import { createHash } from 'node:crypto';
import { execFileSync } from 'node:child_process';
import { existsSync, readdirSync, readFileSync, statSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const PACKAGES_DIR = path.join(ROOT, 'packages');
const PACKAGE_NAMES = ['fcdev', 'create-fcdev', 'fcdev-cc65'];

/** 发布内容里不允许出现的路径 */
const FORBIDDEN_PATTERNS = [/(^|\/)test\//, /(^|\/)node_modules\//, /(^|\/)\.env/, /\.(pem|key|pfx|p12)$/i, /\.tgz$/i];

const errors = [];

/**
 * 读取 JSON 文件。
 * @param {string} file 文件路径
 * @returns {any} 解析结果
 */
function readJson(file) {
  return JSON.parse(readFileSync(file, 'utf8'));
}

/**
 * 计算文件的 SHA-256。
 * @param {string} file 文件路径
 * @returns {string} 十六进制摘要
 */
function sha256Of(file) {
  return createHash('sha256').update(readFileSync(file)).digest('hex');
}

/**
 * 递归列出目录下的全部文件，路径相对于 base 且用 / 分隔。
 * @param {string} dir 目录
 * @param {string} base 相对的基准目录
 * @returns {string[]} 文件列表
 */
function listFiles(dir, base) {
  const result = [];
  for (const entry of readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) result.push(...listFiles(full, base));
    else result.push(path.relative(base, full).split(path.sep).join('/'));
  }
  return result;
}

/**
 * 用 npm pack --dry-run 取得包将要发布的文件列表。
 * @param {string} packageName 包名
 * @returns {string[]} 发布的文件路径
 */
function listPackedFiles(packageName) {
  const output = execFileSync('npm', ['pack', '--dry-run', '--json', '--ignore-scripts', '-w', `packages/${packageName}`], {
    cwd: ROOT,
    encoding: 'utf8',
    shell: true,
    stdio: ['ignore', 'pipe', 'ignore'],
  });
  return JSON.parse(output)[0].files.map((file) => file.path);
}

/** 检查三个包的版本一致，且 fcdev 对工具链包的依赖版本与之相同；工具链包的包名在 fcdev 的依赖里。 */
function checkVersions() {
  const manifests = Object.fromEntries(PACKAGE_NAMES.map((name) => [name, readJson(path.join(PACKAGES_DIR, name, 'package.json'))]));
  const versions = new Set(Object.values(manifests).map((manifest) => manifest.version));
  if (versions.size !== 1) errors.push(`三个包的版本不一致：${Object.entries(manifests).map(([name, manifest]) => `${name}@${manifest.version}`).join('、')}`);
  const [version] = versions;
  for (const [name, spec] of Object.entries(manifests.fcdev.optionalDependencies ?? {})) {
    if (spec !== version) errors.push(`fcdev 对 ${name} 的依赖是 ${spec}，应与版本 ${version} 一致`);
  }
  const dependencies = manifests.fcdev.optionalDependencies ?? {};
  for (const directory of PACKAGE_NAMES.filter((name) => name.startsWith('fcdev-'))) {
    const { name } = manifests[directory];
    if (!(name in dependencies)) errors.push(`fcdev 的 optionalDependencies 里没有 ${name}`);
  }
  for (const name of PACKAGE_NAMES) {
    if (!manifests[name].license) errors.push(`${name} 的 package.json 没有 license`);
  }
}

/**
 * 检查工具链包里的文件与清单记录的校验值一致，且没有清单之外的文件。
 * @param {string} packageName 包目录名
 * @param {string[]} directories 参与清单的目录
 */
function checkManifestFiles(packageName, directories) {
  const packageDir = path.join(PACKAGES_DIR, packageName);
  const manifestFile = path.join(packageDir, 'manifest.json');
  if (!existsSync(manifestFile)) {
    errors.push(`${packageName} 没有 manifest.json，先运行对应的构建脚本`);
    return;
  }
  const manifest = readJson(manifestFile);
  for (const [relative, expected] of Object.entries(manifest.files ?? {})) {
    const full = path.join(packageDir, relative);
    if (!existsSync(full)) errors.push(`${packageName} 缺少清单中的文件：${relative}`);
    else if (sha256Of(full) !== expected) errors.push(`${packageName} 的 ${relative} 与清单的校验值不符，需要重新运行构建脚本`);
  }
  for (const directory of directories) {
    const full = path.join(packageDir, directory);
    if (!existsSync(full) || !statSync(full).isDirectory()) continue;
    for (const relative of listFiles(full, packageDir)) {
      if (!(relative in (manifest.files ?? {}))) errors.push(`${packageName} 有清单之外的文件：${relative}`);
    }
  }
}

/**
 * 检查一个包的发布内容。
 * @param {string} packageName 包名
 * @param {string[]} required 必须出现的路径
 */
function checkPackedFiles(packageName, required) {
  const packed = listPackedFiles(packageName);
  const packedSet = new Set(packed);
  for (const file of ['LICENSE', 'package.json', ...required]) {
    if (!packedSet.has(file)) errors.push(`${packageName} 的发布内容缺少 ${file}`);
  }
  for (const file of packed) {
    if (FORBIDDEN_PATTERNS.some((pattern) => pattern.test(file))) errors.push(`${packageName} 的发布内容含不应发布的文件：${file}`);
  }
  return packedSet;
}

/** 检查 fcdev：说明文件、更新记录、全部开发文档、各目标的运行库都在发布内容里，更新记录里有当前版本的标题。 */
function checkFcdev() {
  const packageDir = path.join(PACKAGES_DIR, 'fcdev');
  const required = ['README.md', 'CHANGELOG.md', 'bin/fcdev.mjs'];
  const changelogFile = path.join(packageDir, 'CHANGELOG.md');
  if (existsSync(changelogFile)) {
    const { version } = readJson(path.join(packageDir, 'package.json'));
    const heading = new RegExp(`^## ${version.replaceAll('.', '\\.')}(?![\\d.])`, 'm');
    if (!heading.test(readFileSync(changelogFile, 'utf8'))) errors.push(`fcdev 的 CHANGELOG.md 里没有 ${version} 的更新说明（标题写成 "## ${version}（日期）"）`);
  } else {
    errors.push('fcdev 没有 CHANGELOG.md');
  }
  const docsDir = path.join(packageDir, 'docs');
  if (existsSync(docsDir)) required.push(...listFiles(docsDir, packageDir));
  else errors.push('fcdev 没有 docs 目录，开发文档要随主包发布');
  const targetsDir = path.join(packageDir, 'targets');
  for (const target of readdirSync(targetsDir, { withFileTypes: true }).filter((entry) => entry.isDirectory())) {
    const libDir = path.join(targetsDir, target.name, 'lib');
    if (!existsSync(libDir)) {
      errors.push(`目标 ${target.name} 没有构建运行库，先运行 node tools/build-libs.mjs`);
      continue;
    }
    for (const file of listFiles(libDir, packageDir)) required.push(file);
  }
  checkPackedFiles('fcdev', required);
}

/** 检查 create-fcdev：说明文件与公共模板文件在发布内容里。 */
function checkCreate() {
  checkPackedFiles('create-fcdev', ['README.md', 'bin/create-fcdev.mjs', 'templates/_common/gitignore']);
}

checkVersions();
checkFcdev();
checkCreate();
checkManifestFiles('fcdev-cc65', ['bin', 'include', 'asminc', 'lib', 'cfg', 'LICENSE']);
checkPackedFiles('fcdev-cc65', ['manifest.json', 'bin/cc65.exe', 'bin/ca65.exe', 'bin/ld65.exe', 'bin/ar65.exe', 'bin/da65.exe']);

if (errors.length > 0) {
  for (const error of errors) console.error(`错误：${error}`);
  console.error(`发布检查未通过：${errors.length} 项错误`);
  process.exitCode = 1;
} else {
  console.log(`发布检查通过（${PACKAGE_NAMES.length} 个包）`);
}
