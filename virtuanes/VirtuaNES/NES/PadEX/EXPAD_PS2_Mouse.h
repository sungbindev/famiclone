//////////////////////////////////////////////////////////////////////////
//      PS/2_Mouse                                                       //
//////////////////////////////////////////////////////////////////////////

class	EXPAD_PS2_Mouse : public EXPAD
{
public:
	EXPAD_PS2_Mouse( NES* parent ) : EXPAD( parent ) {}

	void	Reset();
	BYTE	Read4016();
	BYTE	Read4017();
	void	Write4016( BYTE data );

	void	Sync();
	void	SetMouseStatus( INT nMouseX, INT nMouseY, INT nKeys, INT nFlags );
	void	Clock(INT nCycles);
	void	CPUCycle();

protected:
	LONG	mouse_x, mouse_y;
	BYTE	mouse_button;

	BOOL	bOut;
	BYTE	ScanNo;

private:
};
