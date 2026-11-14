// ------------------------------------------------------------------------
// 名称：sb2000-runner.mjs
// 说明：SB2000 运行库测试的公共工具：扫描码、软盘镜像文件项；在模拟器里运行测试程序的部分暂时停用。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-11
// 备注：Ember 还没有支持学习机整机，运行库的测试（sb2000-library.test.mjs）整组跳过；
//       原来的做法是构建测试程序放进软盘镜像，在模拟器里运行到 sb_test_sync、sb_test_done 停靠点，读出 sb_test_* 变量里的检查结果，
//       Ember 支持 SB2000 后用调试服务（machines.boot、media.floppyInsert、inspector.readMemory 等）重写 startSb2000Program 与会话类。
// ------------------------------------------------------------------------

/** 测试跳过说明：Ember 支持 SB2000 前总是跳过 */
export const SKIP_WITHOUT_SB2000_RUNTIME = 'Ember 还没有支持学习机整机，SB2000 运行库测试暂时跳过';

/** DirectInput 扫描码：字母、数字与回车 */
export const SCAN = {
  enter: 0x1c, space: 0x39,
  0: 0x0b, 1: 0x02, 2: 0x03, 3: 0x04, 4: 0x05, 5: 0x06, 6: 0x07, 7: 0x08, 8: 0x09, 9: 0x0a,
  q: 0x10, w: 0x11, e: 0x12, r: 0x13, t: 0x14, y: 0x15, u: 0x16, i: 0x17, o: 0x18, p: 0x19,
  a: 0x1e, s: 0x1f, d: 0x20, f: 0x21, g: 0x22, h: 0x23, j: 0x24, k: 0x25, l: 0x26,
  z: 0x2c, x: 0x2d, c: 0x2e, v: 0x2f, b: 0x30, n: 0x31, m: 0x32,
};

/**
 * 一次测试运行的会话：Ember 支持 SB2000 后补上按键、读内存、运行到停靠点与读出检查结果的方法。
 * @typedef {object} Sb2000Session
 */

/**
 * 构建测试程序，放进软盘镜像并在模拟器里运行它；Ember 支持 SB2000 前不可用。
 * @param {{dir: string}} _temp 临时目录
 * @param {object} _options sources、extraFiles、children、targetOptions
 * @returns {Promise<Sb2000Session>} 会话
 * @throws {Error} 总是抛出：Ember 还没有支持学习机整机
 */
export async function startSb2000Program(_temp, _options) {
  throw new Error(SKIP_WITHOUT_SB2000_RUNTIME);
}

/**
 * 在临时目录写入一个文本文件的镜像条目，便于给文件类测试提供数据。
 * @param {string} name 8.3 文件名
 * @param {string|Buffer} content 内容
 * @returns {{name: string, data: Buffer}} 镜像文件项
 */
export function imageFile(name, content) {
  return { name, data: Buffer.isBuffer(content) ? content : Buffer.from(content) };
}
