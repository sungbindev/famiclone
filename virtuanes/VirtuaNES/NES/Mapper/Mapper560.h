//////////////////////////////////////////////////////////////////////////
// Mapper560          C/E basic                                         //
//////////////////////////////////////////////////////////////////////////

class	Mapper560 : public Mapper
{
public:
	Mapper560( NES* parent ) : Mapper(parent) {}

	void	Reset();
	void	Write(WORD addr, BYTE data);
	void	PPU_getNTdata( INT bank, WORD addr, BYTE data );
	void	PPU_ExtLatch( WORD ntbladr, BYTE& chr_l, BYTE& chr_h, BYTE& attr );

protected:
	BYTE	Rom_Type;
	BYTE	mode;
	BYTE	OPT1, OPT2;
private:
	void	CRAM_CE2SB();
	void	VROM_SB2CE();
};
