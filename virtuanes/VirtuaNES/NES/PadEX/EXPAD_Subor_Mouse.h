//////////////////////////////////////////////////////////////////////////
//      Subor Mouse                                                     //
//////////////////////////////////////////////////////////////////////////

class	EXPAD_Subor_Mouse : public EXPAD
{
public:
	EXPAD_Subor_Mouse( NES* parent ) : EXPAD( parent ) {}

	void	Reset();

	BYTE	Read4016();
	BYTE	Read4017();
	void	Write4016( BYTE data );

	void	Sync();

	void	SetMouseStatus( INT nMouseX, INT nMouseY, INT nKeys, INT nFlags );

protected:
	INT		mouse_x, mouse_y;

	BOOL	bOut;
	BYTE	ScanNo;
private:
};
