//////////////////////////////////////////////////////////////////////////
// Mapper178  Education / WaiXing / HengGe                              //
//////////////////////////////////////////////////////////////////////////

class	Mapper178 : public Mapper
{
public:
	Mapper178( NES* parent ) : Mapper(parent) {}

	void	Reset();
	void	WriteLow(WORD addr, BYTE data);

protected:
	BYTE reg[4];
	BYTE OP_rom;
private:
	void	SetBank_CPU();
};
