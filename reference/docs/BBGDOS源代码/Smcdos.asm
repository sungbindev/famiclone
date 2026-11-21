	include	tvdef.asm
	org	$6400
	jmp	Initial			;
FileIO		equ	$0600		;
MoveBlockBF	equ	$f8		;
SplitPoint	equ	$5fba		;
BiosUsePage	equ	$5ff4		;*Bios use DRAM page.(depend on hardware).
DRAMPageNotUse	equ	$5ff9		;*Never use page change to 5800-7FFF.
DRAMPageSave	equ	$5ffa		;*Current DRAM page real..
VideoCtrlSave	equ	$5fff		;*
VideoCtrlPort	equ	$ff01		;
VideoDataPort	equ	$ff03		;
DRAMPagePort	equ	$ff04		;
Addr2000	equ	$6000		;
Addr2001	equ	$6001		;

CacheHzBegin	equ	$a800		;
ScreenWidth	equ	31		;
ChineseLine	equ	14		;
Backspace	equ	8		;
Enter		equ	13		;
ErrorType	db	0		;
StgNoMemory	db	10,"Insufficient memory to hold HZ",13,10,10,"$"
StgLibRdError	db	10,"Font file CCLIB16 read error!",13,10,10,"$"
StgNoLibFile	db	10,"Can not find CCLIB16 font file",13,10,10,"$"
StgNoLMemory	db	10,"No enough low RAM to work",13,10,10,"$"
ErrorStgPoint	dw	StgNoLibFile,	StgNoMemory,	StgLibRdError
		dw	StgNoLMemory
ReadHzNumbers	dw	0		;
		db	0		;
CCLIBFile	db	"CCLIB16    "	;
CCLIB0Cache	db	"、。“”『』αβ本表不部产长成出大党到得的等地电定动度对多而二发"
		db	"法方分高革个工国过好和后化会机级家加经就可来理力量了们面民命能年"
		db	"农起前人三上社深生十时实使是水说所他同为我无下现线小行性学要也一"
		db	"以义用有于在战这争政中种主着子自作"
CCLIB0CacheLen	equ	@-CCLIB0Cache
CCLIB1Cache	db	"⒈⒉⒊⑴⑵⑶①②③Ωαβ把本变表不部产长成出从大当党到得的等地第"
		db	"电定动斗都度队对多而二发法反方分干高革个工关国过好和合后化还会机"
		db	"级家加结经就开可来理里力两量了路论们面民命能年农批期起前人如三上"
		db	"社深生十时实使事是水说所他体天同图为我无物下现线想向小些新心形行"
		db	"性学样要也一以义应用有于在战这争正政之制中种主着资子自作"
CCLIB1CacheLen	equ	@-CCLIB1Cache
CCLIB2Cache:
CCLIB2CacheLen	equ	@-CCLIB2Cache
CCLIB3Cache	db	"、。…“”『』【】×÷→←↑↓⒈⒉⒊⑴⑵⑶①②③！，．０１２３４"
		db	"５６７８９Ωαβ─│┈┊┌┐└┘├┤┬┴┼啊阿埃爱安按暗案奥吧八"
		db	"巴拔把霸白百摆败斑班般板版半办帮胞包剥薄保宝报暴爆北背倍备被本泵"
		db	"比笔毕闭必辟壁臂避边编便变辩遍标彪表别兵冰柄病并玻播波勃伯捕补不"
		db	"布步部擦材才财采菜参残藏操槽草策侧测层插茶查察差柴产场常长厂唱超"
		db	"朝潮车彻沉陈称城成呈乘程承吃持池齿尺赤充冲虫抽初出除楚础触处川穿"
		db	"传船床创吹锤垂春纯磁雌此刺次从粗促村存寸措错达答打大带代待担丹单"
		db	"但淡弹蛋当党刀倒岛导到稻道德得的灯登等低滴敌抵底地第帝弟点典垫电"
		db	"掉调丁钉顶定订东冬懂动洞斗豆都毒独读杜度渡端短锻段断堆队对吨盾多"
		db	"夺俄额恶恩而儿耳尔二发阀法翻繁凡反范犯饭泛方房防访放非飞肥废费分"
		db	"纷粉丰封蜂峰锋风缝否夫幅符伏服浮福辅府腐副覆复腹负富附妇该改概盖"
		db	"干甘杆赶感敢刚钢缸纲岗高搞告哥歌割革格隔个各给根跟耕更工攻功供公"
		db	"巩贡共沟构够鼓古骨谷股故顾固刮挂关官观管灌贯光规硅归贵滚国果过哈"
		db	"孩海害含寒喊旱焊汉毫好耗号荷和何合河赫褐黑很狠横衡洪红厚候后呼乎"
		db	"忽胡湖弧虎护互户花华滑画划化话怀坏欢环还缓换荒黄簧皇灰挥辉恢回慧"
		db	"会浑混活伙火获或货击基机积迹激绩极集及急即级几己技季剂济寄计记既"
		db	"际继纪夹家加甲假价架坚尖间艰检柬碱简剪减鉴践见键件健渐建将浆江讲"
		db	"降焦胶交浇脚角教较叫揭接阶截节洁结解界借介斤金今津紧仅进近浸尽劲"
		db	"茎晶京精经井警景静境镜径竟究久九救旧就居局举聚据巨具距句剧卷觉决"
		db	"绝均菌军卡开刊看康抗考靠科壳可克刻客课空孔控口苦酷库块快宽矿况昆"
		db	"困括扩拉啦来兰浪劳牢老勒乐雷累类冷厘黎离理李里礼历利例立粒隶力璃"
		db	"联连脸链炼练粮梁粱良两量亮疗了料列裂烈林磷临零龄灵岭领另令硫留刘"
		db	"流柳六龙漏炉鲁露路录陆虑氯律率滤绿卵乱略轮伦论螺罗落洛妈麻马吗买"
		db	"麦卖脉满慢忙毛矛冒貌么霉煤没每美门们蒙盟猛孟米密棉免面苗灭民敏明"
		db	"名命谬摸模膜磨摩末莫谋某亩母木目牧拿哪那耐南男难脑闹呢内能泥尼你"
		db	"年念娘您凝宁牛浓农弄奴努女诺欧怕排派盘判叛旁刨炮袍跑培配喷批皮篇"
		db	"偏片骗频贫品平评坡破迫剖埔普期七其奇齐旗起企器气汽千钱前浅枪腔强"
		db	"抢桥切且侵亲秦勤青轻倾清情请庆秋球求区曲取去圈权全缺却确群然燃染"
		db	"壤让绕热仁人任认刃仍日荣熔溶容肉如乳入润若弱萨塞赛三散桑色砂杀沙"
		db	"啥山闪善伤商上尚稍烧少绍射社设伸身深沈甚声生升省盛胜师失施湿十石"
		db	"时什食蚀实识史使始式示士世事势是适释氏市室视试收手首守受殊输书熟"
		db	"鼠属术述树束数刷双谁水顺说斯思私司丝死四似饲松送宋苏素速塑诉酸算"
		db	"虽随碎岁穗孙损笋缩索锁所他它她塔台太态谈坦碳探炭唐讨套特提题体替"
		db	"天田腆挑条跳铁听停通同铜筒统痛投头透凸突图徒途涂土团推退拖托脱挖"
		db	"瓦外弯完晚万王亡网往望忘妄威微危围唯为维委伟尾未味位谓卫温文闻纹"
		db	"稳问我握乌污屋无武五午舞伍物务悟误析西吸稀息希袭席习喜铣洗系隙戏"
		db	"细下夏先鲜显险现献县宪陷限线相香箱乡想响项像向象削销消小校笑效些"
		db	"协斜写械谢芯新心信星兴型形行性胸雄休修锈袖需虚须许序畜续宣旋玄选"
		db	"学穴雪血旬训讯迅压呀芽牙亚烟盐严研岩延言沿眼演验央秧杨扬羊洋阳氧"
		db	"养样摇药要爷野也页业叶夜液一医依伊衣遗移仪疑宜已乙以艺易役亦意义"
		db	"益议异因音阴银引印英应营迎影硬映拥永勇用优尤由油游有友右又幼于余"
		db	"鱼渔予雨与语玉域遇愈育预元原援园员圆源缘远愿院约越月云匀允运杂栽"
		db	"灾载再在咱赞遭早造责择则泽怎增曾扎寨展占战站章张掌障招找赵照召折"
		db	"哲者这真针震振镇阵蒸征争整正政证枝支知脂之织职直植殖执值指止只纸"
		db	"志至致置制智质治中忠钟终种重众周州洲轴珠株猪逐竹主著柱助铸筑住注"
		db	"驻抓专转庄装壮状锥追准着资紫子自字宗总纵走足族祖阻组钻嘴最罪尊遵"
		db	"左做作坐座孢"
CCLIB3CacheLen	equ	@-CCLIB3Cache
CCLIBCache	dw	CCLIB0Cache,	CCLIB0CacheLen/2
		dw	CCLIB1Cache,	CCLIB1CacheLen/2
		dw	CCLIB2Cache,	CCLIB2CacheLen/2
		dw	CCLIB3Cache,	CCLIB3CacheLen/2
CCLIBCacheRem	dw	( 113*2-CCLIB0CacheLen)/2 
		dw	( 166*2-CCLIB1CacheLen)/2 
		dw	( 115*2-CCLIB2CacheLen)/2 
		dw	(1479*2-CCLIB3CacheLen)/2 
HelpSignString	db	" 运行SMCDOS/?可以阅读简要说明",13,10
		db	"$"
HelpString1	db	13,10
		db	"==开关说明 (格式SMCDOS/字符)==",13,10
		db	"/U  SMCDOS退出内存驻留",13,10
		db	"/Zn 选择字库驻留方式。",13,10
		db	"    n与菜单的数字相对应。",13,10
		db	"/?  阅读简要使用说明",13,10
		db	"$"
HelpString2	db	13,10
		db	"======= 功能键使用说明 =======",13,10
		db	"Alt/F1或Alt/Shift/1 区位和国标",13,10
		db	"Alt/F9或Alt/Shift/9 符号输入",13,10
		db	"Alt/F10或Alt/Shift/0  西文方式",13,10
		db	"Ctrl/F9  半角/全角字符切换",13,10
		db	"$"
HelpString3	db	13,10
		db	"==== SMCDOS 文件使用说明 =====",13,10
		db	"SMCDOS 中文系统文件:",13,10
		db	" SMCDOS.CMD 中文系统程序。",13,10
		db	" CCLIB16. 国标16×16点阵字库。",13,10
		db	" PINYIN.CMD 全拼输入法程序",13,10
		db	" PINYIN.DOC 全拼输入法使用说明",13,10
		db	" 9P.CMD EPSON兼容9针打印机驱动",13,10
		db	" 24P.CMD EPSON 24针打印机驱动",13,10
		db	"$"
WaitKeySting	db	"──── 按任一键继续 ────$"

;---------------------------------------;
FileIOAccess:				;
	stx	FileIO+HBUF+1		;
	sty	FileIO+HBUF		;
FileIOAccessIN:				;
	sta	FileIO+FILECMD		;
	ldx	#<FileIO		;
	ldy	#>FileIO		;
	jmp	FileIOEntry		;
;---------------------------------------;
;	Input	X|Y= HZ scode.		;
;---------------------------------------;
ReadCCLIBAccess:			;
	txa				;
	sty	HzCodeL			;
	jsr	HZCodeToCURP		;
	lda	FileIOPaged+CURP	;
	sta	FileIO+CURP		;
	lda	FileIOPaged+CURP+1	;
	sta	FileIO+CURP+1		;
	lda	FileIOPaged+CURP+2	;
	sta	FileIO+CURP+2		;
ReadCCLIBAccessIN:			;
	ldx	#5			;
lprca04:asl	ReadHzNumbers		; Read total in byte.
	rol	ReadHzNumbers+1		;
	rol	ReadHzNumbers+2		;
	dex				;
	bne	lprca04			;
lprca08:lda	FileIO+HBUF+1		;
	cmp	#<$c000			;
	bcc	nerca02			;
	inc	FileIO+HPAGE		;
	lda	#<$8000			;
	sta	FileIO+HBUF+1		;
nerca02:lda	#>$c000			; Buffer can read this time.
	sec				;
	sbc	FileIO+HBUF		;
	tay				;
	lda	#<$c000			;
	sbc	FileIO+HBUF+1		;
	tax				;
	cpy	ReadHzNumbers		; Buffer full read this time?
	sbc	ReadHzNumbers+1		;
	lda	#0			;
	sbc	ReadHzNumbers+2		;
	bcc	nerca04			;
	ldx	ReadHzNumbers+1		; No.
	ldy	ReadHzNumbers		;
nerca04:stx	FileIO+BYTES+1		;
	sty	FileIO+BYTES		;
	lda	ReadHzNumbers		;
	sec				;
	sbc	FileIO+BYTES		;
	sta	ReadHzNumbers		;
	lda	ReadHzNumbers+1		;
	sbc	FileIO+BYTES+1		;
	sta	ReadHzNumbers+1		;
	bcs	nerca08			;
	dec	ReadHzNumbers+2		;
nerca08:lda	#FileRead		;
	jsr	FileIOAccessIN		;
	bcs	nerca16			;
	lda	ReadHzNumbers		;
	ora	ReadHzNumbers+1		;
	ora	ReadHzNumbers+2		;
	bne	lprca08			;
	clc				;
nerca16:rts				;
;---------------------------------------;
;	Input	X|A= HZ code.		;
;---------------------------------------;
SaveCacheIndex:				;
	ldy	#0			;
	sta	($f4),y			;
	iny				;
	txa				;
	sta	($f4),y			;
	iny				;
	lda	$f6			;
	sta	($f4),y			;
	iny				;
	lda	$f7			;
	sta	($f4),y			;
	inc	$f6			;
	bne	necct04			;
	inc	$f7			;
necct04:lda	$f4			;
	clc				;
	adc	#4			;
	sta	$f4			;
	bcc	necct08			;
	inc	$f5			;
necct08:rts				;
;---------------------------------------;
CacheCount:				;
	tya				;
	pha				;
	dey				;
	dey				;
	lda	($f8),y			; High
	tax				;
	iny				;
	lda	($f8),y			; Low
	jsr	SaveCacheIndex		;
	pla				;
	tay				;
	dec	$fa			;
	bne	necct12			;
	dec	$fb			;
necct12:rts				;
;---------------------------------------;
ReadCCLIBCache:				;
	asl				;
	tax				;
	asl				;
	tay				;
	lda	CCLIBCacheRem,x		;
	sta	$f2			;
	lda	CCLIBCacheRem+1,x	;
	sta	$f3			;
	ldx	#0			;
	stx	$f6			;
	stx	$f7			;
lprcc04:lda	CCLIBCache,y		;
	sta	$f8,x			;
	iny				;
	inx				;
	cpx	#4			;
	bne	lprcc04			;
	ldx	#<CacheHzBegin		;
	ldy	#>CacheHzBegin		;
	stx	$f5			;
	sty	$f4			;
	lda	$fa			;
	beq	lprcc08			;
	inc	$fb			;
lprcc08:lda	$fa			;
	ora	$fb			;
	beq	nercc36			;
	ldy	#0			;
lprcc12:lda	($f8),y			; High.
	iny				;
	iny				;
	sec				;
	sbc	($f8),y			;
	iny				;
	bcc	nercc04			;
	lda	($f8),y			; High same.
	dey				;
	dey				;
	clc				;
	sbc	($f8),y			;
	beq	nercc08			;
	bne	nercc16		;(jmp)	;
nercc04:cmp	#$ff			;
	bne	nercc12			;
	lda	($f8),y			; High sequence.
	dey				;
	dey				;
	sec				;
	sbc	($f8),y			;
	cmp	#>$a1-$fe		;
	bne	nercc16			;
nercc08:iny				; HANZI sequence.
	sec				;
	beq	nercc24			;
	jsr	CacheCount		;
	bne	lprcc12			;
	clc				;
	bcc	nercc28		;(jmp)	;
nercc12:dey				;
	bne	nercc20		;(jmp)	;
nercc16:iny				;
nercc20:clc				;
nercc24:php				;
	jsr	CacheCount		;
	plp				;
nercc28:tya				;
	pha				;
	ror				;
	sta	ReadHzNumbers		; Read HZ numbser this time.
	lda	#0			;
	sta	ReadHzNumbers+1		;
	ldy	#0			;
	lda	($f8),y			;
	tax				;
	iny				;
	lda	($f8),y			;
	tay				; X|Y = HZ.
	pla				;
	clc				;
	adc	$f8			;
	sta	$f8			;
	bcc	nercc32			;
	inc	$f9			;
nercc32:jsr	ReadCCLIBAccess		;
	jmp	lprcc08			;
nercc36:lda	$f2			;
	beq	nercc40			;
	inc	$f3			;
nercc40:ora	$f3			;
	beq	nercc44			;
lprcc16:ldx	#$ff			; Hanzi code = $ffff.
	txa				;
	jsr	SaveCacheIndex		;
	dec	$f2			;
	bne	lprcc16			;
	dec	$f3			;
	bne	lprcc16			;
nercc44:ldx	$f5			;
	ldy	$f4			;
	stx	CacheHzEnd+1		;
	sty	CacheHzEnd		;
	rts				;
;---------------------------------------;
OpenCCLIB:				;
	lda	CurrentPage		;
	sta	FileIO+HPAGE		;
	lda	#0			;
	sta	FileIO+CURDRIVE		;
	lda	#0			;
	sta	ErrorType		;
	lda	#FileOpen		;
	ldx	#<CCLIBFile		;
	ldy	#>CCLIBFile		;
	jsr	FileIOAccess		;
	bcs	nercb04			;
	ldy	#32-1			;
lprcb04:lda	FileIO,y		;
	sta	FileIOPaged,y		;
	dey				;
	bpl	lprcb04			;
nercb04:rts				;
;---------------------------------------;
ReadCCLIBZero:				;
	ldx	#<113*4+28+CacheHzBegin	; 32 align.
	ldy	#>113*4+28+CacheHzBegin	;
	stx	FileIO+HBUF+1		;
	sty	FileIO+HBUF		;
	ldx	#<113*4+28+CacheHzBegin-$8000/32
	ldy	#>113*4+28+CacheHzBegin-$8000/32
	stx	BaseFontAddr+1		;
	sty	BaseFontAddr+0		;
	ldx	#<113*4+28+CacheHzBegin-$8000/32+64+0
	ldy	#>113*4+28+CacheHzBegin-$8000/32+64+0
	stx	BaseFontAddr+1+3+3	;
	sty	BaseFontAddr+0+3+3	;
	lda	#DosInquirePages	; 1 pages required.
	ldx	#1			;
	jsr	DosIOEntry		;
	bcs	nercz04			;
	inc	ErrorType		;
	ldx	#<128/2			; ---- 128 ASCII.
	ldy	#>128/2			;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	jsr	ReadCCLIBAccessIN	;
	bcs	nercz04			;
	lda	#128/2			;
	sta	ASCIIResident		;
	lda	#0
	sta	CCLIBReadType		;
	clc				;
nercz04:rts				;	
;---------------------------------------;
;    One CCLIB resident.(Total 4096).	;
; and 256 HANZI cache.			;
;---------------------------------------;
ReadCCLIBOne:				;
	lda	#128/2			;
	sta	ASCIIResident		;
	lda	#1			;
	sta	CCLIBReadType		;
	lda	BiosVersion		;
	cmp	#$40			; BBG ?
	bne	nerco08			;
	ldx	#<115*4+20+CacheHzBegin	;
	ldy	#>115*4+20+CacheHzBegin	;
	stx	FileIO+HBUF+1		;
	sty	FileIO+HBUF		;
	ldx	#<115*4+20+CacheHzBegin-$8000/32
	ldy	#>115*4+20+CacheHzBegin-$8000/32
	stx	BaseFontAddr+1		;
	sty	BaseFontAddr+0		;
	ldx	#<115*4+20+CacheHzBegin-$8000/32+64-$a1-3584
	ldy	#>115*4+20+CacheHzBegin-$8000/32+64-$a1-3584
	stx	BaseFontAddr+1+3	;
	sty	BaseFontAddr+0+3	;
	ldx	#<115*4+20+CacheHzBegin-$8000/32+1022
	ldy	#>115*4+20+CacheHzBegin-$8000/32+1022
	stx	BaseFontAddr+1+3+3	; Cache
	sty	BaseFontAddr+0+3+3	;
	lda	#DosInquirePages	; 3 pages required.
	ldx	#3			;
	jsr	DosIOEntry		;
	bcs	nerco04			;
	inc	ErrorType		;
	ldx	#<128/2			; ---- 128 ASCII.
	ldy	#>128/2			;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	jsr	ReadCCLIBAccessIN	;
	bcs	nerco04			;
	ldx	#<1022			; ---- CCLIB1 remained.
	ldy	#>1022			;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	ldx	#<$cdad			;
	ldy	#>$cdad			;
	jsr	ReadCCLIBAccess		;
	bcs	nerco04			;
	lda	#2			;
	clc				;
nerco04:rts				;
nerco08:ldx	#<166*4+8+CacheHzBegin	;
	ldy	#>166*4+8+CacheHzBegin	;
	stx	FileIO+HBUF+1		;
	sty	FileIO+HBUF		;
	ldx	#<166*4+8+CacheHzBegin-$8000/32
	ldy	#>166*4+8+CacheHzBegin-$8000/32
	stx	BaseFontAddr+1		;
	sty	BaseFontAddr+0		;
	ldx	#<166*4+8+CacheHzBegin-$8000/32+64-$a1
	ldy	#>166*4+8+CacheHzBegin-$8000/32+64-$a1
	stx	BaseFontAddr+1+3	;
	sty	BaseFontAddr+0+3	;
	ldx	#<166*4+8+CacheHzBegin-$8000/32+64+3968
	ldy	#>166*4+8+CacheHzBegin-$8000/32+64+3968
	stx	BaseFontAddr+1+3+3	; Cache
	sty	BaseFontAddr+0+3+3	;
	lda	#DosInquirePages	; 9 pages required.
	ldx	#9			;
	jsr	DosIOEntry		;
	bcs	nerco12			;
	inc	ErrorType		;
	ldx	#<128/2			; ---- 128 ASCII.
	ldy	#>128/2			;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	jsr	ReadCCLIBAccessIN	;
	bcs	nerco12			;
	ldx	#<94			; ---- Q01
	ldy	#>94			;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	ldx	#<$a1a1			;
	ldy	#>$a1a1			;
	jsr	ReadCCLIBAccess		;
	bcs	nerco12			;
	ldx	#<94			; ---- Q03
	ldy	#>94			;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	ldx	#<$a3a1			;
	ldy	#>$a3a1			;
	jsr	ReadCCLIBAccess		;
	bcs	nerco12			;
	ldx	#<3760			; ---- Q16-Q55
	ldy	#>3760			;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	ldx	#<$b0a1			;
	ldy	#>$b0a1			;
	jsr	ReadCCLIBAccess		;
	bcs	nerco12			;
	ldx	#<94			; ---- Q09
	ldy	#>94			;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	ldx	#<$a9a1			;
	ldy	#>$a9a1			;
	jsr	ReadCCLIBAccess		;
	bcs	nerco12			;
	lda	#1			;
	clc				;
nerco12:rts				;
;---------------------------------------;
ReadCCLIBTwo:				;
	lda	#>256/2			;
	sta	ASCIIResident		;
	ldx	#<115*4+20+CacheHzBegin	;
	ldy	#>115*4+20+CacheHzBegin	;
	stx	FileIO+HBUF+1		;
	sty	FileIO+HBUF		;
	ldx	#<115*4+20+CacheHzBegin-$8000/32
	ldy	#>115*4+20+CacheHzBegin-$8000/32
	stx	BaseFontAddr+1		;
	sty	BaseFontAddr+0		;
	lda	BiosVersion		;
	cmp	#$40			; BBG ?
	bne	nerct04			;
	ldx	#<115*4+20+CacheHzBegin-$8000/32+128-$a1-3584
	ldy	#>115*4+20+CacheHzBegin-$8000/32+128-$a1-3584
	stx	BaseFontAddr+1+3	;
	sty	BaseFontAddr+0+3	;
	ldx	#<115*4+20+CacheHzBegin-$8000/32+128+7614-3584
	ldy	#>115*4+20+CacheHzBegin-$8000/32+128+7614-3584
	stx	BaseFontAddr+1+3+3	;
	sty	BaseFontAddr+0+3+3	;
	lda	#DosInquirePages	; 9 pages required.
	ldx	#9			;
	jsr	DosIOEntry		;
	bcs	nerct12			;
	inc	ErrorType		;
	ldx	#<128			; ---- 128 ASCII.
	ldy	#>128			;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	jsr	ReadCCLIBAccessIN	;
	bcs	nerct12			;
	ldx	#<7614-3584		;
	ldy	#>7614-3584		;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	ldx	#<$cdad			;
	ldy	#>$cdad			;
	jsr	ReadCCLIBAccess		;
	bcs	nerct12			;
	bcc	nerct08			;
nerct04:ldx	#<115*4+20+CacheHzBegin-$8000/32+128-$a1
	ldy	#>115*4+20+CacheHzBegin-$8000/32+128-$a1
	stx	BaseFontAddr+1+3	;
	sty	BaseFontAddr+0+3	;
	ldx	#<115*4+20+CacheHzBegin-$8000/32+128+7614
	ldy	#>115*4+20+CacheHzBegin-$8000/32+128+7614
	stx	BaseFontAddr+1+3+3	;
	sty	BaseFontAddr+0+3+3	;
	lda	#DosInquirePages	; 16 pages required.
	ldx	#16			;
	jsr	DosIOEntry		;
	bcs	nerct12			;
	inc	ErrorType		;
	ldx	#<128+81*94		; All Q (7742)
	ldy	#>128+81*94		;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	jsr	ReadCCLIBAccessIN	;
	bcs	nerct12			;
nerct08:lda	#FileClose		;
	jsr	FileIOAccessIN		;
	lda	#2			;
	sta	CCLIBReadType		;
	clc				;
nerct12:rts				;
;---------------------------------------;
ReadCCLIBThree:				;
	ldx	#<1479*4+4+CacheHzBegin	;
	ldy	#>1479*4+4+CacheHzBegin	;
	stx	FileIO+HBUF+1		;
	sty	FileIO+HBUF		;
	ldx	#<1479*4+4+CacheHzBegin-$8000/32
	ldy	#>1479*4+4+CacheHzBegin-$8000/32
	stx	BaseFontAddr+1		;
	sty	BaseFontAddr+0		;
	ldx	#<1479*4+4+CacheHzBegin-$8000/32+64+0
	ldy	#>1479*4+4+CacheHzBegin-$8000/32+64+0
	stx	BaseFontAddr+1+3+3	;
	sty	BaseFontAddr+0+3+3	;
	lda	#DosInquirePages	; 4 pages required.
	ldx	#4			;
	jsr	DosIOEntry		;
	bcs	nercu04			;
	inc	ErrorType		;
	ldx	#<128/2			; ---- 128 ASCII.
	ldy	#>128/2			;
	stx	ReadHzNumbers+1		;
	sty	ReadHzNumbers		;
	jsr	ReadCCLIBAccessIN	;
	bcs	nercu04			;
	lda	#128/2			;
	sta	ASCIIResident		;
	lda	FileIO+HPAGE		;
	lda	#0			;
	sta	CCLIBReadType		;
	lda	#7			;
	sta	LRUPlusValue		;
	lda	#1			;
	sta	LRURemainedPlus		;
	lda	#8			;
	sta	LRUCurrentCount		;
	lda	#3			;
	clc				;
nercu04:rts				;	
;---------------------------------------;
BranchFFFE:				;
	stx	$ff			;
	sty	$fe			;
	jmp	($00fe)			;
;---------------------------------------;
TitleBeginLine	equ	1		;
StringAddress	dw	0		;
StringLength	db	0		;
StringLines	db	0		;
;---------------------------------------;
DisplayOLine:				;
	pha				;
	lda	#1			;
	sta	StringLines		;
	pla				;
DisplayLines:				;
	sta	StringLength		;
	stx	StringAddress+1		;
	sty	StringAddress		;
DisplayLinesINT:			;
	lda	CurColumn		;
lpdls04:pha				;
	lda	StringLength		;
	sta	$fd			;
	ldx	StringAddress+1		;
	ldy	StringAddress		;
	jsr	DispStringAt		;
	lda	StringAddress		;
	clc				;
	adc	StringLength		;
	sta	StringAddress		;
	bcc	nedls04			;
	inc	StringAddress+1		;
nedls04:pla				;
	sta	CurColumn		;
	dec	StringLines		;
	bne	lpdls04			;
	rts				;
;---------------------------------------;
CopyrightStg    db      "          中文操作系统  第1.0版",13,10
CopyrightLLen	equ	@-CopyrightStg
                db      "                               ",13,10
                db      "                               ",13,10
CopyrightLen	equ	@-CopyrightStg
DisplayTitle:				;
	lda	#TitleBeginLine		;
	sta	CurLine			;
	ldx	#$40			; String type.
	stx	$fc			;
	lda	#$07			; FG
	sta	$fe			;
	lda	#$05			; BG
	sta	$ff			;
	lda	#CopyrightLen/CopyrightLLen
	sta	StringLines		;
	lda	#CopyrightLLen		;
	ldx	#<CopyrightStg		;
	ldy	#>CopyrightStg		;
	jmp	DisplayLines		;
;---------------------------------------;
CCLIBStg	db	"汉字库驻留选择",13,10
CCLIBLLen	equ	@-CCLIBStg
CCLIBLstStg	db	" 1:  少量汉字 ",13,10
		db	" 2:  常用汉字 ",13,10
		db	" 3:  一级汉字 ",13,10
		db	" 4:  二级汉字 ",13,10
CCLIBLstLen	equ	@-CCLIBLstStg
KeyListStg	db	"[][],["
CCLIBBase	db	"1]-["
CCLIBLimit	db	"?]",13,10
WaitStg		db	"正在装载汉字库,请稍候...."
WaitLen		equ	@-WaitStg
UninstallFlag	db	0		;
CCLIBBases	db	0		; Item can select minimin
CCLIBItems	db	0		; Item can select maximum
CCLIBItem	db	0		;
SelectItem	db	0		;
DspHelpFlag	db	0		;
CCLIBBranchTab	dw	ReadCCLIBZero,	ReadCCLIBThree
		dw	ReadCCLIBOne,	ReadCCLIBTwo
;---------------------------------------;
ShowOneItem:				;
	tax				;
	clc				;
	adc	#CopyrightLen/CopyrightLLen+TitleBeginLine+2
	sta	CurLine			;
	lda	#ScreenWidth-CCLIBLLen+2/2
	sta	CurColumn		;
	txa				;
	ldx	#<CCLIBLstStg		;
	ldy	#>CCLIBLstStg		;
	stx	StringAddress+1		;
	sty	StringAddress		;
	tax				;
	beq	nesoi08			;
lpsoi04:lda	StringAddress		;
	clc				;
	adc	#CCLIBLLen		;
	sta	StringAddress		;
	bcc	nesoi04			;
	inc	StringAddress+1		;
nesoi04:dex				;
	bne	lpsoi04			;
nesoi08:lda	#CCLIBLLen		;
	ldx	StringAddress+1		;
	ldy	StringAddress		;
	jmp	DisplayOLine		;
;---------------------------------------;
CCLIBSelect:				;
	lda	#$ff			; Has enough page to hold CCLIB2?
	sec				;
	sbc	CurrentPage		;
	ldx	BiosVersion		;
	cpx	#$40			; BBG ?
	bne	necse02			;
	ldx	#'3'			;
	stx	CCLIBBase		;
	ldx	#2			;
	stx	CCLIBBases		;
	clc				;
	adc	#7			; 7 additional page.
necse02:ldx	#CCLIBLstLen/CCLIBLLen	; CCLIB items.
	ldy	#3			; Default select item
	cmp	#16+6			; 6 page reserved.
	bcs	necse04			;
	dey				;
	cmp	#16			;
	bcs	necse04			;
	dex				;
	cmp	#9+6			;
	bcs	necse04			;
	dey				;
	cmp	#9			;
	bcs	necse04			;
	dex				;
	cmp	#5+6			;
	bcs	necse04			;
	dey				;
	cmp	#5			;
	bcc	necse16			;
necse04:stx	CCLIBItems		;
	cpy	CCLIBBases		;
	bcs	necse08			;
	ldy	CCLIBBases		;
necse08:sty	SelectItem		;
	txa				;
	clc				;
	adc	#'0'			;
	sta	CCLIBLimit		;
	ldy	CCLIBItem		; Has set CCLIB item in command line?
	beq	necse20			;
	dey				; Yes.
	cpy	CCLIBBases		;
	bcs	necse12			;
	ldy	CCLIBBases		;
	bcc	necse16		;(jmp)	;
necse12:cpy	CCLIBItems		; Exceed item maximum?
	bcc	necse16			;
	ldy	CCLIBItems		; Yes.
	dey				;
necse16:ldx	#0			;
	stx	CCLIBItems		;
	sty	SelectItem		;
	lda	#CopyrightLen/CopyrightLLen+TitleBeginLine
	jmp	necse48			;
necse20:lda	#CopyrightLen/CopyrightLLen+TitleBeginLine
	sta	CurLine			;
	lda	#ScreenWidth-CCLIBLLen+2/2
	sta	CurColumn		;
	lda	#$07			; FG
	sta	$fe			;
	lda	#$04			; BG
	sta	$ff			;
	lda	#CCLIBLLen		;
	ldx	#<CCLIBStg		;
	ldy	#>CCLIBStg		;
	jsr	DisplayOLine		;
	lda	#ScreenWidth-CCLIBLLen+2/2-1
	sta	CurColumn		;
	ldy	#CCLIBLLen-2+2		;
	ldx	#$cd			;
	jsr	DispCharsAt		;
	inc	CurLine			;
	lda	#ScreenWidth-CCLIBLLen+2/2
	sta	CurColumn		;
	ldx	#<CCLIBLstStg		;
	ldy	#>CCLIBLstStg		;
	lda	CCLIBBases		;
	beq	necse24			;
	sta	StringLines		;
	lda	#$01			; FG
	sta	$fe			;
	lda	#$00			; BG
	sta	$ff			;
	lda	#CCLIBLLen		;
	jsr	DisplayLines		;
	lda	#$07			; FG
	sta	$fe			;
	lda	#$04			; BG
	sta	$ff			;
	lda	CCLIBItems		;
	sec				;
	sbc	CCLIBBases		;
	sta	StringLines		;
	jsr	DisplayLinesINT		;
	jmp	necse28			;
necse24:lda	CCLIBItems		;
	sta	StringLines		;
	lda	#CCLIBLLen		; Yes. Display KeyList select.
	jsr	DisplayLines		;
necse28:lda	#CCLIBLLen		;
	ldx	#<KeyListStg		;
	ldy	#>KeyListStg		;
	jsr	DisplayOLine		;
	lda	#CopyrightLen/CopyrightLLen+TitleBeginLine
	sta	CurLine			;
	lda	#$06			; FG
	sta	$fe			;
	lda	CCLIBItems		;
	clc				;
	adc	#3			;
	sta	StringLines		;
lpcse04:lda	#ScreenWidth-CCLIBLLen+2/2-2
	sta	CurColumn		;
	ldy	#0			;
	ldx	#$b0			;
	jsr	DisplayChAt		;
	lda	#ScreenWidth+CCLIBLLen-2/2+1
	sta	CurColumn		;
	ldy	#0			;
	ldx	#$b0			;
	jsr	DisplayChAt		;
	inc	CurLine			;
	dec	StringLines		;
	bne	lpcse04			;
	lda	#ScreenWidth-CCLIBLLen+2/2-2
	sta	CurColumn		;
	ldy	#CCLIBLLen-2+4		;
	ldx	#$b0			;
	jsr	DispCharsAt		;
;---------------------------------------;
	lda	#$07			; FG
	sta	$fe			;
lpcse08:lda	#$06			; BG
	sta	$ff			;
	lda	SelectItem		;
	jsr	ShowOneItem		;
lpcse12:lda	#BiosDevice		; Read a key.
	ldx	#0			;
	jsr	ROMBiosEntry		;
	cmp	#13			;
	beq	necse44			;
	cpx	#$48			; ?
	bne	necse32			;
	ldy	SelectItem		;
	cpy	CCLIBBases		;
	beq	lpcse12			;
	dey				;
	bpl	necse40		;(jmp)	;
necse32:cpx	#$50			; ?
	bne	necse36			;
	ldy	SelectItem		;
	iny				;
	cpy	CCLIBItems		;
	bcs	lpcse12			;
	bcc	necse40		;(jmp)	;
necse36:sec				;
	sbc	#'1'			;
	bcc	lpcse12			;
	cmp	CCLIBBases		;
	bcc	lpcse12			;
	cmp	CCLIBItems		;
	bcs	lpcse12			;
	tay				;
necse40:tya				;
	pha				;
	lda	#4			;
	sta	$ff			;
	lda	SelectItem		;
	jsr	ShowOneItem		;
	pla				;
	sta	SelectItem		;
	jmp	lpcse08			;
necse44:lda	#CopyrightLen/CopyrightLLen+TitleBeginLine+4
necse48:clc				;
	adc	CCLIBItems		;
	sta	CurLine			;
	lda	#ScreenWidth-WaitLen/2	;
	sta	CurColumn		;
	lda	#$07			; FG
	sta	$fe			;
	lda	#$04			; BG
	sta	$ff			;
	lda	#WaitLen		;
	ldx	#<WaitStg		;
	ldy	#>WaitStg		;
	jsr	DisplayOLine		;
	lda	BiosVersion		;
	cmp	#$40			;
	beq	necse52			;
	lda	#0			; Get default driver.
	sta	FileIO+CURDRIVE		;
	lda	#FileDrive		;
	jsr	FileIOAccessIN		;
	lda	FileIO+CURDRIVE		; Read CCLIB from diskette?
	cmp	#2+1			;
	bcs	necse52			;
	ldx	#$80			; Yes.
	jsr	SetScreenSW		;
necse52:jsr	OpenCCLIB		;
	bcs	necse56			;
	lda	CurrentPage		;
	sta	FileIO+HPAGE		;
	lda	#0			;
	sta	FileIO+CURP+2		;
	sta	FileIO+CURP+1		;
	sta	FileIO+CURP		;
	lda	#1			;
	sta	ErrorType		;
	lda	SelectItem		;
	asl				;
	tay				;
	ldx	CCLIBBranchTab+1,y	;
	lda	CCLIBBranchTab,y	;
	tay				;
	jsr	BranchFFFE		;
	bcs	necse56			;
	jsr	ReadCCLIBCache		;
	bcs	necse56			;
	ldx	#0			;
	jsr	SetScreenSW		;
	lda	#CopyrightLen/CopyrightLLen+TitleBeginLine
	sta	CurLine			; Clear window.
	lda	#ScreenWidth-WaitLen/2	;
	sta	CurColumn		;
	lda	#0			;
	sta	$fe			;
	sta	$ff			;
	lda	CCLIBItems		;
	clc				;
	adc	#4+1			;
	sta	StringLines		;
lpcse16:ldy	#WaitLen		;
	ldx	#' '			;
	jsr	DispCharsAt		;
	inc	CurLine			;
	dec	StringLines		;
	bne	lpcse16			;
	lda	#CopyrightLen/CopyrightLLen+TitleBeginLine
	sta	CurLine			;
	lda	#0			;
	sta	CurColumn		;
	clc				;
necse56:rts				;
;---------------------------------------;
GetCommandLineCh:			;
	iny				;
	beq	negcc04			;
	lda	($fe),y			;
	cmp	#13			;
	beq	negcc04			;
	clc				;
	rts				;
negcc04:sec				;
	rts				;
;---------------------------------------;
CommandAccess:				;
	ldx	#<$0100			;
	ldy	#>$0100			;
	stx	$fe+1			;
	sty	$fe			;
	ldy	#0			;
lpcas04:jsr	GetCommandLineCh	;
	bcs	necas20			; String ending
	cmp	#'/'			;
	bne	lpcas04			;
	jsr	GetCommandLineCh	;
	bcs	necas20			;
	cmp	#'?'			;
	bne	necas04			;
	inc	DspHelpFlag		;
	jmp	lpcas04			;
necas04:ora	#$20			; 
	cmp	#'u'			;
	bne	necas08			;
	sta	UninstallFlag		;
	beq	lpcas04			;
necas08:cmp	#'z'			;
	bne	lpcas04			;
	jsr	GetCommandLineCh	;
	bcs	necas20			;
	sec				;
	sbc	#'1'			;
	bcc	lpcas04			;
	cmp	#9+1			;
	bcs	lpcas04			;
	adc	#1			;
	sta	CCLIBItem		;
	bcc	lpcas04		;(jmp)	;
necas20:rts				;

;---------------------------------------;
BIOSIO_LowProg:				;
	php				;
	cmp	#BiosGetSystemFont	; Is get system font function?
	beq	neblp04			;
LowModify1:				;
	bit	HANZIDispMode		; No. Current is HANZI display mode?
	bpl	neblp08			;
	cmp	#BiosDevice		; Yes. $18.
	beq	neblp04			;
	cmp	#BiosDisplayTTY		; $1b-$1d.
	bcc	neblp08			;
	cmp	#BiosDisplayChars+1	;
	bcc	neblp04			;
	cmp	#BiosWaitDisplay	; $22
	beq	neblp04			;
	cmp	#BiosDispChAtTTY	; $28-$3b.
	bcc	neblp08			;
	cmp	#BiosDispInitial	; $3c
	beq	neblp08			;
	cmp	#$3e+1			; $3d-$3e
	bcs	neblp08			;
LowModify5:				;
neblp04:sta	LowSaveA		; $1b-$1d. Save A.
	lda	DRAMPageSave		;
	pha				; Caller's page.
LowModify2:				;
	lda	#0			; Set to BIOS page.
	sta	DRAMPageSave		;
	sta	DRAMPagePort		;
	pla				;
	jsr	CBiosEntry		;
	sta	DRAMPageSave		; Caller's pageN.
	sta	DRAMPagePort		;
LowModify6:				;
	lda	LowSaveA		; Restore return's A.
	plp				;
	rts				;
HANZIDispMode	db	0		;
LowSaveA	db	0		;
neblp08:plp				;
OrgBiosEntry	ds	3		; jmp	$xxxx
BIOSIO_LowLen	equ	@-BIOSIO_LowProg;

SMCDOSUninstall	db	10,"SMCDOS uninstall successful.",13,10,"$"

;=======================================;
Initial:ldx	#$ff			;
	txs				;
	jsr	CommandAccess		;
	lda	#DosGetSetDispMode	; Can set to HZ display mode?
	ldx	#1			;
	ldy	#HZDisplayMode		;
	jsr	DosIOEntry		;
	bcc	neini04			;
	jmp	neini12			;
neini04:lda	UninstallFlag		; Yes. SMCDOS has resident.
	beq	neini08			;
	lda	#DosGetSetDispMode	; Set to ASCII display mode.
	ldx	#1			;
	ldy	#ASDisplayMode		;
	jsr	DosIOEntry		;
	lda	#BiosGetSystemFont	; Restore original BIOS entry.
	ldx	#<'的'			;
	ldy	#<'的'			;
	jsr	ROMBiosEntry		; Get SMCDOS page to (A).
	sta	OriginalPage		;
	ldx	#<OldClearScrAddr	; Restore old clear screen vector.
	ldy	#>OldClearScrAddr	;
	stx	$fe+1			;
	sty	$fe			;
	ldy	#0			;
	jsr	GetOneInputData		;
	pha				;
	jsr	GetOneInputData		;
	pha				;
	jsr	GetOneInputData		;
	tax				;
	pla				;
	tay				;
	pla				;
	sta	$ff			;
	lda	#DosGetSetInitScrn	;
	jsr	DosIOEntry		;
	ldx	#<OrgBiosEntryIn	; Restore original bios entry address.
	ldy	#>OrgBiosEntryIn	;
	stx	$fe+1			;
	sty	$fe			;
	ldy	#0			;
	jsr	GetOneInputData		; Address low.
	pha				;
	jsr	GetOneInputData		; Address high.
	sta	ROMBiosEntry+2		;
	pla				;
	sta	ROMBiosEntry+1		;
	lda	#$4c			;
	sta	ROMBiosEntry		;
	lda	#BiosLowRAMFree		; Free low memory.
	ldx	#<BIOSIO_LowLen		;
	ldy	#>BIOSIO_LowLen		;
	jsr	ROMBiosEntry		;
	lda	#DosReleasePages	; Free paged memory.
	ldx	OriginalPage		;
	jsr	DosIOEntry		;
	lda	#DosDisplayString	;
	ldx	#<SMCDOSUninstall	;
	ldy	#>SMCDOSUninstall	;
	jsr	DosIOEntry		;
	jmp	neini44			;
;---------------------------------------;
neini08:lda	#BiosSetCursor		;
	ldx	#TitleBeginLine		; Display title.
	ldy	#0			;
	jsr	ROMBiosEntry		;
	ldx	#$40			; String type.
	stx	$fc			;
	lda	#CopyrightLLen		;
	sta	$fd			;
	lda	#$07			; FG
	sta	$fe			;
	lda	#$05			; BG
	sta	$ff			;
	lda	#BiosDispStringAt	;
	ldx	#<CopyrightStg		;
	ldy	#>CopyrightStg		;
	jsr	ROMBiosEntry		;
	lda	#CopyrightLLen		;
	sta	$fd			;
	lda	#BiosDispStringAt	;
	ldx	#<CopyrightLLen+CopyrightStg
	ldy	#>CopyrightLLen+CopyrightStg
	jsr	ROMBiosEntry		;
	lda	#CopyrightLLen		;
	sta	$fd			;
	lda	#BiosDispStringAt	;
	ldx	#<CopyrightLLen*2+CopyrightStg
	ldy	#>CopyrightLLen*2+CopyrightStg
	jsr	ROMBiosEntry		;
	jmp	neini32			;
;---------------------------------------;
neini12:lda	#BiosGetVersion		; Get BIOS date.
	ldx	#$ff			;
	jsr	ROMBiosEntry		; Is 1995/8/3 bios version?
	inx				;
	bne	neini16			;
	txa				; Yes.
neini16:sta	BiosVersion		;
	lda	#BiosGetPage		;
	jsr	ROMBiosEntry		;
	sta	CurrentPage		;
	sta	BaseFontAddr+2		;
	sta	BaseFontAddr+2+3	;
	sta	BaseFontAddr+2+3+3	;
	sta	OldExtFontProc		;
	sta	HZFuncProcPage+0	; Alt/F1  QW-GB.
	sta	HZFuncProcPage+8	; Alt/F10 SYMBOL.
	sta	HZFuncProcPage+9	; Alt/F10 ASCII.
	sta	$ff			; Set new clear screen vector.
	lda	#DosGetSetInitScrn	;
	ldx	OldClearScrAddr+2	;
	ldy	OldClearScrAddr+1	;
	jsr	DosIOEntry		;
	sta	OldClearScrAddr+0	;
	stx	OldClearScrAddr+2	;
	sty	OldClearScrAddr+1	;
	lda	#DosGetSetDispMode	; Set to HANZI display
	ldx	#1			;
	ldy	#HZDisplayMode		;
	jsr	DosIOEntry		;
	ldx	#$80+12			; Hide cursor.
	jsr	SetCursorType		;
	jsr	DisplayTitle		;
	jsr	CCLIBSelect		;
	bcs	neini20			;
;---------------------------------------;
	lda	#3			;
	sta	ErrorType		;
	lda	ROMBiosEntry		; Save original BiosIOEntry.
	ldx	ROMBiosEntry+2		;
	ldy	ROMBiosEntry+1		;
	sta	OrgBiosEntry		;
	stx	OrgBiosEntry+2		;
	sty	OrgBiosEntry+1		;
	stx	OrgBiosEntryIn+1	;
	sty	OrgBiosEntryIn		;
	lda	#BiosLowRAMAlloc	; Allocate low memory.
	ldx	#<BIOSIO_LowLen		;
	ldy	#>BIOSIO_LowLen		;
	jsr	ROMBiosEntry		;
	cmp	#1			;
neini20:bcs	neini24			;
	lda	#$4c			;
	sta	ROMBiosEntry		;
	stx	ROMBiosEntry+2		; Set ROMBiosEntry Ins.
	sty	ROMBiosEntry+1		;
	stx	$fe+1			; Low memory beginning in X|Y
	sty	$fe			;
	lda	DRAMPageSave		; Move to Low memory.
	sta	LowModify2+1		;
	tya				; Instruction modify.
	clc				;
	adc	#>HANZIDispMode-BIOSIO_LowProg
	sta	LowModify1+1		;
	sta	LowModify3+1		;
	sta	LowModify4+1		;
	txa				;
	adc	#<HANZIDispMode-BIOSIO_LowProg
	sta	LowModify1+2		;
	sta	LowModify3+2		;
	sta	LowModify4+2		;
	tya				;
	adc	#>LowSaveA-BIOSIO_LowProg
	sta	LowModify5+1		;
	sta	LowModify6+1		;
	sta	LowModify7+1		;
	sta	LowModify8+1		;
	txa				;
	adc	#<LowSaveA-BIOSIO_LowProg
	sta	LowModify5+2		;
	sta	LowModify6+2		;
	sta	LowModify7+2		;
	sta	LowModify8+2		;
	ldy	#0			;
lpini04:lda	BIOSIO_LowProg,y	;
	sta	($fe),y			;
	iny				;
	cpy	#>BIOSIO_LowLen		;
	bne	lpini04			;
	jmp	neini32			;
;---------------------------------------; Error.
neini24:ldx	#0			;
	jsr	SetScreenSW		;
	lda	OldClearScrAddr+0	; Restore clear screen vector.
	sta	$ff			;
	lda	#DosGetSetInitScrn	;
	ldx	OldClearScrAddr+2	;
	ldy	OldClearScrAddr+1	;
	jsr	DosIOEntry		;
	lda	ErrorType		; Error type.
	beq	neini28			;
	lda	#DosReleasePages	; Free paged memory.
	ldx	CurrentPage		;
	jsr	DosIOEntry		;
	lda	#FileClose		;
	jsr	FileIOAccessIN		;
neini28:lda	#DosGetSetDispMode	; Set to ASCII display?
	ldx	#1			;
	ldy	#ASDisplayMode		;
	jsr	DosIOEntry		;
	lda	ErrorType		; Error type.
	asl				;
	tay				;
	ldx	ErrorStgPoint+1,y	; Error string.
	lda	ErrorStgPoint,y		;
	tay				;
	lda	#DosDisplayString	;
	jsr	DosIOEntry		;
	jmp	neini44			;
;---------------------------------------;
DisplayHelpString:			;
	lda	#DosDisplayString	;
	jsr	DosIOEntry		;
	lda	#DosDisplayString	;
	ldx	#<WaitKeySting		;
	ldy	#>WaitKeySting		;
	jsr	DosIOEntry		;
	ldx	#0			;
	jsr	DeviceEntry		;
	ldx	#TitleBeginLine+3	;
lpdhs04:txa				;
	pha				;
	lda	#BiosSetCursor		;
	ldy	#0			;
	jsr	ROMBiosEntry		;
	lda	#BiosDisplayChars	;
	ldy	#ScreenWidth		;
	ldx	#' '			;
	jsr	ROMBiosEntry		;
	pla				;
	tax				;
	inx				;
	cpx	#ChineseLine		;
	bne	lpdhs04			;
	lda	#BiosSetCursor		;
	ldx	#TitleBeginLine+3	;
	ldy	#0			;
	jsr	ROMBiosEntry		;
	rts				;
;---------------------------------------;
neini32:lda	DspHelpFlag		;
	beq	neini36			;
	lda	#BiosSetCursorType	; Hide cursor.
	ldx	#$80+12			;
	jsr	ROMBiosEntry		;
	ldx	#<HelpString1		;
	ldy	#>HelpString1		;
	jsr	DisplayHelpString	;
	ldx	#<HelpString2		;
	ldy	#>HelpString2		;
	jsr	DisplayHelpString	;
	ldx	#<HelpString3		;
	ldy	#>HelpString3		;
	jsr	DisplayHelpString	;
	jmp	neini40			;
neini36:lda	#DosDisplayString	;
	ldx	#<HelpSignString	;
	ldy	#>HelpSignString	;
	jsr	DosIOEntry		;
neini40:lda	#BiosSetCursorType	; Restore cursor.
	ldx	#$00+12			;
	jsr	ROMBiosEntry		;
neini44:lda	#DosProgramReturn	;
	ldy	#0			;
	jmp	DosIOEntry		;

	org	$8000
DisplayBuffer	equ	$5d00		;
SpliteBuffer	ds	256		;
ReturnCode	equ	$fd		; Return code from HZ proc.
DispBfHead	db	0		;
DispBfTail	db	0		;
Mouse0ShowSt	db	0		; Mouse 0 show(-)/unshow(+) status.
Mouse1ShowSt	db	0		; Mouse 1 show(-)/unshow(+) status.
CursorBegRow	db	0		; Cursor beginning row.
CursorEndRow	db	0		; Cursor ending row.
CursorTypeFlag	db	0		; Current cursor type
 ;	Bit 7:	Cursor hide.		;
 ;	Bit 6:	16 dot cursor.		;
CursorBlinkCnt	db	0		;
CursorBlockRow	db	0		; Cursor row in screen.
SpliteDispFlag	db	0		; Send splite data to PPU flag.
OriginalPageN	db	0		; Caller's page not check.
OriginalPage	db	0		; Caller's page has check.
CurrentPage	db	0		; Program current page.
OrgBiosEntryIn	dw	0		; Original bios entry.
BiosVersion	db	0		; ROM bios version(0-255).
LastScreenSW	db	0		; 0/1: HANZI/Wait screen.
CCLIBReadType	db	0		; 0/1/2 cclib reading.
RefColumn	db	0		; HZ refrence line's cursor column.
HZFunction	db	0		; HZ input funtion(Alt/F1-Alt/F10).
HZFunctionSave	ds	1		; HZ function save for Alt/F9
ASCIIFontLow	db	0		; Display ASCII lower row.
KBDTestMode	db	0		; 0: NMI. <0: SEnd.
KBDINTVector	dw	0		;
OpenCloseScreen	db	0		; 0:Open/1:Close screen flag.
HzCodeL		db	0		;
TempSave1	equ	HzCodeL		;
HzCodeH		db	0		;
TempSave2	equ	HzCodeH		;
SearchBegin	dw	0		;
TempSave3	equ	SearchBegin	;
TempSave4	equ	SearchBegin+1	;
SearchEnd	dw	0		;
Full_Half	db	0		; Full/-ASCII proc.
ContinueCount	equ	SearchEnd	;
TempSave5	equ	ContinueCount	;
TempSave6	equ	ContinueCount+1	;
MoveBlockBFSave	ds	8		;
ScrollBuffer	equ	MoveBlockBFSave	;
 ScrollMode	equ	ScrollBuffer+0	;
 LeftUpC	equ	ScrollBuffer+1	;
 LeftUpL	equ	ScrollBuffer+2	;
 RightDownC	equ	ScrollBuffer+3	;
 RightDownL	equ	ScrollBuffer+4	;
 ScrollLines	equ	ScrollBuffer+5	;
 SpaceBColor	equ	ScrollBuffer+6	;
FileIOPaged	ds	32		;|
Attribute	ds	64		;
SplitBuffer	ds	76		;| 15*5+1
CurrentSplit	db	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14	;|
FreeVideoPage	db	15					;|
FontBuffer	ds	32		;
BaseFontAddr	dw	0		;| ASCII.
		db	0		;
		dw	0		;| HANZI.
		db	0		;
		dw	(CacheHzBegin+$140-$8000)/32
					;| CACHE.
		db	0		;
		dw	-$a1		;| BBG ROM
		db	$40		;
CacheHzBeg	equ	CacheHzBegin	; Must 256 align.
CacheHzEnd	dw	CacheHzEndInit	;
ASCIIResident	db	0		;
LRUPlusValue	db	0		;
LRUCurrentCount	db	2		;
LRURemainedPlus	db	$40		;
LRURemained	db	0		;
OldExtFontProc	db	0		; Page
		dw	ExtFontProcIN-1	;
OldClearScrAddr	db	0		; Page
		dw	ClearScreenHZ-1	;
HZFuncParameter	dw	HZFuncReturn-1	;|
PageBase	dw	0		;|
HanziSet	db	$80		;| 00: Used. 40: 1Set. 80: 2Set.
KeyBoardCode	dw	0		;|
StringCount	db	0		;|
StringBuffer	ds	7		;|
OutputNumber	db	0		;|
NextPageHas	db	0		;|
OutputBufferLen	db	0		;|
OutputIndex	db	0		;|
OutputBuffer	equ	OutputIndex+1	;|
DispStringBf	ds	255		;|
DispStringBfLen	equ	@-DispStringBf

;---------------------------------------;
FlashDispBuffer:			;
	txa				; Save X, Y
	pha				;
	tya				;
	pha				;
	lda	$ff			; Save $ff
	pha				;
	lda	$fe			;
	pha				;
	lda	Addr2001		;
	pha				;
	lda	Addr2000		;
	and	#$7f			;
	sta	$2000			;
	lda	VideoCtrlSave		;
	and	#$bf			; MODE=0
	sta	VideoCtrlPort		;
	bit	$2002			;
	lda	#0			;
	sta	$2001			;<- Must late enough else FGChar error!!
	ldx	#226			; Seperated chip's time.
	ldy	DispBfHead		;  One chip can delay $ca
lpfdb04:lda	DisplayBuffer,y		;4
	sta	$ff			;3 Tempare save.
	iny				;2
	bit	$ff			;3
	bmi	nefdb08			;2/4
	jmp	nefdb28			;3 -17
;---------------------------------------; 10xxxxxx. Save font string.
nefdb08:pha				;3 Save High.
	and	#$3f			;2
	sta	$2006			;4
	lda	DisplayBuffer,y		;4
	iny				;2
	sta	$2006			;4
	sta	DispBfHead		;4 Save Low(temp).
	lda	DisplayBuffer,y		;4
	iny				;2
	sta	VideoDataPort		;4 0000-07ff video page.
	pha				;3 Save Page.
	lda	DisplayBuffer,y		;4 Count.
	pha				;3 Save count.
	cpx	#4+5		;!	;2
	bcc	nefdb16			;2
	cmp	#8			;2
	bcc	nefdb16			;2 /4-69
	lsr				;2
	and	#$fc			;2
	sta	$ff			;3
	txa				;2
	sec				;2
	sbc	#5		;!	;2
	pha				;3 Save X.
	and	#$fc			;2
	cmp	$ff			;3 Count can do.
	bcs	nefdb12			;2
	sta	$ff			;3
nefdb12:lda	$ff			;3
	lsr				;2
	lsr				;2
	tax				;2 -- 102=4.72
lpfdb08:iny				;
	lda	DisplayBuffer,y		; Send 8 font one count.
	sta	$2007			; (86 clk)
	iny				;
	lda	DisplayBuffer,y		;
	sta	$2007			;
	iny				;
	lda	DisplayBuffer,y		;
	sta	$2007			;
	iny				;
	lda	DisplayBuffer,y		;
	sta	$2007			;
	iny				;
	lda	DisplayBuffer,y		;
	sta	$2007			;
	iny				;
	lda	DisplayBuffer,y		;
	sta	$2007			;
	iny				;
	lda	DisplayBuffer,y		;
	sta	$2007			;
	iny				;
	lda	DisplayBuffer,y		;
	sta	$2007			;
	dex				;
	bne	lpfdb08			;
	pla				;4 Restore X.
	sec				;2
	sbc	$ff			;3
	tax				;2
	pla				;4 Restore count.
	pha				;3
	sbc	$ff			;3
	sbc	$ff			;3
	beq	nefdb24			;2 /4 -28
	inx			;!	;2
nefdb16:sta	$ff			;3
	cpx	#1+4		;!	;2
	bcc	nefdb20			;2
	txa				;2
	sbc	#4		;!	;2
	tax				;2
	cpx	$ff			;3
	bcs	lpfdb16			;2 -46/85
lpfdb12:iny				;
	lda	DisplayBuffer,y		;
	sta	$2007			;
	dec	$ff			;
	dex				;
	bne	lpfdb12			;
	lda	$ff			; Count remain.
nefdb20:sta	DisplayBuffer,y		;
	dey				;
	pla				; Restore count.
	tax				;
	pla				; Restore Page.
	sta	DisplayBuffer,y		;
	dey				;
	txa				;
	sec				;
	sbc	$ff			; Display passed.
	clc				;
	adc	DispBfHead		; Address low
	sta	DisplayBuffer,y		;
	dey				;
	pla				; Restore High.
	adc	#0			;
	sta	DisplayBuffer,y		;
	jmp	nefdb72			;
lpfdb16:iny				;
	lda	DisplayBuffer,y		;
	sta	$2007			;
	dex				;
	dec	$ff			;
	bne	lpfdb16			;
	inx				;2
nefdb24:pla				;4 Count.
	pla				;4 Page.
	pla				;4 High.
	txa				;2
	sec				;2
	sbc	#3			;2
	jmp	nefdb64			;3
;---------------------------------------; 00xxxxxx. Save One.
nefdb28:bvs	nefdb32			;2/4 -19/21
	sta	$2006			;4
	lda	DisplayBuffer,y		;4 V=0.
	sta	$2006			;4
	iny				;2
	lda	DisplayBuffer,y		;4
	sta	$2007			;4
	txa				;2
	sec				;2
	sbc	#3			;2
	jmp	nefdb64			;3 -50
;---------------------------------------;
nefdb32:and	#$3f			;2 01xxxxxx. Save same string.
	sta	$2006			;4
	lda	DisplayBuffer,y		;4
	sta	$2006			;4
	iny				;2
	lda	DisplayBuffer,y		;4
	iny				;2
	sta	VideoDataPort		;4 0000-07ff video page.
	lda	DisplayBuffer,y		;4
	cpx	#4+6		;!	;2
	bcc	nefdb44			;2/4 -55/57
	cmp	#0			;2
	beq	nefdb36			;2
	cmp	#16			;2
	bcc	nefdb48			;2/4 -63/65
nefdb36:cmp	#1			;2
	ror				;2
	eor	#$80			;2
	lsr				;2
	and	#$fc			;2
	sta	$ff			;3
	txa				;2
	sec				;2
	sbc	#6		;!	;2
	pha				;3 Save X.
	and	#$fc			;2
	cmp	$ff			;3 Count can do.
	bcs	nefdb40			;2
	sta	$ff			;3
nefdb40:lda	$ff			;3
	lsr				;2
	lsr				;2
	pha				;2 Save Count.
	tax				;2
	iny				;2
	lda	DisplayBuffer,y		;4 Character
	dey				;2
	sta	DispBfHead		;4 116
lpfdb20:sta	$2007			;
	sta	$2007			; (70)
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	sta	$2007			;
	dex				;
	bne	lpfdb20			;
	pla				;4 x += C/8*1.5
	lsr				;2
	sta	$fe			;3
	lsr				;2
	clc				;2
	adc	$fe			;3
	sta	$fe			;3
	pla				;4 Restore X.
	adc	$fe			;3
	sec				;2
	sbc	$ff			;3
	tax				;2
	asl	$ff			;5
	asl	$ff			;5
	lda	DisplayBuffer,y		;4
	sec				;2
	sbc	$ff			;3
	beq	nefdb56			;2 -54
nefdb44:cmp	#0			;2
	beq	nefdb52			;2
	cmp	#16			;2
	bcs	nefdb52			;2 -65
nefdb48:cpx	#1+4		;!	;2
	bcc	nefdb52			;2
	sty	$ff			;3 Save Y.
	tay				;2 Count to Y.
	txa				;2
	sbc	#4		;!	;2
	tax				;2
	lda	DispBfHead		;4 -84
lpfdb24:sta	$2007			;
	dey				;
	beq	nefdb60			;
	sta	$2007			;
	dey				;
	beq	nefdb60			;
	dex				;
	bne	lpfdb24			;
	tya				;
	ldy	$ff			; Restore Y.
nefdb52:ldx	DisplayBuffer,y		; Original count.
	sta	DisplayBuffer,y		; Count remain.
	txa				;
	sec				;
	sbc	DisplayBuffer,y		;
	dey				;
	dey				;
	clc				;
	adc	DisplayBuffer,y		;
	sta	DisplayBuffer,y		;
	dey				;
	lda	DisplayBuffer,y		; High
	adc	#0			;
	sta	DisplayBuffer,y		;
	jmp	nefdb72			;
nefdb56:iny				;2
	txa				;2
	sec				;2
	sbc	#4			;2
	jmp	nefdb64			;3
nefdb60:ldy	$ff			;3
	iny				;2
	txa				;2
	sec				;2
	sbc	#1			;2
nefdb64:bcc	nefdb68			;2
	tax				;2
	iny				;2
	cpy	DispBfTail		;4
	beq	nefdb72			;2
	jmp	lpfdb04			;3
nefdb68:iny				;
nefdb72:sty	DispBfHead		;
	lda	Addr2000		;
	ora	#$80			;
	sta	$2000			;
	pla				;
	sta	$2001			;
	pla				;
	sta	$fe			;
	pla				;
	sta	$ff			;
	pla				; Restore X,Y
	tay				;
	pla				;
	tax				;
	rts				;
;---------------------------------------;
FlashAllDspBf:				;
lpfab04:lda	DispBfHead		;
	cmp	DispBfTail		;
	beq	nefab04			;
	jsr	FlashDispBuffer		;
	jmp	lpfab04			;
nefab04:rts				;
;---------------------------------------;
;	Input	(A)= Buffer require.	;
;---------------------------------------;
WaitDBSize	db	0		;
WaitDispBuffer:				;
	sta	WaitDBSize		;
	inc	WaitDBSize		;
lpwdb04:lda	DispBfHead		; Wait display buffer has space.
	sec				;
	sbc	DispBfTail		;
	beq	newdb04			;
	cmp	WaitDBSize		;
	bcs	newdb04			;
	lda	OpenCloseScreen		; Screen closed?
	beq	lpwdb04			;
	jsr	FlashDispBuffer		;
	jmp	lpwdb04			;
newdb04:rts				;
;---------------------------------------;
;	Save	X,Y			;
;---------------------------------------;
ModifySplite:				;
	lda	Mouse0ShowSt		;
	bpl	nemse08			;
	lda	Mouse0X			; Mouse 0.
	clc				;
	adc	#8			;
	cmp	SpliteBuffer+$f4+3	;
	beq	nemse04			;
	sta	SpliteBuffer+$f4+3	;
	inc	SpliteDispFlag		;
nemse04:lda	Mouse0Y			;
	cmp	SpliteBuffer+$f4	;
	beq	nemse08			;
	sta	SpliteBuffer+$f4	;
	inc	SpliteDispFlag		;
nemse08:lda	Mouse1ShowSt		;
	bpl	nemse20			;
	lda	Mouse1X			; Mouse 1.
	clc				;
	adc	#1			;
	cmp	SpliteBuffer+$f0+3	;
	beq	nemse16			;
	sta	SpliteBuffer+$f0+3	;
	inc	SpliteDispFlag		;
nemse16:lda	Mouse1Y			;
	cmp	SpliteBuffer+$f0	;
	beq	nemse20			;
	sta	SpliteBuffer+$f0	;
	bne	nemse24		;(jmp)	;
nemse20:lda	SpliteDispFlag		; Splite refresh?
	beq	nemse28			;
nemse24:lda	#0			; Yes.
	sta	SpliteDispFlag		;
	sta	$2003			;
	lda	#<$8000			;
	sta	$4014			;
nemse28:rts				;
;---------------------------------------;
CursorBlinkAccs:			;
	lda	CursorTypeFlag		; Check cursor blink.
	bmi	necab12			;
	and	#$1f			;
	pha				;
	cmp	CursorBlinkCnt		;
	bne	necba08			;
	lda	#$ff			;
	sta	CursorBlinkCnt		;
	lda	#$f8			;
	sta	SpliteDispFlag		;
	cmp	SpliteBuffer+$fc	; Last is black cursor?
	bne	necba04			;
	lda	CursorBlockRow		; Yes. Blight cursor.
necba04:sta	SpliteBuffer+$fc	;
	bit	CursorTypeFlag		; 16 dot cursor?
	bvc	necba08			;
	sta	SpliteBuffer+$f8	;
necba08:pla				;
	beq	necab12			;
	inc	CursorBlinkCnt		;
necab12:rts				;
;---------------------------------------;
;	Save font string.(10xxxxxx).	;
;		+0:	Address High.	;
;		+1:	Address Low.	;
;		+2:	Page.		;
;		+3:	Count.		;
;		+4-:	Font.		;
;	Save same count.(01xxxxxx).	;
;		+0:	Address High.	;
;		+1:	Address Low.	;
;		+2:	Page.		;
;		+3:	Count.		;
;		+4:	Fonts.		;
;---------------------------------------;
DisplayINT:				;
	lda	DispBfHead		;
	bit	LastScreenSW		;
	bmi	nednt04			;
	cmp	DispBfTail		;
	bne	nedit04			;
	lda	#13			; Delay for display blank.
lpdnt04:sec				; 2
	sbc	#1			; 2
	bne	lpdnt04			; 4
	jsr	CursorBlinkAccs		;
	jsr	ModifySplite		;
	bit	KBDTestMode		;
	bpl	nednt08			;
HighModify:				;
	jmp	nednt08			; Modify when check keyboard at here.
nednt04:sta	DispBfTail		;
nednt08:rts				;
;---------------------------------------;
nedit04:jsr	FlashDispBuffer		;
	jsr	ModifySplite		;
	lda	VideoCtrlSave		;
	sta	VideoCtrlPort		;
	rts				;

;---------------------------------------;
;	Input	X|Y= Video address.	;
;		(A)= Video data.	;
;	Save	Y.			;
;---------------------------------------;
SendVideo:				;
	pha				;
	lda	#3			;
	jsr	WaitDispBuffer		;
	txa				;
	ldx	DispBfTail		;
	sta	DisplayBuffer,x		;
	inx				;
	tya				;
	sta	DisplayBuffer,x		;
	inx				;
	pla				;
	sta	DisplayBuffer,x		;
	inx				;
	stx	DispBfTail		;
	rts				;

;---------------------------------------;
PaletteTable	db	$0e,	$00,	$10,	$30	; BG.
		db	$0e,	$02,	$19,	$30
		db	$0e,	$0e,	$0e,	$0e
		db	$0e,	$0e,	$20,	$12
		db	$0e,	$0e,	$0e,	$0e
		db	$0e,	$0e,	$0e,	$0e
		db	$0e,	$0e,	$0e,	$0e
		db	$0e,	$12,	$0e,	$30
PaletteTableLen	equ	@-PaletteTable
;---------------------------------------;
WaitVSyn:				;
	lda	#BiosWaitVSynCnt	;
	ldx	#1			;
	jmp	ROMBiosEntry		;
;---------------------------------------;
ClearScreenHZ:				;
	jsr	SetScreenSW0		;
	sec				;
	pla				; Screen mode. Set to ASCII mode?
	bpl	necsh04			;
LowModify3:				;
	sta	HANZIDispMode		;
	jsr	InitialScreen		; Yes. Clear HZ screen.
	clc				; Clear OK.
	bcc	necsh08		;(jmp)	;
necsh04:jsr	JSROldClearScr		; Clear OK?
	bcs	necsh08			;
	lda	#0			; Yes. Reset HANZI display mode.
LowModify4:				;
	sta	HANZIDispMode		;
necsh08:pla				;
	tax				;
	lda	#BiosSetPage		;
	jmp	ROMBiosEntry		;
JSROldClearScr:				;
	tax				; Save new screen mode.
	lda	CurrentPage		; Return page.
	pha				;
	txa				; New screen mode to stack.
	pha				;
	lda	OldClearScrAddr+2	; Jump to original clear screen Proc.
	pha				;
	lda	OldClearScrAddr+1	;
	pha				;
	ldx	OldClearScrAddr+0	;
	lda	#BiosSetPage		;
	jmp	ROMBiosEntry		;

WaitDisketteFont:
dw	$1000,	$60c,	$307,	$103,	0,	0,	$8000,	$3f80,	$1820,	$e1c,	$40e,	$ff00,	0,	0,	$400,	$ff0e
dw	$102,	0,	$1e10,	$1c1c,	$8000,	$e0c0,	$7073,	$20,	0,	0,	$ff,	0,	0,	$3810,	$38fc,	$3838
dw	$303,	$303,	$707,	$7ff,	$c000,	$8080,	$1c08,	$fe,	$f0c,	$1c0e,	$181c,	$3038,	0,	0,	0,	$c00
dw	0,	0,	0,	0,	$f0c,	$8c0e,	$e0ff,	$e1e3,	0,	$301,	$3ff,	$c303,	0,	$8000,	$80c0,	$8080
dw	0,	$702,	$7ff,	$707,	0,	0,	$80,	0,	$7070,	$7070,	$7070,	$707f,	0,	0,	$3810,	$38fc
dw	$1c1c,	$1c1c,	$1c1c,	$1c1c,	0,	$3f20,	$3838,	$3838,	$800,	$fe1c,	$1c1c,	$1c1c,	$3838,	$3838,	$3838,	$3838
dw	$f0e,	$1d0d,	$1d1d,	$7f39,	0,	$c0e0,	$c9c0,	$fedd,	$603f,	$c460,	$87c7,	$e0f,	$1ffe,	$381c,	$e030,	0
dw	0,	$7f00,	0,	0,	$e0e0,	$ffe0,	$e6e0,	$e1e3,	$e3e3,	$ff43,	$303,	$c383,	$8880,	$fe9c,	$8080,	$8080
dw	$707,	$707,	$707,	$707,	0,	0,	$800,	$6010,	$7070,	$7070,	$7070,	$e0e0,	$3838,	$3838,	$3838,	$7070
dw	$1c1c,	$1c1c,	$1c1c,	$1c1c,	$3838,	$3838,	$383f,	$3038,	$1c1c,	$1c1c,	$1cfc,	$181c,	$3838,	$3838,	$3838,	$3838
dw	$131,	$101,	$101,	$7f0f,	$c0c0,	$cec3,	$e0f8,	$c0c0,	$e0e,	$1e0e,	$1d1f,	$3939,	0,	0,	0,	$8000
dw	$101,	$703,	$1f0e,	$333,	$c0c1,	$80,	$ff00,	$8e8e,	$8fff,	$406,	$ff00,	$3838,	$80,	0,	$f060,	$e0e0
dw	$707,	$60f,	4,	0,	$81c1,	$703,	$1c0e,	$c030,	$c0c0,	$1f80,	$107,	1,	$7070,	$f070,	$c0e0,	0
dw	$1c1c,	$1c1c,	$1c1c,	$181c,	0,	0,	0,	0,	0,	0,	3,	0,	$3838,	$3838,	$f0f8,	$4060
dw	$61f9,	$101,	$101,	$101,	$c0c0,	$c1c0,	$c7c3,	$f0dc,	$7071,	$c0e0,	$80,	0,	$c080,	$70e0,	$3f7c,	$81e
dw	$303,	$303,	$7f03,	0,	$8e8e,	$8e8e,	$ff8e,	0,	$3838,	$3838,	$ff38,	0,	$e0e0,	$e4e0,	$ffee,	0
dw	$800,	$706,	$303,	$101,	0,	0,	$809f,	$80,	$1e10,	$1c1c,	$1cff,	$1c1c,	0,	$3810,	$fc,	$7020
dw	0,	$300,	$f31f,	$303,	$2000,	$fcf0,	$8183,	$8081,	$f08,	$e0e,	$8e0e,	$eece,	0,	$1c18,	$383e,	$6070
dw	$101,	$301,	$303,	$607,	$e000,	$81c0,	$80,	$4000,	0,	$ff00,	$101,	$101,	0,	$e0c0,	$c0c0,	$c8c0
dw	0,	$702,	$7ff,	$707,	$f,	0,	$bf,	$400,	$1cff,	$1c1c,	$ff,	0,	$f8,	$e04,	$ff,	$7020
dw	$303,	$7ff,	$f0f,	$1f0f,	$b080,	$83fa,	$8383,	$f3c3,	$4fee,	$ff0f,	$8080,	$8080,	$88c0,	$fe1c,	$1c1c,	$1c1c
dw	$c0e,	$1e1f,	$3e3e,	$4e6e,	$7378,	$7070,	$7070,	$7171,	$ff01,	$f080,	$c0e0,	$8fff,	$fedc,	0,	$7020,	$f8
dw	$707,	$707,	$707,	$707,	$707,	$707,	$170f,	$6727,	$ff,	0,	$ff,	0,	$70f8,	$7070,	$70f0,	$7070
dw	$3b1b,	$6333,	$c363,	$383,	$bfbb,	$839b,	$8383,	$8383,	$80ff,	$8080,	$ff80,	$8080,	$1cfc,	$1c1c,	$fc1c,	$1c1c
dw	$e8e,	$e0e,	$e0e,	$e0e,	$7371,	$7072,	$7077,	$7070,	$e8e,	$e0e,	$eff,	$1f1e,	0,	$1c08,	$fe,	0
dw	$707,	$f0f,	$406,	0,	$87c7,	$707,	$707,	$607,	$ff,	0,	$700,	1,	$70f0,	$7070,	$f070,	$c0e0
dw	$303,	$303,	$303,	$303,	$8383,	$8383,	$8383,	$383,	$8080,	$8080,	$8081,	$80,	$1c1c,	$1c1c,	$78fc,	$2030
dw	$e0e,	$e0e,	$e0e,	$c0e,	$60,	0,	$301,	$300c,	$393b,	$e071,	$c0,	0,	$c080,	$f8e0,	$387e,	$10
;---------------------------------------;
VRAMPage0400:				;
	sta	VideoDataPort		;
	ldx	#<$0400			;
VRAMBeginY0:				;
	ldy	#0			;
VRAMBegin:				;
	bit	$2002			;
	stx	$2006			;
	sty	$2006			;
	rts				;
;---------------------------------------;
InitialLine:				;
	lda	#$ff			;
lpile04:ldy	#32			;
lpile08:sta	$2007			;
	dey				;
	bne	lpile08			;
	dex				;
	bne	lpile04			;
	rts				;
;---------------------------------------;
InitialTwoLines:			;
	ldy	#0			;
lpitl04:sty	$2007			; 0,2,4,...,62
	iny				;
	iny				;
	cpy	#64			;
	bcc	lpitl04			;
	ldy	#1			;
lpitl08:sty	$2007			; 1,3,5,...,63
	iny				;
	iny				;
	cpy	#64			;
	bcc	lpitl08			;
	rts				;
;---------------------------------------;
Clear7Char:				;
	ldy	#7			;
	bne	ClearIChar		;
Clear3Char:				;
	ldy	#3			;
ClearIChar:				;
	lda	#$ff			;
lpc3c04:sta	$2007			;
	dey				;
	bne	lpc3c04			;
	rts				;
;---------------------------------------;
Set4Char:				;
	ldy	#4			;
lps4c04:stx	$2007			;
	inx				;
	dey				;
	bne	lps4c04			;
	rts				;
;---------------------------------------;
SaveOneFont:				;
	lda	#0			;
	tax				;
lpsof04:sta	$2007			;
	inx				;
	cpx	#8			;
	bne	lpsof04			;
lpsof08:lda	($fe),y			;
	iny				;
	sta	$2007			;
	dex				;
	bne	lpsof08			;
	tya				;
	bne	nesof04			;
	inc	$fe+1			;
nesof04:rts				;
;---------------------------------------;
;	Input	(A)= Bit 0=0: Set HZ	;
;			  =1: No set HZ	;
;---------------------------------------;
InitialScreen:				;
	pha				; Save mode.
	jsr	WaitVSyn		;
	lda	#0			; Display display and NMI.
	sta	$2001			;
	sta	$2000			;
lpisn04:bit	$2002			;
	bmi	lpisn04			;
lpisn06:bit	$2002			;
	bpl	lpisn06			;
	ldx	#<$3f00			;
	jsr	VRAMBeginY0		;
lpisn08:lda	PaletteTable,y		; Set color palette
	sta	$2007			;
	iny				;
	cpy	#PaletteTableLen	;
	bne	lpisn08			;
	ldx	#<$0000			;
	jsr	VRAMBeginY0		;
	lda	#0			; Display NMI.
	sta	DispBfHead		;
	sta	DispBfTail		;
	sta	LastScreenSW		;
	lda	VideoCtrlSave		;
	and	#$bf			; MODE=0
	sta	VideoCtrlSave		;
	sta	VideoCtrlPort		;
;---------------------------------------;
	ldy	#0			; Clear all(16) page's font.
	ldx	#8			; Page 0 clear from 0000 to 07ff.
lpisn12:tya				;
	pha				;
	sty	VideoDataPort		;
	txa				;
	ldx	#<$0000			;
	jsr	VRAMBeginY0		;
	tax				;
	tya				;
lpisn16:sta	$2007			;
	iny				;
	bne	lpisn16			;
	dex				;
	bne	lpisn16			;
	pla				;
	tay				;
	ldx	#4			; Page 4 clear from 0000 to 03ff.
	iny				;
	cpy	#16			;
	bne	lpisn12			;
;---------------------------------------;
	lda	#BiosSetAV10		; AV10o <- AV11.
	ldx	#1			;
	jsr	ROMBiosEntry		;
	ldx	#<$2000			; Set VRAM(2000-23ff).
	jsr	VRAMBeginY0		;
	ldx	#15			; 15*16=240.
lpisn20:jsr	InitialTwoLines		;
	dex				;
	bne	lpisn20			;
	ldy	#$40-8			; Set Attribute.
	lda	#0			;
lpisn24:sta	$2007			;
	sta	Attribute-1,y		;
	dey				;
	bne	lpisn24			;
	ldy	#8			;
	lda	#$0f			;
lpisn28:sta	$2007			;
	sta	Attribute+$40-8-1,y	;
	dey				;
	bne	lpisn28			;
;---------------------------------------;
	lda	#2			; FG video page1.
	sta	VideoDataPort+16	;
	lda	#3			; FG video page2.
	sta	VideoDataPort+24	;
	ldx	#0			; Initial splite buffer.
	ldy	#$41			;
lpisn32:lda	#$f8			;
	sta	SpliteBuffer,x		; (Y).
	inx				;
	tya				;
	sta	SpliteBuffer,x		; ASCII.
	cmp	#$7f			;
	bne	neisn04			;
	ldy	#$bf			;
neisn04:iny				;
	iny				;
	inx				;
	inx				;
	inx				;
	bne	lpisn32			;
;---------------------------------------;
	lda	$fe+1			;
	pha				;
	lda	$fe			;
	pha				;
	ldx	#<WaitDisketteFont	;
	ldy	#>WaitDisketteFont	;
	stx	$fe+1			;
	sty	$fe			;
	lda	#14			; HZ refline.
	jsr	VRAMPage0400		;
lpisn36:jsr	SaveOneFont		;
	lda	$fe+1			;
	cmp	#<WaitDisketteFont+$200	;
	bne	lpisn36			;
	lda	#0			; Page 0.
	jsr	VRAMPage0400		;
lpisn48:jsr	SaveOneFont		;
	cpy	#$80			;
	bne	lpisn48			;
	lda	$fe+1			;
	cmp	#<WaitDisketteFont+$300	;
	bne	lpisn48			;
	pla				;
	sta	$fe			;
	pla				;
	sta	$fe+1			;
	ldx	#<$2c00			;
	jsr	VRAMBeginY0		;
	ldx	#8			;
	jsr	InitialLine		;
	ldx	#$40			;
	lda	#4			;
lpisn60:pha				;
	ldy	#4			;
	jsr	ClearIChar		;
	lda	#4			;
lpisn64:pha				;
	jsr	Set4Char		;
	jsr	Clear3Char		;
	pla				;
	sec				;
	sbc	#1			;
	bne	lpisn64			;
	pla				;
	sbc	#1			;
	bne	lpisn60			;
	ldx	#4			;
	jsr	InitialLine		;
	ldx	#$c0			;
	lda	#4			;
lpisn68:pha				;
	jsr	Clear7Char		;
	jsr	Set4Char		;
	jsr	Clear3Char		;
	jsr	Set4Char		;
	jsr	Clear3Char		;
	jsr	Set4Char		;
	jsr	Clear7Char		;
	pla				;
	sbc	#1			;
	bne	lpisn68			;
	ldx	#8			;
	jsr	InitialLine		;
	jsr	InitialTwoLines		;
	ldx	#2			; Set Attribute.
	jsr	InitialLine		;
;---------------------------------------;
	lda	VideoCtrlSave		; Restore MODE.
	sta	VideoCtrlPort		;
	ldx	#15			;
lpisn72:txa				;
	sta	CurrentSplit,x		;
	dex				;
	bpl	lpisn72			;
	jsr	SetSplitScreen		;
	lda	#BiosSetSEndVector	;
	ldx	#<DisplayINT		;
	ldy	#>DisplayINT		;
	jsr	ROMBiosEntry		;
;---------------------------------------;
	ldx	#1			; Home and display cursor.
	stx	OpenCloseScreen		;
	ldy	#0			;
	jsr	SetCursor		;
	ldx	#$20+12			; Set cursor shape.
	ldy	#$ef			;
	jsr	SetCursorType		;
	lda	#10-1			; ASCII/NUMBER input mode.
	sta	HZFunction		;
	pla				; Restore mode.
	lsr				;
	bcs	neisn08			;
	ldx	#0			; Clear chinese line
	jsr	DispRefChar		;
neisn08:jsr	FlashAllDspBf		;
;---------------------------------------;
	lda	#$a9			; Enable NMI.
	sta	$2000			;
	jsr	WaitVSyn		; Let splite screen work.
	lda	#0			;
	sta	$2003			;
	lda	#<$8000			;
	sta	$4014			;
	cli				;
	ldx	#0			;
	jmp	DispOpenClose		;
;---------------------------------------;
SetSplitScreen:				;
	ldx	#0			;
	ldy	#0			;
lpssn04:lda	#$f0			;
	sta	SplitBuffer,x		;
	lda	CurrentSplit,y		;
	sta	SplitBuffer+1,x		;
	lda	CurrentSplit+1,y	;
	sta	SplitBuffer+2,x		;
	lda	#$f8			;
	sta	SplitBuffer+3,x		;
	lda	#0			;
	sta	SplitBuffer+4,x		;
	txa				;
	clc				;
	adc	#5			;
	tax				;
	iny				;
	cpy	#15			;
	bne	lpssn04			;
	lda	BiosVersion		;
	cmp	#$10			;
	bcs	nessn04			; Hardware high resolution mode.
	inc	SplitBuffer-5,x		;
nessn04:lda	#0			;
	sta	SplitBuffer,x		;
	lda	#BiosSplitScreen	;
	ldx	#<SplitBuffer		;
	ldy	#>SplitBuffer		;
	jmp	ROMBiosEntry		;
;---------------------------------------;
;	Input	Y|X= ASCII code.	;
;---------------------------------------;
GetASCIIFont:				;
	txa				; Save ASCII.
	pha				;
	lsr				; Even it.
	tax				; Y|X.
	jsr	ReadHanzi		;
	pla				;
	lsr				; Is even ASCII?
	lda	#32			;
	bcs	negaf04			;
	ldx	ASCIIFontLow		; Yes.
	beq	negaf08			;
	lda	#16			;
	sec				;
negaf04:sbc	ASCIIFontLow		;
	tay				;
	ldx	#16			;
lpgag04:lda	FontBuffer-1,y		;
	sta	FontBuffer-1,x		;
	dey				;
	dex				;
	cpx	ASCIIFontLow		;
	bne	lpgag04			;
	txa				;
	beq	negaf08			;
	lda	#0			;
lpgaf08:sta	FontBuffer-1,x		;
	dex				;
	bne	lpgaf08			;
negaf08:rts				;
;---------------------------------------;
SaveFullFontInit:			;
	lda	#64			;
	bne	SaveFontInit	;(jmp)	;
;---------------------------------------;
SaveHalfFontInit:			;
	lda	#32			;
SaveFontInit:				;
	pha				;
	clc				;
	adc	#4			;
	jsr	WaitDispBuffer		;
	ldy	DispBfTail		;
	lda	CurColumn		;
	lsr				;
	lsr				;
	sec				; #$80
	ror				;
	sta	DisplayBuffer,y		; +0
	iny				;
	lda	CurColumn		;
	asl				;
	asl				;
	asl				;
	asl				;
	asl				;
	sta	DisplayBuffer,y		; +1
	iny				;
	ldx	CurLine			;
	lda	CurrentSplit,x		;
	sta	DisplayBuffer,y		; +2
	iny				;
	pla				;
	sta	DisplayBuffer,y		; +3
	iny				;
	rts				;
;---------------------------------------;
SaveFGFontInit:				;
	ldx	$fb			;
SaveFGFontInitIN:			;
	pha				;
	clc				;
	adc	#4			;
	jsr	WaitDispBuffer		;
	ldy	DispBfTail		;
	txa				;
	lsr				;
	lsr				;
	lsr				;
	clc				;
	adc	#$04			; 1400-17e0, 1c00-1fe0
	ora	#$94			; #$80
	sta	DisplayBuffer,y		; +0
	iny				;
	txa				;
	asl				;
	asl				;
	asl				;
	asl				;
	asl				;
	sta	DisplayBuffer,y		; +1
	iny				;
	iny				; +2 (any page).
	pla				;
	sta	DisplayBuffer,y		; +3
	iny				;
	rts				;
;---------------------------------------;
SaveFont32:				;
	lda	#2			;
lpfat04:pha				;
	lda	#2			;
lpfat08:pha				;
	txa				;
	pha				;
lpfat12:lda	FontBuffer,x		; +4-
	sta	DisplayBuffer,y		;
	iny				;
	inx				;
	txa				;
	and	#7			;
	bne	lpfat12			;
	pla				;
	tax				;
	pla				;
	sec				;
	sbc	#1			;
	bne	lpfat08			;
	txa				; Cy=1.
	adc	#8-1			;
	tax				;
	pla				; Cy=0.
	sbc	#1-1			;
	bne	lpfat04			;
	rts				;
;---------------------------------------;
;	Input	Y|X= ASCII code.	;
;---------------------------------------;
SaveOneASCIIFont:			;
	jsr	GetASCIIFont		;
	jsr	SaveHalfFontInit	;
	ldx	#0			;
	jsr	SaveFont32		;
	sty	DispBfTail		;
	rts				;
;---------------------------------------;
;	Input	(A)= Count.		;
;---------------------------------------;
SaveSameFont:				;
	pha				;
	lda	#5			;
	jsr	WaitDispBuffer		;
	ldy	DispBfTail		;
	lda	CurColumn		;
	lsr				;
	sec				; #$40
	ror				;
	lsr				;
	sta	DisplayBuffer,y		; +0
	iny				;
	lda	CurColumn		;
	asl				;
	asl				;
	asl				;
	asl				;
	asl				;
	sta	DisplayBuffer,y		; +1
	iny				;
	ldx	CurLine			;
	lda	CurrentSplit,x		;
	sta	DisplayBuffer,y		; +2
	iny				;
	pla				;
	sta	DisplayBuffer,y		; +3
	iny				;
	rts				;
;---------------------------------------;
LetFBSameColorG:			;
	lda	$fe			; Check Fcolor and Bcolor is
	and	#$0c			;  same color group.
	sta	TempSave1		;
	lda	$ff			;
	and	#$0c			;
	cmp	TempSave1		;
	beq	nelsc04			;
	lda	$ff			; No. Set Bcolor to Fcolor color group.
	and	#$03			;
	ora	TempSave1		;
	sta	$ff			;
nelsc04:rts				;
;---------------------------------------;
SaveFontAt321:				;
	lda	#16			;
	jsr	SaveFontAt16		;
	lda	#24			;
	bne	SaveFontAt16	;(jmp)	;
;---------------------------------------;
SaveFontAt320:				;
	lda	#0			;
	jsr	SaveFontAt16		;
	lda	#8			;
;---------------------------------------;
SaveFontAt16:				;
	sta	TempSave1		;
	lda	$fe			;
	eor	$ff			;
	lsr				;
	pha				;
	lda	#1			;
	jsr	SaveFontAt8		;
	pla				;
	lsr				;
	lda	#2			;
	and	$fe			;
;---------------------------------------;
SaveFontAt8:				;
	bcs	nesfa08			;
	and	$fe			; Bit same.
	beq	nesfa04			;
	lda	#$ff			;
nesfa04:ldx	#8			;
lpsfa04:sta	DisplayBuffer,y		;
	iny				;
	dex				;
	bne	lpsfa04			;
	rts				;
nesfa08:ldx	TempSave1		;
	and	$fe			;
	beq	lpsfa12			;
lpsfa08:lda	FontBuffer,x		;
	sta	DisplayBuffer,y		;
	inx				;
	iny				;
	txa				;
	and	#7			;
	bne	lpsfa08			;
	rts				;
lpsfa12:lda	FontBuffer,x		;
	eor	#$ff			;
	sta	DisplayBuffer,y		;
	inx				;
	iny				;
	txa				;
	and	#7			;
	bne	lpsfa12			;
	rts				;
;---------------------------------------;
;	Input	(A)= Font.		;
;		TempSave3= Char number.	;
;---------------------------------------;
SaveSpaceFont:				;
	sta	TempSave1		; Font.
	lda	TempSave3		;
	tay				;
	asl				; *32
	asl				;
	asl				;
	asl				;
	asl				;
	sta	ContinueCount		;
	tya				;
	lsr				;
	lsr				;
	lsr				;
	sta	ContinueCount+1		;
	lda	CurColumn		;
	pha				;
lpssf04:lda	#0			;
	dec	ContinueCount+1		; Length.
	bpl	nessf04			;
	tax				;
	lda	ContinueCount		;
	stx	ContinueCount+1		;
	stx	ContinueCount		;
nessf04:jsr	SaveSameFont		;
	lda	TempSave1		; Font
	sta	DisplayBuffer,y		; +4
	iny				;
	sty	DispBfTail		;
	lda	CurColumn		;
	clc				;
	adc	#8			;
	sta	CurColumn		;
	lda	ContinueCount+1		;
	ora	ContinueCount		;
	bne	lpssf04			;
	pla				;
	sta	CurColumn		;
	rts				;
;---------------------------------------;
;	Input	(A)= Length.		;
;		(X)= Attribute address.	;
;---------------------------------------;
SendAttributeLine:			;
	pha				; Send length.
	clc				;
	adc	#4			;
	jsr	WaitDispBuffer		;
	ldy	DispBfTail		;
	lda	#<$2300+$8000		;
	sta	DisplayBuffer,y		; +0
	iny				;
	txa				;
	ora	#>$23c0			;
	sta	DisplayBuffer,y		; +1
	iny				;
	ldx	CurLine			;
	lda	CurrentSplit,x		;
	sta	DisplayBuffer,y		; +2
	iny				;
	pla				;
	sta	DisplayBuffer,y		; +3
	rts				;
;---------------------------------------;
;	Input	(Y)= Length.		;
;	Output	TempSave3= Length real.	;
;---------------------------------------;
CheckLineLength:			;
	sty	TempSave3		; Length.
	lda	#ScreenWidth		;
	sec				;
	sbc	CurColumn		;
	cmp	TempSave3		;
	bcs	necll04			;
	sta	TempSave3		;
necll04:lda	TempSave3		;
	rts				;
;---------------------------------------;
;	Input	($FE)= Color.		;
;---------------------------------------;
SendAttributeOne:			;
	lda	$fe			;
	lsr				;
	lsr				;
	tax				;
;---------------------------------------;
;	Input	(X)= Color group.	;
;---------------------------------------;
SendAttributeOneIN:			;
	lda	CurColumn		;
;---------------------------------------;
;	Input	(A)= Column		;
;---------------------------------------;
SendAttributeOneC:			;
	pha				;
	ldy	#$03			;
	and	#2			;
	beq	nesao04			;
	ldy	#$0c			;
	txa				;
	asl				;
	asl				;
	tax				;
nesao04:lda	CurLine			; Even or Odd line?
	pha				;
	lsr				;
	bcc	nesao08			;
	txa				;
	asl				;
	asl				;
	asl				;
	asl				;
	tax				;
	tya				;
	asl				;
	asl				;
	asl				;
	asl				;
	tay				;
nesao08:stx	TempSave2		; Set bits.
	sty	TempSave1		; Clear bits.
	pla				; (Line/2*16)+Column/4 = Attrib Addr.
	and	#$0e			;
	asl				;
	asl				;
	sta	TempSave4		;
	pla				; Column.
	lsr				;
	lsr				;
	clc				;
	adc	TempSave4		;
	tay				;
	lda	Attribute,y		;
	and	TempSave1		;
	cmp	TempSave2		;
	beq	nesao12			;
	lda	TempSave1		; Attribute no same.
	eor	#$ff			;
	and	Attribute,y		;
	ora	TempSave2		;
	sta	Attribute,y		;
	pha				;
	ldx	#<$2300			; Send it to VRAM.
	tya				;
	ora	#$c0			;
	tay				;
	pla				;
	jsr	SendVideo		;
nesao12:rts				;

CBiosBranchAddr	dw	KeyBoardProc-1				; $18
		dw	DisplayTTY-1,		SetCursor-1	; $1b,$1c
		dw	DisplayChars-1,		WaitDisplay-1	; $1d,$22
		dw	DispChAtTTY-1,		DispCharsAt-1	; $28,$29
		dw	ScreenScroll-1,		SetPalette-1	; $2a,$2b
		dw	DisplayChAt-1,		DispRefChar-1	; $2c,$2d
		dw	ChgKBDINTMode-1,	DispFGCharAt-1	; $2e,$2f
		dw	SetCursorType-1,	GetSetExtFont-1	; $30,$31
		dw	DispStringAt-1,		GetSystemFont-1	; $32,$33
		dw	SetScreenSW-1,		MouseOperate-1	; $34,$35
		dw	SetBGFont-1,		Return-1	; $36,$37
		dw	SetFGFont-1,		Return-1	; $38,$39
		dw	DispHalfChAt-1,		MoveFGChar-1	; $3a,$3b
		dw	DispOpenClose-1,	ChgBGColor-1	; $3d,3e
;---------------------------------------;
CBiosBranch:				;
	lda	CBiosBranchAddr+1,x	;
	pha				;
	lda	CBiosBranchAddr,x	;
	pha				;
	ldx	TempSave6		; Restore X.
	rts				;
;=======================================;
CBiosEntry:				;
	stx	TempSave6		; Save X.
	tax				;
	pha				; Save caller's pageN.
	lda	OriginalPage		;
	pha				; Save caller's page.
	cpx	BiosUsePage		;
	bne	necbe04			;
	ldx	DRAMPageNotUse		;
necbe04:cpx	#0			;
	bne	necbe08			;
	ldx	BiosUsePage		;
necbe08:stx	OriginalPage		;
LowModify7:				;
	lda	LowSaveA		;
	cmp	#BiosGetSystemFont	; $33
	beq	necbe16			;
	cmp	#BiosSetScreenSW	; $34
	beq	necbe16			;
	cmp	#BiosWaitDisplay	; $22
	beq	necbe16			;
	jsr	SetScreenSW0		;
necbe16:sec				;
	sbc	#BiosDevice		; $18?(0)
	beq	necbe20			;
	sbc	#2			;
	cmp	#3+1			; $1b-$1d?(1-3)
	bcc	necbe20			;
	sbc	#4			;
	cmp	#4			; $22?(4)
	beq	necbe20			;
	sbc	#5			;
	cmp	#$18+1			; $28-$3b(5-18).
	bcc	necbe20			;
	sbc	#1			; $3d-
necbe20:asl				;
	tax				;
	jsr	CBiosBranch		;
LowModify8:				;
	sta	LowSaveA		;
	lda	OpenCloseScreen		;
	beq	necbe24			;
	jsr	FlashAllDspBf		;
necbe24:pla				; Save caller's page.
	sta	OriginalPage		;
	pla				; Save caller's pageN.
	rts				;

AltF1FuncStg	db	"区位/国标:",0	;
AltF1FuncStgL	equ	@-AltF1FuncStg-1;
AltF9FuncStg	db	"符号",0
AltF9FuncStgL	equ	@-AltF9FuncStg-1;
AltF10FuncStg	db	"半角 英文/数字",0
AltF10FuncStgL	equ	@-AltF10FuncStg-1
CtrlF9FuncStg	db	"半角",0
AltF2FuncStg	db	0,0,0,0,0,0,0,0,0,0
AltF3FuncStg	db	0,0,0,0,0,0,0,0,0,0
AltF4FuncStg	db	0,0,0,0,0,0,0,0,0,0
AltF5FuncStg	db	0,0,0,0,0,0,0,0,0,0
AltF6FuncStg	db	0,0,0,0,0,0,0,0,0,0
AltF7FuncStg	db	0,0,0,0,0,0,0,0,0,0
AltF8FuncStg	db	0,0,0,0,0,0,0,0,0,0
HZFunctionStgP	dw	AltF1FuncStg,	AltF2FuncStg,	AltF3FuncStg,	AltF4FuncStg
		dw	AltF5FuncStg,	AltF6FuncStg,	AltF7FuncStg,	AltF8FuncStg
		dw	AltF9FuncStg,	AltF10FuncStg
HZFunctionStgL	db	AltF1FuncStgL,	0,		0,		0
		db	0,		0,		0,		0
		db	AltF1FuncStgL,	AltF10FuncStgL

;---------------------------------------;
ChangeAToHex:				;
	sec				;
	sbc	#'a'-10			;
	bcs	necah04			;
	adc	#'a'-10-'0'		;
necah04:rts				;
;---------------------------------------;
ChangeAYToHex:				;
	jsr	ChangeAToHex		;
	asl				;
	asl				;
	asl				;
	asl				;
	sta	TempSave1		;
	tya				;
	jsr	ChangeAToHex		;
	ora	TempSave1		;
	rts				;
;---------------------------------------;
ChangeAYToDec:				;
	sec				;
	sbc	#'0'			; (A)*10.
	sta	$ff			;
	asl				;
	asl				;
	adc	$ff			;
	asl				;
	sta	$ff			;
	tya				; Cy=1.
	sbc	#'0'-1			;
	clc				;
	adc	$ff			;
	rts				;
;---------------------------------------;
CheckHexNumber:				;
	cmp	#'a'			;
	bcc	necnr04			;
	cmp	#'f'+1			;
	rts				;
CheckNumber:				;
necnr04:cmp	#'0'			;
	bcc	necnr08			;
	cmp	#'9'+1			;
	rts				;
necnr08:sec				;
	rts				;

;---------------------------------------;
;	符号 1区 01XX02XX03XX04XX05XX
FuHaoSignStg	db	"符号 ?区 "	;
FuHaoSignStgL	equ	@-FuHaoSignStg	;
SymbolQu	ds	1		; Symbol's Qu
SymbolBase	ds	1		; Symbol select base.
SymbolBaseASC	ds	2		;
;---------------------------------------;
FuHaoList:				;
	ldx	#FuHaoSignStgL-1	;
lpfhl04:lda	FuHaoSignStg,x		;
	sta	OutputBuffer,x		;
	dex				;
	bpl	lpfhl04			;
	lda	SymbolQu		;
	sta	OutputBuffer+5		;
	lda	SymbolBase		;
	sta	$ff			;
	lda	SymbolBaseASC		;
	ldy	SymbolBaseASC+1		;
	ldx	#9			;
lpfhl08:sta	OutputBuffer+0,x	;
	pha				;
	tya				;
	sta	OutputBuffer+1,x	;
	lda	#2			;
	sta	OutputBuffer+2,x	;
	lda	SymbolQu		;
	clc				;
	adc	#$a1-'1'		;
	sta	OutputBuffer+3,x	;
	lda	$ff			;
	sta	OutputBuffer+4,x	;
	inc	$ff			;
	pla				;
	cpy	#'5'			; Is X5
	beq	nefhl20			;
	cpy	#'0'			; or is X1
	beq	nefhl20			;
	cmp	#'9'			; or is 94
	bne	nefhl12			;
	cpy	#'4'			;
	beq	nefhl16			;
nefhl12:iny				;
	inx				;
	inx				;
	inx				;
	inx				;
	inx				;
	cpy	#'9'+1			; Is X9
	bne	lpfhl08			;
	ldy	#'0'			; Yes.
	clc				;
	adc	#1			;
	bcc	lpfhl08		;(jmp)	;
nefhl16:ldx	#4			;
	lda	#FuHaoSignStgL+20	;
	bne	nefhl24		;(jmp)	;
nefhl20:ldx	#5			;
	lda	#FuHaoSignStgL+25	;
nefhl24:stx	OutputNumber		;
	sta	OutputBufferLen		;
	lda	#$14			; List/ClearReflineToEnd
	sta	ReturnCode		;
	jmp	HZFuncReturn		;
;---------------------------------------;
FuHaoProc:				;
	lda	KeyBoardCode		;
	cmp	#'1'			;
	bcc	nefhp12			;
	cmp	#'5'			;
	bne	nefhp04			;
	ldy	SymbolBaseASC		;
	cpy	#<'94'			;
	bne	nefhp08			;
	ldy	SymbolBaseASC+1		;
	cpy	#>'91'			;
	bcs	nefhp28			;
nefhp04:bcs	nefhp12			;
nefhp08:sec				; Select OK.
	sbc	#'1'			;
	sta	OutputIndex		;
	lda	#$20			; OutputOne
	sta	ReturnCode		;
	jmp	HZFuncReturn		;
nefhp12:lda	KeyBoardCode+1		;
	cmp	#$33			; Page up?
	bne	nefhp16			;
	lda	SymbolBase		;
	cmp	#$a1			;
	beq	nefhp24			;
	sbc	#5			; Cy=1.
	sta	SymbolBase		;
	lda	SymbolBaseASC+1		;
	sec				;
	sbc	#5			;
	cmp	#'1'			;
	bcs	nefhp20			;
	adc	#10			;
	dec	SymbolBaseASC		;
	bne	nefhp20		;(jmp)	;
nefhp16:cmp	#$34			; Page down?
	bne	nefhp28			;
	lda	SymbolBase		;
	cmp	#$fe-4			;
	bcs	nefhp24			;
	adc	#5			; Cy=0.
	sta	SymbolBase		;
	lda	SymbolBaseASC+1		;
	adc	#5			;
	cmp	#'9'+1			;
	bcc	nefhp20			;
	sbc	#10			;
	inc	SymbolBaseASC		;
nefhp20:sta	SymbolBaseASC+1		;
	jmp	FuHaoList		;
nefhp24:lda	#$80			;
	bne	neasp04		;(jmp)	;
nefhp28:lda	HZFunctionSave		; Restore original HZFunction.
	sta	HZFunction		;
;	jmp	ASCIIProc		;

;---------------------------------------;
ASCIIProc:				;
	lda	#$40			; ASCII output.
neasp04:sta	ReturnCode		;
	jmp	HZFuncReturn		;
;---------------------------------------;
;	Output	Cy==1: Original ASCII.	;
;		Cy==0: X==0: No output.	;
;		       X!=0: X|Y= HANZI.;
;---------------------------------------;
QW_GBProc:				;
	jsr	QW_GBProcIN		;
	sta	ReturnCode		;
	jmp	HZFuncReturn		;
;---------------------------------------;
QW_GBProcIN:				;
	ldx	StringCount		; Is eat code?
	lda	StringBuffer,x		;
	cmp	#$fe			;
	bcs	neqgp12			;
	jsr	CheckHexNumber		; Is QW_GB character?
	bcc	neqgk04			;
	lda	#$40			; No. Invalid input
	rts				;
neqgk04:ldx	StringCount		; StringCount==4?
	cpx	#4-1			;
	bcs	neqgp16			;
neqgp08:lda	#$02			; No. Display&PutInBuffer.
	rts				;
neqgp12:lda	#0			;
	rts				;
neqgp16:lda	StringBuffer		;
	cmp	#'a'			;
	bcc	neqgp24			;
	ldy	StringBuffer+1		; GuoBiao access.
	jsr	ChangeAYToHex		;
	cmp	#$a1			;
	bcc	neqgp20			;
	cmp	#$fe+1			;
	bcs	neqgp20			;
	tax				;
	lda	StringBuffer+2		;
	ldy	StringBuffer+3		;
	jsr	ChangeAYToHex		;
	cmp	#$a1
	bcs	neqgp28			;
neqgp20:lda	#$8a			; Display/ClearStringCount/ErrorEncode
	rts				;
neqgp24:lda	StringBuffer		; QuWei access.
	ldy	StringBuffer+1		;
	jsr	ChangeAYToDec		;
	beq	neqgp20			;
	clc				;
	adc	#$a0			;
	bcs	neqgp20			;
	cmp	#$fe+1			;
	bcs	neqgp20			;
	tax				;
	lda	StringBuffer+2		;
	ldy	StringBuffer+3		;
	jsr	ChangeAYToDec		;
	beq	neqgp20			;
	clc				;
	adc	#$a0			;
	bcs	neqgp20			;
neqgp28:cmp	#$fe+1			;
	bcs	neqgp20			;
	ldy	#'['			;
	sty	OutputBuffer+0		;
	ldy	#2			;
	sty	OutputBuffer+1		;
	stx	OutputBuffer+2		;
	sta	OutputBuffer+3		;
	lda	#']'			;
	sta	OutputBuffer+4		;
	lda	#5			;
	sta	OutputBufferLen		;
	lda	#0			;
	sta	OutputIndex		;
	lda	#$3e			; Display/ListOutput/ClearStringCount/
	rts				; OutputOne.

HZFuncProcAddr	dw	QW_GBProc-1,	0,		0,		0
		dw	0,		0,		0,		0
		dw	FuHaoProc-1,	ASCIIProc-1
HZFuncProcPage	db	0,		0,		0,		0
		db	0,		0,		0,		0
		db	0,		0
ListBeginColumn	db	16,		0,		0,		0
		db	0,		0,		0,		0
		db	0
ListNumberAPage	db	0,		0,		0,		0
		db	0,		0,		0,		0
		db	0
		db	0
;---------------------------------------;
;   Check if HZ function key and enable.;
;	Output	Cy==1: No.		;
;		Cy==0: (A)= HZ Function.;
;---------------------------------------;
Ctrl_F9		equ	$6600		;
Alt_F1		equ	$6800		;
Alt_F2		equ	$6900		;
Alt_F8		equ	$6F00		;
Alt_F9		equ	$7000		;
Alt_F10		equ	$7100		;
Alt_1		equ	$7800		;
Alt_0		equ	$8100		;
;---------------------------------------;
CheckCtrlF9:				;
	cpx	#<Ctrl_F9		; No. Is Ctrl-F9?
	bne	necc912			;
	ldx	#<'全'			;
	ldy	#>'全'			;
	lda	Full_Half		;
	eor	#$80			;
	sta	Full_Half		;
	bne	necc904			;
	ldx	#<'半'			;
	ldy	#>'半'			;
necc904:stx	AltF10FuncStg		;
	sty	AltF10FuncStg+1		;
	stx	CtrlF9FuncStg		;
	sty	CtrlF9FuncStg+1		;
	lda	HZFunction		; Is ASCII input mode?
	cmp	#9			;
	beq	necc908			;
	lda	CurrentPage		;
	sta	OriginalPage		;
	ldx	#5			; Display refrence string.
	lda	#<CtrlF9FuncStg		;
	ldy	#>CtrlF9FuncStg		;
	jsr	DispRefCharsIN
	clc				;
	rts				;
necc908:ldx	#<Alt_F10		;
	lda	#>Alt_F10		;
necc912:sec				;
	rts				;
;---------------------------------------;
CheckAltF9:				;
	cpx	#<Alt_F9		; Is Alt/F9 key?
	bne	neca916			;
	ldx	HZFunction		; Yes.
	cpx	#9-1			; Last is Symbol input mode?
	beq	neca904			;
	stx	HZFunctionSave		; No.
	bne	neca908		;(jmp)	; 1 Qu first.
neca904:ldx	SymbolQu		; Yes. Next Qu.
	inx				;
	cpx	#'9'+1			;
	bcc	neca912			;
neca908:ldx	#'1'			;
neca912:stx	SymbolQu		;
	lda	#9-1			;
	sta	HZFunction		;
	lda	#$a1			;
	sta	SymbolBase		;
	lda	#<'01'			;
	sta	SymbolBaseASC		;
	lda	#>'01'			;
	sta	SymbolBaseASC+1		;
	lda	$fd			;
	pha				;
	lda	$fe			;
	pha				;
	jsr	FuHaoList		;
	pla				;
	sta	$fe			;
	pla				;
	sta	$fd			;
	clc				;
	rts				;
neca916:sec				;
	rts				;
;---------------------------------------;
CheckHZFuncKey:				;
	cpx	#<Alt_F1		;
	bcc	nechf12			;
	cpx	#<Alt_F10+$100		; Is Alt/F1-AltF10?
	bcc	nechf04			; Yes.
	lda	KeyStatus1		; No. Shift pressing?
	and	#3			;
	beq	nechf12			; Is Alt/Shift/1-0?
	cpx	#<Alt_1			;
	bcc	nechf12			;
	cpx	#<Alt_0+$100		;
	bcs	nechf12			;
	lda	#<Alt_1			;
	bne	nechf08		;(jmp)	;
nechf04:lda	#<Alt_F1		;
nechf08:sta	$ff			;
	txa				;
	sec				;
	sbc	$ff			;
	tax				;
	ldy	HZFuncProcPage,x	; HZ function key has define?
	beq	nechf12			; No.
	clc				;
	rts				;
nechf12:sec				;
	rts				;
;---------------------------------------;
;   HZ input method install.		;
;	Input	($FF|FE)= Parameter Pot.;
;	Output	(A)= Data.		;
;---------------------------------------;
GetOneInputData:			;
	lda	$ff			;
	pha				;
	lda	#BiosGetPagedChar	;
	ldx	OriginalPage		;
	jsr	ROMBiosEntry		;
	tax				;
	pla				;
	sta	$ff			;
	txa				;
	iny				;
	rts				;
;---------------------------------------;
CDOSKeyBoard:				;
	ldy	#1			; Get active key.
	jsr	GetOneInputData		;
	cmp	#<Alt_F8+$100		; Is Alt/F2-Alt/F8?
	bcs	neckb08			;
	cmp	#<Alt_F2		;
	bcc	neckb08			;
	sbc	#<Alt_F1		; Yes.
	sta	TempSave1		;
	tax				;
	asl				;
	sta	TempSave2		;
	lda	OriginalPage		; Save entry page.
	sta	HZFuncProcPage,x	;
	jsr	GetOneInputData		; Get entry point.
	ldx	TempSave2		;
	sta	HZFuncProcAddr,x	;
	jsr	GetOneInputData		;
	ldx	TempSave2		;
	sta	HZFuncProcAddr+1,x	;
	lda	HZFunctionStgP,x	;
	sta	mockb04+1		;
	lda	HZFunctionStgP+1,x	;
	sta	mockb04+2		;
	lda	#0			;
lpckb04:pha				;
	jsr	GetOneInputData		; Get HZ function string.
	tax				;
	beq	neckb04			;
	pla				;
	cmp	#9			;
	bcs	lpckb04			;
	sta	TempSave3		;
	txa				;
	ldx	TempSave3		;
mockb04:sta	$ffff,x			;
	inx				;
	txa				;
	bne	lpckb04		;(jmp)	;
neckb04:pla				;
	ldx	TempSave1		;
	sta	HZFunctionStgL,x	;
	jsr	GetOneInputData		; Get list begin column.
	ldx	TempSave1		;
	sta	ListBeginColumn,x	;
	jsr	GetOneInputData		; Get list begin column.
	ldx	TempSave1		;
	sta	ListNumberAPage,x	;
	lda	#0			;
neckb08:rts				;
;---------------------------------------;
OriginalKeyBoard:			;
	lda	#BiosDevice		;
	jmp	(OrgBiosEntryIn)	;
;=======================================;
KeyBoardFunc	db	0		;
KeyBoardProc:				;
	cpx	#$20			;
	beq	CDOSKeyBoard		;
	cpx	#1+1			; Function 0 or 1?
	bcs	OriginalKeyBoard	;
	stx	KeyBoardFunc		; Yes. Save keyboard function.
	lda	$ff			; Save $FF.
	pha				;
lpkbp04:ldx	KeyBoardFunc		;
	beq	nekbp08			;
	jsr	DeviceEntry		; Test keyboard.
	cpy	#0			;
	beq	nekbp04			;
	ldx	#1			;
	jsr	OriginalKeyBoard	;
	cpy	#0			;
	bne	nekbp12			;
nekbp04:pla				; Restore $FF.
	sta	$ff			;
	rts				;
nekbp08:jsr	OriginalKeyBoard	;
nekbp12:sta	KeyBoardCode		;
	stx	KeyBoardCode+1		;
	cpx	#$e2			; Is HZ code?
	bcc	nekbp16			;
	cpx	#$e3+1			;
	tay				; Yes.
	jmp	nekbp76			;
nekbp16:cmp	#0			;
	bne	nekbp20			;
	jsr	CheckCtrlF9		;
	bcc	nehfk16			;
	jsr	CheckAltF9		;
	bcc	nehfk16			;
	jsr	CheckHZFuncKey		; Yes. Is HZ function key?
	bcs	nekbp20			;
	sta	HZFunction		; Yes.
	ldx	#0			;
	jsr	DispRefChar		;
nehfk16:ldx	#0			;
	stx	StringCount		;
	stx	OutputNumber		;
	ldx	KeyBoardFunc		;
	beq	lpkbp04			;
	dex				;
	jsr	OriginalKeyBoard	;
	jmp	lpkbp04			;
;---------------------------------------;
;   Spacial code.			;
;    Before:				;
;	Enter: Delete encode string	;
;	Backspace: Delete last character;
;	[,<]/[.>]: Page Up/Page Down	;
;    After:				;
;	Spacebar: Select first list.	;
;	1-0: Select 1-10 list.		;
;	Alt/1-Alt/0: Select 1-10 list.	;
;---------------------------------------;
nekbp20:ldx	StringCount		;
	beq	nekbp44			;
	lda	KeyBoardCode		;
	cmp	#Enter			; Is Enter code?
	beq	nekbp40			;
;---------------------------------------;
	ldy	KeyBoardCode+1		;
	cpy	#$34			; Page down?
	bne	nekbp24			;
	lda	NextPageHas		;
	beq	nekbp44			;
	lda	OutputNumber		;
	beq	nekbp44			;
	clc				;
	adc	PageBase		;
	sta	PageBase		;
	bcc	nekbp32			;
	inc	PageBase+1		;
	bne	nekbp32		;(jmp)	;
;---------------------------------------;
nekbp24:cpy	#$33			; Page up?
	bne	nekbp36			;
	lda	PageBase		; Yes.
	tax				;
	ora	PageBase+1		;
	beq	nekbp44			;
	txa				;
	sec				;
	ldx	HZFunction		;
	sbc	ListNumberAPage,x	;
	bcs	nekbp28			;
	dec	PageBase+1		;
	bpl	nekbp28			;
	lda	#0			;
	sta	PageBase+1		;
nekbp28:sta	PageBase		;
nekbp32:lda	#$ff			;
	bne	nekbp52		;(jmp)	;
;---------------------------------------;
nekbp36:cmp	#Backspace		; Is Backspace code?
	bne	nekbp44			;
	dex				;
	bne	nekbp48			;
nekbp40:ldx	#0			;
	stx	StringCount		;
	stx	OutputNumber		;
	ldx	HZFunction		;
	lda	HZFunctionStgL,x	;
	jsr	ClearRefGive		;
	ldx	#0			;
	beq	nekbp56		;(jmp)	;
nekbp44:lda	KeyBoardCode		; New character to StringBuffer.
	jmp	nekbp52		;(jmp)	;
nekbp48:stx	StringCount		;
	txa				; Cover character displayed.
	ldx	HZFunction		;
	clc				;
	adc	HZFunctionStgL,x	;
	sta	RefColumn		;
	lda	#' '			;
	jsr	DispRefASCIITTY		;
	lda	#$fe			;
nekbp52:ldx	StringCount		;
	sta	StringBuffer,x		;
	lda	$fe			; Save $FD|FE.
	pha				;
	lda	$fd			;
	pha				;
	jsr	HZFuncBranch		; Output in X/Y.
	pla				; Restore $FD|$FE
	sta	$fd			;
	pla				;
	sta	$fe			;
	bcs	nekbp76			; Keycode used out?
nekbp56:txa				; Yes. Read out this keycode.
	pha				;
	ldx	KeyBoardFunc		;
	beq	nekbp60			;
	tya				; Save X/Y
	pha				;
	dex				; X=0.
	jsr	OriginalKeyBoard	;
	pla				;
	tay				;
nekbp60:pla				; Has HANZI output?
	bmi	nekbp68			;
	beq	nekbp64			; String output?
	tax				; Yes.
	dex				;
	stx	TempSave1		;
	lda	#$e3			; Next half.
	sta	$ff			;
	tya				; Position in outputbuffer.
	clc				;
	adc	TempSave1		; Save reversed.
lpkbp08:tax				; Saveback string to keyboard buffer.
	ldy	OutputBuffer,x		;
	dex				;
	txa				; Save X.
	pha				;
	ldx	#5			; Saveback.
	jsr	OriginalKeyBoard	;
	lda	$ff			;
	eor	#1			;
	sta	$ff			;
	pla				;
	dec	TempSave1		;
	bne	lpkbp08			;
	tax				;
	ldy	OutputBuffer,x		;
	jmp	nekbp72			;
nekbp64:jmp	lpkbp04			;
nekbp68:pha				; HANZI output. Save X.
	lda	#$e3			; Save last half HANZI back.
	sta	$ff			; $ff/Y= keycode.
	ldx	#5			;
	jsr	OriginalKeyBoard	;
	pla				;
	tay				;
nekbp72:ldx	#$e2			;
	lsr	KeyBoardFunc		; Is read keyboard function?
	bcc	nekbp76			;
	pha				;
	stx	$ff			; No. Save first half HANZI back also.
	ldx	#5			;
	jsr	OriginalKeyBoard	;
	ldx	$ff			;
	pla				;
	tay				;
nekbp76:pla				; Restore $FF.
	sta	$ff			;
	tya				; Output X|A.
	ldy	#1			;
	rts				;

;---------------------------------------;
;	Return	(ReturnCode)		;
;	 Bit 0: Clear StringCount and OutputNumber
;	 Bit 1:	Display & Put to buffer	;
;	 Bit 2: List outputs.		;
;	 Bit 3: Clear StringCount	;
;	 Bit 4: Clear refline to end.	;
;	 Bit 5: Output one.		;
;	 Bit 6: Invalid input		;
;	 Bit 7: Error encode.		;
;---------------------------------------;
;	Use	$fd-$ff			;
;---------------------------------------;
HZFuncReturn:				;
	ldx	HZFunction		;
	lda	ListBeginColumn,x	;
	sta	RefColumn		;
	lda	ReturnCode		;
;---------------------------------------; Clear StringCount and OutputNumber.
	lsr				;
	bcc	nehfr04			;
	ldx	#0			;
	stx	StringCount		;
	stx	OutputNumber		;
;---------------------------------------; Display & Put to buffer.
nehfr04:lsr				;
	bcc	nehfr16			;
	ldx	StringCount		; First character?
	bne	nedcr04			;
	ora	#$04			; Clear refline to end
	pha				;
	ldx	#4			; Yes. Display HZ function string.
	jsr	DispRefChar		;
	lda	KeyBoardCode		;
	jsr	DispRefASCIITTY		;
	ldx	HZFunction		;
	lda	ListBeginColumn,x	;
	jsr	ClearRefLineLen		;
	jmp	nedcr08			;
nedcr04:pha				;
	txa				;
	clc				;
	ldx	HZFunction		;
	adc	HZFunctionStgL,x	;
	sta	RefColumn		;
	lda	KeyBoardCode		;
	jsr	DispRefASCIITTY		;
nedcr08:lda	KeyBoardCode		;
	ldx	StringCount		;
	sta	StringBuffer,x		;
	inx				;
	stx	StringCount		;
	pla				;
;---------------------------------------;
nehfr16:lsr				; List output HANZIs.
	bcc	nehfr20			;
	pha				;
	ldx	HZFunction		;
	lda	ListBeginColumn,x	;
	sta	RefColumn		;
	lda	OutputBufferLen		;
	beq	neloh16			;
	lda	#3			; ASCII lower display.
	sta	ASCIIFontLow		;
	ldx	#0			;
lploh04:lda	OutputBuffer,x		; HANZI?
	bmi	neloh04			;
	ldy	#0			;
	cmp	#' '			;
	bcs	neloh08			;
	bcc	neloh12		;(jmp)	;
neloh04:dec	OutputBufferLen		;
	tay				; Yes.
	inx				;
	lda	OutputBuffer,x		;
neloh08:sta	$ff			;
	stx	$fd			;
	ldx	#3			; Y|$ff= Character.
	jsr	DispRefChar		;
	ldx	$fd			;
neloh12:inx				;
	dec	OutputBufferLen		;
	bne	lploh04			;
	lda	#0			;
	sta	ASCIIFontLow		;
neloh16:pla				;
	ora	#2			;
;---------------------------------------; Clear StringCount
nehfr20:lsr				;
	bcc	nehfr24			;
	ldx	#0			;
	stx	StringCount		;
;---------------------------------------;
nehfr24:lsr				; Clear to line end.
	bcc	nehfr32			;
	pha				;
	jsr	ClearRefLineIN		;
	pla				;
;---------------------------------------; Output one.
nehfr32:lsr				;
	bcc	nehfr36			;
	lda	OutputIndex		; Output select.
	jmp	OutputOne		;
;---------------------------------------;
nehfr36:lsr				; Invalid input.
	bcc	nehfr40			;
	ldx	OutputNumber		;
	beq	neidt04			;
	lda	KeyBoardCode+1		;
	cmp	#<Alt_1			; Is Alt/1-Alt/0?
	bcc	neidt04			;
	cmp	#<Alt_0+$100		;
	bcs	neidt04			;
	sec				; Yes.
	sbc	#<Alt_1			;
	cmp	OutputNumber		;
	bcc	OutputOne		;
neidt04:ldx	StringCount		;
	bne	neidt20			;
	bit	Full_Half		; Full ASCII?
	bpl	neidt16			;
	lda	KeyBoardCode		; Yes.
	cmp	#' '			; Is ASCII?
	bcc	neidt16			;
	bne	neidt08			;
	ldx	#$a1			; HANZI ' '
	txa				;
	bne	neidt12		;(jmp)	;
neidt08:cmp	#$7f			;
	bcs	neidt16			;
	ldx	#$a3			;
	ora	#$80			;
neidt12:tay				;
	clc				;
	rts				;
neidt16:ldx	KeyBoardCode+1		;
	ldy	KeyBoardCode		;
	sec				;
	rts				;
neidt20:lda	KeyBoardCode		;
	cmp	#'9'+1			;
	bcs	nehfr44			;
	cmp	#' '			;
	bne	neidt24			;
	lda	#'1'			;
neidt24:cmp	#'0'			;
	bcc	nehfr44			;
	bne	neidt28			;
	lda	#'9'+1			;
neidt28:sec				;
	sbc	#'1'			;
	cmp	OutputNumber		;
	bcs	nehfr44			;
	ldx	#0			;
	stx	StringCount		;
	jmp	OutputOne		;
;---------------------------------------; Error encode.
nehfr40:lsr				;
	bcc	nehfr48			;
nehfr44:jsr	SoundBell		;
nehfr48:ldx	#0			; No output.
	clc				;
	rts				;
;---------------------------------------;
;	Input	(A)= Output index.	;
;---------------------------------------;
OutputOne:				;
	tax				;
	ldy	#0			;
lpooe04:lda	OutputBuffer,y		;
	iny				;
	cmp	#' '			;
	bcs	lpooe04			;
	dex				; This select?
	bpl	lpooe04			;
	tax				;
	cmp	#2			; Yes. Output one HANZI?
	bne	neooe04			;
	ldx	OutputBuffer,y		; Yes.
	lda	OutputBuffer+1,y	;
	tay				;
neooe04:clc				;
	rts				;

;---------------------------------------;
HZFuncBranch:				;
	lda	CurrentPage		; Parameter pointer send to HZ proc.
	ldx	#<HZFuncParameter	;
	ldy	#>HZFuncParameter	;
	sta	$fd			;
	stx	$ff			;
	sty	$fe			;
	ldy	HZFunction		;
	ldx	HZFuncProcPage,y	; HZ function page.
	tya				;
	asl				;
	tay				;
	lda	HZFuncProcAddr+1,y	; Make HZ function address in stack.
	pha				;
	lda	HZFuncProcAddr,y	;
	pha				;
	lda	#BiosSetPage		; Jump to HZ function.
	jmp	ROMBiosEntry		;

;---------------------------------------;
SoundBell:				;
	lda	#BiosDisplayTTY		;
	ldx	#7			;
	jmp	(OrgBiosEntryIn)	;
;=======================================;
DisplayTTY:				;
	tya				;
	bmi	DisplayTTYHanzi		; Is full HZ
	bne	nedty28			; Is half AS
	txa				;
	cmp	#$07			;
	beq	SoundBell		;
	cmp	#$08			;
	bne	nedty12			; Backspace.
nedty06:ldy	CurColumn		;
	bne	nedty08			;
	ldx	CurLine			; In line begin.
	beq	nedty20			;
	dec	CurLine			; Not in screen begin.
	ldy	#ScreenWidth		;
nedty08:dey				; Column minus.
	jmp	nedty16			;	
nedty12:cmp	#$0d			;
	bne	nedty24			;
	ldy	#0			; Return.
nedty16:sty	CurColumn		;
nedty20:jmp	DisplayCursor		;
nedty24:cmp	#$0a			;
	beq	CurLinePlus1		; LineFeed.
;---------------------------------------;
	cmp	#$20			;
	beq	nedty32			;
nedty28:jsr	SaveOneASCIIFont	;
	jmp	CurColumnPlus1		;
;---------------------------------------;
nedty32:lda	#32			; Length.
	jsr	SaveSameFont		;
	lda	#0			; Font.
	sta	DisplayBuffer,y		; +4
	iny				;
	sty	DispBfTail		;
	jmp	CurColumnPlus1		;
;---------------------------------------;
DisplayTTYHanzi:			;
	jsr	ReadHanzi		;
	ldy	CurColumn		; Last column?
	cpy	#ScreenWidth-1		;
	bcs	nedth04			;
	jsr	SaveFullFontInit	; No.
	ldx	#0			;
	jsr	SaveFont32		;
	ldx	#16			;
	jsr	SaveFont32		;
	sty	DispBfTail		;
	inc	CurColumn		;
	bne	CurColumnPlus1	;(jmp)	;
nedth04:jsr	SaveHalfFontInit	;
	ldx	#0			;
	jsr	SaveFont32		;
	sty	DispBfTail		;
	jsr	CurColumnPlus1		;
	jsr	SaveHalfFontInit	;
	ldx	#16			;
	jsr	SaveFont32		;
	sty	DispBfTail		;
;---------------------------------------;
CurColumnPlus1:				;
	ldy	CurColumn		; Cursor column +1.
	iny				;
CheckCurColumn:				;
	cpy	#ScreenWidth		; Cursor column reach to line end?
	bcc	nedty16			;
	ldy	#0			; Yes.
	sty	CurColumn		;
;---------------------------------------;
CurLinePlus1:				;
	ldx	CurLine			;
	cpx	#ChineseLine-1		; Cursor line reach to window end?
	bne	neclp04			;
	lda	#0			; Scroll up.
	sta	ScrollBuffer		;
	sta	ScrollBuffer+1		; Left-Up corner
	sta	ScrollBuffer+2		;
	lda	#ScreenWidth-1		; Right-Down corner
	sta	ScrollBuffer+3		;
	lda	#ChineseLine-1		;
	sta	ScrollBuffer+4		;
	lda	#1			; Lines.
	sta	ScrollBuffer+5		;
	lda	#0			; Attribute.
	sta	ScrollBuffer+6		;
	lda	CurrentPage		;
	sta	OriginalPage		;
	ldx	#<ScrollBuffer		;
	ldy	#>ScrollBuffer		;
	jsr	ScreenScroll		;
	jmp	DisplayCursor		;
neclp04:inc	CurLine			; Cursor line +1.
	jmp	DisplayCursor		;

;=======================================;
;	Input	(X)= Cursor line.	;
;		(Y)= Cursor column.	;
;---------------------------------------;
SetCursor:				;
	cpx	#ChineseLine+1		;
	bcc	nescr04			;
	ldx	#ChineseLine		;
nescr04:stx	CurLine			;
	cpy	#ScreenWidth		;
	bcc	nescr08			;
;	ldy	#ScreenWidth-1		;
nescr08:sty	CurColumn		;
	jmp	DisplayCursor		;

;=======================================;
DisplayChars:				;
	jsr	CheckLineLength		;
	beq	nedcs08			; Length==0?
	cpx	#$20			; No.
	bne	nedcs04			;
	lda	#0			;
	jmp	SaveSpaceFont		;
;---------------------------------------;
nedcs04:lda	CurColumn		;
	pha				;
	ldy	TempSave3		;
lpdcs04:tya				;
	pha				;
	txa				;
	pha				;
	ldy	#0			;
	jsr	SaveOneASCIIFont	;
	inc	CurColumn		;
	pla				;
	tax				;
	pla				;
	tay				;
	dey				;
	bne	lpdcs04			;
	pla				;
	sta	CurColumn		;
nedcs08:rts				;

;=======================================;
;    Wait display buffer empty.		;
;	Save	X.			;
;---------------------------------------;
WaitDisplay:				;
	lda	DispBfHead		;
	cmp	DispBfTail		;
	bne	WaitDisplay		;
	rts				;
;=======================================;
DispChAtTTY:				;
	tya				;
	bmi	DispChAtTTYHanzi	;
	bne	nedct20			;
	cpx	#$07			;
	bne	nedct08			;
	jmp	SoundBell		;
nedct08:cpx	#$08			;
	bne	nedct12			; Backspace.
	jmp	nedty06			;
nedct12:cpx	#$0d			;
	bne	nedct16			;
	ldy	#0			; Return.
	sty	CurColumn		;
	jmp	DisplayCursor		;
nedct16:cpx	#$0a			;
	bne	nedct20			;
	jmp	CurLinePlus1		; LineFeed.
;---------------------------------------;
nedct20:jsr	LetFBSameColorG		;
	tya				;
	bne	nedct28			;
	cpx	#$20			;
	bne	nedct28			;
	lda	$ff			; B color.
	and	#3			; Low two bits is 00 or 11?
	beq	nedct24			;
	cmp	#3			;
	bne	nedct28			;
	lda	#$ff			; Font.
nedct24:pha				;
	lda	#32			; Length.
	jsr	SaveSameFont		;
	pla				;
	sta	DisplayBuffer,y		; +4
	iny				;
	jmp	nedct32			;
;---------------------------------------;
nedct28:jsr	GetASCIIFont		;
	jsr	SaveHalfFontInit	;
	jsr	SaveFontAt320		;
nedct32:sty	DispBfTail		;
	jmp	nedch04			;
;---------------------------------------;
DispChAtTTYHanzi:			;
	jsr	ReadHanzi		;
	ldy	CurColumn		; Last column?
	cpy	#ScreenWidth-1		;
	bcs	nedch08			;
	jsr	SaveFullFontInit	; No.
	jsr	SaveFontAt320		;
	jsr	SaveFontAt321		;
	sty	DispBfTail		;
	jsr	SendAttributeOne	;
	inc	CurColumn		; Cursor column +1.
nedch04:jsr	SendAttributeOne	;
	jmp	CurColumnPlus1		;
nedch08:jsr	SaveHalfFontInit	;
	jsr	SaveFontAt320		;
	sty	DispBfTail		;
	jsr	SendAttributeOne	;
	jsr	CurColumnPlus1		;
	jsr	SaveHalfFontInit	;
	jsr	SaveFontAt321		;
	sty	DispBfTail		;
	jsr	SendAttributeOne	;
	jmp	CurColumnPlus1		;
;=======================================;
;	Input	(X)= ASCII.		;
;		(Y)= Length.		;
;		($FE)= F color.		;
;		($FF)= B color.		;
;---------------------------------------;
DispCharsAt:				;
	jsr	CheckLineLength		; Length==0?
	beq	nedca08			;
	jsr	LetFBSameColorG		; No.
	cpx	#$20			;
	bne	nedca04			;
	lda	$ff			; B color.
	and	#3			; Low two bits is 00 or 11?
	beq	nedca12			;
	cmp	#3			;
	bne	nedca04			;
	lda	#$ff			;
	bne	nedca12		;(jmp)	;
;---------------------------------------;
nedca04:ldy	#0			;
	lda	TempSave3		; length.
	pha				;
	jsr	GetASCIIFont		;
	pla				;
	tay				;
	lda	CurColumn		;
	pha				;
lpdca04:tya				;
	pha				;
	jsr	SaveHalfFontInit	;
	jsr	SaveFontAt320		;
	sty	DispBfTail		;
	jsr	SendAttributeOne	;
	inc	CurColumn		;
	pla				;
	tay				;
	dey				;
	bne	lpdca04			;
	pla				;
	sta	CurColumn		;
nedca08:rts				;
;---------------------------------------; Yes. Font data all same.
nedca12:jsr	SaveSpaceFont		;
	lda	$fe			; Access attribute.
	and	#$0c			; Bits set.
	sta	TempSave1		;
	lsr				;
	lsr				;
	ora	TempSave1		;
	tax				;
	lda	CurLine			;
	lsr				;
	txa				;
	ldx	#$f0			;
	bcc	nedca16			;
	asl				;
	asl				;
	asl				;
	asl				;
	ldx	#$0f			;
nedca16:sta	TempSave1		; Bits set.
	stx	TempSave2		; Bits clear.
	ldx	TempSave3		;
	lda	CurColumn		;
	lsr				;
	bcc	nedca20			;
	inx				;
nedca20:inx				;
	txa				;
	and	#$fe			;
	sta	TempSave3		;
	lda	CurLine			; (Line/2*16)+Column/4 = Attrib Addr.
	and	#$0e			;
	asl				;
	asl				;
	sta	TempSave5		;
	lda	CurColumn		;
	lsr				;
	lsr				;
	php				; Save C.
	clc				;
	adc	TempSave5		;
	tay				;
	plp				; Rest C.
	pha				;
	bcc	nedca24			;
	lda	#$33			;
	ora	TempSave2		;
	and	Attribute,y		;
	sta	Attribute,y		;
	lda	#$cc			;
	and	TempSave1		;
	ora	Attribute,y		;
	sta	Attribute,y		;
	iny				;
	dec	TempSave3		;
	dec	TempSave3		;
nedca24:lda	TempSave3		; Middle.
	sec				;
lpdca08:sbc	#4			;
	bcc	nedca28			;
	tax				;
	lda	TempSave2		; Bits clear.
	and	Attribute,y		;
	ora	TempSave1		; Bits set.
	sta	Attribute,y		;
	iny				;
	txa				;
	bcs	lpdca08		;(jmp)	;
nedca28:adc	#4			; Last.
	beq	nedcr32			;
	lda	TempSave2		;
	ora	#$cc			;
	and	Attribute,y		;
	sta	Attribute,y		;
	lda	TempSave1		;
	and	#$33			;
	ora	Attribute,y		;
	sta	Attribute,y		;
	iny				;
nedcr32:pla				;
	sta	TempSave1		;
	tax				;
	tya				;
	sec				;
	sbc	TempSave1		;
	jsr	SendAttributeLine	;
	iny				;
	ldx	TempSave1		;
	sta	TempSave1		;
lpdca12:lda	Attribute,x		;
	sta	DisplayBuffer,y		; +4-
	inx				;
	iny				;
	dec	TempSave1		;
	bne	lpdca12			;
	sty	DispBfTail		;
	rts				;

;=======================================;
;	Input	Y|X = System Char code.	;
;	Output	A|X|Y= Font buffer.	;
;---------------------------------------;
GetSystemFont:				;
	tya				; Half character?
	bmi	negsf04			;
	txa				; Yes.
	lsr				;
	php				;
	tax				;
	jsr	ReadHanzi		;
	plp				; Odd half character?
	bcc	negsf08			;
	ldx	#<FontBuffer+16		; Yes.
	ldy	#>FontBuffer+16		;
	bcs	negsf12		;(jmp)	;
negsf04:jsr	ReadHanzi		;
negsf08:ldx	#<FontBuffer		;
	ldy	#>FontBuffer		;
negsf12:lda	CurrentPage		;
	rts				;

;---------------------------------------;
GetExternFont:				;
	jsr	SaveMoveBlockBF		;
	lda	OriginalPage		;
	jmp	MoveToFontBF		; Move A|X|Y L20 to FontBuffer
;---------------------------------------;
MoveFontToDisplay:			;
	ldx	#0			;
lpmfd04:lda	FontBuffer,x		;
	inx				;
	sta	DisplayBuffer,y		;
	iny				;
	cpx	#32			;
	bne	lpmfd04			;
	sty	DispBfTail		;
	rts				;
;=======================================;
;   Set extern font to current cursor.	;
;	Input	X|Y = Font buffer.	;
;		($FF)= Color group.	;
;---------------------------------------;
SetBGFont:				;
	jsr	GetExternFont		;
	jsr	SaveHalfFontInit	;
	jsr	MoveFontToDisplay	;
	ldx	$ff			; Color group.
	jmp	SendAttributeOneIN	;

;=======================================;
;   Set extern font 8*16 to FG.		;
;	Input	X|Y = Font buffer.	;
;		($FC)= Color group.	;
;		($FD)= FG index.	;
;		($FE)= X(dot).		;
;		($FF)= Y(dot).		;
;---------------------------------------;
SetFGFont:				;
	jsr	GetExternFont		;
	lda	#32			;
	ldx	$fd			;
	jsr	SaveFGFontInitIN	;
	jsr	MoveFontToDisplay	;
	lda	$fd			; Splite index.
	asl				;
	asl				;
	tax				;
	ldy	$ff			; Splite Y.
	beq	nesff04			;
	dey				;
nesff04:tya				;
	sta	SpliteBuffer,x		;
	lda	$fc			; Color group
	sta	SpliteBuffer+2,x	;
	lda	#8			;
	sta	SpliteDispFlag		;
	clc				;
	adc	$fe			; Split X.
	sta	SpliteBuffer+3,x	;
	rts				;

;=======================================;
;   Set extern font 8*16 to FG.		;
;	Input	($FE)= 0: F->B, 1:B->F	;
;		($FF)= Color group.	;
;		X= X(dot).		;
;		Y= Y(dot).		;
;---------------------------------------;
MoveFGChar:				;
	txa				;
	pha				;
	lda	$ff			; Splite index.
	asl				;
	asl				;
	tax				;
	tya				; Splite Y.
	beq	nemfc04			;
	dey				;
	tya				;
nemfc04:sta	SpliteBuffer,x		;
	lda	SpliteBuffer+2,x	;
	and	#$df			;
	ldy	$fe			;
	beq	nemfc08			;
	ora	#$20			;
nemfc08:sta	SpliteBuffer+2,x	;
	pla				; Split X.
	clc				;
	adc	#8			;
	sta	SpliteBuffer+3,x	;
	inc	SpliteDispFlag		;
	rts				;

;---------------------------------------;
DispOpenClose:				;
	stx	OpenCloseScreen		; Open/Close screen flag.
	txa				;
	beq	nedoc04			;
	jsr	WaitDisplay		;
	lda	#0			;
	sta	$2001			;
	rts				;
nedoc04:inc	SpliteDispFlag		;
	jsr	WaitVSyn		;
	lda	#$08			;
	sta	$2001			;
	jsr	WaitVSyn		;
	lda	#$1a			;
	sta	$2001			;
	rts				;

Return:	rts				;

;---------------------------------------;
SetScreenSW0:				; Switch screen to NORMAL.
	ldx	LastScreenSW		;
	beq	ness004			;
	pha				;
	tya				;
	pha				;
	ldx	#0			;
	jsr	SetScreenSW0IN		;
	pla				;
	tay				;
	pla				;
ness004:rts				;
;=======================================;
;	Input	(X)==0: Switch to NORMAL;
;		(X) <0: Switch to WAIT.	;
;---------------------------------------;
SetScreenSW:				;
	lda	BiosVersion		;
	cmp	#$10			;
	bcs	nessw04			;
	txa				;
	eor	LastScreenSW		;
	bpl	nessw04			; No chnage screen.
	txa				;
	beq	SetScreenSW0IN		;
;---------------------------------------; Set to Wait mode.
	jsr	WaitDisplay		;
	stx	LastScreenSW		;
	lda	#16			; 240 row.(No split)
	sta	SplitBuffer		;
	lda	#14			; Refline page.
	sta	SplitBuffer+1		;
	lda	#0			; Another page.
	sta	SplitBuffer+2		;
	lda	#0			;
	sta	SplitBuffer+5		;
	lda	#BiosSplitScreen	; Disable split screen.
	ldx	#<SplitBuffer		;
	ldy	#>SplitBuffer		;
	jsr	ROMBiosEntry		;
	lda	#$8b			; VRAM page 2.
	sta	$2000			;
	lda	#$08			; Only enable BG display.
	sta	$2001			;
	jsr	WaitVSyn		;
	sei				;
nessw04:rts				;
;---------------------------------------; Set to HZ mode.
SetScreenSW0IN:				;
	stx	LastScreenSW		;
	jsr	SetSplitScreen		;
	lda	#BiosSetSEndVector	;
	ldx	#<DisplayINT		;
	ldy	#>DisplayINT		;
	jsr	ROMBiosEntry		;
	lda	#$a9			;
	sta	$2000			;
	lda	#$1a			;
	sta	$2001			;
	cli				;
	rts				;

;=======================================;
;	(X)==0	Mouse Disable/-Enable.	;
;	Input	(Y)'Bit7: Disable/-Enable
;		(Y)'Bit6: Set/-NoSet Font
;		(Y)'Bit0: Mouse 1/-0	;
;	(X)==2	Mouse initial.		;
;	Input	(Y)'Bit0: Mouse 1/-0	;
;	Output	(A)==0 Mouse Ok.	;
;	(X)==3: Get mouse.		;
;	Input	(Y)'Bit0: Mouse 1/-0	;
;	Output	(A)= Current key pressing
;		 Bit7: Right key pressing
;		 Bit6: Left key pressing;
;		 Bit5: Down key pressing;
;		 Bit4: Up key pressing.	;
;		 Bit3: Start  key pressing
;		 Bit2: Select key pressing
;		 Bit1: B key pressing.	;
;		 Bit0: A key pressing.	;
;		(X)= B key press count.	;
;		(Y)= A key press count.	;
;		($FE)= Start key press count
;		($FF)= Select key press count
;	(X)==4: Set mouse.		;
;	Input	(Y)'Bit0: Mouse 1/-0	;
;		$FE/$FF: Mouse X/Y	;
;	(X)==5: Set mouse range.	;
;	Input	(Y)'Bit0: Mouse 1/-0	;
;		$FC/$FD: Mouse LU's X/Y.;
;		$FE/$FF: Mouse RD's X/T.;
;---------------------------------------;
MouseFont	dw	$c080,	$f0e0,	$fcf8,	$fffe,	$e0c0,	$98b0,	$868c,	$8f83,	$fcf8,	$3ebc,	$1f1e,	$101c,	$e68f,	$e3e6,	$3733,	$383f
MouseOperate:				;
	cpx	#2			;
	bcc	nemoe04			;
	txa				;
	adc	#12-2-1			;
	tax				;
	jmp	DeviceEntry		;
nemoe04:tya				;
	lsr				; Mouse 0?
	bcs	nemoe12			;
	asl				; Yes.
	sta	Mouse0ShowSt		;
	bpl	nemoe08			; Enable mouse?
	asl				; Yes. Set mouse font?
	bpl	nemoe24			;
	lda	#3			; Yes.
	sta	SpliteBuffer+$f4+2	;
	ldx	#$f4/4			;
	bne	nemoe20		;(jmp)	;
nemoe08:lda	#$f8			;
	sta	SpliteBuffer+$f4	;
	bne	nemoe24		;(jmp)	;
nemoe12:asl				; Mouse 1.
	sta	Mouse1ShowSt		;
	bpl	nemoe16			; Enable mouse?
	asl				; Yes. Set mouse font?
	bpl	nemoe24			;
	lda	#$43			; Yes. (Rotate X).
	sta	SpliteBuffer+$f0+2	;
	ldx	#$f0/4			;
	bne	nemoe20		;(jmp)	;
nemoe16:lda	#$f8			;
	sta	SpliteBuffer+$f0	;
	bne	nemoe24		;(jmp)	;
nemoe20:lda	#32			;
	jsr	SaveFGFontInitIN	;
	ldx	#0			;
lpmoe04:lda	MouseFont,x		;
	sta	DisplayBuffer,y		;
	iny				;
	inx				;
	cpx	#32			;
	bne	lpmoe04			;
	sty	DispBfTail		;
nemoe24:inc	SpliteDispFlag		;
	rts				;

;=======================================;
;  Screen window scrool up/down.	;
;	Input	+0: Scroll mode.	;
;		  0: Up, 1: Down.	;
;		+1,2: Window Left-Up.	;
;		+3,4: Window Right-Down.;
;		+5: Scroll lines.	;
;		+6: Space line color.	;
;---------------------------------------;
ScreenScroll:				;
	lda	$ff			;
	pha				;
	lda	$fe			;
	pha				;
	stx	$ff			;
	sty	$fe			;
	ldy	#0			;
lpssl04:jsr	GetOneInputData		;
	sta	ScrollBuffer-1,y	;
	cpy	#7			;
	bne	lpssl04			;
	lda	CurLine			;
	pha				;
	lda	CurColumn		;
	pha				;
	lda	RightDownL		; Clear window?
	sec				;
	sbc	LeftUpL			;
	ldx	LeftUpC			;
	stx	CurColumn		;
	cmp	ScrollLines		;
	bcc	nessl08			;
	txa				;
	bne	nessl16			;
	lda	RightDownC		; Is scroll with screen width?
	cmp	#ScreenWidth-1		;
	bne	nessl16			;
	lda	ScrollMode		; No.
	bne	nessl04			;
	jsr	ScrollUp		;
	jmp	nessl16			;
nessl04:jsr	ScrollDown		;
	jmp	nessl16			;
nessl08:lda	LeftUpL			; Yes.
	sta	CurLine			;
	lda	SpaceBColor		;
	sta	$fe			;
	sta	$ff			;
	lda	RightDownC		;
	sec				;
	sbc	LeftUpC			;
	tay				;
	iny				;
	ldx	ScrollLines		;
lpssl08:txa				;
	pha				;
	tya				;
	pha				;
	ldx	#$20			;
	jsr	DispCharsAt		;
	pla				;
	tay				;
	pla				;
	tax				;
	inc	CurLine			;
	dex				;
	bne	lpssl08			;
nessl16:pla				;
	sta	CurColumn		;
	pla				;
	sta	CurLine			;
	pla				;
	sta	$fe			;
	pla				;
	sta	$ff			;
	rts				;
;---------------------------------------;
;	Input	(X)= Source line.	;
;		(Y)= Target line.	;
;---------------------------------------;
MoveOneLine:				;
	lda	CurrentSplit,x		;
	sta	CurrentSplit,y		;
	txa				;
	pha				; Save X
	lsr				;
	ldx	#$0f			;
	bcc	nemol04			;
	ldx	#$f0			;
nemol04:stx	TempSave1		; Source mask bits.
	asl				;
	asl				;
	asl				;
	tax				;
	tya				;
	pha				; Save Y.
	lsr				;
	ldy	#$f0			;
	bcc	nemol08			;
	ldy	#$0f			;
nemol08:sty	$fe			; Target clear bits.
	asl				;
	asl				;
	asl				;
	tay				;
	lda	#8			;
	sta	TempSave4		;
lpmol04:lda	TempSave1		; Mask source.
	cmp	$fe			;
	beq	nemol12			; Source mask same as target clear?
	clc				;
nemol12:and	Attribute,x		;
	bcc	nemol20			; Yes. O/E of source and target no same.
	bit	$fe			; Target in high 4 bits?
	bmi	nemol16			;
	asl				; Yes. Shift left 4 bits.
	asl				;
	asl				;
	asl				;
	jmp	nemol20			;
nemol16:lsr				; No. Shift right 4 bits.
	lsr				;
	lsr				;
	lsr				;
nemol20:sta	$ff			;
	lda	$fe			;
	and	Attribute,y		; Clear target.
	ora	$ff			; Set target.
	sta	Attribute,y		; Modify target.
	inx				;
	iny				;
	dec	TempSave4		; Yes. Compare next.
	bne	lpmol04			;
	pla				;
	tay				; Restore Y.
	pla				;
	tax				; Restore X.
	rts				;
;---------------------------------------;
;	Input	(A)= Begin line.	;
;		(Y)= End+1 line.	;
;---------------------------------------;
SendAttributes:				;
	and	#$0e			;
	sta	$fe			;
	tya				;
	sec				;
	sbc	$fe			; C=1.
	adc	#0			;
	and	#$fe			;
	asl				;
	asl				;
	tay				;
	sty	$ff			; Length.
	lda	$fe			;
	asl				;
	asl				;
	tax				;
	stx	$fe			; Address.
	lda	Attribute,x		;
lpsab04:cmp	Attribute,x		;
	bne	nesab04			;
	inx				;
	dey				;
	bne	lpsab04			;
	pha				;
	lda	#5			;
	jsr	WaitDispBuffer		;
	ldy	DispBfTail		;
	lda	#<$23c0+$4000		;
	sta	DisplayBuffer,y		; +0
	iny				;
	lda	$fe			;
	ora	#>$23c0			;
	sta	DisplayBuffer,y		; +1
	iny				;
	lda	#4			;
	sta	DisplayBuffer,y		; +2
	iny				;
	lda	$ff			;
	sta	DisplayBuffer,y		; +3
	iny				;
	pla				;
	sta	DisplayBuffer,y		; +4
	iny				;
	sty	DispBfTail		;
	rts				;
nesab04:lda	$ff			;
	ldx	$fe			;
	jsr	SendAttributeLine	;
	iny				;
	ldx	$fe			;
lpsab08:lda	Attribute,x		;
	sta	DisplayBuffer,y		; +4-
	inx				;
	iny				;
	dec	$ff			;
	bne	lpsab08			;
	sty	DispBfTail		;
	rts				;
;---------------------------------------; Scroll Up.
ScrollUp:				;
	ldx	LeftUpL			;
	ldy	ScrollLines		; Save replaced pages.
lpsup04:lda	CurrentSplit,x		;
	inx				;
	pha				;
	dey				;
	bne	lpsup04			;
	ldy	LeftUpL			; Move scroll pages.
lpsup08:cpx	RightDownL		;
	bcc	nesup04			;
	bne	nesup08			;
nesup04:jsr	MoveOneLine		;
	inx				;
	iny				;
	bne	lpsup08		;(jmp)	;
nesup08:sty	CurLine			;
	lda	LeftUpL			;
	cmp	CurLine			;
	bcs	nesup12			;
	jsr	SendAttributes		;
nesup12:lda	FreeVideoPage		; Fill spaces.
lpsup12:ldy	CurLine			;
	sta	CurrentSplit,y		;
	lda	SpaceBColor		;
	sta	$fe			;
	sta	$ff			;
	ldx	#$20			;
	ldy	#ScreenWidth		;
	jsr	DispCharsAt		;
	jsr	WaitVSyn		;
	lda	BiosVersion		;
	bne	nesup16			;
	jsr	WaitDisplay		; 1995.8.3 version's SplitScreen required.
lpsup16:lda	SplitPoint		;
	cmp	#$46			;
	bcs	lpsup16			;
	cmp	#15			;
	bcc	lpsup16			;
lpsup20:cmp	SplitPoint		;
	beq	lpsup20			;
	pha				;
	lda	#$09			;
;	lda	#$29			;
	sta	$2000			;
	jsr	SetSplitScreen		;
	pla				;
	clc				;
	adc	#10			;
	sta	SplitPoint		;
	lda	#$a9			;
	sta	$2000			;
	bne	nesup20		;(jmp)	;
nesup16:jsr	SetSplitScreen		;
nesup20:inc	CurLine			;
	pla				;
	dec	ScrollLines		;
	bne	lpsup12			;
	sta	FreeVideoPage		;
	rts				;
;---------------------------------------; Scroll down.
ScrollDown:				;
	ldx	RightDownL		;
	ldy	ScrollLines		; Save replaced pages.
lpsdn04:lda	CurrentSplit,x		;
	dex				;
	pha				;
	dey				;
	bne	lpsdn04			;
	ldy	RightDownL		; Move scroll pages.
	txa				;
	bmi	nesdn04			;
lpsdn08:cpx	LeftUpL			;
	bcc	nesdn04			;
	jsr	MoveOneLine		;
	dey				;
	dex				;
	bpl	lpsdn08			;
nesdn04:sty	CurLine			;
	cpy	RightDownL		;
	bcs	nesdn08			;
	iny				;
	tya				;
	ldy	RightDownL		;
	iny				;
	jsr	SendAttributes		;
nesdn08:lda	FreeVideoPage		; Fill spaces.
lpsdn12:ldy	CurLine			;
	sta	CurrentSplit,y		;
	lda	SpaceBColor		;
	sta	$FE			;
	sta	$FF			;
	ldx	#$20			;
	ldy	#ScreenWidth		;
	jsr	DispCharsAt		;
	lda	BiosVersion		;
	bne	nesdn16			;
	jsr	WaitDisplay		; 1995.8.3 version's SplitScreen required.
lpsdn16:lda	SplitPoint		;
	cmp	#$46			;
	bcs	lpsdn16			;
	cmp	#15			;
	bcc	lpsdn16			;
lpsdn20:cmp	SplitPoint		;
	beq	lpsdn20			;
	pha				;
	lda	#$29			;
	sta	$2000			;
	jsr	SetSplitScreen		;
	pla				;
	clc				;
	adc	#10			;
	sta	SplitPoint		;
	lda	#$a9			;
	sta	$2000			;
	bne	nesdn20		;(jmp)	;
nesdn16:jsr	SetSplitScreen		;
nesdn20:dec	CurLine			;
	pla				;
	dec	ScrollLines		;
	bne	lpsdn12			;
	sta	FreeVideoPage		;
	rts				;

;=======================================;
;	Input	(Y)= Palette position.	;
;		(X)= Palette data.	;
;---------------------------------------;
SetPalette:				;
	txa				;
	ldx	#$3f			;
	jmp	SendVideo		;
;=======================================;
;  Display half character with attribute;
;and cursor no move.			;
;	Input	Y|X= Character.		;
;		$FD= 0: first, 1: last.	;
;		$FE/$FF= F/B color.	;
;---------------------------------------;
DispHalfChAt:				;
	jsr	LetFBSameColorG		;
	jsr	ReadHanzi		;
	jsr	SaveHalfFontInit	;
	lda	$fd			;
	beq	nedpt12			;
	jsr	SaveFontAt321		;
	sty	DispBfTail		;
	jmp	SendAttributeOne	;
;=======================================;
;   Display character with attribute and;
;cursor no move.			;
;	Input	Y|X= Character.		;
;		$FE/$FF= F/B color.	;
;---------------------------------------;
DisplayChAt:				;
	jsr	LetFBSameColorG		;
	tya				;
	bmi	DispChAtHanzi		;
	cpx	#$20			;
	bne	nedpt08			;
	lda	$ff			; B color.
	and	#3			; Low two bits is 00 or 11?
	beq	nedpt04			;
	cmp	#3			;
	bne	nedpt08			;
	lda	#$ff			; Font.
nedpt04:pha				;
	lda	#32			; Length.
	jsr	SaveSameFont		;
	pla				;
	sta	DisplayBuffer,y		; +4
	iny				;
	jmp	nedpt16			;
;---------------------------------------;
nedpt08:jsr	GetASCIIFont		;
	jsr	SaveHalfFontInit	;
nedpt12:jsr	SaveFontAt320		;
nedpt16:sty	DispBfTail		;
	jmp	SendAttributeOne	;
;---------------------------------------;
DispChAtHanzi:				;
	jsr	ReadHanzi		;
	jsr	SaveFullFontInit	;
	jsr	SaveFontAt320		;
	jsr	SaveFontAt321		;
	sty	DispBfTail		;
	jsr	SendAttributeOne	;
	inc	CurColumn		;
	jsr	SendAttributeOne	;
	dec	CurColumn		;
	rts				;
;---------------------------------------;
DispHZFuncStg:				;
	lda	HZFunction		;
	asl				;
	tay				;
	ldx	HZFunctionStgP+1,y	;
	lda	HZFunctionStgP,y	;
	tay				;
	lda	CurrentPage		;
	sta	OriginalPage		;
;---------------------------------------;
;	Input	X|Y=HZFuncPoint.	;
;---------------------------------------;
DispRefString:				;
	lda	#DispStringBfLen-1	; String length.
	jsr	MoveDispString		;
	ldy	#0			;
	sty	CurColumn		;
lpdrs04:lda	#0			;
	sta	TempSave1		;
	lda	DispStringBf,y		;
	beq	nedrs08			; String ending?
	bpl	nedrs04			; No. HANZI?
	sta	TempSave1		; Yes.
	iny				;
	lda	DispStringBf,y		;
nedrs04:tax				;
	tya				;
	pha				;
	ldy	TempSave1		;
	jsr	DispChAtTTY		;
	pla				;
	jsr	SetScreenSW0		;
	tay				;
	iny				;
	bne	lpdrs04		;(jmp)	;
nedrs08:rts				;
;---------------------------------------;
;   Clear refline from giving column.	;
;	Input	(A)= Refline column	;
;---------------------------------------;
ClearRefGive:				;
	sta	RefColumn		;
;---------------------------------------;
;   Clear refline from ref column.	;
;---------------------------------------;
ClearRefLineIN:				;
	lda	#ScreenWidth		;
;---------------------------------------;
;   Clear refline from refcolumn to Y.	;
;---------------------------------------;
ClearRefLineLen:			;
	sec				;
	sbc	RefColumn		;
	tay				;
	lda	#' '			;
	ldx	#1			;
	bne	DispRefCharsIN	;(jmp)	;
;=======================================;
;    Chinese refrence line operation.	;
;	(X)==1: ($FF)= Char, (Y)=Length.;
;	(X)==3: Y|$FF= Character.	;
;---------------------------------------;
DispRefASCIITTY:			;
	ldy	#0			;
	ldx	#3			;
DispRefCharsIN:				;
	sta	$ff			;
DispRefChar:				;
	txa				;
	jsr	SetScreenSW0		;
	tax				;
	lda	CurLine			; Save CURLC
	pha				;
	lda	CurColumn		;
	pha				;
	lda	$ff			;
	pha				;
	lda	$fe			;
	pha				;
	lda	#ChineseLine		; Set CURLC to chinese one.
	sta	CurLine			;
	lda	RefColumn		;
	sta	CurColumn		;
	lda	$ff			;
	sta	TempSave1		;
	lda	#14			; Set F and B color.
	sta	$fe			;
	lda	#15			;
	sta	$ff			;
;---------------------------------------; (X)==0: Clear line.
	txa				;
	bne	nedrc04			;
	sta	StringCount		;
	sta	OutputNumber		;
	jsr	DispHZFuncStg		;
	jmp	nedrc20			;
;---------------------------------------; (X)==1: Display chars.
nedrc04:dex				;
	bne	nedrc08			;
	ldx	TempSave1		;
	jsr	DispCharsAt		;
	jmp	nedrc32			;
;---------------------------------------; (X)==2: Set cursor.
nedrc08:dex				;
	beq	nedrc28			;
;---------------------------------------; (X)==3: Display TTY.
	dex				;
	bne	nedrc12			;
	ldx	TempSave1		;
	jsr	DispChAtTTY		;
	ldy	CurColumn		;
	bne	nedrc28			;
	ldy	#ScreenWidth		;
	bne	nedrc28		;(jmp)	;
;---------------------------------------; (X)==4: Display HZ function string.
nedrc12:dex				;
	bne	nedrc16			;
	jsr	DispHZFuncStg		;
	jmp	nedrc24			;
;---------------------------------------; (X)==5: Display String in Ref line.
nedrc16:dex				;
	bne	nedrc32			;
	ldx	TempSave1		;
	jsr	DispRefString		;
nedrc20:ldx	#' '			;
	ldy	#ScreenWidth		;
	jsr	DispCharsAt		;
nedrc24:ldy	CurColumn		;
nedrc28:sty	RefColumn		;
nedrc32:pla				;
	sta	$fe			;
	pla				;
	sta	$ff			;
	pla				;
	sta	CurColumn		;
	pla				;
	sta	CurLine			;
	rts				;

;=======================================;
;    Change keyboard INT key test mode.	;
;	Input	(X)==0: NMI test mode.	;
;		    <0: SEnd test mode.	;
;---------------------------------------;
ChgKBDINTMode:				;
	txa				; (X)==0?
	bmi	neckm04			;
	sta	KBDTestMode		; Yes.
	ldx	#6			;
	ldy	#0			;
	jmp	OriginalKeyBoard	;
neckm04:pha				; No.
	ldx	#6			;
	ldy	#$80			;
	jsr	OriginalKeyBoard	; Set KBD no NMI test.
	stx	HighModify+2		;
	sty	HighModify+1		;
	pla				;
	sta	KBDTestMode		;
	rts				;

;---------------------------------------;
;	Input	(X)= Splite index.	;
;		(Y)= Splite X.		;
;---------------------------------------;
SetSpliteData:				;
	txa				;
	asl				; Splite index.
	asl				;
	tax				;
	lda	$fd			; Splite Y.
	asl				;
	asl				;
	asl				;
	asl				;
	beq	nessd04			;
	sbc	#0			;
nessd04:sta	SpliteBuffer,x		;
	lda	$fe			;
	lsr				;
	lsr				;
	and	#3			;
	sta	SpliteBuffer+2,x	;
	tya				;
	asl				;
	asl				;
	asl				;
	adc	#8			;
	sta	SpliteBuffer+3,x	;
	rts				;
;=======================================;
;	Input	($FB)= FG index.	;
;		($FC)= Display column.	;
;		($FD)= Display line.	;
;		($FE)= F color.		;
;		($FF)= B color.		;
;		Y|X= Character.		;
;---------------------------------------;
DispFGCharAt:				;
	cpx	#' '			; Is clear?
	bne	nedfa04			;
	lda	$ff			;
	and	#3			;
	bne	nedfa04			;
	lda	$fb			; Yes.
	asl				;
	asl				;
	tax				;
	lda	#$f8			;
	sta	SpliteBuffer,x		;
	bne	nedfa12		;(jmp)	;
nedfa04:tya				;
	bmi	DispFGChAtHanzi		;
	jsr	GetASCIIFont		;
	lda	#32			;
	jsr	SaveFGFontInit		;
	jsr	SaveFontAt320		;
	sty	DispBfTail		;
	jmp	nedfa08			;
;---------------------------------------;
DispFGChAtHanzi:			;
	jsr	ReadHanzi		;
	lda	#64			;
	jsr	SaveFGFontInit		;
	jsr	SaveFontAt320		;
	jsr	SaveFontAt321		;
	sty	DispBfTail		;
	ldx	$fb			;
	inx				;
	ldy	$fc			;
	iny				;
	jsr	SetSpliteData		;
nedfa08:ldx	$fb			;
	ldy	$fc			;
	jsr	SetSpliteData		;
nedfa12:inc	SpliteDispFlag		;
	rts				;

;---------------------------------------;
SaveCursorRowsM:			;
lpsfm04:lda	TempSave2		;
	sta	DisplayBuffer,y		;
	iny				;
	inc	TempSave1		; Row current.
	lda	TempSave1		;
	and	#7			; Accross to next 8*8 font?
	bne	nesfm04			;
	tya				; Yes.
	clc				;
	adc	#8			;
	tay				;
nesfm04:dex				;
	bne	lpsfm04			;
	rts				;
;---------------------------------------;
;	Input	(A)= Rows.		;
;		(X)= Data.		;
;---------------------------------------;
SaveCursorRows:				;
	stx	TempSave2		;
	tax				;
	beq	nescf04			;
	pha				; Save X,Cot,Y
	lda	TempSave1		;
	pha				;
	tya				;
	pha				;
	jsr	SaveCursorRowsM		; First map.
	pla				;
	clc				;
	adc	#8			;
	tay				;
	pla				;
	sta	TempSave1		;
	pla				;
	tax				;
	jsr	SaveCursorRowsM		; Next map.
	tya				;
	sec				;
	sbc	#8			;
	tay				;
nescf04:rts				;
;---------------------------------------;
SaveCursorFont:				;
	lda	CursorBegRow		; Cursor from first row?
	ldx	#0			;
	stx	TempSave1		;
	jsr	SaveCursorRows		;
	lda	CursorEndRow		;
	sec				;
	sbc	CursorBegRow		;
	adc	#0			;
	ldx	#$ff			;
	jsr	SaveCursorRows		;
	lda	#15			;
	sec				;
	sbc	CursorEndRow		;
	ldx	#0			;
	jsr	SaveCursorRows		;
	rts				;
;=======================================;
;   Set cursor type.			;
; Input	(X)'s:				;
;	Bit7==0: Cursor display.	;
;	    ==1: Cursor hidden.		;
;	Bit6==0: 8dot wide		;
;	    ==1: 16dot wide		;
;	Bit5==0: Cursor shape unchange.	;
;	    ==1: (Y) = Cursor shape.	;
;	 (Y)'s Bit7-4: Cursor begin row.;
;	 (Y)'s Bit3-0: Cursor end row.	;
;	Bit4-0:  Cursor blinking rate.	;
;		 ==0: No blinking.	;
;---------------------------------------;
SetCursorType:				;
	bit	CursorTypeFlag		; Last cursor enable?
	bmi	nesct04			;
	lda	#$f8			; Yes.
	sta	SpliteBuffer+$fc	;
	bvc	nesct04			; Last is 16 dot cursor?
	sta	SpliteBuffer+$f8	; Yes. Clear next 8 dor cursor splite.
nesct04:stx	CursorTypeFlag		;
	inc	SpliteDispFlag		;
	txa				;
	and	#$20			;
	beq	DisplayCursor		;
	tya				;
	lsr				;
	lsr				;
	lsr				;
	lsr				;
	sta	CursorBegRow		;
	tya				;
	and	#$0f			;
	sta	CursorEndRow		;
	lda	#36			;
	ldx	#>$1fe0+$8000		;
	ldy	#32			;
	bit	CursorTypeFlag		;
	bvc	nesct08			;
	lda	#68			;
	ldx	#>$1fc0+$8000		;
	ldy	#64			;
nesct08:jsr	WaitDispBuffer		;
	tya				;
	pha				;
	ldy	DispBfTail		; $1fe0-$1fff.
	lda	#<$1f00+$8000		; #$80
	sta	DisplayBuffer,y		; +0
	iny				;
	txa				;	
	sta	DisplayBuffer,y		; +1
	iny				;
	iny				; +2 (any page).
	pla				;
	sta	DisplayBuffer,y		; +3
	iny				;
	jsr	SaveCursorFont		;
	bit	CursorTypeFlag		;
	bvc	nesct16			;
	jsr	SaveCursorFont		;
nesct16:sty	DispBfTail		;
;---------------------------------------;
DisplayCursor:				;
	ldx	CurLine			;
	cpx	#ChineseLine		;
	bcs	nedcu20			;
	lda	CursorTypeFlag		; Cursor display enable?
	bmi	nedcu20			;
	ora	#$80			;
	sta	CursorTypeFlag		;
	and	#$1f			;
	sta	CursorBlinkCnt		;
	txa				; Splite Y.
	asl				;
	asl				;
	asl				;
	asl				;
	beq	nedcu08			;
	sbc	#0			;
nedcu08:sta	CursorBlockRow		;
	lda	#$f8			;
	sta	SpliteBuffer+$fc	;
	bit	CursorTypeFlag		;
	bvc	nedcu12			;
	sta	SpliteBuffer+$f8	; 16 dot cursor.
nedcu12:lda	CurColumn		;
	asl				;
	asl				;
	asl				;
	adc	#8			;
	sta	SpliteBuffer+$fc+3	; Splite X.
	ldy	#$03			; Splite attribute.
	sty	SpliteBuffer+$fc+2	;
	bit	CursorTypeFlag		;
	bvc	nedcu16			;
	adc	#8			;
	sta	SpliteBuffer+$f8+3	;
	sty	SpliteBuffer+$f8+2	;
nedcu16:lda	CursorTypeFlag		;
	sta	SpliteDispFlag		; Splite display flag.
	and	#$7f			;
	sta	CursorTypeFlag		;
nedcu20:rts				;

;=======================================;
;  Get old and set new extend font Proc.;
;	Input	$FF|X|Y= New Ext. Proc.	;
;	Output	A|X|Y  = Old Ext. Proc.	;
;---------------------------------------;
GetSetExtFont:				;
	lda	OldExtFontProc+2	;
	stx	OldExtFontProc+2	;
	tax				;
	lda	OldExtFontProc+1	;
	sty	OldExtFontProc+1	;
	tay				;
	lda	OldExtFontProc+0	;
	pha				;
	lda	$ff			;
	sta	OldExtFontProc+0	;
	pla				;
	rts				;

;---------------------------------------;
MoveDispString:				;
	pha				;
	jsr	SaveMoveBlockBF		; Save string to display string buffer.
	lda	OriginalPage		;
	sta	MoveBlockBF+0		;
	stx	MoveBlockBF+2		;
	sty	MoveBlockBF+1		;
	lda	CurrentPage		;
	sta	MoveBlockBF+3		;
	ldx	#<DispStringBf		;
	ldy	#>DispStringBf		;
	stx	MoveBlockBF+5		;
	sty	MoveBlockBF+4		;
	pla				; String length.
	bne	nemds04			;
	rts				;
nemds04:cmp	#DispStringBfLen	;
	bcc	nemds08			;
	lda	#DispStringBfLen	;
	sta	MoveBlockBFSave+5	; String length.
nemds08:tay				;
	jsr	MoveDataSmall		;
	jmp	RestMoveBlockBF		;
;=======================================;
;	Input	X|Y= String point.	;
;	 ($FC): Bit7==0: Stg. has ASCII.;
;		    ==1: Stg. no ASCII.	;
;		Bit6==1: Change Cursor.	;
;		    ==0: No Change Cur.	;
;	 ($FD)= String length.		;
;	 ($FE)= F color.		;
;	 ($FF)= B color.		;
;---------------------------------------;
DispStringAt:				;
	lda	$fd			; String length.
	jsr	MoveDispString		;
	lda	CurColumn		;
	pha				;
	ldy	#0			;
lpdas04:ldx	DispStringBf,y		; No check one code ASCII?
	iny				;
	txa				; or Is HANZI?
	ora	$fc			;
	bmi	nedas12			;
	txa				; No.
	ldx	#0			;
	cmp	#$0d			; Is Enter code?
	beq	nedas24			; Yes.
	cmp	#$0a			; Is Linefeed code?
	beq	nedas08			;
	lda	CurColumn		; No. Can display half AS?
	cmp	#ScreenWidth		;
	bcc	nedas16			;
	bcs	nedas28		;(jmp)	; No.
nedas08:tya				;
	pha				;
	jsr	CurLinePlus1		;
	pla				;
	tay				;
	bne	nedas28		;(jmp)	;
nedas12:dec	$fd			; Yes. Has enough char?
	beq	nedas32			;
	iny				;
	txa				;
	bpl	nedas16			;
	ldx	CurColumn		; Yes. Last column?
	inx				;
	cpx	#ScreenWidth		;
	bcs	nedas24			;
	tax				;
nedas16:tya				; Save Y.
	pha				;
	txa				; High.
	ldx	DispStringBf-1,y	; Low.
	tay				;
	pha				; Save High.
	jsr	DisplayChAt		;
	jsr	SetScreenSW0		;
	ldx	CurColumn		;
	inx				;
	pla				; Display HZ?
	bpl	nedas20			;
	inx				; Yes.
nedas20:pla				;
	tay				;
nedas24:stx	CurColumn		;
nedas28:
;jsr	WaitDisplay
	dec	$fd			;
	bne	lpdas04			;
nedas32:pla				;
	bit	$fc			;
	bvs	nedas36			;
	sta	CurColumn		;
	jmp	DisplayCursor		;
nedas36:ldy	CurColumn		;
	jmp	CheckCurColumn		;

;---------------------------------------;
ChgBGColor:				;
	sty	TempSave5		;
	ldy	CurColumn		;
lpcbc04:cpy	#ScreenWidth		;
	bcs	necbc04			;
	txa				;
	pha				;
	tya				;
	pha				;
	jsr	SendAttributeOneC	;
	pla				;
	tay				;
	pla				;
	tax				;
	iny				;
	dec	TempSave5		;
	bne	lpcbc04			;
necbc04:rts				;
;---------------------------------------;
;	Input	(2|A)= HZ index.	;
;		X= Base addressN index.	;
;---------------------------------------;
IndexToFont:				;
	clc				;
	adc	BaseFontAddr,y		; Low.
	sta	MoveBlockBF+1		;
	lda	MoveBlockBF+2		;
	adc	BaseFontAddr+1,y	; High.
	tax				;
	lsr				;
	clc				;
	adc	BaseFontAddr+2,y	; Page.
	pha				;
	txa				;
	lsr				;
	lda	MoveBlockBF+1		;
	ror				;
	lsr				;
	sec				;
	ror				; 10xyyyyy
	tax				;
	lda	MoveBlockBF+1		;
	asl				;
	asl				;
	asl				;
	asl				;
	asl				;
	tay				;
	pla				;
	rts				;
;---------------------------------------;
;	Output	X|A= X*94;		;
;	Save	Y.			;
;---------------------------------------;
AMul94:	sta	MoveBlockBF+4		;
	lda	#0			;
	ldx	#8			;
lpm9404:ror	MoveBlockBF+4		;
	bcc	nem9404			;
	adc	#94-1			;
nem9404:lsr				;
	dex				;
	bne	lpm9404			;
	tax				;
	lda	MoveBlockBF+4		;
	ror				;
	rts				;
;---------------------------------------;
SearchHanzi:				;
	ldx	#<CacheHzBeg		; 1/2 search.
	ldy	#>CacheHzBeg		;
	stx	SearchBegin+1		;
	sty	SearchBegin		;
	ldx	CacheHzEnd+1		;
	ldy	CacheHzEnd		;
	stx	SearchEnd+1		;
	sty	SearchEnd		;
lpshi04:ldx	SearchBegin+1		;
	lda	SearchBegin		;
lpshi08:cmp	SearchEnd		; Search ending?
	bcc	neshi04			;
	cpx	SearchEnd+1		;
	bcs	neshi28			;
neshi04:adc	SearchEnd		; No. (End-Beg)/2+Beg = (Beg+End)/2.
	tay				;
	txa				;
	adc	SearchEnd+1		;
	ror				;
	sta	MoveBlockBF+7		;
	tya				;
	ror				;
	and	#$fc			;
	sta	MoveBlockBF+6		;
	ldy	#1			;
	lda	HzCodeH			; Hanzi low equal?
	cmp	(MoveBlockBF+6),y	;
	bcc	neshi16			;
	bne	neshi08			;
	lda	HzCodeL			; Yes.
	dey				; High equal also?
	cmp	(MoveBlockBF+6),y	;
	bcc	neshi20			;
	beq	neshi24			; =
	iny				;
neshi08:ldx	MoveBlockBF+7		; >. Y=1.
	lda	MoveBlockBF+6		;
	clc				;
	adc	#4			;
	bcc	neshi12			;
	inx				;
neshi12:stx	SearchBegin+1		;
	sta	SearchBegin		;
	jmp	lpshi08			;
neshi16:dey				;
neshi20:ldx	MoveBlockBF+7		; <. Y=0.
	lda	MoveBlockBF+6		;
	stx	SearchEnd+1		;
	sta	SearchEnd		;
	jmp	lpshi04			;
neshi24:clc				;
neshi28:rts				;
;---------------------------------------;
;	Input	A|HzCodeL= HZcode.	;
;---------------------------------------;
HZCodeToCURP:				;
	ldy	HzCodeL			;
	tax				;
	beq	nehtc12			;
	cmp	#$aa			;
	bcc	nehtc04			;
	sbc	#6			;
nehtc04:sec				;
	sbc	#$a1			;
	jsr	AMul94			;
	clc				;
	adc	HzCodeL			;
	bcc	nehtc08			;
	inx				;
nehtc08:sec				;
	sbc	#$a1-128		; Pass ASCII.
	bcs	nehtc16			;
	dex				;
	tay				;
nehtc12:tya				;
nehtc16:stx	FileIOPaged+CURP+1	;
	ldy	#0			;
	sty	FileIOPaged+CURP+2	;
	ldy	#5			; *32.
lphtc04:asl				;
	rol	FileIOPaged+CURP+1	;
	rol	FileIOPaged+CURP+2	;
	dey				;
	bne	lphtc04			;
	sta	FileIOPaged+CURP	;
	rts				;
;---------------------------------------;
FileIOPagedAccess:			;
	stx	FileIOPaged+HBUF+1	;
	sty	FileIOPaged+HBUF	;
	sta	FileIOPaged+FILECMD	;
	ldx	#<FileIOPaged		;
	ldy	#>FileIOPaged		;
	jmp	FileIOEntry		;
;---------------------------------------;
;	Input	$F8 = Caller's page.	;
;		$FA|$F9= HZ code.	;
;	Output	Cy==0			;
;		 ($F8)= Font page.	;
;		 ($FA|$F9)= Font address;
;---------------------------------------;
ExtFontProcIN:				;
	pla				; Caller's page.
	tax				;
	lda	#BiosSetPage		;
	sec				;
	jmp	ROMBiosEntry		;
;---------------------------------------;
;    Move paged font to font buffer.	;
;---------------------------------------;
MoveFontToFontBF:			;
	lda	CurrentPage		;
	ldx	#<FontBuffer		;
	ldy	#>FontBuffer		;
	sta	MoveBlockBF+3		;
	stx	MoveBlockBF+5		;
	sty	MoveBlockBF+4		;
MoveFontTo:				;
	ldy	#>$0020			;
MoveDataSmall:				;
	ldx	#0			;
	stx	MoveBlockBF+7		;
	sty	MoveBlockBF+6		;
	lda	#BiosMoveBlock		;
	ldx	#MoveBlockBF		;
	jmp	ROMBiosEntry		;
;=======================================;
;	Input	Y|X= Hanzi code.	;
;---------------------------------------;
ReadHanzi:				;
	sty	HzCodeH			;
	stx	HzCodeL			;
	jsr	SaveMoveBlockBF		;
	lda	HzCodeH			;
	bpl	nerhi04			;
	ldx	BiosVersion		;
	ldy	CCLIBReadType		;
	dey				; Full HANZI. No resident?
	bmi	nerhi32			; Yes.
	bne	nerhi08			;
;---------------------------------------; Residnet 1 CCLIB.
	cmp	#$d8			; Q16-55? (Posi=2-41)
	bcs	nerhi32			;
	ldy	#3			;
	cpx	#$40			;
	beq	nerhi12			;
	tax				;
	lda	#0			;
	cpx	#$a1			; Q01? (Posi=0)
	beq	nerhi24			;
	lda	#1			;
	cpx	#$a3			; Q02? (Posi=1)
	beq	nerhi24			;
	lda	#42			;
	cpx	#$a9			; Q09? (Posi=42)
	beq	nerhi24			;
	txa				;
	cmp	#$b0			;
	bcc	nerhi32			;
	sbc	#$b0-2			; Yes.
	bne	nerhi24		;(jmp)	;
;---------------------------------------; Half ASCII.
nerhi04:bne	nerhi32			;
	lda	HzCodeL			; ASCII. This ASCII residnet?
	cmp	ASCIIResident		;
	bcs	nerhi32			;
	lda	HzCodeL			; Yes.
	ldy	#0			; (Y)=0
	sty	MoveBlockBF+2		;
	beq	nerhi36		;(jmp)	;
;---------------------------------------; Resident 2 CCLIB.
nerhi08:ldy	#3			;
	cpx	#$40			; (Y)=3
	bne	nerhi18			;
nerhi12:cmp	#$cd			;
	bcc	nerhi16			;
	bne	nerhi18			;
	ldx	HzCodeL			;
	cpx	#$ad			;
	bcs	nerhi18			;
nerhi16:ldy	#9			;
nerhi18:cmp	#$a1			;
	bcc	nerhi32			;
	cmp	#$aa			;
	bcc	nerhi20			;
	cmp	#$b0			;
	bcc	nerhi32			;
	cmp	#$f8			;
	bcs	nerhi32			;
	adc	#>$a1-1+9-$b0		;
nerhi20:sbc	#$a1-1			;
;---------------------------------------;
nerhi24:jsr	AMul94			;
	clc				;
	adc	HzCodeL			;
	bcc	nerhi28			;
	inx				;
nerhi28:stx	MoveBlockBF+2		;
	jmp	nerhi36			;
;---------------------------------------;
nerhi32:jsr	SearchHanzi		; Hanzi in cache?
	bcs	nerhi48			;
	ldy	#3			; Yes.
	lda	(MoveBlockBF+6),y	; Font index high.
	and	LRUPlusValue		;
	sta	MoveBlockBF+2		;
	ora	LRUCurrentCount		;
	sta	(MoveBlockBF+6),y	;
	dey				;
	lda	(MoveBlockBF+6),y	; Low
	ldy	#6			; CACHE.
nerhi36:jsr	IndexToFont		;
MoveToFontBF:				;
	stx	MoveBlockBF+2		;
	sty	MoveBlockBF+1		;
	cmp	CurrentPage		; Font buffer in current page?
	bne	nerhi40			;
	ldy	#31			; Yes. Copy font directly.
lprhi08:lda	(MoveBlockBF+1),y	;
	sta	FontBuffer,y		;
	dey				;
	bpl	lprhi08			;
	bmi	nerhi44		;(jmp)	;
nerhi40:sta	MoveBlockBF+0		;
	jsr	MoveFontToFontBF	;
nerhi44:jmp	RestMoveBlockBF		;
;---------------------------------------; HZ not in RAM and cache.
nerhi48:tya				; HZ < (Table) ?
	beq	nerhi52			;
	lda	MoveBlockBF+6		; Yes. Let HZ < (Table)
	clc				;
	adc	#4			;
	sta	MoveBlockBF+6		;
	bcc	nerhi52			;
	inc	MoveBlockBF+7		;
nerhi52:lda	HzCodeH			; Is ASCII?
	beq	nerhi60			; Yes.
	cmp	#$a1			; No Is HZ in CCLIB16 file?
	bcc	nerhi56			;
	cmp	#$aa			;
	bcc	nerhi60			;
	cmp	#$b0			;
	bcc	nerhi56			;
	cmp	#$f8			;
	bcc	nerhi60			;
;---------------------------------------; HZ not in CCLIB16.
nerhi56:lda	HzCodeH			; HZ code.
	sta	$fa			;
	lda	HzCodeL			;
	sta	$f9			;
	jsr	CallExtFontProc		;
	bcs	neefr04			; Get extend HZ font OK?
	lda	MoveBlockBF+6		; Yes. Save insert point.
	pha				;
	lda	MoveBlockBF+7		;
	pha				;
	jsr	MoveFontToFontBF	; Save font to font buffer.
	pla				;
	sta	MoveBlockBF+7		;
	tax				;
	pla				;
	sta	MoveBlockBF+6		;
	jmp	nerhi64			; and save font to cache.
neefr04:ldx	#32-1			;
	lda	#0			;
lpefr04:sta	FontBuffer,x		;
	dex				;
	bpl	lpefr04			;
	jmp	RestMoveBlockBF		;
;---------------------------------------;
CallExtFontProc:			;
	lda	CurrentPage		;
	pha				;
	lda	OldExtFontProc+2	; Proc's vector.
	pha				;
	lda	OldExtFontProc+1	;
	pha				;
	lda	#BiosSetPage		;
	ldx	OldExtFontProc		; Proc's page.
	jmp	ROMBiosEntry		; Extend HZ font process.
;---------------------------------------;
nerhi60:lda	CurrentPage		;
	sta	FileIOPaged+HPAGE	;
	ldx	#<32			; Read this HANZI from disk.
	ldy	#>32			;
	stx	FileIOPaged+BYTES+1	;
	sty	FileIOPaged+BYTES	;
	lda	HzCodeH			;
	jsr	HZCodeToCURP		;
	lda	#FileRead		;
	ldx	#<FontBuffer		;
	ldy	#>FontBuffer		;
	jsr	FileIOPagedAccess	;
;---------------------------------------; Search replace cache positon.
	ldx	MoveBlockBF+7		;
	lda	MoveBlockBF+6		;
nerhi64:sec				;
	sbc	#$c0			;
	bcs	necrp04			;
	dex				;
	cpx	#<CacheHzBeg		;
	bcs	necrp04			;
	ldx	#<CacheHzBeg		;
	lda	#>CacheHzBeg		;
necrp04:inx				;
	cpx	CacheHzEnd+1		;
	bcc	necrp12			;
	bne	necrp08			;
	cmp	CacheHzEnd		;
	bcc	necrp12			;
necrp08:ldx	CacheHzEnd+1		;
	lda	CacheHzEnd		;
necrp12:dex				;
	stx	MoveBlockBF+5		;
	sta	MoveBlockBF+4		;
	lda	#$ff			;
	sta	MoveBlockBF+3		;
	eor	LRUPlusValue		;
	sta	MoveBlockBF+1		;
	lda	#3			;
lpcrp04:tay				;
	lda	(MoveBlockBF+4),y	;
	and	MoveBlockBF+1		;
	sec				;
	sbc	LRUCurrentCount		;
	beq	necrp20			;
	cmp	MoveBlockBF+3		;
	beq	necrp16			;
	bcs	necrp20			;
necrp16:sta	MoveBlockBF+3		;
	sty	MoveBlockBF+2		;
necrp20:tya				;
	clc				;
	adc	#4			;
	bcc	lpcrp04			;
	lda	MoveBlockBF+2		;
	and	#$fc			;
	clc				;
	adc	MoveBlockBF+4		;
	sta	MoveBlockBF+4		;
	bcc	necrp24			;
	inc	MoveBlockBF+5		;
necrp24:ldy	#2			; Save delete point font index.
	lda	(MoveBlockBF+4),y	;
	pha				;
	iny				;
	lda	(MoveBlockBF+4),y	;
	and	LRUPlusValue		;
	pha				;
	lda	MoveBlockBF+4		; [3|2] = [5|4]+4
	clc				;
	adc	#4			;
	sta	MoveBlockBF+2		;
	lda	MoveBlockBF+5		;
	adc	#0			;
	sta	MoveBlockBF+3		;
	lda	MoveBlockBF+4		;
	sec				; 5|4: Delete point.
	sbc	MoveBlockBF+6		; 7|6: Insert point.
	lda	MoveBlockBF+5		;
	sbc	MoveBlockBF+7		;
	bcs	necrp32			;
;---------------------------------------; [5|4] < [7|6].
	lda	MoveBlockBF+6		; [1]|X = ([7|6]-[3|2])/4
	sec				;
	sbc	MoveBlockBF+2		;
	beq	necrp28			;
	tax				;
	ldy	#0			;
lpcrp08:lda	(MoveBlockBF+2),y	; [3|2] -> [5|4]
	sta	(MoveBlockBF+4),y	;
	iny				;
	dex				;
	bne	lpcrp08			;
necrp28:lda	MoveBlockBF+6		;
	sec				;
	sbc	#4			;
	sta	MoveBlockBF+6		;
	bcs	necrp36			;
	dec	MoveBlockBF+7		;
	bcc	necrp36		;(jmp)	;
;---------------------------------------; [5|4] >= [7|6].
necrp32:lda	MoveBlockBF+4		;
	sec				;
	sbc	MoveBlockBF+6		;
	beq	necrp36			;
	tax				; [1]|X = ([3|2]-[7|6])/4
	ldy	#0			;
	dec	MoveBlockBF+3		;
	dec	MoveBlockBF+5		;
lpcrp12:dey				; [5|4] -> [3|2]
	lda	(MoveBlockBF+4),y	;
	sta	(MoveBlockBF+2),y	;
	dex				;
	bne	lpcrp12			;
necrp36:lda	LRURemained		;
	clc				;
	adc	LRURemainedPlus		;
	sta	LRURemained		;
	bcc	necrp40			;
	lda	LRUCurrentCount		;
	sec				;
	adc	LRUPlusValue		;
	sta	LRUCurrentCount		;
necrp40:ldy	#0			;
	lda	HzCodeL			;
	sta	(MoveBlockBF+6),y	;
	iny				;
	lda	HzCodeH			;
	sta	(MoveBlockBF+6),y	;
	iny				;
	iny				;
	pla				; Restore delete point font index.
	sta	MoveBlockBF+2		;
	ora	LRUCurrentCount		;
	sta	(MoveBlockBF+6),y	;
	dey				;
	pla				;
	sta	(MoveBlockBF+6),y	;
	ldy	#6			;
	jsr	IndexToFont		;
	sta	MoveBlockBF+3		;
	stx	MoveBlockBF+5		;
	sty	MoveBlockBF+4		;
	lda	CurrentPage		;
	ldx	#<FontBuffer		;
	ldy	#>FontBuffer		;
	sta	MoveBlockBF+0		;
	stx	MoveBlockBF+2		;
	sty	MoveBlockBF+1		;
	jsr	MoveFontTo		;
;---------------------------------------;
RestMoveBlockBF:			;
	ldx	#8-1			; Restore MoveBlockBF
lprmb04:lda	MoveBlockBFSave,x	;
	sta	MoveBlockBF,x		;
	dex				;
	bpl	lprmb04			;
	rts				;
;---------------------------------------;
SaveMoveBlockBF:			;
	txa				;
	pha				;
	ldx	#8-1			; Save MoveBlockBF
lpsmb04:lda	MoveBlockBF,x		;
	sta	MoveBlockBFSave,x	;
	dex				;
	bpl	lpsmb04			;
	pla				;
	tax				;
	rts				;

	org	CacheHzBegin		;
dw	$c,	$1e,	$10,	$22,	$14,	$12,	$16,	$20
dw	$17,	$04,	$18,	$0e,	$19,	$15,	$1a,	$1b
dw	$1b,	$13,	$1c,	$18,	$1d,	$0f,	$25,	$14
dw	$2d,	$1d,	$2e,	$1f,	$58,	$21,	$66,	$0d
dw	$a1ba,	$2a,	$a1bb,	$2e,	$a6c1,	$34,	$b0e6,	$4b,	$b0eb,	$00
dw	$b1b1,	$40,	$b2d9,	$30,	$b2e2,	$35,	$b3a3,	$16,	$b3f6,	$4d
dw	$b4ba,	$3c,	$b4f3,	$38,	$b5da,	$4f,	$b6ab,	$49,	$b6fe,	$1c
dw	$b9ab,	$45,	$baa3,	$48,	$baba,	$07,	$baf2,	$29
dw	$bbaa,	$37,	$bcb6,	$1a,	$bdc7,	$01,	$bdf0,	$42
dw	$bea9,	$41,	$bfe2,	$08,	$c1bf,	$11,	$c1f4,	$0a
dw	$c4dc,	$2c,	$c4ea,	$3f,	$c6b7,	$4e,	$c7e5,	$36,	$c7eb,	$27
dw	$c8cb,	$2d,	$c8d5,	$43,	$c8f0,	$3b,	$c9cf,	$47
dw	$c9d4,	$28,	$c9d9,	$10,	$cad4,	$4a,	$cafd,	$05,	$cbbe,	$46
dw	$ccc6,	$3a,	$cda8,	$44,	$cdb3,	$33,	$cec4,	$03
dw	$cfb5,	$32,	$d0a1,	$2b,	$d1a1,	$0b,	$d1a7,	$39
dw	$d1d0,	$3d,	$d2bb,	$19,	$d3a2,	$02,	$d3c3,	$17
dw	$d4c2,	$4c,	$d4d8,	$26,	$d4da,	$24,	$d4f1,	$0c,	$d5fd,	$23
dw	$d6c6,	$3e,	$d6d0,	$2f,	$d7a4,	$09,	$d7b0,	$25
dw	$d7d6,	$06,	$d7f7,	$31
CacheHzEndInit:
	org	CacheHzBegin+$140
dw	$2101,	$919,	$3f01,	$101,	$ff01,	$101,	$101,	$101
dw	$800,	$2018,	$f800,	0,	$fe04,	0,	0,	0
dw	$f08,	$2010,	$915f,	$1f11,	$1111,	$111f,	$2111,	$8041
dw	$e000,	$8840,	$8fc,	$f808,	$808,	$8f8,	$808,	$1028
dw	$404,	$4ff,	$1f01,	$1111,	$ff11,	$201,	$804,	$c030
dw	$4440,	$40fe,	$f000,	$1010,	$fe10,	$8000,	$3040,	$40e
dw	$102,	$ff01,	$810,	$408,	$204,	$201,	$804,	$c030
dw	0,	$fe04,	$1010,	$2020,	$8040,	$8000,	$1060,	$40e
dw	0,	0,	0,	0,	0,	$1800,	$18,	0
dw	0,	0,	$200,	$c06,	$3018,	$c060,	$80,	0
dw	$4908,	$82a,	$19ff,	$4a2c,	$fe10,	$2222,	$1814,	$4225
dw	$4040,	$8440,	$8fe,	$8888,	$9088,	$2050,	$8850,	$40e
dw	$102,	$203f,	$1f40,	0,	$ff01,	$101,	$101,	$205
dw	0,	$4fc,	$e048,	$8040,	$fe04,	0,	0,	0
dw	$4000,	$1037,	$6282,	$922,	$1009,	$6010,	$2221,	$2824
dw	$800,	$8fc,	$808,	$1010,	$a020,	$a040,	$810,	$40e
dw	1,	$213f,	$3f21,	$2422,	$2f28,	$2020,	$403f,	$8040
dw	$8800,	$fc,	$fc08,	$8000,	$f890,	$8480,	$80fe,	$8080
dw	$fc08,	$4b08,	$4848,	$4848,	$47d,	$1c04,	$44e4,	$817
dw	$2040,	$fe24,	$2020,	$2020,	$20fc,	$2020,	$2420,	$fe
dw	$7806,	$4840,	$4a46,	$170,	$213f,	$3f21,	$2121,	$203f
dw	$fe04,	$4444,	$4444,	$894,	$8f8,	$f808,	$808,	$8f8
dw	$4200,	$1332,	$402,	$10f7,	$1010,	$1111,	$2c12,	$344
dw	$4040,	$fc48,	$4440,	$a0fe,	$a0a0,	$2220,	$1e22,	$fe00
dw	$1310,	$1011,	$10fc,	$1a15,	$d035,	$1710,	$1010,	$2050
dw	$f800,	$a010,	$a040,	$4e10,	$40f4,	$fc48,	$4040,	$4040
dw	$3636,	$3636,	$3736,	$3730,	$3636,	$3636,	$3636,	$3636
dw	0,	0,	$ff00,	$ff00,	0,	0,	0,	0
dw	0,	$3800,	$c66c,	$d6c6,	$c6d6,	$6cc6,	$38,	0
dw	0,	$1800,	$7838,	$1818,	$1818,	$1818,	$7e,	0
dw	0,	0,	$1800,	$18,	0,	$1818,	0,	0
dw	0,	0,	$1800,	$18,	0,	$1818,	$30,	0
dw	$101,	$901,	$1109,	$2111,	$141,	1,	$201,	$700c
dw	0,	$2000,	$c10,	$1004,	$2010,	$8040,	0,	0
dw	$1f00,	$1f10,	$ff10,	$1f00,	$1f11,	$1f11,	$1f01,	$7f01
dw	$f810,	$f010,	$fe10,	$f000,	$f010,	$f010,	$f000,	$fc00
dw	0,	$c00,	$3018,	$3030,	$3030,	$1830,	$c,	0
dw	0,	$3000,	$c18,	$c0c,	$c0c,	$180c,	$30,	0
dw	0,	$3800,	$c060,	$fcc0,	$c6c6,	$c6c6,	$7c,	0
dw	0,	$fe00,	$6c6,	$c06,	$3018,	$3030,	$30,	0
dw	0,	$1e00,	$c0c,	$c0c,	$cc0c,	$cccc,	$78,	0
dw	0,	$e600,	$6666,	$786c,	$6c78,	$6666,	$e6,	0
dw	0,	$7c00,	$6c6,	$180c,	$6030,	$c6c0,	$fe,	0
dw	0,	$7c00,	$6c6,	$3c06,	$606,	$c606,	$7c,	0
dw	$1101,	$7f09,	$8f40,	$f08,	$1f01,	$1111,	$1111,	$101
dw	$1000,	$fe20,	$e402,	$e020,	$f000,	$1010,	$2050,	0
dw	$3f00,	$2121,	$3f21,	$2121,	$3f21,	$2121,	$4121,	$8041
dw	$fc08,	$808,	$f808,	$808,	$f808,	$808,	$808,	$1028
dw	0,	$7c00,	$c6c6,	$7cc6,	$c6c6,	$c6c6,	$7c,	0
dw	0,	$7c00,	$c6c6,	$7ec6,	$606,	$c06,	$78,	0
dw	0,	0,	0,	$ff00,	0,	0,	0,	0
dw	0,	0,	0,	$fe04,	0,	0,	0,	0
dw	$1710,	$2121,	$f949,	$2111,	$f941,	$202,	$e41c,	$249
dw	$fc08,	$808,	$1010,	$843c,	$4888,	$2050,	$8850,	$40e
dw	0,	$c00,	$3c1c,	$cc6c,	$fecc,	$c0c,	$1e,	0
dw	0,	$fe00,	$c0c0,	$fcc0,	$606,	$c606,	$7c,	0
dw	0,	$3f00,	0,	0,	0,	0,	$ff,	0
dw	0,	$f810,	0,	0,	0,	$400,	$fe,	0
dw	0,	$fe00,	$86c6,	$180c,	$6030,	$c6c2,	$fe,	0
dw	0,	$3c00,	$3030,	$3030,	$3030,	$3030,	$3c,	0
dw	0,	$1800,	$7e3c,	$1818,	$1818,	$1818,	$18,	0
dw	0,	$1800,	$1818,	$1818,	$1818,	$3c7e,	$18,	0
dw	0,	0,	$c080,	$70e0,	$1c38,	$60e,	2,	0
dw	0,	$3c00,	$c0c,	$c0c,	$c0c,	$c0c,	$3c,	0
dw	0,	0,	0,	0,	0,	$1818,	$3018,	0
dw	0,	0,	0,	0,	$fe,	0,	0,	0
dw	$4411,	$4411,	$4411,	$4411,	$4411,	$4411,	$4411,	$4411
dw	$aa55,	$aa55,	$aa55,	$aa55,	$aa55,	$aa55,	$aa55,	$aa55
dw	0,	0,	0,	0,	0,	0,	0,	0
dw	0,	$1800,	$3c3c,	$183c,	$1818,	$1800,	$18,	0
dw	$7f00,	$101,	$101,	$1111,	$1111,	$1111,	$1111,	$ff
dw	$fc08,	0,	0,	$f810,	0,	0,	$400,	$fe
dw	$202,	$ff02,	$404,	$808,	$3013,	$9050,	$1010,	$1017
dw	0,	$fe04,	$4000,	$4840,	$40fc,	$4040,	$4440,	$fe
dw	$4808,	$82b,	$2918,	$94a,	$2ff,	$c05,	$c534,	$406
dw	$4840,	$40fc,	$f840,	$400,	$fe,	$a010,	$3040,	$40e
dw	$808,	$87f,	$ff08,	$ff10,	$2820,	$87e,	$f80f,	$a09
dw	$5040,	$4848,	$fe40,	$4040,	$4848,	$2030,	$9260,	$60a
dw	$4000,	$2027,	$e003,	$202f,	$2223,	$2223,	$322b,	$222
dw	$4840,	$40fc,	$40f8,	$fe,	$8f8,	$8f8,	$8f8,	$1028
dw	$1d08,	$10f0,	$fd10,	$3911,	$5135,	$9151,	$1111,	$1111
dw	$2420,	$a8a4,	$fc20,	$404,	$4fc,	$fc04,	$404,	$814
dw	$1310,	$2820,	$a96f,	$2b29,	$282c,	$282f,	$2128,	$2422
dw	$f810,	$1010,	$fe,	$fc00,	$4440,	$40fe,	$10a0,	$40e
dw	0,	0,	0,	0,	0,	0,	0,	0
dw	$fc00,	$bc84,	$a0a0,	$a0a0,	$a0a0,	$e0a0,	0,	0
dw	$101,	$101,	$101,	$1909,	$4121,	$181,	$101,	$205
dw	0,	0,	0,	$1020,	$c08,	4,	0,	0
dw	$1010,	$4424,	$2fe,	$447c,	$7c44,	$4444,	$447c,	$4854
dw	$8880,	$a098,	$84c0,	$7c84,	$8800,	$a098,	$82c0,	$7e82
dw	$101,	$101,	$101,	$201,	$202,	$404,	$1008,	$4020
dw	0,	0,	0,	$8000,	$8080,	$4040,	$1020,	$40e
dw	0,	0,	$a0e,	$a0a,	$a0a,	$a0a,	$427a,	$7e
dw	0,	0,	0,	0,	0,	0,	0,	0
dw	$101,	$7f01,	$4141,	$4141,	$417f,	$101,	$101,	$101
dw	0,	$fe04,	$404,	$404,	$4fc,	0,	0,	0
dw	$2120,	$2121,	$27f8,	$2f24,	$e030,	$202f,	$2221,	$40a4
dw	$f810,	$f010,	$bc00,	$bca4,	$4440,	$e0fe,	$4e50,	$4044
dw	$909,	$1109,	$3212,	$9854,	$1010,	$1010,	$1010,	$1010
dw	0,	$fe04,	$8080,	$f890,	$8080,	$fc88,	$8080,	$8080
dw	$7f00,	$402,	$3f08,	$601,	$3f18,	$111,	$2111,	$245
dw	$c038,	$2000,	$8040,	$2000,	$f810,	8,	$810,	8
dw	$1010,	$2720,	$f948,	$2712,	$f941,	$141,	$e21a,	$844
dw	$4080,	$fc48,	$1080,	$fc08,	$2024,	$2020,	$2222,	$1e
dw	0,	$700,	$1008,	$1010,	$1010,	$708,	0,	0
dw	0,	$6000,	$40c0,	$4040,	$4040,	$30c0,	0,	0
dw	$4000,	$1437,	$6585,	$d25,	$1515,	$2165,	$2422,	$2028
dw	$404,	$54c4,	$5454,	$5454,	$5454,	$454,	$4484,	$854
dw	$4000,	$1037,	$6083,	$82f,	$1213,	$2263,	$2223,	$2222
dw	$4840,	$40fc,	$40f8,	$fe,	$8f8,	$8f8,	$8f8,	$1028
dw	$404,	$808,	$2918,	$84a,	$109,	$1ff,	$101,	$101
dw	$4840,	$6058,	$40c0,	$4444,	$3c,	$fe,	0,	0
dw	$101,	$101,	$ff01,	$201,	$202,	$404,	$1008,	$60
dw	0,	0,	$fe04,	$8000,	$4080,	$2040,	$e10,	4
dw	$1122,	$11,	$407f,	$1f80,	$100,	$1ff,	$101,	$205
dw	$1010,	$4420,	$4fe,	$e008,	$8440,	$fe,	0,	0
dw	1,	$203f,	$202f,	$203f,	$202f,	$282f,	$4848,	$88f
dw	$8800,	$80fc,	$88f8,	$88fe,	$80f8,	$8f8,	$808,	$8f8
dw	0,	$22fa,	$2023,	$f720,	$2020,	$2223,	$e23a,	$242
dw	$4040,	$4848,	$8f8,	$fe00,	$8040,	$a4fc,	$a4a4,	$ca4
dw	$7f02,	$3f02,	$ff04,	$808,	$281f,	$fc8,	$808,	$80f
dw	$fc00,	$f800,	$fe00,	$2040,	$2ef0,	$e024,	$2020,	$20e0
dw	$fd08,	$1010,	$2020,	$a77c,	$2424,	$2524,	$223d,	4
dw	$fc00,	$8888,	$8888,	$fe88,	$8888,	$888,	$808,	$808
dw	$2404,	$3f24,	$4424,	$4ff,	$3f04,	$2424,	$2526,	$404
dw	$404,	$a404,	$2424,	$24e4,	$a424,	$84a4,	$484,	$814
dw	$808,	$110f,	$2111,	$115f,	$1111,	$1ff,	$101,	$101
dw	$800,	$fc,	$1000,	$f8,	$400,	$fe,	0,	0
dw	$404,	$404,	$7c04,	$404,	$404,	$404,	$e41c,	$444
dw	$4040,	$4040,	$5844,	$4060,	$4040,	$4240,	$4242,	$3e
dw	$102,	$ff,	$1f00,	$1010,	$1f10,	$901,	$2119,	$245
dw	$400,	$fe,	$f000,	$1010,	$f010,	$2000,	$c18,	4
dw	$101,	$402,	$1008,	$c12f,	$1f01,	$1101,	$509,	$7f
dw	0,	$4080,	$1020,	$4ee,	$f000,	$1000,	$4020,	$fc
dw	$1f00,	$1010,	$1010,	$1f10,	$1010,	$1010,	$1f10,	$10
dw	$f810,	$1010,	$1010,	$f010,	$1010,	$1010,	$f010,	$10
dw	$4003,	$1030,	$203,	$13f2,	$1212,	$1213,	$2a12,	$344
dw	$10f8,	$40a0,	$48f8,	$f848,	$4848,	$48f8,	$5068,	$fe00
dw	0,	$404,	$808,	$2111,	$2c2,	$804,	$3f10,	$10
dw	$c080,	$4080,	$2040,	$e10,	4,	$4000,	$f020,	$10
dw	$3f00,	0,	$ff,	$3f00,	$2020,	$2020,	$203f,	0
dw	$fc08,	$4808,	$8e8,	$c888,	$8888,	$8888,	$8888,	$1028
dw	$101,	$101,	$101,	$101,	$101,	$101,	$101,	$ff
dw	0,	0,	$f810,	0,	0,	0,	$400,	$fe
dw	$4302,	$1432,	$628b,	$a22,	$120f,	$6412,	$2027,	$2020
dw	$fc00,	0,	$88f8,	$4848,	$88fe,	$4848,	$8fc,	$1028
dw	$202,	$ff02,	$904,	$1109,	$13f,	$909,	$2111,	$245
dw	0,	$fe04,	0,	$1000,	$f8,	$2040,	$1810,	8
dw	$4000,	$2020,	$f,	$27e0,	$2121,	$2121,	$3729,	$22
dw	$2820,	$2024,	$20fe,	$e020,	$2020,	$1020,	$ad2,	$40a
dw	$4808,	$4949,	$7d49,	$4141,	$4979,	$4a49,	$4c4a,	$b88
dw	$1c08,	$e0,	$fc00,	$8404,	$4848,	$2050,	$5020,	$48e
dw	$f00,	$808,	$f08,	$808,	$f08,	$808,	$1010,	$4020
dw	$f810,	$1010,	$f010,	$1010,	$f010,	$1010,	$1010,	$2050
dw	$101,	$2121,	$2121,	$213f,	$4101,	$4141,	$4141,	$407f
dw	0,	$808,	$808,	$8f8,	$400,	$404,	$404,	$4fc
dw	$1f00,	$1010,	$1f10,	$10,	$447c,	$4444,	$4444,	$447c
dw	$f810,	$1010,	$f010,	$10,	$447c,	$4444,	$4444,	$447c
dw	$2420,	$513E,	$3F88,	$101,	$213F,	$3F21,	$D03,	$171
dw	$4440,	$20FE,	$F810,	$808,	$F8,	$FE04,	$404,	$1028
