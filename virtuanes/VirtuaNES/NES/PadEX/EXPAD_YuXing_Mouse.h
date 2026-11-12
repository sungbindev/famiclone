//////////////////////////////////////////////////////////////////////////
// YuXing_Mouse                                                         //
//////////////////////////////////////////////////////////////////////////

#include <queue>

class	EXPAD_YuXing_Mouse : public EXPAD
{
public:
	EXPAD_YuXing_Mouse( NES* parent ) : EXPAD( parent ) {}

	void	Reset();
	BYTE	Read4016();
	BYTE	Read4017();
	void	Write4016( BYTE data );
	void	Sync();

	static void	AddByte(std::queue<bool>& to, BYTE data );
	void	SetMouseStatus( INT nMouseX, INT nMouseY, INT nKeys, INT nFlags );

protected:
	LONG	mouse_x, mouse_y, mouse_x_move, mouse_y_move;
	BYTE	mouse_button;
	BOOL	mouse_clock;
	std::queue<bool> bits;

	BOOL	bOut;
	BYTE	ScanNo;
private:

};
