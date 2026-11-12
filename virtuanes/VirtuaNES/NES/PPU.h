//////////////////////////////////////////////////////////////////////////
//                                                                      //
//      NES PPU core                                                    //
//                                                           Norix      //
//                                               written     2001/02/22 //
//                                               last modify ----/--/-- //
//////////////////////////////////////////////////////////////////////////
#ifndef	__PPU_INCLUDED__
#define	__PPU_INCLUDED__

#include "typedef.h"
#include "macro.h"

// class prototype
class	NES;

class	PPU
{
public:
	PPU( NES* parent );
	virtual	~PPU();

	enum { SCREEN_WIDTH = 256+16, SCREEN_HEIGHT = 240 };

	virtual void Reset() = 0;

	virtual BYTE	Read ( WORD addr ) = 0;
	virtual void	Write( WORD addr, BYTE data ) = 0;

	virtual void	DMA( BYTE data ) = 0;

	virtual void	VBlankStart() = 0;
	virtual void	VBlankEnd() = 0;

	virtual void	FrameStart() = 0;
	virtual void	FrameEnd() = 0;

	virtual void	SetRenderScanline( INT scanline ) = 0;

	virtual void	ScanlineStart() = 0;
	virtual void	ScanlineNext() = 0;

	virtual WORD	GetPPUADDR() = 0;
	virtual WORD	GetTILEY() = 0;

	virtual BYTE	PaletteLookup(BYTE index, BOOL isBG) = 0;
	virtual INT		GetColorMode() = 0;

	// Scanline base render
	virtual void	Scanline( INT scanline, BOOL bMax, BOOL bLeftClip ) = 0;
	virtual void	DummyScanline( INT scanline ) = 0;

	// For mapper
	virtual void	SetExtLatchMode( BOOL bMode ) = 0;
	virtual void	SetChrLatchMode( BOOL bMode ) = 0;
	virtual void	SetExtNameTableMode( BOOL bMode ) = 0;
	virtual void	SetExtMonoMode( BOOL bMode ) = 0;
	virtual BOOL	GetExtMonoMode() = 0;

	virtual void	SetVromWrite(BOOL bMode) = 0;
	virtual BOOL	IsVromWrite() = 0;

	virtual BOOL	IsNMIEnabled() = 0;
	virtual BOOL	IsDispON() = 0;
	virtual BOOL	IsBGON() = 0;
	virtual BOOL	IsSPON() = 0;

	virtual BYTE	GetBGCOLOR() = 0;

	virtual BOOL	IsBGCLIP() = 0;
	virtual BOOL	IsSPCLIP() = 0;

	virtual BOOL	IsMONOCROME() = 0;

	virtual BOOL	IsVBLANK() = 0;
	virtual BOOL	IsSPHIT() = 0;
	virtual BOOL	IsSPMAX() = 0;
	virtual BOOL	IsPPUWE() = 0;

	virtual BOOL	IsSprite0( INT scanline ) = 0;

	virtual void	SetScreenPtr( LPBYTE lpScn, LPBYTE lpMode ) = 0;
	virtual LPBYTE	GetScreenPtr() = 0;

	virtual INT	GetScanlineNo() = 0;

	// For VS-Unisystem
	virtual void	SetVSMode( BOOL bMode ) = 0;
	virtual void	SetVSSecurity( BYTE data ) = 0;
	virtual void	SetVSColorMap( INT nColorMap ) = 0;

	enum PPUVARIANT {
		PPUVARIANT_STD = 0,
		PPUVARIANT_UM6576
	};
	virtual void	SetVariant( PPUVARIANT var ) = 0;
protected:
	NES*	nes;

	BOOL	bExtLatch;	// For MMC5
	BOOL	bChrLatch;	// For MMC2/MMC4
	BOOL	bExtNameTable;	// For Super Monkey no Dai Bouken
	BOOL	bExtMono;	// For Final Fantasy
	BOOL	bVromWe;	// For Mapper74 and others

	WORD	loopy_y;
	WORD	loopy_shift;

	LPBYTE	lpScreen;
	LPBYTE	lpScanline;
	INT	ScanlineNo;
	LPBYTE	lpColormode;

	typedef	struct	tagSPRITE {
		BYTE	y;
		BYTE	tile;
		BYTE	attr;
		BYTE	x;
	} SPRITE, *LPSPRITE;

	// Reversed bits
	BYTE	Bit2Rev[256];

	PPUVARIANT m_Variant;

	// For VS-Unisystem
	BOOL	bVSMode;
	BYTE	VSSecurityData;
	INT	nVSColorMap;
	static	BYTE	VSColorMap[5][64];
private:
};


#endif	// !__PPU_INCLUDED__

