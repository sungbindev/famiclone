// ------------------------------------------------------------------------
// 名称：create.test.mjs
// 说明：测试 create-fcdev：参数解析、模板列表、项目生成、目录检查、交互选择、安装流程，以及四个模板都能用 fcdev 构建。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：安装与 IDE 配置命令用替身，不真的运行 npm；构建部分需要 cc65，找不到时跳过。
// ------------------------------------------------------------------------
import assert from 'node:assert/strict';
import { existsSync, mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { Readable } from 'node:stream';
import { afterEach, beforeEach, describe, it } from 'node:test';
import { buildPath } from '../../fcdev/src/build/index.mjs';
import { findTestToolchain } from '../../fcdev/test/helpers.mjs';
import { parseCreateArguments } from '../src/args.mjs';
import { runCreate } from '../src/index.mjs';
import { readOwnVersion, scaffoldProject, toPackageName } from '../src/scaffold.mjs';
import { findTemplate, listTemplates } from '../src/templates.mjs';

describe('参数解析', () => {
  it('目录、--target、--template、--no-install', () => {
    const parsed = parseCreateArguments(['my-game', '--target', 'nes', '--template=asm', '--no-install']);
    assert.deepEqual(parsed, { directory: 'my-game', target: 'nes', language: 'asm', install: false, help: false, version: false });
  });

  it('缺省时目录为 undefined，默认安装', () => {
    const parsed = parseCreateArguments([]);
    assert.equal(parsed.directory, undefined);
    assert.equal(parsed.install, true);
  });

  it('未知选项、缺少值、多个目录都报错', () => {
    assert.throws(() => parseCreateArguments(['--nope']), /未知的选项/);
    assert.throws(() => parseCreateArguments(['--target']), /--target 缺少值/);
    assert.throws(() => parseCreateArguments(['--target', '--no-install']), /--target 缺少值/);
    assert.throws(() => parseCreateArguments(['a', 'b']), /只能指定一个目录/);
  });
});

describe('模板', () => {
  it('有四个模板：nes 与 sb2000 各有 c 和 asm', () => {
    assert.deepEqual(listTemplates().map((item) => item.name), ['nes-asm', 'nes-c', 'sb2000-asm', 'sb2000-c']);
  });

  it('按目标与语言找模板，找不到时列出可用的目标与语言', () => {
    assert.equal(findTemplate('sb2000', 'c').name, 'sb2000-c');
    assert.throws(() => findTemplate('nope', 'c'), (error) => error.code === 'TEMPLATE_NOT_FOUND' && error.hint.includes('sb2000') && error.hint.includes('asm'));
  });

  it('每个模板都有示例源码和说明，SB2000 模板带 disk 脚本', () => {
    for (const template of listTemplates()) {
      const source = template.language === 'c' ? 'src/hello.c' : 'src/hello.s';
      assert.ok(existsSync(path.join(template.dir, source)), `${template.name} 缺少 ${source}`);
      assert.ok(existsSync(path.join(template.dir, 'README.md')), `${template.name} 缺少 README.md`);
    }
    assert.equal(findTemplate('sb2000', 'asm').scripts.disk, 'fcdev disk src');
    assert.equal(findTemplate('nes', 'c').scripts.disk, undefined);
  });
});

describe('生成项目', () => {
  let root;

  beforeEach(() => {
    root = mkdtempSync(path.join(tmpdir(), 'create-fcdev-test-'));
  });

  afterEach(() => rmSync(root, { recursive: true, force: true }));

  it('包名由目录名得到：小写，非法字符换成连字符', () => {
    assert.equal(toPackageName('My Game'), 'my-game');
    assert.equal(toPackageName('.hidden'), 'hidden');
    assert.equal(toPackageName('***'), 'fcdev-app');
    assert.equal(toPackageName('ok_name-1'), 'ok_name-1');
  });

  it('生成源码、配置、README 与 .gitignore，不复制 template.json', () => {
    const directory = path.join(root, 'My Game');
    const { name, files } = scaffoldProject({ directory, template: findTemplate('sb2000', 'c') });
    assert.equal(name, 'my-game');
    assert.deepEqual(files, ['.gitignore', 'README.md', 'fcdev.json', 'package.json', 'src/hello.c']);
    assert.match(readFileSync(path.join(directory, 'README.md'), 'utf8'), /^# my-game\n/);
    assert.match(readFileSync(path.join(directory, '.gitignore'), 'utf8'), /node_modules\//);
  });

  it('package.json 依赖与创建包同版本的 fcdev，脚本包含 build、start、sync，SB2000 另有 disk', () => {
    const directory = path.join(root, 'p');
    scaffoldProject({ directory, template: findTemplate('sb2000', 'asm') });
    const document = JSON.parse(readFileSync(path.join(directory, 'package.json'), 'utf8'));
    assert.equal(document.devDependencies.fcdev, readOwnVersion());
    assert.deepEqual(document.scripts, { build: 'fcdev build src', start: 'fcdev run src', sync: 'fcdev sync', disk: 'fcdev disk src' });
    assert.equal(document.private, true);
  });

  it('fcdev.json 写目标与程序名 hello', () => {
    const directory = path.join(root, 'p');
    scaffoldProject({ directory, template: findTemplate('nes', 'c') });
    assert.deepEqual(JSON.parse(readFileSync(path.join(directory, 'fcdev.json'), 'utf8')), { target: 'nes', name: 'hello' });
  });

  it('目录不存在时创建，已有 .git 的空目录可以用', () => {
    const directory = path.join(root, 'with-git');
    mkdirSync(path.join(directory, '.git'), { recursive: true });
    const { files } = scaffoldProject({ directory, template: findTemplate('nes', 'asm') });
    assert.ok(files.includes('src/hello.s'));
    assert.ok(!files.some((file) => file.startsWith('.git/')));
  });

  it('目录里已有其他文件时报错，并且不动已有文件', () => {
    const directory = path.join(root, 'busy');
    mkdirSync(directory);
    writeFileSync(path.join(directory, 'notes.txt'), 'keep');
    assert.throws(() => scaffoldProject({ directory, template: findTemplate('nes', 'c') }), (error) => error.code === 'DIRECTORY_NOT_EMPTY' && error.hint.includes('notes.txt'));
    assert.equal(readFileSync(path.join(directory, 'notes.txt'), 'utf8'), 'keep');
    assert.equal(existsSync(path.join(directory, 'package.json')), false);
  });
});

describe('创建流程', () => {
  let root;
  let stdout;
  let stderr;
  let commands;

  /** 构造运行环境；commandResults 依次给出外部命令的退出码 */
  function context({ interactive = false, input = '', commandResults = [] } = {}) {
    return {
      cwd: root,
      stdin: Readable.from([input]),
      stdout: { write: (text) => stdout.push(text) },
      stderr: { write: (text) => stderr.push(text) },
      interactive,
      runCommand: async (command, args, cwd) => {
        commands.push({ command, args, cwd });
        return commandResults.shift() ?? 0;
      },
    };
  }

  beforeEach(() => {
    root = mkdtempSync(path.join(tmpdir(), 'create-fcdev-flow-'));
    stdout = [];
    stderr = [];
    commands = [];
  });

  afterEach(() => rmSync(root, { recursive: true, force: true }));

  it('--help 与 --version', async () => {
    assert.equal(await runCreate(['--help'], context()), 0);
    assert.match(stdout.join(''), /npm create fcdev/);
    stdout.length = 0;
    assert.equal(await runCreate(['--version'], context()), 0);
    assert.equal(stdout.join('').trim(), readOwnVersion());
  });

  it('参数齐全时直接生成，并依次运行 npm install 与 npm run sync', async () => {
    assert.equal(await runCreate(['game', '--target', 'nes', '--template', 'c'], context()), 0);
    assert.deepEqual(commands, [
      { command: 'npm', args: ['install'], cwd: path.join(root, 'game') },
      { command: 'npm', args: ['run', 'sync'], cwd: path.join(root, 'game') },
    ]);
    assert.ok(existsSync(path.join(root, 'game', 'src', 'hello.c')));
    assert.match(stdout.join(''), /cd game/);
    assert.match(stdout.join(''), /npm run build/);
    assert.doesNotMatch(stdout.join(''), /npm run sync/);
    assert.match(stdout.join(''), /npx fcdev docs/);
  });

  it('--no-install 不运行外部命令，并提示先安装依赖', async () => {
    assert.equal(await runCreate(['game', '--target', 'sb2000', '--template', 'asm', '--no-install'], context()), 0);
    assert.deepEqual(commands, []);
    assert.match(stdout.join(''), /npm install/);
    assert.match(stdout.join(''), /npm run sync/);
    assert.match(stdout.join(''), /npm run disk/);
    assert.match(stdout.join(''), /firmware/);
    assert.doesNotMatch(stdout.join(''), /npx fcdev api/, 'asm 模板没有 C 接口');
  });

  it('SB2000 的 C 模板提示 npx fcdev api', async () => {
    assert.equal(await runCreate(['game', '--target', 'sb2000', '--template', 'c', '--no-install'], context()), 0);
    assert.match(stdout.join(''), /npx fcdev api/);
  });

  it('目录参数缺省或为 . 时在当前目录生成', async () => {
    assert.equal(await runCreate(['.', '--target', 'nes', '--template', 'asm', '--no-install'], context()), 0);
    assert.ok(existsSync(path.join(root, 'src', 'hello.s')));
    assert.doesNotMatch(stdout.join(''), /cd /);
  });

  it('不能交互又缺少 --target 或 --template 时报错并提示参数', async () => {
    assert.equal(await runCreate(['game'], context({ interactive: false })), 1);
    assert.match(stderr.join(''), /--target/);
    assert.equal(existsSync(path.join(root, 'game')), false);
  });

  it('可以交互时询问目标与语言，输入序号或名称都行', async () => {
    const code = await runCreate(['game', '--no-install'], context({ interactive: true, input: '2\nasm\n' }));
    assert.equal(code, 0);
    assert.ok(existsSync(path.join(root, 'game', 'src', 'hello.s')), '第 2 个目标是 sb2000，语言 asm');
    assert.equal(JSON.parse(readFileSync(path.join(root, 'game', 'fcdev.json'), 'utf8')).target, 'sb2000');
  });

  it('交互时输入不合法会重新询问，连续 3 次不合法就报错', async () => {
    assert.equal(await runCreate(['game', '--no-install'], context({ interactive: true, input: 'x\ny\nz\n' })), 1);
    assert.match(stderr.join(''), /连续 3 次输入不合法/);
  });

  it('目录不为空时报错，退出码 1', async () => {
    mkdirSync(path.join(root, 'game'));
    writeFileSync(path.join(root, 'game', 'a.txt'), '');
    assert.equal(await runCreate(['game', '--target', 'nes', '--template', 'c'], context()), 1);
    assert.match(stderr.join(''), /不为空/);
    assert.deepEqual(commands, []);
  });

  it('npm install 失败时保留已生成的文件并提示怎么继续', async () => {
    assert.equal(await runCreate(['game', '--target', 'nes', '--template', 'c'], context({ commandResults: [1] })), 1);
    assert.match(stderr.join(''), /npm install 失败/);
    assert.match(stderr.join(''), /重新运行 npm install/);
    assert.ok(existsSync(path.join(root, 'game', 'package.json')));
    assert.equal(commands.length, 1);
  });

  it('未知的目标报错并列出可用目标', async () => {
    assert.equal(await runCreate(['game', '--target', 'nope', '--template', 'c', '--no-install'], context()), 1);
    assert.match(stderr.join(''), /sb2000/);
  });
});

describe('四个模板都能用 fcdev 构建', { skip: findTestToolchain() ? false : '本机没有 cc65 工具链' }, () => {
  let root;

  beforeEach(() => {
    root = mkdtempSync(path.join(tmpdir(), 'create-fcdev-build-'));
  });

  afterEach(() => rmSync(root, { recursive: true, force: true }));

  for (const template of listTemplates()) {
    it(`${template.name} 生成后 fcdev build src 成功`, async () => {
      const directory = path.join(root, template.name);
      scaffoldProject({ directory, template });
      const { result } = await buildPath({ pathArg: 'src', cwd: directory });
      const image = readFileSync(result.outputFile);
      if (template.target === 'nes') {
        assert.equal(path.basename(result.outputFile), 'hello.nes');
        assert.equal(image.subarray(0, 4).toString('latin1'), 'NES\x1a');
      } else {
        assert.equal(path.basename(result.outputFile), 'HELLO.EXE');
        assert.deepEqual([image[0], image[3], image[4]], [0x4c, 0xaa, 0x55]);
      }
    });
  }
});
