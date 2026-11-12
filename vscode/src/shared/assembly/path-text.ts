// ------------------------------------------------------------------------
// 名称：path-text.ts
// 说明：路径字符串的纯文字处理：取目录、取文件名、拼接并规整 . 与 .. 段。
// 作者：sungbin
// 邮箱：sungbin@3578.cn
// 日期：2026-11-10
// 备注：同时接受“/”与“\”，拼接结果沿用基准目录的分隔符；逻辑层不能引用 node:path，所以放这里。
// ------------------------------------------------------------------------

/** 路径里的分隔符。 */
const SEPARATORS = /[\\/]/;

/** 取路径里最后一个分隔符的位置。 */
function lastSeparator(path: string): number {
  return Math.max(path.lastIndexOf('/'), path.lastIndexOf('\\'));
}

/**
 * 取路径所在的目录。
 * @param path 文件路径。
 * @returns 目录；路径里没有分隔符时为空字符串。
 */
export function dirnameOf(path: string): string {
  const index = lastSeparator(path);
  return index < 0 ? '' : path.slice(0, index);
}

/**
 * 取路径里的文件名。
 * @param path 文件路径。
 * @returns 最后一段。
 */
export function basenameOf(path: string): string {
  return path.slice(lastSeparator(path) + 1);
}

/**
 * 把相对路径拼到目录后面，并规整 `.` 与 `..` 段。
 * @param directory 基准目录，可以是 Windows 盘符路径。
 * @param relative 相对路径；已经是绝对路径时原样规整后返回。
 * @returns 规整后的路径，分隔符沿用基准目录，盘符改为小写，与 VS Code 的文件路径写法一致。
 */
export function joinPath(directory: string, relative: string): string {
  const separator = directory.includes('\\') && !directory.includes('/') ? '\\' : '/';
  const absolute = /^([A-Za-z]:)?[\\/]/.test(relative) || /^[A-Za-z]:/.test(relative);
  const full = absolute ? relative : `${directory}${separator}${relative}`;
  const drive = (/^[A-Za-z]:/.exec(full)?.[0] ?? '').toLowerCase();
  const rest = full.slice(drive.length);
  const rooted = rest.startsWith('/') || rest.startsWith('\\');
  const segments: string[] = [];
  for (const segment of rest.split(SEPARATORS)) {
    if (segment === '' || segment === '.') continue;
    if (segment === '..') segments.pop();
    else segments.push(segment);
  }
  return `${drive}${rooted ? separator : ''}${segments.join(separator)}`;
}
