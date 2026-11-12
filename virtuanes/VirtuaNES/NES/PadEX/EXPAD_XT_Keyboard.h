//////////////////////////////////////////////////////////////////////////
// 8088 XT Keyboard                                                     //
//                                                   written by XXXXXX  //
//                                                     last 2025/06/08  //
//////////////////////////////////////////////////////////////////////////

class	EXPAD_XT_Keyboard : public EXPAD
{
public:
	EXPAD_XT_Keyboard( NES* parent ) : EXPAD( parent ) {}

	void	Reset();

	BYTE	Read4016();
	BYTE	Read4017();
	void	Write4016( BYTE data );
	void	Sync();

protected:
	bool bOut, Enabled;
	bool r_LSB, r_MSB;
	unsigned char ScanNo, ScanNo_pre;
	bool b1, b2;

private:
};
