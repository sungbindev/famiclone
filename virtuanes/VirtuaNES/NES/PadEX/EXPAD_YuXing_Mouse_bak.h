//////////////////////////////////////////////////////////////////////////
// YuXing_Mouse                                                         //
//////////////////////////////////////////////////////////////////////////

class	EXPAD_YuXing_Mouse : public EXPAD
{
public:
	EXPAD_YuXing_Mouse( NES* parent ) : EXPAD( parent ) {}

	void	Reset();
	BYTE	Read4016();
	BYTE	Read4017();
	void	Write4016( BYTE data );
	void	Write4017( BYTE data );
	void	Sync();

protected:
	LONG	mouse_x, mouse_y, mouse_x_old, mouse_y_old, mouse_x_move, mouse_y_move;
	BYTE	mouse_button, mouse_bits, mouse_data;
	BYTE	reg[8];

private:
	BYTE	SetMouse();

};
