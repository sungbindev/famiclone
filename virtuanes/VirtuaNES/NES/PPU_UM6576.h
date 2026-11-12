/*******************************************************************************
 * PPU for UM6576
 *
 *  Author:  <87430545@qq.com>
 *
 *  Create:   2024-05-17, by fanoble
 *******************************************************************************
 */

#ifndef	__PPU_UM6576_INCLUDED__
#define	__PPU_UM6576_INCLUDED__

#include "typedef.h"
#include "macro.h"

// class prototype
class	NES;

class	PPU_UM6576 : public PPU
{
public:
	PPU_UM6576( NES* parent );
	virtual	~PPU_UM6576();

	void	Reset();

	BYTE	Read ( WORD addr );
	void	Write( WORD addr, BYTE data );

	void	DMA( BYTE data );

	void	VBlankStart();
	void	VBlankEnd();

	void	FrameStart();
	void	FrameEnd();

	void	SetRenderScanline( INT scanline );

	void	ScanlineStart();
	void	ScanlineNext();

	WORD	GetPPUADDR();
	WORD	GetTILEY();

	BYTE	PaletteLookup(BYTE index, BOOL isBG) {
		if (isBG)
			return m_bPalette[index & 0x3F];
		else
			return m_bPalette[(index & 0x0F) + 0x10];
	}

	INT		GetColorMode() { return (m_bReg2008 & 0x80) ? 16 : 4; }

	// Scanline base render
	void	Scanline( INT scanline, BOOL bMax, BOOL bLeftClip );
	void	DummyScanline( INT scanline );

	// For mapper
	void	SetExtLatchMode( BOOL bMode );
	void	SetChrLatchMode( BOOL bMode );
	void	SetExtNameTableMode( BOOL bMode );
	void	SetExtMonoMode( BOOL bMode );
	BOOL	GetExtMonoMode();

	void	SetVromWrite(BOOL bMode);
	BOOL	IsVromWrite();

	BOOL	IsNMIEnabled();
	BOOL	IsDispON();
	BOOL	IsBGON();
	BOOL	IsSPON();

	BYTE	GetBGCOLOR();

	BOOL	IsBGCLIP();
	BOOL	IsSPCLIP();

	BOOL	IsMONOCROME();

	BOOL	IsVBLANK();
	BOOL	IsSPHIT();
	BOOL	IsSPMAX();
	BOOL	IsPPUWE();

	BOOL	IsSprite0( INT scanline );

	void	SetScreenPtr( LPBYTE lpScn, LPBYTE lpMode );
	LPBYTE	GetScreenPtr();

	INT	GetScanlineNo();

	// For VS-Unisystem
	void	SetVSMode( BOOL bMode );
	void	SetVSSecurity( BYTE data );
	void	SetVSColorMap( INT nColorMap );

	void	SetVariant( PPUVARIANT var ) {}

protected:
	NES*	nes;

	typedef	struct	tagSPRITE {
		BYTE	y;
		BYTE	tile;
		BYTE	attr;
		BYTE	x;
	} SPRITE, *LPSPRITE;

	// Reversed bits
	BYTE	Bit2Rev[256];

private:
	WORD	GA;

	LPBYTE	lpScreen;
	LPBYTE	lpScanline;
	INT		nScanlineNo;
	LPBYTE	lpColormode;

	BYTE	m_bRegPosX;
	BYTE	m_bRegPosY;

	BOOL	m_bW2005Toggle;
	BOOL	m_bW2006Toggle;
	BYTE	m_nVramReg;

	BYTE	m_bReg2000;
	BYTE	m_bReg2001;
	BYTE	m_bReg2002;
	BYTE	m_bReg2003;
	BYTE	m_bReg2008;
	BYTE	m_bPalette[64];
};

#endif	// !__PPU_UM6576_INCLUDED__

