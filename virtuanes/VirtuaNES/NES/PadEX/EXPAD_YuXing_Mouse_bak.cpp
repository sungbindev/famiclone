//////////////////////////////////////////////////////////////////////////
// YuXing_Mouse                                                         //
//////////////////////////////////////////////////////////////////////////

void	EXPAD_YuXing_Mouse::Reset()
{
	mouse_button = 0;
	mouse_x = mouse_y = 0;
	mouse_x_old = mouse_y_old = 0;
	mouse_x_move = mouse_y_move = 0;
	reg[0] = 0;
	reg[1] = 0;
	reg[2] = 0;	//Read401X函数调用次数计数器
	reg[3] = 0;	//鼠标数据获取计数器
}

BYTE	EXPAD_YuXing_Mouse::Read4016()
{
	return	mouse_data;
}

BYTE	EXPAD_YuXing_Mouse::Read4017()
{
	return	mouse_data;
}

void	EXPAD_YuXing_Mouse::Write4016( BYTE data )
{
	if (~data&4 && data&1) {
		mouse_data = SetMouse();
	}
}

void	EXPAD_YuXing_Mouse::Write4017( BYTE data )
{
	;
}

void	EXPAD_YuXing_Mouse::Sync()
{
	mouse_button = 0;
	if( ::GetAsyncKeyState(VK_LBUTTON)&0x8000 )
		mouse_button |= 0x01;
	if( ::GetAsyncKeyState(VK_RBUTTON)&0x8000 )
		mouse_button |= 0x02;

	mouse_x_old = mouse_x;
	mouse_y_old = mouse_y;
	nes->GetZapperPos( mouse_x, mouse_y );
	mouse_x_move=(mouse_x-mouse_x_old)&0xFF;
	mouse_y_move=(mouse_y-mouse_y_old)&0xFF;

}

BYTE	EXPAD_YuXing_Mouse::SetMouse()
{
	BYTE data = 0x40;

	if((mouse_x==mouse_x_old)&&((mouse_y==mouse_y_old))&&(mouse_button==0)){
		reg[0]=0;reg[1]=0;reg[2]=0;reg[3]=0;
		return 0x40;	//鼠标没输入，初始化计数器
	}

	reg[2]++;	//鼠标开始有输入后，记录调用Read401X函数的次数
	switch(reg[2]){
		case	0x01:
			return 0x41;
			break;
		case	0x09:
			reg[2]=0;
			reg[3]++;
			if(reg[3]==3) reg[3]=0;
			return 0x40;
			break;
	}

	switch(reg[3]){
		case	0x00:	//第一次获取按键值和移动方向属性值
			if(mouse_button&0x01) data|=0x20;
			if(mouse_button&0x02) data|=0x10;
			data|=mouse_x_move>>4&0x0C;
			data|=mouse_y_move>>6&0x03;
			DEBUGOUT( "data = %04X\n", data );
			break;
		case	0x01:	//第二次获取鼠标垂直移动变化量
			data=mouse_y_move&0x3F;
			break;
		case	0x02:	//第三次获取鼠标水平移动变化量
			data=mouse_x_move&0x3F;
			break;
	}
	return (((data&0x7F)>>(8-reg[2]))&1)^1;
}
