/*
 * ------------------------------------------------------------------------
 * 名称：build-info.mjs
 * 说明：提供界面用的虚拟模块 virtual:ember-build-info：默认导出编译时间（本机时间，格式 YYYY-MM-DD HH:mm:ss）。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-16
 * 备注：编译时间在 Vite 读取配置时取得，一次打包里只有一个值；开发模式下是开发服务器的启动时间。
 * ------------------------------------------------------------------------
 */

/** 界面用的虚拟模块。 */
export const BUILD_INFO_MODULE = 'virtual:ember-build-info';

/** 补零到两位。 */
function pad(value) {
  return String(value).padStart(2, '0');
}

/**
 * 格式化为本机时间 YYYY-MM-DD HH:mm:ss。
 * @param {Date} date 时间。
 */
export function formatBuildTime(date) {
  const day = `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}`;
  return `${day} ${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(date.getSeconds())}`;
}

/** Vite 插件：提供虚拟模块，创建插件的时刻就是编译时间。 */
export function buildInfoPlugin() {
  const resolved = `\0${BUILD_INFO_MODULE}`;
  const buildTime = formatBuildTime(new Date());
  return {
    name: 'ember-build-info',
    resolveId(id) {
      return id === BUILD_INFO_MODULE ? resolved : undefined;
    },
    load(id) {
      return id === resolved ? `export default ${JSON.stringify(buildTime)};` : undefined;
    }
  };
}
