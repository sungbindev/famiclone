/*
 * ------------------------------------------------------------------------
 * 名称：make-icons.cjs
 * 说明：由标志图生成应用图标：resources/icon.ico（Windows，含 16 到 256 像素的 7 种尺寸，PNG 压缩）与 src/ui/shell/ember-logo.png（128 像素，界面标题栏的标志）；
 *       原图不是正方形时居中放进透明的正方形画布，缩小时逐级对半再到目标尺寸，小尺寸也清晰。
 * 作者：sungbin
 * 邮箱：sungbin@3578.cn
 * 日期：2026-11-15
 * 备注：用 Electron 运行（画布和 PNG 编码都用它自带的），不增加依赖：npx electron tools/build/make-icons.cjs <标志图.png>；
 *       标志图的原件在文档仓库的 02-设计/Ember/logo.png，换标志时重新运行并提交生成的两个文件；命令行参数里有中文时 PowerShell 5.1 会丢字，先把原图复制到纯英文路径。
 * ------------------------------------------------------------------------
 */

const { app, BrowserWindow } = require('electron');
const { mkdirSync, readFileSync, writeFileSync } = require('node:fs');
const { dirname, resolve } = require('node:path');

/** Windows 图标里的尺寸（像素），从大到小。 */
const ICO_SIZES = [256, 128, 64, 48, 32, 24, 16];
/** 界面里的标志图尺寸（像素）：标题栏里只显示 16 像素，留足高分屏的像素。 */
const LOGO_SIZE = 128;
/** 输出位置。 */
const ICON_FILE = resolve(__dirname, '..', '..', 'resources', 'icon.ico');
const LOGO_FILE = resolve(__dirname, '..', '..', 'src', 'ui', 'shell', 'ember-logo.png');

/** 在页面里执行：把原图放进正方形画布，按各尺寸缩小，返回尺寸到 PNG 数据地址的对照。 */
const RENDER = `(async (source, sizes) => {
  const image = new Image();
  image.src = source;
  await image.decode();
  const canvasOf = (side) => {
    const canvas = document.createElement('canvas');
    canvas.width = side;
    canvas.height = side;
    const context = canvas.getContext('2d');
    context.imageSmoothingEnabled = true;
    context.imageSmoothingQuality = 'high';
    return [canvas, context];
  };
  const side = Math.max(image.width, image.height);
  let [current, context] = canvasOf(side);
  context.drawImage(image, (side - image.width) / 2, (side - image.height) / 2);
  const result = {};
  for (const size of sizes) {
    while (current.width >= size * 2) {
      const [half, halfContext] = canvasOf(Math.floor(current.width / 2));
      halfContext.drawImage(current, 0, 0, half.width, half.height);
      current = half;
    }
    const [final, finalContext] = canvasOf(size);
    finalContext.drawImage(current, 0, 0, size, size);
    result[size] = final.toDataURL('image/png');
  }
  return result;
})`;

/**
 * 把各尺寸的 PNG 排成一个 ICO 文件。
 * @param {Array<{ size: number, png: Buffer }>} images 各尺寸的 PNG。
 */
function buildIco(images) {
  const header = Buffer.alloc(6 + images.length * 16);
  header.writeUInt16LE(0, 0);
  header.writeUInt16LE(1, 2);
  header.writeUInt16LE(images.length, 4);
  let offset = header.length;
  images.forEach((image, index) => {
    const entry = 6 + index * 16;
    // 宽高为 256 时写 0。
    header.writeUInt8(image.size >= 256 ? 0 : image.size, entry);
    header.writeUInt8(image.size >= 256 ? 0 : image.size, entry + 1);
    header.writeUInt16LE(1, entry + 4);
    header.writeUInt16LE(32, entry + 6);
    header.writeUInt32LE(image.png.length, entry + 8);
    header.writeUInt32LE(offset, entry + 12);
    offset += image.png.length;
  });
  return Buffer.concat([header, ...images.map((image) => image.png)]);
}

/** 数据地址里的 PNG 字节。 */
function pngOf(dataUrl) {
  return Buffer.from(dataUrl.slice(dataUrl.indexOf(',') + 1), 'base64');
}

async function main() {
  const sourcePath = process.argv[2];
  if (sourcePath === undefined) throw new Error('用法：npx electron tools/build/make-icons.cjs <标志图.png>');
  const source = `data:image/png;base64,${readFileSync(sourcePath).toString('base64')}`;
  await app.whenReady();
  const window = new BrowserWindow({ show: false });
  await window.loadURL('about:blank');
  const sizes = [...new Set([...ICO_SIZES, LOGO_SIZE])].toSorted((left, right) => right - left);
  const rendered = await window.webContents.executeJavaScript(`${RENDER}(${JSON.stringify(source)}, ${JSON.stringify(sizes)})`);
  mkdirSync(dirname(ICON_FILE), { recursive: true });
  writeFileSync(ICON_FILE, buildIco(ICO_SIZES.map((size) => ({ size, png: pngOf(rendered[size]) }))));
  writeFileSync(LOGO_FILE, pngOf(rendered[LOGO_SIZE]));
  console.log(`已生成 ${ICON_FILE} 与 ${LOGO_FILE}`);
}

main().then(
  () => app.quit(),
  (error) => {
    console.error(error instanceof Error ? error.message : String(error));
    app.exit(1);
  }
);
