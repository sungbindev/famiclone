//////////////////////////////////////////////////////////////////////////
// Mapper163  NanJing Games                                             //
//////////////////////////////////////////////////////////////////////////

class	Mapper163 : public Mapper
{
public:
	Mapper163( NES* parent ) : Mapper(parent) {}

	void	Reset();
	BYTE	ReadLow ( WORD addr );
	void	WriteLow(WORD addr, BYTE data);
	void	HSync( INT scanline );

protected:
	BYTE	reg5000, reg5100, reg5200, reg5300;
	BYTE	strobe, trigger;
	BYTE	rom_type;

private:

};
