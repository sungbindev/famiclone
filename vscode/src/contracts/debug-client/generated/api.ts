// 由 ember 的 tools/schema-gen 根据 schema/*.schema.json 生成，不要手改。
/* eslint-disable */

/** 调试协议的版本，握手时校验。 */
export const PROTOCOL_VERSION = 1;

/** 最近打开的清单里的一项：给界面看的显示名与编号，不含路径。 */
export interface CartridgesRecentCartridge {
  /** 编号：本次运行内有效，用于 openRecent */
  id: number;
  /** 显示名：文件名 */
  name: string;
}

/** 加载完成的卡带与加载后的运行状态。 */
export interface CartridgesLoadedCartridge {
  cartridge: RuntimeCartridgeSummary;
  state: RuntimeState;
}

/** 一个已经通过握手的调试连接。 */
export interface DebugConnection {
  /** 连接编号，本次运行内唯一 */
  id: number;
  /** 客户端在 hello 里自报的名称 */
  client: string;
  /** 握手完成的时间，UTC，ISO 8601 */
  connectedAt: string;
}

/** 手柄的一个键绑定到键盘的一个物理键。 */
export interface InputPadKeyBinding {
  /** 手柄的键：turboA、turboB 是连发 A、连发 B */
  button: "a" | "b" | "select" | "start" | "up" | "down" | "left" | "right" | "turboA" | "turboB";
  /** 物理键位，取 KeyboardEvent.code，如 KeyZ、ArrowUp、Enter */
  code: string;
}

/** 手柄的一个键绑定到实体手柄的一个按钮或一个轴的一个方向。 */
export interface InputPadGamepadBinding {
  /** 手柄的键 */
  button: "a" | "b" | "select" | "start" | "up" | "down" | "left" | "right" | "turboA" | "turboB";
  /** 来源：button 是一个按钮，axis 是一个轴的一个方向 */
  kind: "button" | "axis";
  /** 按钮或轴的编号，同浏览器 Gamepad API 的数组下标 */
  index: number;
  /** 轴的方向：negative 是小于 -0.5，positive 是大于 0.5；只有 axis 用 */
  direction?: "negative" | "positive";
}

/** 一个玩家槽位用的实体手柄，以及它的键绑定。 */
export interface InputPadGamepadAssignment {
  /** 手柄的设备标识，取 Gamepad.id（含设备名） */
  id: string;
  /** 同一个设备标识连着多个手柄时，按编号从小到大排的第几个，从 0 开始 */
  occurrence: number;
  /** 手柄的键的绑定；没有写的键没有绑定，同一个键只能写一次 */
  bindings: InputPadGamepadBinding[];
}

/** 一个玩家槽位：键盘的绑定，以及可选的一个实体手柄。 */
export interface InputPadSlot {
  /** 键盘的绑定；没有写的键没有绑定，同一个键只能写一次 */
  keys: InputPadKeyBinding[];
  gamepad?: InputPadGamepadAssignment;
}

/** 一个内存空间。 */
export interface InspectorMemorySpaceInfo {
  /** 空间标识：cpu、ppu（总线视角，读不产生副作用）；board.ram 等主板上的存储；chip.oam、chip.palette 等芯片内的存储；cart.<存储区标识> 卡带上的存储区 */
  id: string;
  /** 说明 */
  description: string;
  /** 大小（字节），地址从 0 到大小减 1 */
  size: number;
  /** 能否写入；卡带的 ROM 存储区不能写 */
  writable: boolean;
}

/** 一个总线地址对应的东西。 */
export interface InspectorAddressInfo {
  /** 种类：ram 与 rom 是存储区里的字节；register 是芯片、设备或 mapper 脚本的寄存器；none 没有设备响应（读到总线上最近的值） */
  kind: "ram" | "rom" | "register" | "none";
  /** 所在存储区的空间标识（如 board.ram、cart.prg），或寄存器所属的芯片（如 chip.ppu、cart.mapper）；没有时省略 */
  region?: string;
  /** 在存储区里的偏移，或在寄存器组里的序号；没有时省略 */
  offset?: number;
  /** 说明 */
  description: string;
}

/** 一项状态。 */
export interface InspectorStateField {
  /** 状态项名称 */
  name: string;
  /** 取值；标志位是 0 或 1 */
  value: number;
  /** 说明（读出时有，写入时可省略） */
  description?: string;
}

/** 一个断点。 */
export interface InspectorBreakpoint {
  /** 断点编号 */
  id: number;
  /** 种类：execute CPU 要执行这个地址的指令之前停下；read、write 总线读写这个范围里的地址，那条指令执行完后停下 */
  kind: "execute" | "read" | "write";
  /** 地址范围的第一个地址 */
  address: number;
  /** 地址范围的最后一个地址（含）；单个地址时与 address 相同 */
  endAddress: number;
  /** 已经命中的次数 */
  hits: number;
}

/** 一个字节的取值条件：CPU 总线上这个地址的字节等于这个值。 */
export interface InspectorByteCondition {
  /** 地址 */
  address: number;
  /** 取值 */
  value: number;
}

/** 一条反汇编的指令。 */
export interface InspectorDisassembledInstruction {
  /** 指令的地址 */
  address: number;
  /** 指令字节，十六进制，空格隔开，如 A9 42 */
  bytes: string;
  /** 助记符与操作数，带有效地址与取值的注释，如 LDA $0180 = 33；写法同 nestest 日志，芯片寄存器类地址的取值显示 00 */
  text: string;
  /** 是不是非官方指令 */
  unofficial: boolean;
}

/** 一段内存，CPU 总线上的地址。 */
export interface InspectorMemoryRangeSpec {
  /** 起始地址 */
  address: number;
  /** 字节数 */
  length: number;
}

/** 一个监视中的状态项。 */
export interface InspectorWatchedValue {
  /** 状态路径，如 cpu.pc、ppu.scanline、mapper.prgBank */
  path: string;
  /** 当前取值 */
  value: number;
}

/** 一个模块的状态。 */
export interface InspectorModuleState {
  /** 模块标识：cpu、ppu；卡带的脚本状态是 mapper */
  module: string;
  /** 状态是否可以用 setState 修改（cpu 的 cycles 除外） */
  writable: boolean;
  /** 状态项 */
  fields: InspectorStateField[];
}

/** 已添加的 ROM 文件：文件名与大小。 */
export interface MachinesRomFile {
  /** 文件名，不含路径 */
  fileName: string;
  /** 文件大小（字节），等于机型的 ROM 容量 */
  size: number;
  /** 界面上显示的 ROM 版本：认得出的 ROM 是版本名或型号名（如 V9.0-98、98型），认不出时是文件名 */
  label: string;
}

/** 从目录添加时没有添加的文件：文件名与原因。 */
export interface MachinesRomSkipped {
  /** 文件名，不含路径 */
  fileName: string;
  /** 原因：unrecognized 认不出是哪个机型的 ROM；exists 这个机型已经有 ROM，保持不变；duplicate 目录里有更靠前的文件已是这个机型的 ROM */
  reason: "unrecognized" | "exists" | "duplicate";
}

/** 一个机型：标识、显示名称、是否需要 ROM、是否已支持与已添加的 ROM。 */
export interface MachinesMachineInfo {
  /** 机型标识：fc、sb97、sb2000、bbg1、bbg98、pec9588、pec586、yuxing */
  id: string;
  /** 显示名称 */
  name: string;
  /** 是否需要 ROM；fc 为 false，其余整机机型（学习机）为 true */
  needsRom: boolean;
  /** 是否已支持；未支持的机型不能添加 ROM，也不能开机 */
  supported: boolean;
  /** 已添加的 ROM；没有添加、或不需要 ROM 时没有这一项 */
  rom?: MachinesRomFile;
}

/** 一项匹配条件：mapper 号必写，其余可选；写出的条件必须全部满足。 */
export interface MappersMapperMatchRule {
  /** mapper 号 */
  mapper: number;
  /** 变体 */
  variant?: number;
  /** PRG 的字节数 */
  prgSize?: number;
  /** CHR 的字节数 */
  chrSize?: number;
  /** PRG 的 CRC-32，8 位十六进制 */
  crc32Prg?: string;
  /** PRG 加 CHR 的 CRC-32，8 位十六进制 */
  crc32All?: string;
}

/** 插件摘要，列表里的一行。 */
export interface MappersMapperSummary {
  /** 插件标识 */
  id: string;
  /** 显示名称 */
  name: string;
  /** 插件版本，主.次.修订 */
  version: string;
  /** 发布者 */
  publisher: string;
  /** 说明 */
  description?: string;
  /** 要求的主机型号 */
  host: string[];
  /** 卡带用到的外设，目前只有 mouse（鼠标）；没有声明的外设，界面不会把它接到卡带上 */
  devices: "mouse"[];
  /** 匹配条件，任一项满足即匹配 */
  match: MappersMapperMatchRule[];
  /** 来源：bundled 预装，user 用户安装 */
  source: "bundled" | "user";
  /** 是否启用 */
  enabled: boolean;
}

/** 清单里的一个存储区。 */
export interface MappersMapperStorageInfo {
  /** 存储区标识 */
  id: string;
  /** 种类 */
  kind: "rom" | "ram";
  /** rom 的数据来源 */
  from?: "file.prg" | "file.chr";
  /** ram 的字节数 */
  size?: number;
  /** ram 是否带电池 */
  battery: boolean;
  /** 来源为空时改用的 ram 的字节数 */
  fallbackSize?: number;
}

/** 清单里的一个窗口。 */
export interface MappersMapperWindowInfo {
  /** 窗口标识 */
  id: string;
  /** 所在总线 */
  bus: "cpu" | "ppu";
  /** 地址范围，如 $8000-$9FFF */
  range: string;
  /** 访问的存储区 */
  storage: string;
  /** 一个 bank 的字节数 */
  size: number;
  /** 复位后指向的 bank：整数、first 或 last */
  initial: string;
  /** 访问方式 */
  access: "r" | "rw";
}

/** 清单里的一段寄存器范围。 */
export interface MappersMapperRegisterInfo {
  /** 所在总线 */
  bus: "cpu" | "ppu";
  /** 地址范围 */
  range: string;
  /** 是否回调读 */
  read: boolean;
  /** 是否回调写 */
  write: boolean;
}

/** 清单里声明的一项脚本状态。 */
export interface MappersMapperStateInfo {
  /** 状态名 */
  name: string;
  /** 位宽 */
  bits: number;
  /** 说明 */
  description: string;
}

/** choice 设置项的一个选项。 */
export interface MappersMapperSettingOption {
  /** 取值 */
  value: string;
  /** 显示文字 */
  label: string;
}

/** 清单里的一个设置项。 */
export interface MappersMapperSettingInfo {
  /** 设置键 */
  key: string;
  /** 显示文字 */
  label: string;
  /** 种类 */
  kind: "text" | "choice" | "switch";
  /** 默认值：text 与 choice 为字符串，switch 为布尔值 */
  defaultValue: unknown;
  /** 说明 */
  description?: string;
  /** choice 的选项 */
  options?: MappersMapperSettingOption[];
}

/** 插件详情：摘要加清单的各部分。 */
export interface MappersMapperDetail {
  summary: MappersMapperSummary;
  /** 存储区 */
  storage: MappersMapperStorageInfo[];
  /** 窗口 */
  windows: MappersMapperWindowInfo[];
  /** 名称表的初始连法 */
  nametable: string;
  /** 寄存器范围 */
  registers: MappersMapperRegisterInfo[];
  /** 脚本状态 */
  state: MappersMapperStateInfo[];
  /** 设置项 */
  settings: MappersMapperSettingInfo[];
}

/** 软驱上注入的故障；全部关闭是正常状态。 */
export interface MediaFloppyFaults {
  /** 取盘：盘片被暂时取出（镜像还在，放回后内容不变） */
  ejected: boolean;
  /** 没有索引脉冲：读扇区编号、读写与格式化命令一直不结束 */
  noIndex: boolean;
  /** 磁头卡住：寻道与回零不移动 */
  seekFail: boolean;
  /** 控制器无响应：收到命令字节后不再处理 */
  unresponsive: boolean;
  /** 读时数据校验出错的逻辑扇区号 */
  badSectors: number[];
}

/** 一个软驱的状态。 */
export interface MediaFloppyStatus {
  /** 驱动器编号，从 0 开始 */
  drive: number;
  /** 里面有没有盘片 */
  inserted: boolean;
  /** 盘片来源的文件名，不含路径；没有盘片时没有这一项 */
  fileName?: string;
  /** 盘片是否写保护 */
  writeProtected: boolean;
  /** 镜像是否有尚未写回文件的修改（学习机写盘后置位，核心自动写回后清除） */
  modified: boolean;
  /** 镜像的大小（字节）；没有盘片时为 0 */
  size: number;
  /** 磁头所在的磁道 */
  cylinder: number;
  /** 马达是否在转 */
  motor: boolean;
  faults: MediaFloppyFaults;
}

/** 并口打印机的状态。 */
export interface MediaPrinterStatus {
  /** 状态：idle 空闲（机器没有运行，或还没有内容），printing 打印中（当前页上有内容），done 打印完成（输出过一页且下一页还没有内容） */
  state: "idle" | "printing" | "done";
  /** 当前页上是否已经有内容 */
  hasContent: boolean;
  /** 本次运行以来已输出的页数 */
  pages: number;
  /** 最近一页的文件名，不含路径；还没有输出过时没有这一项 */
  lastFile?: string;
}

/** 磁带机的状态。 */
export interface MediaTapeStatus {
  /** 磁带机是否接在通电的主机上（有主机并已通电，暂停也算，与主机型号无关）；没接上时可以载入与取出磁带，但不能播放与录音 */
  connected: boolean;
  /** 里面有没有磁带 */
  loaded: boolean;
  /** 磁带（声音文件）的文件名，不含路径；没有磁带时没有这一项 */
  fileName?: string;
  /** 工作状态：stopped 停止，playing 播放，recording 录音 */
  mode: "stopped" | "playing" | "recording";
  /** 当前位置（秒）；录音时是已录的时长 */
  position: number;
  /** 总时长（秒）；不知道或没有磁带时为 0 */
  duration: number;
  /** 是否是立体声磁带：是则左声道是声音、右声道是数据；否则整路既是声音也是数据 */
  stereo: boolean;
  /** 磁带的序号：每放入、取出或换掉一盘磁带加 1（同一个文件重新放入也加 1），时间轴据此判断读到的是不是当前这盘磁带的 */
  serial: number;
}

/** 磁带上的一段空白：数据线上连续没有声音的一段。 */
export interface MediaTapeBlank {
  /** 起点（秒） */
  start: number;
  /** 终点（秒） */
  end: number;
}

/** 磁带的时间轴：数据线上各时刻的声音大小与空白段，载入磁带后由核心在后台分析。 */
export interface MediaTapeTimeline {
  /** 分析的是哪一盘磁带（对应 TapeStatus 的 serial）；没有磁带时为当前的序号，其余项为空 */
  serial: number;
  /** 状态：none 没有磁带或没有可分析的内容，analyzing 分析中，ready 分析完成，failed 分析失败（文件无法读取或解码） */
  state: "none" | "analyzing" | "ready" | "failed";
  /** 分析的进度，0 到 1；分析完成时为 1 */
  progress: number;
  /** 磁带的总时长（秒），按实际解码出的采样数算；分析完成后才有，否则为 0 */
  duration: number;
  /** peaks 里每一项占的时间（秒）；分析完成后才有，否则为 0 */
  step: number;
  /** 数据线的声音大小：每 step 秒一项，取这段时间里的最大幅度占满幅的百分比（有声音时至少为 1）；分析完成后才有 */
  peaks: number[];
  /** 空白段，按时间排列：连续 0.5 秒以上、幅度低于满幅 3% 的一段（含磁带开头与结尾的空白）；分析完成后才有 */
  blanks: MediaTapeBlank[];
}

/** 设置项的分组，首选项页左侧的一项。 */
export interface PreferencesPreferenceGroup {
  /** 分组标识，如 appearance */
  id: string;
  /** 分组的显示名称 */
  label: string;
}

/** 选择类设置项的一个可选值。 */
export interface PreferencesPreferenceOption {
  /** 保存的取值 */
  value: string;
  /** 显示名称 */
  label: string;
}

/** 一个设置项：登记的内容与当前的取值。 */
export interface PreferencesPreferenceSetting {
  /** 设置项标识，分组标识加点再加名称，如 sound.buffer */
  id: string;
  /** 所属分组的标识 */
  group: string;
  /** 显示名称 */
  label: string;
  /** 一两句话的说明 */
  description: string;
  /** 种类：choice 从 options 里选一个，switch 开关（布尔值），integer 范围内的整数，text 不超过 maxLength 个字节的文字 */
  kind: "choice" | "switch" | "integer" | "text";
  /** choice 的可选值，其他种类没有这一项 */
  options?: PreferencesPreferenceOption[];
  /** integer 的最小值，其他种类没有这一项 */
  minimum?: number;
  /** integer 的最大值，其他种类没有这一项 */
  maximum?: number;
  /** text 的最大字节数（UTF-8），其他种类没有这一项 */
  maxLength?: number;
  /** 为 true 时首选项页不显示，只保存取值（如音量） */
  hidden: boolean;
  /** text 设置项输入框旁的按钮：取值是文件夹路径时为 directory，页面显示“选择”按钮；取值由核心生成（置空即重新生成）时为 regenerate，页面显示“更换”按钮（请求恢复默认）并且不显示“恢复默认”；其他设置项没有这一项 */
  picker?: "directory" | "regenerate";
  /** 出厂默认值：choice 为字符串，switch 为布尔值，integer 为整数 */
  defaultValue: unknown;
  /** 当前取值，类型同 defaultValue */
  value: unknown;
}

/** 卡带上的一块存储区。 */
export interface RuntimeCartridgeStorage {
  /** 清单里的存储区标识 */
  id: string;
  /** 种类 */
  kind: "rom" | "ram";
  /** 实际容量（字节） */
  size: number;
  /** 是否带电池 */
  battery: boolean;
}

/** 插在卡槽里的卡带：文件头描述、匹配到的插件与存储区清单。 */
export interface RuntimeCartridgeSummary {
  /** NES 文件的路径 */
  path: string;
  /** mapper 号 */
  mapper: number;
  /** 变体 */
  variant: number;
  /** PRG 的字节数 */
  prgSize: number;
  /** CHR 的字节数，没有 CHR 时为 0 */
  chrSize: number;
  /** 文件头里的镜像方式 */
  mirroring: "horizontal" | "vertical" | "four";
  /** 文件头里的电池标志 */
  battery: boolean;
  /** PRG 的 CRC-32，8 位十六进制 */
  crc32Prg: string;
  /** PRG 加 CHR 的 CRC-32，8 位十六进制 */
  crc32All: string;
  /** 匹配到的插件标识 */
  pluginId: string;
  /** 匹配到的插件显示名称 */
  pluginName: string;
  /** 匹配到的插件版本 */
  pluginVersion: string;
  /** 装配的主机型号 */
  host: string;
  /** 匹配到的插件声明的外设，目前只有 mouse（鼠标） */
  devices: "mouse"[];
  /** 存储区清单 */
  storage: RuntimeCartridgeStorage[];
}

/** 运行状态：电源与运行模式；运行模式只在通电时有意义，断电时为 paused。 */
export interface RuntimeState {
  /** 电源：off 断电，on 通电 */
  power: "off" | "on";
  /** 运行模式：paused 暂停，running 连续运行 */
  mode: "paused" | "running";
}

/** 应用升级的状态；只由外壳维护。 */
export interface SystemUpdateStatus {
  /** unsupported 当前运行方式不支持升级（开发版、免安装目录、非 Windows），idle 还没有检查过，checking 正在检查，downloading 发现新版本、正在下载，ready 新版本已下载、重启后生效，upToDate 已是最新版本，error 检查或下载失败 */
  state: "unsupported" | "idle" | "checking" | "downloading" | "ready" | "upToDate" | "error";
  /** 新版本的版本号，只在 ready 时有，升级服务器没有给出版本号时没有这一项 */
  version?: string;
  /** 失败的原因，只在 error 时有 */
  message?: string;
}

/** cartridges.load 的参数。 */
export interface CartridgesLoadParams {
  /** NES 文件的路径，由外壳根据用户选择的文件给出 */
  path: string;
  /** 装入后是否上电，默认 true */
  powerOn?: boolean;
  /** 上电后是否直接运行，默认 true；powerOn 为 false 时无效 */
  resume?: boolean;
}

/** cartridges.load 的结果。 */
export interface CartridgesLoadResult {
  cartridge: RuntimeCartridgeSummary;
  state: RuntimeState;
}

/** cartridges.eject 的参数。 */
export type CartridgesEjectParams = Record<string, never>;

/** cartridges.eject 的结果。 */
export interface CartridgesEjectResult {
  state: RuntimeState;
}

/** cartridges.info 的参数。 */
export type CartridgesInfoParams = Record<string, never>;

/** cartridges.info 的结果。 */
export interface CartridgesInfoResult {
  cartridge: RuntimeCartridgeSummary;
}

/** debug.hello 的参数。 */
export interface DebugHelloParams {
  /** 客户端名称，如 copilot、ember-debug-cli */
  client: string;
  /** 调试服务的令牌 */
  token: string;
  /** 客户端使用的协议版本，目前只有 1；省略时按 1 */
  protocol?: number;
}

/** debug.hello 的结果。 */
export interface DebugHelloResult {
  /** 服务端的协议版本 */
  protocol: number;
  /** 核心版本 */
  coreVersion: string;
  /** 应用版本 */
  appVersion: string;
  /** 本连接的编号 */
  session: number;
}

/** debug.subscribe 的参数。 */
export interface DebugSubscribeParams {
  /** 事件名或通配写法 */
  events: string[];
}

/** debug.subscribe 的结果。 */
export interface DebugSubscribeResult {
  /** 订阅之后本连接订阅的全部事件名或通配写法 */
  events: string[];
}

/** debug.unsubscribe 的参数。 */
export interface DebugUnsubscribeParams {
  /** 事件名或通配写法 */
  events: string[];
}

/** debug.unsubscribe 的结果。 */
export interface DebugUnsubscribeResult {
  /** 取消之后本连接订阅的全部事件名或通配写法 */
  events: string[];
}

/** debug.status 的参数。 */
export type DebugStatusParams = Record<string, never>;

/** debug.status 的结果。 */
export interface DebugStatusResult {
  /** 是否开启（首选项里打开，或启动参数 --debug-port 指定） */
  enabled: boolean;
  /** 是否正在监听；开启但端口被占用时为 false */
  listening: boolean;
  /** 监听的端口，没有监听时为 0 */
  port: number;
  /** 开启了但没有监听时的原因；其他情况没有这一项 */
  error?: string;
  /** 当前的连接 */
  connections: DebugConnection[];
}

/** input.getBindings 的参数。 */
export type InputGetBindingsParams = Record<string, never>;

/** input.getBindings 的结果。 */
export interface InputGetBindingsResult {
  /** 玩家槽位，第 1 项是手柄口 1 */
  slots: InputPadSlot[];
}

/** input.setBindings 的参数。 */
export interface InputSetBindingsParams {
  /** 玩家槽位，第 1 项是手柄口 1 */
  slots: InputPadSlot[];
}

/** input.setBindings 的结果。 */
export interface InputSetBindingsResult {
  /** 保存之后的玩家槽位 */
  slots: InputPadSlot[];
}

/** input.resetBindings 的参数。 */
export type InputResetBindingsParams = Record<string, never>;

/** input.resetBindings 的结果。 */
export interface InputResetBindingsResult {
  /** 恢复之后的玩家槽位 */
  slots: InputPadSlot[];
}

/** input.getFamilyKeyboard 的参数。 */
export type InputGetFamilyKeyboardParams = Record<string, never>;

/** input.getFamilyKeyboard 的结果。 */
export interface InputGetFamilyKeyboardResult {
  /** 是否已接入 */
  connected: boolean;
}

/** input.setFamilyKeyboard 的参数。 */
export interface InputSetFamilyKeyboardParams {
  /** true 接入，false 断开 */
  connected: boolean;
}

/** input.setFamilyKeyboard 的结果。 */
export interface InputSetFamilyKeyboardResult {
  /** 设置之后是否已接入 */
  connected: boolean;
}

/** input.key 的参数。 */
export interface InputKeyParams {
  /** 物理键位，取 KeyboardEvent.code，如 KeyW、Space、Numpad1、Enter */
  code: string;
  /** true 按下，false 松开 */
  down: boolean;
}

/** input.key 的结果。 */
export type InputKeyResult = Record<string, never>;

/** input.mouse 的参数。 */
export interface InputMouseParams {
  /** 事件种类 */
  kind: "move" | "down" | "up" | "wheel" | "leave";
  /** 指针在标准帧上的横坐标，可以超出画面范围 */
  x?: number;
  /** 指针在标准帧上的纵坐标，可以超出画面范围 */
  y?: number;
  /** 相对上一次事件的横向位移 */
  dx?: number;
  /** 相对上一次事件的纵向位移 */
  dy?: number;
  /** 当前按住的按键位：1 左键、2 右键、4 中键 */
  buttons?: number;
  /** 滚轮的滚动量，向下为正，只有 wheel 事件用 */
  wheel?: number;
}

/** input.mouse 的结果。 */
export type InputMouseResult = Record<string, never>;

/** input.gamepad 的参数。 */
export interface InputGamepadParams {
  /** 手柄编号，同浏览器 Gamepad API 的 index */
  index: number;
  /** 手柄的设备标识，取 Gamepad.id；省略时按空处理 */
  id?: string;
  /** 是否连着 */
  connected: boolean;
  /** 各按键是否按下，顺序同浏览器的标准映射 */
  buttons?: boolean[];
  /** 各轴的值，范围 -1 到 1，顺序同浏览器的标准映射 */
  axes?: number[];
}

/** input.gamepad 的结果。 */
export type InputGamepadResult = Record<string, never>;

/** input.pad 的参数。 */
export interface InputPadParams {
  /** 手柄口编号，从 0 开始 */
  port: number;
  /** 按键掩码，1 表示按下 */
  buttons: number;
}

/** input.pad 的结果。 */
export type InputPadResult = Record<string, never>;

/** input.releaseAll 的参数。 */
export type InputReleaseAllParams = Record<string, never>;

/** input.releaseAll 的结果。 */
export type InputReleaseAllResult = Record<string, never>;

/** inspector.listSpaces 的参数。 */
export type InspectorListSpacesParams = Record<string, never>;

/** inspector.listSpaces 的结果。 */
export interface InspectorListSpacesResult {
  /** 内存空间 */
  spaces: InspectorMemorySpaceInfo[];
}

/** inspector.readMemory 的参数。 */
export interface InspectorReadMemoryParams {
  /** 空间标识，见 listSpaces */
  space: string;
  /** 起始地址 */
  address: number;
  /** 要读的字节数 */
  length: number;
}

/** inspector.readMemory 的结果。 */
export interface InspectorReadMemoryResult {
  /** 空间标识 */
  space: string;
  /** 起始地址 */
  address: number;
  /** 字节数 */
  length: number;
  /** 读到的字节（二进制，以二进制帧送达） */
  data?: Uint8Array;
}

/** inspector.writeMemory 的参数。 */
export interface InspectorWriteMemoryParams {
  /** 空间标识，见 listSpaces */
  space: string;
  /** 起始地址 */
  address: number;
  /** 要写入的字节，每项 0 到 255 */
  bytes: number[];
}

/** inspector.writeMemory 的结果。 */
export interface InspectorWriteMemoryResult {
  /** 空间标识 */
  space: string;
  /** 起始地址 */
  address: number;
  /** 写入的字节数 */
  written: number;
}

/** inspector.resolve 的参数。 */
export interface InspectorResolveParams {
  /** 总线：cpu 或 ppu */
  space: "cpu" | "ppu";
  /** 总线地址 */
  address: number;
}

/** inspector.resolve 的结果。 */
export interface InspectorResolveResult {
  /** 解析结果 */
  target: InspectorAddressInfo;
}

/** inspector.getState 的参数。 */
export interface InspectorGetStateParams {
  /** 模块标识；省略时返回全部 */
  module?: string;
}

/** inspector.getState 的结果。 */
export interface InspectorGetStateResult {
  /** 各模块的状态 */
  states: InspectorModuleState[];
}

/** inspector.snapshot 的参数。 */
export interface InspectorSnapshotParams {
  /** 只取这个模块的状态；省略时取全部 */
  module?: string;
}

/** inspector.snapshot 的结果。 */
export interface InspectorSnapshotResult {
  /** 自上电以来的 CPU 周期数 */
  cycles: number;
  /** 各模块的状态 */
  states: InspectorModuleState[];
  /** 全部断点 */
  breakpoints: InspectorBreakpoint[];
  /** 全部监视项与当前取值 */
  watches: InspectorWatchedValue[];
}

/** inspector.watch 的参数。 */
export interface InspectorWatchParams {
  /** 状态路径 */
  path: string;
}

/** inspector.watch 的结果。 */
export interface InspectorWatchResult {
  /** 监视项与当前取值 */
  watch: InspectorWatchedValue;
}

/** inspector.unwatch 的参数。 */
export interface InspectorUnwatchParams {
  /** 状态路径 */
  path: string;
}

/** inspector.unwatch 的结果。 */
export type InspectorUnwatchResult = Record<string, never>;

/** inspector.disassemble 的参数。 */
export interface InspectorDisassembleParams {
  /** 起始地址 */
  address: number;
  /** 指令条数，默认 16 */
  count?: number;
}

/** inspector.disassemble 的结果。 */
export interface InspectorDisassembleResult {
  /** 反汇编的指令 */
  instructions: InspectorDisassembledInstruction[];
}

/** inspector.cdlStart 的参数。 */
export type InspectorCdlStartParams = Record<string, never>;

/** inspector.cdlStart 的结果。 */
export interface InspectorCdlStartResult {
  /** PRG-ROM 的字节数，也是标记的字节数 */
  size: number;
}

/** inspector.cdlStop 的参数。 */
export type InspectorCdlStopParams = Record<string, never>;

/** inspector.cdlStop 的结果。 */
export interface InspectorCdlStopResult {
  /** PRG-ROM 的字节数 */
  size: number;
  /** 标了 Code 的字节数 */
  codeBytes: number;
  /** 标了 Data 的字节数 */
  dataBytes: number;
}

/** inspector.cdlSave 的参数。 */
export interface InspectorCdlSaveParams {
  /** 文件名，只能含字母、数字、点、下划线、短横线与空格，不含目录；省略时用 prg.cdl */
  file?: string;
}

/** inspector.cdlSave 的结果。 */
export interface InspectorCdlSaveResult {
  /** 保存的文件的完整路径 */
  file: string;
  /** PRG-ROM 的字节数，也是文件的字节数 */
  size: number;
  /** 标了 Code 的字节数 */
  codeBytes: number;
  /** 标了 Data 的字节数 */
  dataBytes: number;
}

/** inspector.traceStart 的参数。 */
export interface InspectorTraceStartParams {
  /** 跟踪的种类 */
  kind: "instruction" | "io" | "irq";
  /** 文件名，只能含字母、数字、点、下划线、短横线与空格，不含目录；省略时按种类与编号生成（trace-编号.txt、calllog-编号.txt）。文件写在调试输出目录 */
  file?: string;
  /** 最多记录的行数，满了就不再记录，默认 100000 */
  maxLines?: number;
  /** 只对 instruction 有效：每个地址只记第一次，默认 false */
  unique?: boolean;
}

/** inspector.traceStart 的结果。 */
export interface InspectorTraceStartResult {
  /** 记录编号 */
  id: number;
}

/** inspector.traceStop 的参数。 */
export interface InspectorTraceStopParams {
  /** traceStart 返回的记录编号 */
  id: number;
}

/** inspector.traceStop 的结果。 */
export interface InspectorTraceStopResult {
  /** 保存的文件的完整路径 */
  file: string;
  /** 写入的行数 */
  lines: number;
  /** 是否因为行数满了而少记了 */
  truncated: boolean;
}

/** inspector.callLogStart 的参数。 */
export interface InspectorCallLogStartParams {
  /** 要记录的指令地址 */
  address: number;
  /** 每次记录栈顶的字节数，默认 0 */
  stackBytes?: number;
  /** 每次记录的内存段，最多 4 段 */
  ranges?: InspectorMemoryRangeSpec[];
  /** 跳过条件，最多 4 项：执行到这条指令时，任何一项成立（这个地址上的字节等于这个值）就不记录这一次，用来滤掉不关心的调用 */
  skip?: InspectorByteCondition[];
  /** 文件名，只能含字母、数字、点、下划线、短横线与空格，不含目录；省略时按种类与编号生成（trace-编号.txt、calllog-编号.txt）。文件写在调试输出目录 */
  file?: string;
  /** 最多记录的行数，满了就不再记录，默认 100000 */
  maxLines?: number;
}

/** inspector.callLogStart 的结果。 */
export interface InspectorCallLogStartResult {
  /** 记录编号 */
  id: number;
}

/** inspector.callLogStop 的参数。 */
export interface InspectorCallLogStopParams {
  /** callLogStart 返回的记录编号 */
  id: number;
}

/** inspector.callLogStop 的结果。 */
export interface InspectorCallLogStopResult {
  /** 保存的文件的完整路径 */
  file: string;
  /** 写入的行数 */
  lines: number;
  /** 是否因为行数满了而少记了 */
  truncated: boolean;
}

/** inspector.addBreakpoint 的参数。 */
export interface InspectorAddBreakpointParams {
  /** 种类 */
  kind: "execute" | "read" | "write";
  /** 地址，或地址范围的第一个地址 */
  address: number;
  /** 地址范围的最后一个地址（含），省略时只有 address 一个地址 */
  endAddress?: number;
}

/** inspector.addBreakpoint 的结果。 */
export interface InspectorAddBreakpointResult {
  /** 添加的断点 */
  breakpoint: InspectorBreakpoint;
}

/** inspector.listBreakpoints 的参数。 */
export type InspectorListBreakpointsParams = Record<string, never>;

/** inspector.listBreakpoints 的结果。 */
export interface InspectorListBreakpointsResult {
  /** 断点 */
  breakpoints: InspectorBreakpoint[];
}

/** inspector.removeBreakpoint 的参数。 */
export interface InspectorRemoveBreakpointParams {
  /** 断点编号 */
  id: number;
}

/** inspector.removeBreakpoint 的结果。 */
export type InspectorRemoveBreakpointResult = Record<string, never>;

/** inspector.clearBreakpoints 的参数。 */
export type InspectorClearBreakpointsParams = Record<string, never>;

/** inspector.clearBreakpoints 的结果。 */
export interface InspectorClearBreakpointsResult {
  /** 删除的个数 */
  removed: number;
}

/** inspector.setState 的参数。 */
export interface InspectorSetStateParams {
  /** 模块标识 */
  module: string;
  /** 要修改的状态项 */
  fields: InspectorStateField[];
}

/** inspector.setState 的结果。 */
export interface InspectorSetStateResult {
  /** 修改之后的状态 */
  state: InspectorModuleState;
}

/** machines.list 的参数。 */
export type MachinesListParams = Record<string, never>;

/** machines.list 的结果。 */
export interface MachinesListResult {
  /** 机型项 */
  machines: MachinesMachineInfo[];
}

/** machines.setRom 的参数。 */
export interface MachinesSetRomParams {
  /** 机型标识 */
  machine: string;
  /** ROM 文件的路径，由外壳根据用户选择的文件给出，调试服务直接给出 */
  path: string;
}

/** machines.setRom 的结果。 */
export interface MachinesSetRomResult {
  /** 更新后的机型项 */
  machine: MachinesMachineInfo;
}

/** machines.setRoms 的参数。 */
export interface MachinesSetRomsParams {
  /** 目录的路径，由外壳根据用户选择的目录给出，调试服务直接给出 */
  path: string;
}

/** machines.setRoms 的结果。 */
export interface MachinesSetRomsResult {
  /** 添加了 ROM 的机型项（更新后） */
  added: MachinesMachineInfo[];
  /** 没有添加的文件 */
  skipped: MachinesRomSkipped[];
}

/** machines.removeRom 的参数。 */
export interface MachinesRemoveRomParams {
  /** 机型标识 */
  machine: string;
}

/** machines.removeRom 的结果。 */
export interface MachinesRemoveRomResult {
  /** 更新后的机型项 */
  machine: MachinesMachineInfo;
}

/** machines.boot 的参数。 */
export interface MachinesBootParams {
  /** 机型标识 */
  machine: string;
  /** 开机后是否上电，默认 true */
  powerOn?: boolean;
  /** 上电后是否直接运行，默认 true；powerOn 为 false 时无效 */
  resume?: boolean;
}

/** machines.boot 的结果。 */
export interface MachinesBootResult {
  /** 开机的机型项 */
  machine: MachinesMachineInfo;
  state: RuntimeState;
}

/** mappers.list 的参数。 */
export type MappersListParams = Record<string, never>;

/** mappers.list 的结果。 */
export interface MappersListResult {
  /** 插件摘要，按标识排序 */
  plugins: MappersMapperSummary[];
}

/** mappers.info 的参数。 */
export interface MappersInfoParams {
  /** 插件标识 */
  id: string;
}

/** mappers.info 的结果。 */
export interface MappersInfoResult {
  plugin: MappersMapperDetail;
}

/** mappers.install 的参数。 */
export interface MappersInstallParams {
  /** 插件包的文件路径，由外壳根据用户选择的文件给出 */
  path: string;
}

/** mappers.install 的结果。 */
export interface MappersInstallResult {
  plugin: MappersMapperSummary;
}

/** mappers.uninstall 的参数。 */
export interface MappersUninstallParams {
  /** 插件标识 */
  id: string;
}

/** mappers.uninstall 的结果。 */
export type MappersUninstallResult = Record<string, never>;

/** mappers.setEnabled 的参数。 */
export interface MappersSetEnabledParams {
  /** 插件标识 */
  id: string;
  /** 是否启用 */
  enabled: boolean;
}

/** mappers.setEnabled 的结果。 */
export interface MappersSetEnabledResult {
  plugin: MappersMapperSummary;
}

/** mappers.reload 的参数。 */
export type MappersReloadParams = Record<string, never>;

/** mappers.reload 的结果。 */
export interface MappersReloadResult {
  /** 重新加载后生效的插件摘要 */
  plugins: MappersMapperSummary[];
}

/** media.floppyInsert 的参数。 */
export interface MediaFloppyInsertParams {
  /** 驱动器编号 */
  drive: number;
  /** 镜像文件的路径 */
  path: string;
  /** 是否写保护，默认 false */
  writeProtected?: boolean;
}

/** media.floppyInsert 的结果。 */
export interface MediaFloppyInsertResult {
  status: MediaFloppyStatus;
}

/** media.floppyEject 的参数。 */
export interface MediaFloppyEjectParams {
  /** 驱动器编号 */
  drive: number;
}

/** media.floppyEject 的结果。 */
export interface MediaFloppyEjectResult {
  status: MediaFloppyStatus;
}

/** media.floppySave 的参数。 */
export interface MediaFloppySaveParams {
  /** 驱动器编号 */
  drive: number;
}

/** media.floppySave 的结果。 */
export interface MediaFloppySaveResult {
  status: MediaFloppyStatus;
}

/** media.floppyWriteProtect 的参数。 */
export interface MediaFloppyWriteProtectParams {
  /** 驱动器编号 */
  drive: number;
  /** 是否写保护 */
  writeProtected: boolean;
}

/** media.floppyWriteProtect 的结果。 */
export interface MediaFloppyWriteProtectResult {
  status: MediaFloppyStatus;
}

/** media.floppyStatus 的参数。 */
export type MediaFloppyStatusParams = Record<string, never>;

/** media.floppyStatus 的结果。 */
export interface MediaFloppyStatusResult {
  /** 各驱动器的状态，按编号排列 */
  drives: MediaFloppyStatus[];
}

/** media.floppyFault 的参数。 */
export interface MediaFloppyFaultParams {
  /** 驱动器编号 */
  drive: number;
  /** 故障：ejected 取盘（放回时内容不变），writeProtected 写保护，noIndex 无索引脉冲，seekFail 磁头卡住，unresponsive 控制器无响应，badSector 指定扇区校验错，clear 关闭全部故障、放回盘片、取消写保护 */
  fault: "ejected" | "writeProtected" | "noIndex" | "seekFail" | "unresponsive" | "badSector" | "clear";
  /** 打开还是关闭这个故障，默认 true；clear 不用这一项 */
  enabled?: boolean;
  /** badSector 的逻辑扇区号（磁道×每道扇区数×面数 + 磁头×每道扇区数 + 扇区号 - 1），badSector 必填 */
  sector?: number;
}

/** media.floppyFault 的结果。 */
export interface MediaFloppyFaultResult {
  status: MediaFloppyStatus;
}

/** media.printerStatus 的参数。 */
export type MediaPrinterStatusParams = Record<string, never>;

/** media.printerStatus 的结果。 */
export interface MediaPrinterStatusResult {
  status: MediaPrinterStatus;
}

/** media.printerFlush 的参数。 */
export type MediaPrinterFlushParams = Record<string, never>;

/** media.printerFlush 的结果。 */
export interface MediaPrinterFlushResult {
  status: MediaPrinterStatus;
}

/** media.tapeLoad 的参数。 */
export interface MediaTapeLoadParams {
  /** 声音文件的路径 */
  path: string;
}

/** media.tapeLoad 的结果。 */
export interface MediaTapeLoadResult {
  status: MediaTapeStatus;
}

/** media.tapeEject 的参数。 */
export type MediaTapeEjectParams = Record<string, never>;

/** media.tapeEject 的结果。 */
export interface MediaTapeEjectResult {
  status: MediaTapeStatus;
}

/** media.tapePlay 的参数。 */
export type MediaTapePlayParams = Record<string, never>;

/** media.tapePlay 的结果。 */
export interface MediaTapePlayResult {
  status: MediaTapeStatus;
}

/** media.tapeRecord 的参数。 */
export interface MediaTapeRecordParams {
  /** 录音文件的路径；不给时在磁带机里已有的磁带上录 */
  path?: string;
}

/** media.tapeRecord 的结果。 */
export interface MediaTapeRecordResult {
  status: MediaTapeStatus;
}

/** media.tapeStop 的参数。 */
export type MediaTapeStopParams = Record<string, never>;

/** media.tapeStop 的结果。 */
export interface MediaTapeStopResult {
  status: MediaTapeStatus;
}

/** media.tapeStatus 的参数。 */
export type MediaTapeStatusParams = Record<string, never>;

/** media.tapeStatus 的结果。 */
export interface MediaTapeStatusResult {
  status: MediaTapeStatus;
}

/** media.tapeSeek 的参数。 */
export interface MediaTapeSeekParams {
  /** 位置（秒） */
  position: number;
}

/** media.tapeSeek 的结果。 */
export interface MediaTapeSeekResult {
  status: MediaTapeStatus;
}

/** media.tapeTimeline 的参数。 */
export type MediaTapeTimelineParams = Record<string, never>;

/** media.tapeTimeline 的结果。 */
export interface MediaTapeTimelineResult {
  timeline: MediaTapeTimeline;
}

/** output.setVolume 的参数。 */
export interface OutputSetVolumeParams {
  /** 音量百分比，0 到 100 */
  volume?: number;
  /** 是否静音 */
  muted?: boolean;
}

/** output.setVolume 的结果。 */
export interface OutputSetVolumeResult {
  /** 当前的音量百分比 */
  volume: number;
  /** 当前是否静音 */
  muted: boolean;
}

/** output.screenshot 的参数。 */
export interface OutputScreenshotParams {
  /** 图像格式，默认 png；rgba 是未压缩的像素，每像素 4 字节，按行从上到下 */
  format?: "png" | "rgba";
  /** 保存的文件名（不含目录，只能是字母、数字、点、下划线、短横线与空格）；png 没有扩展名时自动补 .png */
  file?: string;
}

/** output.screenshot 的结果。 */
export interface OutputScreenshotResult {
  /** 图像格式 */
  format: "png" | "rgba";
  /** 图像宽度（像素） */
  width: number;
  /** 图像高度（像素） */
  height: number;
  /** 图像数据的字节数 */
  byteLength: number;
  /** 保存后的完整路径；没有带 file 参数时没有这一项 */
  file?: string;
  /** 图像数据（二进制，以二进制帧送达）；带 file 参数时没有这一项 */
  data?: Uint8Array;
}

/** preferences.list 的参数。 */
export type PreferencesListParams = Record<string, never>;

/** preferences.list 的结果。 */
export interface PreferencesListResult {
  /** 分组 */
  groups: PreferencesPreferenceGroup[];
  /** 设置项 */
  settings: PreferencesPreferenceSetting[];
}

/** preferences.set 的参数。 */
export interface PreferencesSetParams {
  /** 设置项标识 */
  id: string;
  /** 新的取值：choice 为 options 里的字符串，switch 为布尔值，integer 为 minimum 到 maximum 之间的整数 */
  value: unknown;
}

/** preferences.set 的结果。 */
export interface PreferencesSetResult {
  /** 设置项标识 */
  id: string;
  /** 修改之后的取值 */
  value: unknown;
}

/** preferences.reset 的参数。 */
export interface PreferencesResetParams {
  /** 设置项标识 */
  id: string;
}

/** preferences.reset 的结果。 */
export interface PreferencesResetResult {
  /** 设置项标识 */
  id: string;
  /** 恢复之后的取值 */
  value: unknown;
}

/** runtime.status 的参数。 */
export type RuntimeStatusParams = Record<string, never>;

/** runtime.status 的结果。 */
export interface RuntimeStatusResult {
  /** 电源 */
  power: "off" | "on";
  /** 运行模式 */
  mode: "paused" | "running";
  /** 主机型号；没有装入卡带时没有这一项 */
  machine?: string;
  /** 整机机型（学习机）开机所用 ROM 的版本：认得出的 ROM 是版本名或型号名（如 V9.0-98、98型），认不出时是文件名；经典 FC 与没有主机时没有这一项 */
  romLabel?: string;
  /** 当前的卡带；没有时没有这一项 */
  cartridge?: RuntimeCartridgeSummary;
  /** 自上电以来完成的帧数 */
  frame: number;
  /** 自上电以来的 CPU 周期数 */
  cycles: number;
  /** 速度百分比，目前固定为 100 */
  speed: number;
  /** 机器现在是否带鼠标：学习机在 6576 模式下带串口鼠标（进入 FC 兼容模式后没有），经典 FC 只有卡带的 mapper 插件声明了 mouse 外设才有；没有主机时为 false */
  mouse: boolean;
}

/** runtime.powerOn 的参数。 */
export interface RuntimePowerOnParams {
  /** 上电后是否直接运行，默认 false */
  resume?: boolean;
}

/** runtime.powerOn 的结果。 */
export interface RuntimePowerOnResult {
  state: RuntimeState;
}

/** runtime.powerOff 的参数。 */
export type RuntimePowerOffParams = Record<string, never>;

/** runtime.powerOff 的结果。 */
export interface RuntimePowerOffResult {
  state: RuntimeState;
}

/** runtime.reset 的参数。 */
export interface RuntimeResetParams {
  /** 复位方式 */
  kind: "soft" | "hard";
}

/** runtime.reset 的结果。 */
export interface RuntimeResetResult {
  state: RuntimeState;
}

/** runtime.pause 的参数。 */
export type RuntimePauseParams = Record<string, never>;

/** runtime.pause 的结果。 */
export interface RuntimePauseResult {
  state: RuntimeState;
}

/** runtime.resume 的参数。 */
export type RuntimeResumeParams = Record<string, never>;

/** runtime.resume 的结果。 */
export interface RuntimeResumeResult {
  state: RuntimeState;
}

/** runtime.run 的参数。 */
export interface RuntimeRunParams {
  /** 要运行的帧数 */
  frames: number;
  /** 是否等运行结束再返回，默认 false */
  wait?: boolean;
}

/** runtime.run 的结果。 */
export interface RuntimeRunResult {
  /** 返回时的运行状态 */
  state: RuntimeState;
  /** 实际运行的帧数；wait 为 false 时为 0 */
  frames: number;
  /** 实际经过的 CPU 周期数；wait 为 false 时为 0 */
  cycles: number;
  /** 停止原因（只有 wait 为 true 时有）：completed 运行完授权的帧数；interrupted 被暂停、继续、硬复位、换卡带、新的授权运行或请求被取消打断；powerOff 断电；breakpoint 命中断点；error 运行出错 */
  stopped?: "completed" | "interrupted" | "powerOff" | "breakpoint" | "error";
}

/** runtime.step 的参数。 */
export interface RuntimeStepParams {
  /** 单位 */
  unit: "instruction" | "cycle" | "scanline" | "frame";
  /** 数量，默认 1；要运行更长时间用 runtime.run */
  count?: number;
}

/** runtime.step 的结果。 */
export interface RuntimeStepResult {
  /** 单步后的运行状态（总是暂停） */
  state: RuntimeState;
  /** 实际执行的指令数 */
  instructions: number;
  /** 实际经过的 CPU 周期数 */
  cycles: number;
  /** 期间完成的帧数 */
  frames: number;
  /** 停下时程序计数器的值，即下一条要执行的指令地址 */
  pc: number;
}

/** runtime.setFrameEvents 的参数。 */
export interface RuntimeSetFrameEventsParams {
  /** 间隔帧数，0 表示不发 */
  frames: number;
}

/** runtime.setFrameEvents 的结果。 */
export interface RuntimeSetFrameEventsResult {
  /** 设置之后的间隔帧数 */
  frames: number;
}

/** system.version 的参数。 */
export type SystemVersionParams = Record<string, never>;

/** system.version 的结果。 */
export interface SystemVersionResult {
  /** 应用版本，取自程序的版本号 */
  appVersion: string;
  /** 核心版本，取自核心的构建参数 */
  coreVersion: string;
}

/** rpc.discover 的参数。 */
export type RpcDiscoverParams = Record<string, never>;

/** rpc.discover 的结果。 */
export type RpcDiscoverResult = Record<string, unknown>;

/** cartridges.loaded 事件的数据。 */
export interface CartridgesLoadedData {
  cartridge: RuntimeCartridgeSummary;
}

/** debug.statusChanged 事件的数据。 */
export interface DebugStatusChangedData {
  /** 是否开启 */
  enabled: boolean;
  /** 是否正在监听 */
  listening: boolean;
  /** 监听的端口，没有监听时为 0 */
  port: number;
  /** 当前的连接数 */
  connections: number;
}

/** debug.eventsDropped 事件的数据。 */
export interface DebugEventsDroppedData {
  /** 自上一次通知以来丢弃的事件条数 */
  count: number;
}

/** input.bindingsChanged 事件的数据。 */
export interface InputBindingsChangedData {
  /** 新的玩家槽位 */
  slots: InputPadSlot[];
}

/** input.familyKeyboardChanged 事件的数据。 */
export interface InputFamilyKeyboardChangedData {
  /** 新的接入状态 */
  connected: boolean;
}

/** inspector.stateChanged 事件的数据。 */
export interface InspectorStateChangedData {
  /** 状态路径 */
  path: string;
  /** 上一次检查时的取值 */
  oldValue: number;
  /** 现在的取值 */
  newValue: number;
  /** 检查时的 CPU 周期数 */
  cycles: number;
}

/** machines.changed 事件的数据。 */
export interface MachinesChangedData {
  /** 机型标识 */
  machine: string;
}

/** mappers.changed 事件的数据。 */
export interface MappersChangedData {
  /** 变化的插件标识；重新加载时为 null */
  id: string | null;
}

/** media.changed 事件的数据。 */
export interface MediaChangedData {
  /** 设备 */
  device: "floppy" | "tape";
  /** 驱动器编号；磁带机恒为 0 */
  drive: number;
}

/** media.tapeAnalyzed 事件的数据。 */
export interface MediaTapeAnalyzedData {
  /** 分析的是哪一盘磁带（对应 TapeStatus 的 serial） */
  serial: number;
}

/** media.printed 事件的数据。 */
export interface MediaPrintedData {
  /** 文件名，不含路径 */
  fileName: string;
  /** 本次运行以来的页序号 */
  page: number;
}

/** media.printFailed 事件的数据。 */
export interface MediaPrintFailedData {
  /** 原因，面向用户 */
  message: string;
}

/** media.floppySaveFailed 事件的数据。 */
export interface MediaFloppySaveFailedData {
  /** 驱动器编号 */
  drive: number;
  /** 原因，面向用户 */
  message: string;
}

/** preferences.changed 事件的数据。 */
export interface PreferencesChangedData {
  /** 设置项标识 */
  id: string;
  /** 新的取值 */
  value: unknown;
}

/** runtime.stateChanged 事件的数据。 */
export interface RuntimeStateChangedData {
  /** 新的电源状态 */
  power: "off" | "on";
  /** 新的运行模式 */
  mode: "paused" | "running";
  /** 原因：run 开始授权运行，runEnded 授权运行跑完后暂停，step 单步，breakpoint 命中断点后暂停，inputChanged 机器的输入设备变了（如学习机进入 FC 兼容模式后不再带鼠标），电源与运行模式不变 */
  reason: "load" | "eject" | "powerOn" | "powerOff" | "reset" | "pause" | "resume" | "run" | "runEnded" | "step" | "breakpoint" | "error" | "inputChanged";
}

/** runtime.frame 事件的数据。 */
export interface RuntimeFrameData {
  /** 帧号 */
  frame: number;
  /** 自上电以来的 CPU 周期数 */
  cycles: number;
  /** CPU 的程序计数器 */
  pc: number;
}

/** runtime.runFinished 事件的数据。 */
export interface RuntimeRunFinishedData {
  /** 实际运行的帧数 */
  frames: number;
  /** 实际经过的 CPU 周期数 */
  cycles: number;
  /** 停止原因，含义同 run 的结果 */
  stopped: "completed" | "interrupted" | "powerOff" | "breakpoint" | "error";
}

/** runtime.breakpointHit 事件的数据。 */
export interface RuntimeBreakpointHitData {
  /** 断点编号 */
  id: number;
  /** 断点种类 */
  kind: "execute" | "read" | "write";
  /** 执行断点是将要执行的指令地址；读写断点是被访问的地址 */
  address: number;
  /** 读写断点读到或写入的值；执行断点没有这一项 */
  value?: number;
  /** 停下时的程序计数器 */
  pc: number;
  /** 累加器 */
  a: number;
  /** X 寄存器 */
  x: number;
  /** Y 寄存器 */
  y: number;
  /** 栈指针 */
  sp: number;
  /** 状态寄存器 */
  status: number;
}

/** runtime.error 事件的数据。 */
export interface RuntimeErrorData {
  /** 出错的模块 */
  module: string;
  /** 错误码 */
  code: string;
  /** 面向用户的说明 */
  message: string;
}

/** 全部请求：方法名对应参数与结果。 */
export interface RequestMap {
  "cartridges.load": { params: CartridgesLoadParams; result: CartridgesLoadResult };
  "cartridges.eject": { params: CartridgesEjectParams; result: CartridgesEjectResult };
  "cartridges.info": { params: CartridgesInfoParams; result: CartridgesInfoResult };
  "debug.hello": { params: DebugHelloParams; result: DebugHelloResult };
  "debug.subscribe": { params: DebugSubscribeParams; result: DebugSubscribeResult };
  "debug.unsubscribe": { params: DebugUnsubscribeParams; result: DebugUnsubscribeResult };
  "debug.status": { params: DebugStatusParams; result: DebugStatusResult };
  "input.getBindings": { params: InputGetBindingsParams; result: InputGetBindingsResult };
  "input.setBindings": { params: InputSetBindingsParams; result: InputSetBindingsResult };
  "input.resetBindings": { params: InputResetBindingsParams; result: InputResetBindingsResult };
  "input.getFamilyKeyboard": { params: InputGetFamilyKeyboardParams; result: InputGetFamilyKeyboardResult };
  "input.setFamilyKeyboard": { params: InputSetFamilyKeyboardParams; result: InputSetFamilyKeyboardResult };
  "input.key": { params: InputKeyParams; result: InputKeyResult };
  "input.mouse": { params: InputMouseParams; result: InputMouseResult };
  "input.gamepad": { params: InputGamepadParams; result: InputGamepadResult };
  "input.pad": { params: InputPadParams; result: InputPadResult };
  "input.releaseAll": { params: InputReleaseAllParams; result: InputReleaseAllResult };
  "inspector.listSpaces": { params: InspectorListSpacesParams; result: InspectorListSpacesResult };
  "inspector.readMemory": { params: InspectorReadMemoryParams; result: InspectorReadMemoryResult };
  "inspector.writeMemory": { params: InspectorWriteMemoryParams; result: InspectorWriteMemoryResult };
  "inspector.resolve": { params: InspectorResolveParams; result: InspectorResolveResult };
  "inspector.getState": { params: InspectorGetStateParams; result: InspectorGetStateResult };
  "inspector.snapshot": { params: InspectorSnapshotParams; result: InspectorSnapshotResult };
  "inspector.watch": { params: InspectorWatchParams; result: InspectorWatchResult };
  "inspector.unwatch": { params: InspectorUnwatchParams; result: InspectorUnwatchResult };
  "inspector.disassemble": { params: InspectorDisassembleParams; result: InspectorDisassembleResult };
  "inspector.cdlStart": { params: InspectorCdlStartParams; result: InspectorCdlStartResult };
  "inspector.cdlStop": { params: InspectorCdlStopParams; result: InspectorCdlStopResult };
  "inspector.cdlSave": { params: InspectorCdlSaveParams; result: InspectorCdlSaveResult };
  "inspector.traceStart": { params: InspectorTraceStartParams; result: InspectorTraceStartResult };
  "inspector.traceStop": { params: InspectorTraceStopParams; result: InspectorTraceStopResult };
  "inspector.callLogStart": { params: InspectorCallLogStartParams; result: InspectorCallLogStartResult };
  "inspector.callLogStop": { params: InspectorCallLogStopParams; result: InspectorCallLogStopResult };
  "inspector.addBreakpoint": { params: InspectorAddBreakpointParams; result: InspectorAddBreakpointResult };
  "inspector.listBreakpoints": { params: InspectorListBreakpointsParams; result: InspectorListBreakpointsResult };
  "inspector.removeBreakpoint": { params: InspectorRemoveBreakpointParams; result: InspectorRemoveBreakpointResult };
  "inspector.clearBreakpoints": { params: InspectorClearBreakpointsParams; result: InspectorClearBreakpointsResult };
  "inspector.setState": { params: InspectorSetStateParams; result: InspectorSetStateResult };
  "machines.list": { params: MachinesListParams; result: MachinesListResult };
  "machines.setRom": { params: MachinesSetRomParams; result: MachinesSetRomResult };
  "machines.setRoms": { params: MachinesSetRomsParams; result: MachinesSetRomsResult };
  "machines.removeRom": { params: MachinesRemoveRomParams; result: MachinesRemoveRomResult };
  "machines.boot": { params: MachinesBootParams; result: MachinesBootResult };
  "mappers.list": { params: MappersListParams; result: MappersListResult };
  "mappers.info": { params: MappersInfoParams; result: MappersInfoResult };
  "mappers.install": { params: MappersInstallParams; result: MappersInstallResult };
  "mappers.uninstall": { params: MappersUninstallParams; result: MappersUninstallResult };
  "mappers.setEnabled": { params: MappersSetEnabledParams; result: MappersSetEnabledResult };
  "mappers.reload": { params: MappersReloadParams; result: MappersReloadResult };
  "media.floppyInsert": { params: MediaFloppyInsertParams; result: MediaFloppyInsertResult };
  "media.floppyEject": { params: MediaFloppyEjectParams; result: MediaFloppyEjectResult };
  "media.floppySave": { params: MediaFloppySaveParams; result: MediaFloppySaveResult };
  "media.floppyWriteProtect": { params: MediaFloppyWriteProtectParams; result: MediaFloppyWriteProtectResult };
  "media.floppyStatus": { params: MediaFloppyStatusParams; result: MediaFloppyStatusResult };
  "media.floppyFault": { params: MediaFloppyFaultParams; result: MediaFloppyFaultResult };
  "media.printerStatus": { params: MediaPrinterStatusParams; result: MediaPrinterStatusResult };
  "media.printerFlush": { params: MediaPrinterFlushParams; result: MediaPrinterFlushResult };
  "media.tapeLoad": { params: MediaTapeLoadParams; result: MediaTapeLoadResult };
  "media.tapeEject": { params: MediaTapeEjectParams; result: MediaTapeEjectResult };
  "media.tapePlay": { params: MediaTapePlayParams; result: MediaTapePlayResult };
  "media.tapeRecord": { params: MediaTapeRecordParams; result: MediaTapeRecordResult };
  "media.tapeStop": { params: MediaTapeStopParams; result: MediaTapeStopResult };
  "media.tapeStatus": { params: MediaTapeStatusParams; result: MediaTapeStatusResult };
  "media.tapeSeek": { params: MediaTapeSeekParams; result: MediaTapeSeekResult };
  "media.tapeTimeline": { params: MediaTapeTimelineParams; result: MediaTapeTimelineResult };
  "output.setVolume": { params: OutputSetVolumeParams; result: OutputSetVolumeResult };
  "output.screenshot": { params: OutputScreenshotParams; result: OutputScreenshotResult };
  "preferences.list": { params: PreferencesListParams; result: PreferencesListResult };
  "preferences.set": { params: PreferencesSetParams; result: PreferencesSetResult };
  "preferences.reset": { params: PreferencesResetParams; result: PreferencesResetResult };
  "runtime.status": { params: RuntimeStatusParams; result: RuntimeStatusResult };
  "runtime.powerOn": { params: RuntimePowerOnParams; result: RuntimePowerOnResult };
  "runtime.powerOff": { params: RuntimePowerOffParams; result: RuntimePowerOffResult };
  "runtime.reset": { params: RuntimeResetParams; result: RuntimeResetResult };
  "runtime.pause": { params: RuntimePauseParams; result: RuntimePauseResult };
  "runtime.resume": { params: RuntimeResumeParams; result: RuntimeResumeResult };
  "runtime.run": { params: RuntimeRunParams; result: RuntimeRunResult };
  "runtime.step": { params: RuntimeStepParams; result: RuntimeStepResult };
  "runtime.setFrameEvents": { params: RuntimeSetFrameEventsParams; result: RuntimeSetFrameEventsResult };
  "system.version": { params: SystemVersionParams; result: SystemVersionResult };
  "rpc.discover": { params: RpcDiscoverParams; result: RpcDiscoverResult };
}

/** 全部事件：事件名对应数据；订阅后以通知的形式送达。 */
export interface EventMap {
  /** 卡带装配完成。 */
  "cartridges.loaded": CartridgesLoadedData;
  /** 服务的状态变了：开始或停止监听、有连接建立或断开。 */
  "debug.statusChanged": DebugStatusChangedData;
  /** 只发给订阅事件的那条连接：客户端读得太慢，事件队列已满，丢弃了若干条事件；响应不会被丢弃。 */
  "debug.eventsDropped": DebugEventsDroppedData;
  /** 手柄绑定变了（界面修改、恢复默认都会发出）。 */
  "input.bindingsChanged": InputBindingsChangedData;
  /** Family BASIC 键盘的接入状态变了（界面或调试服务设置后发出，没有变化时不发）。 */
  "input.familyKeyboardChanged": InputFamilyKeyboardChangedData;
  /** 监视的状态项取值变了。同一次检查里变化的各项依次发出；监视项属于检查器，重新上电或换卡带之后的变化也会报告。 */
  "inspector.stateChanged": InspectorStateChangedData;
  /** 整机机型的 ROM 添加、替换或移除。 */
  "machines.changed": MachinesChangedData;
  /** 插件安装、卸载、启停或重新加载。 */
  "mappers.changed": MappersChangedData;
  /** 介质的状态变化：盘片插入、取出、保存、注入故障与马达开关，磁带的载入、取出、播放、录音与停止，磁带机接上或断开主机（connected 变化时随运行状态事件刷新，不另发本事件）。 */
  "media.changed": MediaChangedData;
  /** 磁带的时间轴分析结束（完成或失败）；用 media.tapeTimeline 读取结果。 */
  "media.tapeAnalyzed": MediaTapeAnalyzedData;
  /** 打印机输出了一页：已经保存成 PNG 文件。 */
  "media.printed": MediaPrintedData;
  /** 打印机输出的一页无法保存（目录无法写入等），这一页已丢弃。 */
  "media.printFailed": MediaPrintFailedData;
  /** 盘片镜像无法自动写回文件（文件被占用、没有写权限等）；修改仍在内存里，核心之后每 5 秒重试一次，写回成功前只通知这一次。 */
  "media.floppySaveFailed": MediaFloppySaveFailedData;
  /** 一个设置项的取值变了（界面修改、恢复默认、核心里的模块修改都会发出）。 */
  "preferences.changed": PreferencesChangedData;
  /** 电源或运行模式改变（含装入或取出卡带）。 */
  "runtime.stateChanged": RuntimeStateChangedData;
  /** 每完成一帧发出；默认不订阅，开启后才发布。 */
  "runtime.frame": RuntimeFrameData;
  /** 授权运行（run）结束：跑完、或被暂停、继续、复位、断电、换卡带、新的授权运行或出错打断。 */
  "runtime.runFinished": RuntimeRunFinishedData;
  /** 命中断点，机器已暂停（自由运行与授权运行都会停下）。执行断点停在指令执行之前，读写断点停在那条指令执行完之后；断点由 inspector.addBreakpoint 添加。 */
  "runtime.breakpointHit": RuntimeBreakpointHitData;
  /** 运行中的错误，如 mapper 脚本异常；发出后模拟器已暂停。 */
  "runtime.error": RuntimeErrorData;
}

/** 按模块分组的请求接口，每个方法对应同名的请求。 */
export interface EmberApi {
  readonly cartridges: {
    /** 读取 NES 文件，解析文件头，按 mapper 号与变体匹配插件，装配主机并插入卡槽；已有的卡带与主机被替换，失败时原有的保持不变。匹配失败的错误码是 mappers.noMatch、mappers.conflict、mappers.hostUnknown，插件加载失败的是 mappers.notFound、mappers.invalidPackage、mappers.limitExceeded。 */
    load(params: CartridgesLoadParams): Promise<CartridgesLoadResult>;
    /** 取出卡带：主机随之拆除，电源状态变为断电；没有卡带时没有效果。 */
    eject(): Promise<CartridgesEjectResult>;
    /** 当前卡带的描述、匹配到的插件与存储区清单。 */
    info(): Promise<CartridgesInfoResult>;
  };
  readonly debug: {
    /** 握手：每条连接上第一个请求必须是它，令牌不对时连接被关闭（关闭码 4001）。成功后才能调用其他请求。 */
    hello(params: DebugHelloParams): Promise<DebugHelloResult>;
    /** 订阅事件。事件名写成“模块.事件”，也可以写“模块.*”订阅一个模块的全部事件，或“*”订阅全部；已经订阅的重复订阅没有效果。高频事件默认不推送，要订阅才有。 */
    subscribe(params: DebugSubscribeParams): Promise<DebugSubscribeResult>;
    /** 取消订阅：写法要与订阅时一致；没有订阅过的忽略。 */
    unsubscribe(params: DebugUnsubscribeParams): Promise<DebugUnsubscribeResult>;
    /** 调试服务的状态：是否开启、是否在监听、端口与当前的连接。 */
    status(): Promise<DebugStatusResult>;
  };
  readonly input: {
    /** 读取手柄绑定：两个玩家槽位（对应手柄口 1、2）。 */
    getBindings(): Promise<InputGetBindingsResult>;
    /** 替换全部手柄绑定：核心校验后保存到文件，并发出 input.bindingsChanged；内容没有变化时不发事件。 */
    setBindings(params: InputSetBindingsParams): Promise<InputSetBindingsResult>;
    /** 把手柄绑定恢复为出厂默认；内容没有变化时不发事件。 */
    resetBindings(): Promise<InputResetBindingsResult>;
    /** 读取是否给经典 FC 接上了 Family BASIC 键盘；每次启动都是未接入，不保存。 */
    getFamilyKeyboard(): Promise<InputGetFamilyKeyboardResult>;
    /** 给经典 FC 接入或断开 Family BASIC 键盘：对当前主机立即生效，之后装配的主机也按它设置；接入后电脑键盘的按键按 Family BASIC 键盘的键位送给机器，不再当作手柄按键；学习机用自己内置的键盘，不受影响。状态只在本次运行里保留，不保存；有变化时发出 input.familyKeyboardChanged。 */
    setFamilyKeyboard(params: InputSetFamilyKeyboardParams): Promise<InputSetFamilyKeyboardResult>;
    /** 按下或松开一个物理键：键盘属于机器时交给机器，否则按手柄绑定变成手柄口的按键；没有开机时被丢弃。 */
    key(params: InputKeyParams): Promise<InputKeyResult>;
    /** 鼠标事件，交给机器；坐标单位是标准帧（256x240）的像素，省略的数值按 0 处理。 */
    mouse(params: InputMouseParams): Promise<InputMouseResult>;
    /** 游戏手柄一个手柄的当前全部状态，按手柄绑定变成手柄口的按键；拔掉时 connected 为 false，按键与轴省略。 */
    gamepad(params: InputGamepadParams): Promise<InputGamepadResult>;
    /** 直接设置主机某个手柄口上标准手柄的按键，不经 mapper 脚本；位：1 A、2 B、4 选择、8 开始、16 上、32 下、64 左、128 右。 */
    pad(params: InputPadParams): Promise<InputPadResult>;
    /** 松开经 key 按下的全部键、把经 pad 设置过的手柄口清零、并让经 gamepad 注入的手柄断开。 */
    releaseAll(): Promise<InputReleaseAllResult>;
  };
  readonly inspector: {
    /** 列出全部内存空间：总线视角的 cpu 与 ppu，主板与芯片里的存储，以及卡带上的存储区（cart.<存储区标识>）。需要已开机。 */
    listSpaces(): Promise<InspectorListSpacesResult>;
    /** 读一段内存，不改变主机的任何状态：cpu 与 ppu 空间里寄存器类地址读出 0（不读寄存器，所以不会清标志），卡带脚本的寄存器也不会被回调。数据以二进制帧返回（结果里的 data，见 docs/04-调试服务.md 第 2.3 节）。需要已开机。 */
    readMemory(params: InspectorReadMemoryParams): Promise<InspectorReadMemoryResult>;
    /** 写一段内存。cpu 与 ppu 空间的写经真实的总线，与 CPU 写一样有副作用（写寄存器会触发相应的行为）；卡带的 ROM 存储区不能写。需要已开机。 */
    writeMemory(params: InspectorWriteMemoryParams): Promise<InspectorWriteMemoryResult>;
    /** 解析一个总线地址：它落在哪块存储区的哪个偏移，或哪个寄存器；卡带的地址按当前的窗口映射（分页）解析，不回调脚本。需要已开机。 */
    resolve(params: InspectorResolveParams): Promise<InspectorResolveResult>;
    /** 读状态：cpu（寄存器与周期数）、ppu（扫描线、点、帧数、滚动寄存器、标志）与卡带的脚本状态（mapper）。省略 module 时返回全部模块。读取在帧边界或暂停时进行，是一致的快照。需要已开机。 */
    getState(params?: InspectorGetStateParams): Promise<InspectorGetStateResult>;
    /** 取得完整快照：各模块的状态、断点与监视项，以及此刻的 CPU 周期数，都在同一个时刻读出，是一致的。内存不在快照里，用 readMemory 读。省略 module 时包含全部模块。需要已开机。 */
    snapshot(params?: InspectorSnapshotParams): Promise<InspectorSnapshotResult>;
    /** 监视一个状态项：它的取值变化时发事件 inspector.stateChanged。路径是 <模块>.<状态项>，如 cpu.pc、ppu.scanline、mapper.prgBank（状态项见 getState）。取值在每一帧完成后、单步、命中断点和检查请求执行完之后检查，一帧里变化多次只报告最后的取值。已经在监视的路径再监视没有效果。监视最多 64 项，属于检查器本身，不随卡带更换清除。需要已开机。 */
    watch(params: InspectorWatchParams): Promise<InspectorWatchResult>;
    /** 取消监视。没有监视这个路径时返回 inspector.noWatch。需要已开机。 */
    unwatch(params: InspectorUnwatchParams): Promise<InspectorUnwatchResult>;
    /** 从 address 起反汇编 count 条指令（CPU 总线上的地址）。读内存没有副作用；带索引的寻址用当前的 X、Y 算有效地址与注释里的取值；地址超过 $FFFF 时回到 $0000。需要已开机。 */
    disassemble(params: InspectorDisassembleParams): Promise<InspectorDisassembleResult>;
    /** 开始代码与数据标记（CDL）：给卡带 PRG-ROM 的每个字节打标记，标记清零后，运行与单步执行的每条指令都记：Code（0x01，被当作指令执行，含操作数字节）、Data（0x02，被指令当数据读取，不含只写的存储指令、栈与中断向量）、JumpTarget（0x04，JMP 或成立的分支的目标）、SubEntry（0x08，JSR 的目标）。标记的是物理位置（按当前的分页换算）。记录属于当前的主机，装入新卡带后作废。需要已开机，卡带要有 PRG-ROM 存储区。 */
    cdlStart(): Promise<InspectorCdlStartResult>;
    /** 停止标记，已有的标记保留到下一次 cdlStart 或换卡带；已经停止时只返回统计。没有记录过时返回 inspector.noCdl。需要已开机。 */
    cdlStop(): Promise<InspectorCdlStopResult>;
    /** 把当前的标记写成调试输出目录下的文件：PRG-ROM 每个字节一个标记字节，没有文件头，位定义同 Mesen。记录中或停止后都可以保存。没有记录过时返回 inspector.noCdl。需要已开机。 */
    cdlSave(params?: InspectorCdlSaveParams): Promise<InspectorCdlSaveResult>;
    /** 开始跟踪，内容记在内存里，traceStop 时写成调试输出目录下的文件。kind：instruction 每执行一条指令记一行（地址、指令字节、反汇编、寄存器与周期数，写法同 nestest 日志；芯片寄存器的取值显示 00）；io 每次访问 $2000-$5FFF（芯片、手柄口、卡带扩展寄存器）以及对 $8000 以上的写（mapper 寄存器）记一行（方向、地址、值、发起访问的指令地址与周期数）；irq 每次 CPU 响应 IRQ、NMI 或 BRK 记一行（种类、返回地址、处理程序地址、周期数）。单步与运行产生的内容都记，调试写内存不记。同时进行的记录最多 8 项，记录属于当前的主机，装入新卡带后作废。需要已开机。 */
    traceStart(params: InspectorTraceStartParams): Promise<InspectorTraceStartResult>;
    /** 结束跟踪并把内容写成文件。需要已开机。 */
    traceStop(params: InspectorTraceStopParams): Promise<InspectorTraceStopResult>;
    /** 开始调用记录：每次执行到 address 这条指令之前记一行，含寄存器、周期数、栈顶的若干字节（SP 之上，栈顶在前）与指定的内存段。内容记在内存里，callLogStop 时写成调试输出目录下的文件。同时进行的记录最多 8 项，记录属于当前的主机，装入新卡带后作废。需要已开机。 */
    callLogStart(params: InspectorCallLogStartParams): Promise<InspectorCallLogStartResult>;
    /** 结束调用记录并把内容写成文件。需要已开机。 */
    callLogStop(params: InspectorCallLogStopParams): Promise<InspectorCallLogStopResult>;
    /** 添加断点。断点在机器运行时生效（自由运行与 runtime.run 都会停下，发事件 runtime.breakpointHit，运行模式变为暂停）；单步（runtime.step）忽略断点；继续运行时会先越过刚命中的执行断点。断点属于当前的主机，装入新卡带后清空。地址是 CPU 总线上的地址。需要已开机。 */
    addBreakpoint(params: InspectorAddBreakpointParams): Promise<InspectorAddBreakpointResult>;
    /** 列出全部断点，按编号升序。需要已开机。 */
    listBreakpoints(): Promise<InspectorListBreakpointsResult>;
    /** 删除一个断点。需要已开机。 */
    removeBreakpoint(params: InspectorRemoveBreakpointParams): Promise<InspectorRemoveBreakpointResult>;
    /** 删除全部断点。需要已开机。 */
    clearBreakpoints(): Promise<InspectorClearBreakpointsResult>;
    /** 修改一个模块的状态：目前只有 cpu 可以改（pc、a、x、y、sp、status，只在指令之间生效），其余模块只读。先全部检查，有任何一项不合格就一项也不改。需要已开机。 */
    setState(params: InspectorSetStateParams): Promise<InspectorSetStateResult>;
  };
  readonly machines: {
    /** 机型清单，顺序固定为 fc、sb97、sb2000、bbg1、bbg98、pec9588、pec586、yuxing。 */
    list(): Promise<MachinesListResult>;
    /** 读取并检查 ROM 文件，复制进 ROM 目录（新文件写好之后才删旧文件，失败时原有的 ROM 不变）；成功后发 machines.changed。 */
    setRom(params: MachinesSetRomParams): Promise<MachinesSetRomResult>;
    /** 读取目录里的 .bin 文件（不含子目录，按文件名排序），按内容的校验值认出各是哪个机型的 ROM，把还没有 ROM 的机型的 ROM 添加进来；已有 ROM 的机型保持不变，认不出的文件跳过；有添加时发 machines.changed。 */
    setRoms(params: MachinesSetRomsParams): Promise<MachinesSetRomsResult>;
    /** 删除机型的 ROM 文件；没有 ROM 时没有效果也不报错；删除后发 machines.changed。 */
    removeRom(params: MachinesRemoveRomParams): Promise<MachinesRemoveRomResult>;
    /** 整机机型开机：读入已设置的 ROM，装配该机型的主机，已有的卡带与主机被替换，任何一步失败都不改动原有的；成功后发 runtime.stateChanged（原因 load），不发 cartridges.loaded。 */
    boot(params: MachinesBootParams): Promise<MachinesBootResult>;
  };
  readonly mappers: {
    /** 列出生效的插件：同一标识有多个来源时只列版本最高的，版本相同时用户安装的优先。 */
    list(): Promise<MappersListResult>;
    /** 返回一个插件的摘要与清单详情。 */
    info(params: MappersInfoParams): Promise<MappersInfoResult>;
    /** 安装一个 .emap 插件包：检查通过后放进用户插件目录；已有同标识的用户插件时替换。 */
    install(params: MappersInstallParams): Promise<MappersInstallResult>;
    /** 卸载用户安装的插件；预装的插件只能停用。 */
    uninstall(params: MappersUninstallParams): Promise<MappersUninstallResult>;
    /** 启用或停用一个插件；停用的插件不参与匹配。 */
    setEnabled(params: MappersSetEnabledParams): Promise<MappersSetEnabledResult>;
    /** 重新读取预装目录与用户目录里的插件包，下次装配或硬复位时生效；不合格的包被跳过并写入错误日志。 */
    reload(): Promise<MappersReloadResult>;
  };
  readonly media: {
    /** 读取盘片镜像文件（上限 16 MB）放进驱动器，替换已有的盘片；软驱是独立于主机的设备，没有主机、没有通电时也可以插入。镜像的格式由大小推断（160、180、320、360、720、1200、1440、2880 KB，其他大小按 1.44MB 处理）。 */
    floppyInsert(params: MediaFloppyInsertParams): Promise<MediaFloppyInsertResult>;
    /** 取出盘片并丢弃镜像；有尚未写回的修改时先写回放入时的文件，写回失败则不取出；没有盘片时没有效果。 */
    floppyEject(params: MediaFloppyEjectParams): Promise<MediaFloppyEjectResult>;
    /** 立即把驱动器里的镜像写回放入时的文件，并清除“已修改”标志；学习机写盘后核心会自动写回，通常不必调用。 */
    floppySave(params: MediaFloppySaveParams): Promise<MediaFloppySaveResult>;
    /** 设置或取消盘片的写保护；写保护的盘片拒绝学习机写入。 */
    floppyWriteProtect(params: MediaFloppyWriteProtectParams): Promise<MediaFloppyWriteProtectResult>;
    /** 全部软驱的状态；软驱是独立于主机的设备，一直存在（目前只有软驱 A）。 */
    floppyStatus(): Promise<MediaFloppyStatusResult>;
    /** 故障注入：取盘、写保护、无索引脉冲、磁头卡住、控制器无响应、指定扇区校验错、全部清除；用来检查学习机程序对软驱故障的处理。 */
    floppyFault(params: MediaFloppyFaultParams): Promise<MediaFloppyFaultResult>;
    /** 打印机的状态：空闲、打印中、打印完成，当前页是否有内容，已输出的页数与最近一页的文件名；还没有主机或主机没有打印机时为空闲。 */
    printerStatus(): Promise<MediaPrinterStatusResult>;
    /** 立即输出当前页，等同打印机收到换页；页上的内容画成 PNG 保存到首选项“打印机”里设置的目录，成功后发 media.printed。 */
    printerFlush(): Promise<MediaPrinterFlushResult>;
    /** 把声音文件放进磁带机当磁带（解码由核心内置的 FFmpeg 库完成，m4a、mp3、wav 等都可以），位置回到开头，替换已有的磁带；磁带机与主机无关，没有主机、没有通电时也可以载入；磁带机播放或录音时不能载入。播放时磁带的声音（立体声取左声道，单声道取唯一的声道）经主机的声音输出播放；单声道文件整路是数据线，立体声文件右声道是数据线。 */
    tapeLoad(params: MediaTapeLoadParams): Promise<MediaTapeLoadResult>;
    /** 取出磁带；没有磁带时没有效果；播放或录音时先停止再取出（录音的文件先写完）。 */
    tapeEject(): Promise<MediaTapeEjectResult>;
    /** 开始播放；磁带放完后再播放从头开始；已经在播放或录音时没有效果；主机没有通电时不能播放（暂停也算通电，与主机型号无关）。 */
    tapePlay(): Promise<MediaTapePlayResult>;
    /** 开始录音：录的是主机的声音（游戏、学习机里的声音）加主机经数据线送来的 1 位信号，与主机型号无关；给出 path 时放入一盘空白磁带，录成 32 kHz、单声道、16 位的 WAV 文件（已有的文件被覆盖）；没有 path 时在磁带机里已有的磁带上从当前位置开始录，覆盖到哪录到哪，位置之前与录音结束处之后的内容保留，录好的内容写回这盘磁带的文件，磁带文件必须是 WAV（96 kHz、16 位、立体声，新录的部分左右声道相同）；停止后这个文件自动成为磁带机里的磁带，位置停在录音停止处，可以直接播放；磁带机播放或录音时不能开始录音；主机没有通电时不能录音。 */
    tapeRecord(params?: MediaTapeRecordParams): Promise<MediaTapeRecordResult>;
    /** 停止播放或录音；录音停止时把录到的内容写完，文件成为磁带机里的磁带；已经停止时没有效果。 */
    tapeStop(): Promise<MediaTapeStopResult>;
    /** 磁带机的状态；磁带机一直存在，没有接在通电的主机上时 connected 为 false。 */
    tapeStatus(): Promise<MediaTapeStatusResult>;
    /** 把磁带定位到某个位置；停止与播放时都可以（播放时从新位置继续），没有通电的主机也可以；录音时不能定位；位置超过总时长按总时长。 */
    tapeSeek(params: MediaTapeSeekParams): Promise<MediaTapeSeekResult>;
    /** 磁带的时间轴：载入磁带后核心在后台读一遍声音，得出数据线上各时刻的声音大小与空白段，用来在磁带上找到程序的位置；分析中只给进度，完成后给出全部内容并发 media.tapeAnalyzed；分析结果只存在内存里，取出或换磁带后作废。 */
    tapeTimeline(): Promise<MediaTapeTimelineResult>;
  };
  readonly output: {
    /** 设置声音的音量与静音；两个参数都可以省略，省略的保持不变。音量保存到首选项的 sound.volume，核心重启后保持；静音不保存，每次启动由 sound.startMuted 决定；变化平滑过渡，不会产生爆音。 */
    setVolume(params?: OutputSetVolumeParams): Promise<OutputSetVolumeResult>;
    /** 截取最近一次完成的画面（256x240，暂停与断电后也有上一帧）。不带 file 时图像以二进制帧返回（结果里的 data，见 docs/04-调试服务.md 第 2.3 节）；带 file 时保存到调试输出目录下，结果里的 file 是保存后的完整路径，不再返回 data。 */
    screenshot(params?: OutputScreenshotParams): Promise<OutputScreenshotResult>;
  };
  readonly preferences: {
    /** 列出全部分组与设置项（含当前取值）；分组与设置项按登记的顺序排列。 */
    list(): Promise<PreferencesListResult>;
    /** 修改一个设置项：核心按登记的种类校验取值，保存到文件，并发出 preferences.changed；取值没有变化时不发事件。 */
    set(params: PreferencesSetParams): Promise<PreferencesSetResult>;
    /** 把一个设置项恢复为出厂默认值；取值没有变化时不发事件。 */
    reset(params: PreferencesResetParams): Promise<PreferencesResetResult>;
  };
  readonly runtime: {
    /** 返回电源、运行模式、主机型号、卡带、帧号、总周期数、速度与是否带鼠标。 */
    status(): Promise<RuntimeStatusResult>;
    /** 上电：创建或重置全部芯片，卡带的存储区重新从文件载入；默认进入暂停，resume 为 true 时直接运行；已通电时等同硬复位。 */
    powerOn(params?: RuntimePowerOnParams): Promise<RuntimePowerOnResult>;
    /** 断电：停止运行，芯片状态丢弃；卡带保持插着。已断电时没有效果。 */
    powerOff(): Promise<RuntimePowerOffResult>;
    /** 复位：soft 只发 RESET 线，不改变电源与运行模式；hard 断电后再上电，运行模式保持。 */
    reset(params: RuntimeResetParams): Promise<RuntimeResetResult>;
    /** 暂停：已暂停时没有效果；只在通电时有效。 */
    pause(): Promise<RuntimePauseResult>;
    /** 继续：按真实帧率连续运行；已在运行时没有效果；只在通电时有效。 */
    resume(): Promise<RuntimeResumeResult>;
    /** 授权运行：不限速地连续运行指定的帧数后暂停（运行中不输出声音，每帧之间照常处理输入等命令）。wait 为 true 时等运行结束才返回结果（结果里有 stopped）；否则立即返回，运行结束时发事件 runtime.runFinished。已在授权运行中时旧的被打断，由新的替代；等待中的请求被取消（如连接断开）时运行被打断并暂停。之后会加入 cycles、scanlines、instructions 等其他授权量。 */
    run(params: RuntimeRunParams): Promise<RuntimeRunResult>;
    /** 单步：先暂停（正在进行的授权运行被打断），再按单位运行指定的数量后保持暂停。instruction 一条指令（含随后响应的中断序列）；cycle 至少这么多 CPU 周期（指令不被打断，实际周期数可能多几个）；scanline PPU 的扫描线推进这么多行；frame 完成这么多帧画面。结果里的 pc 是停下时下一条要执行的指令地址。经过的帧会照常显示。 */
    step(params: RuntimeStepParams): Promise<RuntimeStepResult>;
    /** 设置 runtime.frame 事件的发布间隔：每 frames 帧发一次，0 表示不发（默认）。这是全局设置，对订阅了 runtime.frame 的所有连接生效。 */
    setFrameEvents(params: RuntimeSetFrameEventsParams): Promise<RuntimeSetFrameEventsResult>;
  };
  readonly system: {
    /** 返回应用版本与核心版本。 */
    version(): Promise<SystemVersionResult>;
  };
  readonly rpc: {
    /** 返回描述全部请求与事件的 OpenRPC 文档，工具可据此生成客户端与界面。 */
    discover(): Promise<RpcDiscoverResult>;
  };
}
