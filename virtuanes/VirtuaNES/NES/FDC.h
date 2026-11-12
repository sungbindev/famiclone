/*******************************************************************************
 *  Floppy Disk Controller for SB2K/BBK/YuXing（uPD765 兼容）
 *
 *  Author:  fanoble <87430545@qq.com>
 *
 *  Create:   Nov 6, 2022, by fanoble
 *
 *  控制器状态机取自 NintendulatorNRS（NewRisingSun）的 SimpleFDC，各机型的映射器经
 *  readIO/writeIO 等接口使用；盘镜像的读写与脏标志在本类里。
 *******************************************************************************
 */

//==============================================================================
#include <queue>
#include <string>
#include <vector>
struct Drive {
	bool	exists;
	bool	ready;
	bool	inserted;
	bool	running;
	bool	writeProtected;
	bool	changed;
	int	cylinder;
	int	cylinders;
	int	sides;
	int	sectors;
	int	correctDataRate;
	unsigned char *data;
	bool	*modifiedFlag;
};
#pragma warning(disable:4121)	// "alignment of a member was sensitive to packing" - most useless warning ever
class FDC;
struct CommandInfo {
	unsigned char length;
	void (FDC::*handler)(void);
};
//==============================================================================

class FDC {

//==============================================================================
	Drive drives[4], *drive;
	unsigned char DOR, MSR, ST0, ST1, ST2, SRA, CONFIG, latch;
	unsigned char cylinder, head, sectorNumber, *data;
	bool	resetPin, useDMA, dmaRequest, dmaTerminate;
	int	dataRate, sectorSize, bytesLeft;
	void	(FDC::*phaseHandler)(void);
	std::vector<unsigned char> command;
	std::queue<unsigned char> resultQueue;
	static const CommandInfo commandInfo[0x20];
	// Polling
	int	pollTimer;
	unsigned char	pollDrive;
	bool	previouslyReady[4];
	void	phCommand (void);
	void	phResult (void);
	void	endCommand (bool, int, ...);
	void	nextSector (void);
	void	nextSectorFormat (void);
	void	cmReadID (void);
	void	cmReadData (void);
	void	cmWriteData (void);
	void	cmFormatTrack (void);
	void	cmSeek (void);
	void	cmRecalibrate (void);
	void	cmSenseInterruptStatus (void);
	void	cmSpecify (void);
	void	cmSenseDriveStatus (void);
	void	cmConfigure (void);
	void	cmInvalid (void);
	void	pollDrives (void);
//==============================================================================

public:
	FDC();
	~FDC();
	// 盘镜像
	int LoadDiskImage(const char* fname);
	int SaveDiskImage();
	int IsDirty() { return bDirty; }
	int IsPresent() { return (pDiskImage != 0); }

	// 机型标记，由各机型的映射器在复位时设置
	bool isYuXing;
	bool isSB97;
	bool isBBK;
	// 调试服务的故障注入，默认全部关闭
	bool	dbgNoIndex;	// 没有索引脉冲：READ ID、读写、格式化命令一直不结束
	bool	dbgSeekFail;	// 磁头卡住：SEEK 与 RECALIBRATE 不移动
	bool	dbgDead;	// 控制器不响应：收到命令字节后不再处理，主状态一直不就绪
	std::vector<int>	dbgBadLba;	// 读时数据校验出错的逻辑扇区号
	void	DbgEject();	// 取出盘片（镜像仍留在内存里）
	int	DbgInsert();	// 放回盘片，没有加载过镜像时返回 0
	void	DbgSetWriteProtect(bool protect);
	void	DbgClearFaults();	// 关闭全部故障，放回盘片并取消写保护
	std::string	DbgStatus();

//==============================================================================
	void	insertDisk (unsigned char, unsigned char*, size_t, bool, bool*);
	void	ejectDisk (unsigned char);
	void	reset();
	void	run (void);
	int	readIO (int);
	int	readIODebug (int);
	void	writeIO (int, int);
	bool	irqRaised (void) const;
	bool	haveDMARequest (void) const;
	void	setTerminalCount (void);
	void	sendToDMA (unsigned char);
	unsigned char	receiveFromDMA (void);
	void	setDriveMask (unsigned char);
	void	setResetPin (bool);
//==============================================================================

protected:
	int            bDirty;
	bool           bDiskModified;
	unsigned char* pDiskImage;
	int            nDiskSize;
	char           szDiskName[2048];
};

/*******************************************************************************
                           E N D  O F  F I L E
*******************************************************************************/
