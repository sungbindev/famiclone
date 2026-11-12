/*----------------------------------------------------------------------*/
/*                                                                      */
/*      NES Mapper                                                      */
/*                                                           Norix      */
/*                                               written     2001/02/05 */
/*                                               last modify ----/--/-- */
/*----------------------------------------------------------------------*/
#ifndef	__MAPPER_INCLUDED__
#define	__MAPPER_INCLUDED__

#define	WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "DirectSound.h"

#include "typedef.h"
#include "macro.h"

// class prototype
class	NES;

// Mapper base class
class	Mapper
{
public:
	Mapper( NES* parent );
	virtual	~Mapper();

	// For Mapper
	// Reset
	virtual	void	Reset() = 0;

	virtual	BYTE	Peek ( INT addr, BOOL is_logical ) {
		return CPU_MEM_BANK[(addr & 0xFFFF) >> 13][addr & 0x1FFF];
	}

	// $8000-$FFFF Memory write
	virtual	void	Write( WORD addr, BYTE data ) {}

	// $8000-$FFFF Memory read(Dummy)
	virtual	void	Read( WORD addr, BYTE data ) {}
	virtual	BOOL	ReadHigh( WORD addr, LPBYTE pdata ) { return FALSE; }

	// $4100-$7FFF Lower Memory read/write
	virtual	BYTE	ReadLow ( WORD addr );
	virtual	void	WriteLow( WORD addr, BYTE data );

	// $4018-$40FF Extention register read/write
	virtual	BYTE	ExRead ( WORD addr )	{ return 0x00; }
	virtual	void	ExWrite( WORD addr, BYTE data ) {}

	virtual	BYTE	PPU_Peek ( INT addr, BOOL is_logical ) {
		if (addr >= 0x3000)
			return 0xEE;
		else
			return PPU_MEM_BANK[addr >> 10][addr & 0x3FF];
	}

	virtual	BYTE	PPU_Read ( WORD addr ) { return 0; }
	virtual	void	PPU_Write( WORD addr, BYTE data ) {}

	// Extension commands
	// For ExCmdRead command
	enum	EXCMDRD {
		EXCMDRD_NONE = 0,
		EXCMDRD_DISKACCESS,
	};
	// For ExCmdWrite command
	enum	EXCMDWR {
		EXCMDWR_NONE = 0,
		EXCMDWR_DISKINSERT,
		EXCMDWR_DISKEJECT,
	};

	virtual	BYTE	ExCmdRead ( EXCMDRD cmd )	{ return 0x00; }
	virtual	void	ExCmdWrite( EXCMDWR cmd, BYTE data ) {}

	// speed up
	virtual	BOOL	IsThrottleRequired() { return FALSE; }

	// H sync/V sync/Clock sync
	virtual	void	HSync( INT scanline ) {}
	virtual	void	VSync() {}
	virtual	void	Clock( INT cycles ) {}

	// PPU address bus latch
	virtual	void	PPU_Latch( WORD addr ) {}

	// PPU Character latch
	virtual	void	PPU_ChrLatch( WORD addr ) {}

	// PPU Extension character/palette
	virtual	void	PPU_ExtLatchX( INT x ) {}
	virtual	void	PPU_ExtLatch( WORD addr, BYTE& chr_l, BYTE& chr_h, BYTE& attr ) {}

	// For State save
	virtual	BOOL	IsStateSave() { return FALSE; }
	virtual	void	SaveState( LPBYTE p ) {}
	virtual	void	LoadState( LPBYTE p ) {}

	//for YuXing and PYRAMID
	virtual	BYTE	PPU_ExtLatchSP() { return 0; }

	//Get PPU NameTable data
	virtual	void	PPU_getNTdata( INT bank, WORD addr, BYTE data ) {}

	// added by fanoble for debug
	virtual LPBYTE  GetPatternTable( int scanline, int addr ) { return 0; }
	virtual LPBYTE  GetExtPMEM( int addr ) { return 0; }
	virtual LPBYTE  GetExtVMEM( int addr ) { return 0; }
	virtual INT     GetBankNo( int addr ) {return 0; }

protected:
	NES*	nes;

private:
};

// Create class instance
extern	Mapper*	CreateMapper( NES* parent, INT no, INT variant );

#endif
