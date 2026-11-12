//////////////////////////////////////////////////////////////////////////
// BBK FD1 Keyboard & Mouse                                             //
//////////////////////////////////////////////////////////////////////////
class	EXPAD_BBK_FD1 : public EXPAD
{
public:
	EXPAD_BBK_FD1( NES* parent ) : EXPAD( parent ) {}

	void	Reset();

	void	Strobe();

	BYTE	Read4016();
	BYTE	Read4017();
	void	Write4016( BYTE data );

	void	SetMouseStatus( INT nMouseX, INT nMouseY, INT nKeys, INT nFlags );

protected:
	BOOL	bOut;
	BYTE	ScanNo;

	BOOL	bMouseActived;

	int		nMouseKey;
	int		nMouseDeltaX;
	int		nMouseDeltaY;

private:
	BYTE	nPrev$4016;
	int		nRead$4016Count;
	int		nRead$4017Count;
	int		nEM84502_State;
	int		nEM84502_Command;
	BYTE	nEM84502_BitCount;
	int		nEM84502_TxData;
	BYTE	nEM84502_TxCount;
	BYTE	nEM84502_Data[3];
};
