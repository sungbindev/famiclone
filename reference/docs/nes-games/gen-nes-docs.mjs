// 生成 NES 游戏分类文档：读取 iNES 头得到映射号，按名称关键词归类，并把 .nes 文件整理到分类目录。
import fs from 'node:fs';
import path from 'node:path';

const SRC = 'D:/GitCode/famiclone/reference/nes-games';
const OUT = 'D:/GitCode/famiclone/reference/docs/nes-games';
const NES_WIN = 'D:\\GitCode\\famiclone\\reference\\nes-games';

const GENRES = ['动作', '射击', '平台', '角色扮演', '策略模拟', '格斗', '体育', '赛车', '益智', '棋牌', '冒险解谜', '学习教育', '其他'];

const MAPPER_NAMES = {
  0: 'NROM', 1: 'MMC1', 2: 'UxROM', 3: 'CNROM', 4: 'MMC3', 5: 'MMC5', 7: 'AxROM',
  9: 'MMC2', 10: 'MMC4', 11: 'Color Dreams', 19: 'Namco 129/163', 21: 'VRC4', 22: 'VRC2',
  23: 'VRC2/VRC4', 24: 'VRC6', 25: 'VRC2/VRC4', 26: 'VRC6', 66: 'GxROM', 69: 'Sunsoft FME-7',
  71: 'Camerica', 75: 'VRC1', 85: 'VRC7', 87: 'J87', 88: 'Namco 118', 95: 'Namco 118',
  118: 'TxSROM', 119: 'TQROM', 140: 'Jaleco JF-11/14',
};

// 先按作品名做人工覆盖，再按关键词归类；顺序决定优先级
const OVERRIDES = [
  [/无双/, '动作'],
  [/医生/, '益智'],
  [/Windows|视窗/, '学习教育'],
  [/星际争霸|机器人大战|超级机器人/, '策略模拟'],
  [/蝙蝠侠|恶魔城|成龙/, '动作'],
  [/马里奥|马力欧|玛利欧|超级玛|玛利兄弟|马利兄弟/, '平台'],
];

const RULES = [
  ['学习教育', /小博士|音乐|KTV|学习|学生电脑|学卡|学习卡|学习机|教学|词典|词霸|声霸|小状元|状元|中英文|英语|英汉|数学|识字|启智|认知|八德|奔力电脑|小百科|金卡|图形推理|趣味成语|教授系统|动动脑|王中霸|科王|金字塔|中学|电脑教室|挑战卡|MXM|ABM|仓库世家|智力小/],
  ['益智', /宾果|大力工头|愤怒的小鸟|惊慌|Q波特|轰炸超人|方块|泡泡龙|推箱子|吃豆|打砖块|挖地雷|挖金|挖掘|炸弹人|俄罗斯|拼图|拼盘|数独|迷宫|魔塔|气球大战|踩气球|敲冰块|弹珠|踩地雷|掉落解谜|解谜|消除|沙漏|打鸭子|打空气|弹射球|上海|淘金|迷糊蛋|摩艾君|趣味|智慧|小猪猪/],
  ['赛车', /赛车|拉力|摩托|公路|火箭|碰碰|超音速机车|自动二轮|竞速|飞车|冲刺赛|机车|F1/i],
  ['棋牌', /游戏王|麻将|麻雀|雀士|扑克|五子棋|围棋|象棋|将棋|花牌|黑白棋|暗棋|推倒胡|锄大D|赌神|掷骰|财神到|棋王|斗地主|桥牌|百家乐/],
  ['射击', /射击|大蜈蚣|千足虫|地道战|合金风暴|机甲战士|交叉火力|荒野大镖客|荒野枪手|火凤凰|空间暗影|双鹰|杀戮战场|神鹰|最终任务|荣誉勋章|雷霆|星际武士|铁板阵|野狼行动|洋枪丸|超越地平线|机械化突袭|杀虫大战|赤色要塞|小人子|重力装甲|幽灵行动|拯救世纪|战机|直升机|空中|沙罗曼蛇|雷电|兵蜂|宇宙巡航|1942|1943|1944|太空入侵|太空射击|太空立体|星际战机|星际战士|魂斗罗|超时空要塞|组合战机|超惑星|小蜜蜂|大蜜蜂|蜜蜂|铁甲威龙|星河战士|高机动|反恐|立体战|海战|突击|坦克|导弹|守护者|战场之狼|飞行|飞龙|超浮游|星空飞箭|外星战/],
  ['策略模拟', /三国忠烈|一揆|七国|林则徐|岳飞|三十六计|破釜|菩故|王之霸者|虎门硝烟|决战太平洋|丝路|哥伦布|隋唐|中东战争|二战|风云|三国志|三国群英|群英|大战略|帝国|战略|大富翁|富翁|大航海|文明|模拟|经营|农场|牧场|争霸|欧陆|楚汉|汉刘邦|西楚|霸王|战国|群雄|乱世|赤壁|千军|英雄无敌|魔法门|黄帝|三国春秋|三国桃园|富甲三国|美国总统|美国富豪|征服|征战|列国|大话三国/],
  ['体育', /金牌|国夫|库尼沃库|踢王|桌球|足球|篮球|网球|排球|棒球|曲棍|高尔夫|台球|撞球|九球|躲避球|康体|跳舞|舞王|冰球|滑雪|奥运|世界杯|花式|热血高校/],
  ['格斗', /少林|霍元甲|黄飞鸿|方世玉|悟空斗剧|格斗|街霸|拳霸|拳|武士魂|真侍魂|幽游白书|筋肉人|摔角|猫党|飞人战士|热血格斗|龙珠|功夫|醉拳|武斗|激斗|大拳/],
  ['平台', /玛琍|原人|猴王|伍佑卫门|吉米克|机器猫|哇咿|三目童子|三眼神童|兔子邦妮|水果狸|松鼠大作战|太空青蛙|亚当斯|马戏团|可鲁可鲁|企鹅|玛丽|玛莉|马利|马力|水管|洛克人|高桥名人|冒险岛|卡比|索尼克|耀西|奇奇与蒂蒂|兔宝宝|大眼蛙|可洛比|可乐比|大金刚|三只小猪|哆啦|蜘蛛侠|小魔怪|雪人兄弟|茶茶丸|罗罗大冒险|小红帽|唐老鸭|米老鼠|顽皮老鸭|汤姆和杰瑞|猫和老鼠|猫捉老鼠|蜡笔小新|樱桃小丸子|鬼之Q太郎|鬼太郎|大力水手|小美人鱼|香蕉王子|美猴王|齐天大圣|怪鸭|企鹅先生|凯蒂猫|加菲猫|宾尼兔|淘气厨师|狮王|摩登原始人|原始人|飞翼人|顽皮狗|小玛琍|鸟的故事|汉堡|幽灵列车/],
  ['角色扮演', /石器时代|皮卡丘|东方不败|格兰蒂亚|黄金太阳|镜花缘|里见八犬|灵界|绿野仙踪|哪吒|女神的救赎|热带之星|荣耀之骑士|屠龙|圣魔术师|时空之轮|双月传|唐伯虎|天龙八部|武林|西天|杨家将|妖怪道中|沙加|重装机兵|SD英雄|神奇宝贝|半熟英雄|傲视天地|绝代英雄|上古神殿|无极魔界|魔幻世界|魔兽世界|梦境之王|混沌世界|勇者|最终幻想|仙剑|轩辕剑|塞尔达|伊苏|圣剑|火焰之纹章|火焰纹章|龙魂|口袋|宠物|数码|圣斗士|圣火|圣铃|魔界塔|光之神话|光神话|光明之神|侠|金庸|英雄传|传说|仙界|魔城传说|魔神|魔法|梦幻|龙之谷|大话|西游|幻世录|幻想|红楼梦|梁山|水浒|吞食|秦记|楚留香|香帅|鹿鼎|射雕|倚天|笑傲|神剑|魔道|灵魂|龙域|御猫|魔天|高达|太空战士|星战情缘|神鬼传奇|神魔|骑士物语|封神|神风剑|剑|轩辕|黑暗之蛊|龙战士|龙之城堡|最游记|神器|失落的/],
  ['冒险解谜', /合金装备|黑客|列车奇案|包青天|泰坦尼克|星际之门|哈利传奇|古墓丽影|生化危机|神探柯南|异形|不可能完成|骷髅13|侦探|地狱神探|拉格朗日|鲁邦三世|探险|寻宝|解谜|大冒险|冒险|盗帅|五号特工|谜/],
  ['动作', /美国队长|国王骑士|特救指令|威洛|鲨鱼|阿尔戈斯|阿拉丁|爆笑三国|戈德曼|地狱极乐丸|恶魔世界|孤独战士|马克斯战士|虎穴|火焰使者|绝影战士|卡洛夫|卡诺夫|荆轲|救火英雄|究极虎|恐龙战队|蕾拉|鸟人战队|魔界霸主|人间兵器|前线大作战|兽王记|神密金三角|时空斗士|时空小子|脱狱|无间道|王子复仇|宇宙警备|宇宙刑警|月风魔传|真田|中华大仙|天王降魔|超级战魂|超级魂|匹诺槽|亡灵崛起|烈火|^怒|机器子|轰天|新魔界|盟军赶死|异域诛魂|太阳超人|无尽的任务|快乐猫|鸵鸟|双截龙|快打|忍者|神龟|赤影|暗黑破坏神|银河战士|吸血|猫捉|热血物语|热血进行曲|街头|突然君|怪物制造者|魔界村|金属装甲|机械战警|铁血战士|战神|战将|激龟|鬼魅|神风|超级忍|刀锋|赏金|能源战士|绿色兵团|特种部队|特攻|铁手套|英雄萨姆|飞天战神/],
];

const collator = new Intl.Collator('zh-Hans-CN');

function readMapper(file) {
  const fd = fs.openSync(file, 'r');
  const b = Buffer.alloc(16);
  fs.readSync(fd, b, 0, 16, 0);
  fs.closeSync(fd);
  if (b.toString('latin1', 0, 4) !== 'NES\x1a') return null;
  let m = (b[6] >> 4) | (b[7] & 0xf0);
  if ((b[7] & 0x0c) === 0x08) m |= (b[8] & 0x0f) << 8;
  return m;
}

function parseName(folder) {
  const tags = [...folder.matchAll(/[\[［]([^\]］]*)[\]］]/g)].map((m) => m[1]).filter(Boolean);
  const notes = [...folder.matchAll(/[\(（]([^)）]*)[\)）]/g)].map((m) => m[1]).filter(Boolean);
  const ver = (folder.match(/-(v[\w.]+)/) || [])[1] || '';
  const base = folder
    .replace(/[\[［].*$/, '')
    .replace(/[\(（].*$/, '')
    .replace(/-v[\w.]+.*$/, '')
    .trim() || folder;
  return { base, notes, ver, tags };
}

function classify(base) {
  for (const [re, g] of OVERRIDES) if (re.test(base)) return g;
  for (const [g, re] of RULES) if (re.test(base)) return g;
  return '其他';
}

const entries = [];
const nesFiles = [];
for (const d of fs.readdirSync(SRC, { withFileTypes: true })) {
  if (d.isDirectory()) {
    for (const f of fs.readdirSync(path.join(SRC, d.name))) if (f.toLowerCase().endsWith('.nes')) nesFiles.push(path.join(SRC, d.name, f));
  } else if (d.name.toLowerCase().endsWith('.nes')) {
    nesFiles.push(path.join(SRC, d.name));
  }
}
for (const file of nesFiles) {
  const f = path.basename(file);
  const folder = f.slice(0, -4);
  const info = parseName(folder);
  const genre = classify(info.base);
  const mapper = readMapper(file);
  const target = path.join(SRC, genre, f);
  if (file !== target) {
    fs.mkdirSync(path.join(SRC, genre), { recursive: true });
    fs.renameSync(file, target);
  }
  entries.push({ folder, nesFile: f, ...info, mapper, genre });
}

entries.sort((a, b) =>
  GENRES.indexOf(a.genre) - GENRES.indexOf(b.genre) ||
  collator.compare(a.base, b.base) ||
  collator.compare(a.folder, b.folder));

entries.forEach((e, i) => { e.id = String(i + 1).padStart(4, '0'); });

const esc = (s) => String(s).replace(/\|/g, '\\|');

const fileOf = (g) => `${String(GENRES.indexOf(g) + 1).padStart(2, '0')}-${g}.md`;

const table = (list) => [
  '| 缩略图 | 游戏 | 映⁠射⁠号 | 文件路径 |',
  '|---|---|---|---|',
  ...list.map((e) => `| ![](thumbs/${e.id}.jpg) | ${esc(e.base)} | ${e.mapper} | ${esc(path.win32.join(NES_WIN, e.genre, e.nesFile))} |`),
].join('\n');

for (const g of GENRES) {
  const list = entries.filter((e) => e.genre === g);
  const body = [
    `# ${g}（${list.length} 个）`,
    '',
    '返回 [总览](README.md)',
    '',
    list.length ? table(list) : '暂无游戏。',
    '',
  ].join('\n');
  fs.writeFileSync(path.join(OUT, fileOf(g)), body, 'utf8');
}

const mapperCount = new Map();
for (const e of entries) mapperCount.set(e.mapper, (mapperCount.get(e.mapper) || 0) + 1);
const mapperRows = [...mapperCount.entries()]
  .sort((a, b) => b[1] - a[1] || a[0] - b[0])
  .map(([m, n]) => `| ${m} | ${MAPPER_NAMES[m] || '—'} | ${n} |`);

const genreRows = GENRES.map((g) => {
  const n = entries.filter((e) => e.genre === g).length;
  return `| [${g}](${fileOf(g)}) | ${n} |`;
});

const readme = [
  '# 红白机（NES）游戏分类',
  '',
  `共 ${entries.length} 个游戏。映射号取自每个 .nes 文件的 iNES 头；类型按游戏名称关键词归类，未逐个人工核对，归入“其他”的需要人工判断。`,
  '',
  '## 类型',
  '',
  '| 类型 | 数量 |',
  '|---|---|',
  ...genreRows,
  '',
  '## 映射号',
  '',
  '| 映射号 | 芯片 | 数量 |',
  '|---|---|---|',
  ...mapperRows,
  '',
  '未标注芯片的映射号多为国产或盗版卡带的专用映射号，此处只显示号码。',
  '',
  '## 说明',
  '',
  '- 同一作品的不同版本（修正版、大字体、汉化等）分别列出，以文件路径区分。',
  '- 缩略图为 192×180，存放在 thumbs 目录。',
  '- 带复制按钮的网页版见 index.html，用浏览器打开即可复制路径。',
  '- “文件路径”列为对应 .nes 文件完整路径，可直接复制到资源管理器或编辑器中打开。',
  '',
  '## 脚本',
  '',
  '- 文件：gen-nes-docs.mjs，与本 README 在同一目录。',
  '- 用途：读取 reference/nes-games 中每个 .nes 文件的 iNES 头得到映射号，按游戏名关键词归入类型，把 .nes 文件移入对应类型目录，并生成本目录的分类文档与网页版。',
  '- 运行：安装 Node.js 后执行 `node gen-nes-docs.mjs`，路径已写为绝对路径，在任意目录运行结果相同。',
  '- 修改分类：编辑脚本中的 OVERRIDES（优先匹配）与 RULES（按类型顺序匹配）。',
  '- 注意：每次运行会覆盖本目录的 README、分类文件和 index.html，请勿直接手改。',
  '- 注意：缩略图按排序编号命名（thumbs/0001.jpg 起），新增或删除游戏会改变后续编号；封面图已删除，脚本不再生成缩略图，需要另行处理。',
  '',
].join('\n');
fs.writeFileSync(path.join(OUT, 'README.md'), readme, 'utf8');

const htmlEsc = (s) => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');

const htmlSections = GENRES.map((g, gi) => {
  const list = entries.filter((e) => e.genre === g);
  const rows = list.map((e) => {
    const p = path.win32.join(NES_WIN, e.genre, e.nesFile);
    return `<tr><td><img src="thumbs/${e.id}.jpg" width="192" height="180" alt=""></td><td>${htmlEsc(e.base)}</td><td>${e.mapper}</td><td class="path">${htmlEsc(p)}</td><td><button type="button" data-path="${htmlEsc(p)}">复制</button></td></tr>`;
  }).join('\n');
  return `<section id="g${gi + 1}"><h2>${g}（${list.length} 个）</h2><table><thead><tr><th>缩略图</th><th>游戏</th><th>映射号</th><th>文件路径</th><th></th></tr></thead><tbody>\n${rows}\n</tbody></table></section>`;
}).join('\n');

const htmlNav = GENRES.map((g, gi) => `<a href="#g${gi + 1}">${g}</a>`).join(' ');

const htmlPage = [
  '<!DOCTYPE html>',
  '<html lang="zh-CN">',
  '<head>',
  '<meta charset="utf-8">',
  '<title>红白机（NES）游戏分类</title>',
  '<style>',
  'body{font-family:"Microsoft YaHei",sans-serif;margin:16px;color:#222}',
  'nav a{margin-right:10px}',
  'table{border-collapse:collapse;width:100%;margin-bottom:24px}',
  'th,td{border-bottom:1px solid #ddd;padding:4px 8px;text-align:left;vertical-align:middle}',
  'td.path{font-family:Consolas,monospace;font-size:12px;word-break:break-all}',
  'th{white-space:nowrap}',
  'button{cursor:pointer;padding:2px 10px}',
  '</style>',
  '</head>',
  '<body>',
  '<h1>红白机（NES）游戏分类</h1>',
  `<p>共 ${entries.length} 个游戏。点击“复制”即可复制文件完整路径。</p>`,
  `<nav>${htmlNav}</nav>`,
  htmlSections,
  '<script>',
  "document.addEventListener('click', function (ev) {",
  "  var btn = ev.target.closest('button[data-path]');",
  '  if (!btn) return;',
  "  var text = btn.getAttribute('data-path');",
  "  var done = function () { btn.textContent = '已复制'; setTimeout(function () { btn.textContent = '复制'; }, 1200); };",
  '  function fallback() {',
  "    var ta = document.createElement('textarea');",
  '    ta.value = text; document.body.appendChild(ta); ta.select();',
  "    document.execCommand('copy'); ta.remove(); done();",
  '  }',
  '  if (navigator.clipboard && window.isSecureContext) {',
  '    navigator.clipboard.writeText(text).then(done, fallback);',
  '  } else { fallback(); }',
  '});',
  '</script>',
  '</body>',
  '</html>',
  '',
].join('\n');
fs.writeFileSync(path.join(OUT, 'index.html'), htmlPage, 'utf8');

console.log('total', entries.length);
for (const g of GENRES) console.log(g, entries.filter((e) => e.genre === g).length);
