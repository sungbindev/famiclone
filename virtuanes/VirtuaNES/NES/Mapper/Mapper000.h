//////////////////////////////////////////////////////////////////////////
// Mapper000                                                            //
//////////////////////////////////////////////////////////////////////////
class	Mapper000 : public Mapper
{
public:
	Mapper000( NES* parent ) : Mapper(parent) {}

	void	Reset();
	void	Write(WORD addr, BYTE data);
	void	PPU_Latch( WORD addr );

protected:
	INT		SP_ROM;
	BYTE	regFFFF, NT_data;
private:
};
