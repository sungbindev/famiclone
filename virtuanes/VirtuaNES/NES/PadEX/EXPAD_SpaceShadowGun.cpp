//////////////////////////////////////////////////////////////////////////
// Zapper                                                               //
//////////////////////////////////////////////////////////////////////////
void	EXPAD_SpaceShadowGun::Reset()
{
	bomb_bits = 0;
	zapper_button = 0;
}

void	EXPAD_SpaceShadowGun::Strobe()
{
	// 显式转换为 BYTE，避免从更宽整型到 BYTE 的窄化警告 (C4244)
	bomb_bits = (BYTE)(nes->pad->pad1bit & 0xFC);
	bomb_bits |= (BYTE)(zapper_button & 0x02);
}

BYTE	EXPAD_SpaceShadowGun::Read4016()
{
BYTE	data = (bomb_bits & 0x01)<<1;
	bomb_bits >>= 1;
	return	data;
}

BYTE	EXPAD_SpaceShadowGun::Read4017()
{
BYTE	data = 0x08;

	if( zapper_button & 0x01 ) {
		data |= 0x10;
	}

	if( nes->GetZapperHit() ) {
		if( DirectDraw.GetZapperHit() >= 0xFE )
			data &= ~0x08;
	}
	return	data;
}

void	EXPAD_SpaceShadowGun::Sync()
{
	nes->GetZapperPos( zapper_x, zapper_y );

	zapper_button = 0;
	if( ::GetAsyncKeyState(VK_LBUTTON)&0x8000 )
		zapper_button |= 0x01;
	if( ::GetAsyncKeyState(VK_RBUTTON)&0x8000 )
		zapper_button |= 0x02;
}

void	EXPAD_SpaceShadowGun::SetSyncData( INT type, LONG data )
{
	if( type == 0 ) {
		zapper_x = data;
	} else if( type == 1 ) {
		zapper_y = data;
	} else if( type == 2 ) {
		// 将 LONG 参数显式截断为 BYTE，避免窄化警告 (C4244)
		zapper_button = (BYTE)data;
	}
}

LONG	EXPAD_SpaceShadowGun::GetSyncData( INT type )
{
LONG	data = 0;

	if( type == 0 ) {
		data = zapper_x;
	} else if( type == 1 ) {
		data = zapper_y;
	} else if( type == 2 ) {
		data = zapper_button;
	}
	return	data;
}

