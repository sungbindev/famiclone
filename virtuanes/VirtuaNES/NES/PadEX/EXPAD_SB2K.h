//////////////////////////////////////////////////////////////////////////
// SB2K Keyboard & Mouse                                             //
//////////////////////////////////////////////////////////////////////////
class	EXPAD_SB2K : public EXPAD
{
public:
	EXPAD_SB2K( NES* parent ) : EXPAD( parent )
	{
		kbd_data_ready = 0;
		kbd_rsp_delay  = 0;
		kbd_ret_delay  = 0;

		mouse_data_ready = 0;
		mouse_ret_delay  = 0;
	}

	void	Reset();

	void	Strobe();

	BYTE	Read4016();
	BYTE	Read4017();
	void	Write4016( BYTE data );

	void	SetMouseStatus( INT nMouseX, INT nMouseY, INT nKeys, INT nFlags );

	void	Clock(INT nCycles);
	void	MouseProc(INT nCycles);
	void	KeyboardProc(INT nCycles);
	void	ExWrite(INT nCode, INT nData);
	INT		ExRead(INT nCode);
protected:
	int		nMouseKey;
	int		nMouseDeltaX;
	int		nMouseDeltaY;

	int		kbd_initialized;

	int		kbd_data_ready;
	int		kbd_tx_data;
	int		kbd_rsp_delay;

	int		kbd_cmd;
	int		kbd_rx_option;

	int		kbd_led_status;
	int		kbd_scan_code_set;

	int		kbd_report_timer;

	int		kbd_ret_delay;
	int		kbd_ret_delay_reload;
	int		kbd_ret_index;
	int		kbd_ret_length;
	int		kbd_ret_data[16];

	int		mouse_initialized;

	int		mouse_data_ready;
	int		mouse_tx_data;
	int		mouse_report_timer;

	int		mouse_ret_delay;
	int		mouse_ret_delay_reload;
	int		mouse_ret_index;
	int		mouse_ret_length;
	BYTE	mouse_ret_data[32];

	BYTE	kbd_key_status[256];
	BYTE	kbd_key_changed[256];
private:
};
