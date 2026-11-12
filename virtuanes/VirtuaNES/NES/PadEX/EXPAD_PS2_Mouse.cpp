//////////////////////////////////////////////////////////////////////////
//      PS/2_Mouse                                                       //
//////////////////////////////////////////////////////////////////////////
// code form NintendulatorNRS by NewRisingSun.

#include <queue>

//------------------------------------------------------------------------------
#define PS2_CLOCKS 70 // Too low, and BBKDOS' mouse driver will fail.
class PS2DevicePort {
	bool hostClock;
	bool hostData;
	bool deviceClock;
	bool deviceData;
	int state;
	int cycles;
	unsigned char latch;
	unsigned char oneBits;
	void setDeviceClock(bool);
	void setDeviceData(bool);
	void setNextState(int);
	void checkForSendRequest();
public:
	std::queue<unsigned char> dataFromHost;
	std::queue<unsigned char> dataToHost;
	void setHostClock(bool);
	void setHostData(bool);
	bool getClock() const;
	bool getData() const;
	void cpuCycle();
	PS2DevicePort();
};
PS2DevicePort::PS2DevicePort():
	hostClock(true),
	hostData(true),
	deviceClock(true),
	deviceData(true),
	state(0),
	cycles(0),
	latch(0),
	oneBits(0) {
}
void PS2DevicePort::setHostClock(bool setting) {
	hostClock =setting;	
}
void PS2DevicePort::setHostData(bool setting) {
	hostData =setting;	
}

void PS2DevicePort::setDeviceClock(bool setting) {
	deviceClock =setting;	
}
void PS2DevicePort::setDeviceData(bool setting) {
	deviceData =setting;	
}
bool PS2DevicePort::getClock() const {
	return hostClock && deviceClock;
}
bool PS2DevicePort::getData() const {
	return hostData && deviceData;
}
void PS2DevicePort::setNextState(int newState) {
	cycles =PS2_CLOCKS;
	state =newState;
}
void PS2DevicePort::checkForSendRequest() {
	setDeviceData(true); // Release DATA line
	oneBits =0;
	// If host is sending a Start Bit, start clocking and receiving from host
	if (!getData())
		setNextState(100);
	else			
	if (!dataToHost.empty()) {
		// If host is not sending a Start Bit, and we have something to send, then start clocking and sending to host
		latch =dataToHost.front();
		dataToHost.pop();
		setNextState(200);
	} else {
		state =0;
	}
}
void PS2DevicePort::cpuCycle() {
	if (cycles) {
		if (!--cycles) {
			setDeviceClock(!deviceClock);
			if (!deviceClock) cycles =PS2_CLOCKS;
		}
	} else
	if (deviceClock) {
		if (!getClock()) { // If CLOCK should be high but is low, then host is holding CLOCK low, which means Terminate Command and Inhibit Communication
			state =0;
		} else {
			switch(state) {
			case 0: // Idle state, check if it needs to change
				checkForSendRequest();
				break;		
			case 100: // Receive the start bit (0) from host
				setNextState(getData()? 0: state +1);
				break;
			case 101: case 102: case 103: case 104: case 105: case 106: case 107: case 108: // Receive eight data bits from host
				latch =latch >>1 | getData()*0x80;
				if (getData()) oneBits++;
				setNextState(state +1);
				break;
			case 109: // Receive parity bit from host
				if (getData() !=!!(~oneBits &1)) { // Wrong parity bit
					//dataToHost.push(0xFE); // Re-send
				}
				setNextState(state +1);
				break;
			case 110: // Receive stop bit from host
				if (getData()) {					
					setDeviceData(false); // Line-control bit
					dataFromHost.push(latch);
				} else { // Wrong stop bit
					setDeviceData(true);
					dataToHost.push(0xFE);
				}
				setNextState(state +1);
				break;
			case 111: // End
				setDeviceData(true); // Release data line to end transmission
				checkForSendRequest();
				break;
			case 200: // Send start bit to host
				setDeviceData(true); // Check if line held
				if (deviceData && !getData()) { // Data reception interrupted by host wanting to send data
					while (!dataToHost.empty()) dataToHost.pop();
					setNextState(101);
					setDeviceData(true); // Release data line
				} else {
					setNextState(state +1);
					setDeviceData(false); // Start bit
				}
				break;
			case 201: case 202: case 203: case 204: case 205: case 206: case 207: case 208: // Send eight data bits to host
				setDeviceData(true); // Check if line held
				if (deviceData && !getData()) {	// Data reception interrupted by host wanting to send data
					while (!dataToHost.empty()) dataToHost.pop();
					setNextState(101);
					setDeviceData(true); // Releae data line
				} else {
					setNextState(state +1);
					setDeviceData(latch &0x01);
					if (latch &0x01) oneBits++;
					latch =latch >>1;
				}
				break;
			case 209: // Send parity bit to host
				setDeviceData(~oneBits &1);
				setNextState(state +1);
				break;
			case 210: // Send stop bit to host
				setDeviceData(true); // Stop bit
				setNextState(state +1);
				break;
			case 211: // End
				checkForSendRequest();
				break;
			}
		}
	}
}
//------------------------------------------------------------------------------

struct PS2Mouse_State {
	unsigned long Bits;
	unsigned char BitPtr;
	int Xdelta, Ydelta;
	unsigned char Buttons;
	PS2DevicePort port;
	bool direction;
} *STATE;

void	EXPAD_PS2_Mouse::Reset()
{
	mouse_x = mouse_y = 0;

	bOut = FALSE;
	ScanNo = 0;

	STATE =new PS2Mouse_State;
	STATE->Bits = 0;
	STATE->BitPtr = 0;
	STATE->Xdelta = 0;
	STATE->Ydelta = 0;
	STATE->Buttons = 0;
	STATE->direction =false;

}

BYTE	EXPAD_PS2_Mouse::Read4016()
{
//	DEBUGOUT( "Read4016...\n" );

	BYTE data = 0x40;

	Write4016( 0xCC );
//	STATE->port.setHostClock(false);

	return data;
}

BYTE	EXPAD_PS2_Mouse::Read4017()
{
	BYTE data = 0x40;

	STATE->port.setHostClock(true);
	data |= (STATE->direction? !STATE->port.getData(): STATE->port.getClock()) *0x01;

	if (nes->pad->Subor_KB) data |= Subor_KeyMap(ScanNo, bOut, 0xFE);

	return data;
}

void	EXPAD_PS2_Mouse::Write4016( BYTE data )
{
//	DEBUGOUT( "Write4016...%2x\n", data );

	if (data ==0xCC){
		STATE->port.setHostClock(false);
	}else
	if (nes->pad->Mouse_type==0) {
		STATE->direction =!(data &0x04);
		STATE->port.setHostData(!(data &0x01));
	} else {
		STATE->direction =!!(data &0x04);
		STATE->port.setHostData(!!(data &0x01));
	}	

	if(nes->pad->Subor_KB){
		if (data & 4) {
			if ( data == 0x05 ) {
				bOut = FALSE;
				ScanNo = 0;
			} else if ( data == 0x04 ) {
				if ( ++ScanNo > ((nes->pad->Mouse_type==0)?13:9) )
					ScanNo = 0;
				bOut = !bOut;
			} else if ( data == 0x06 ) {
				bOut = !bOut;
			}
		}
	}
}

void	EXPAD_PS2_Mouse::Sync()
{
	STATE->Buttons = 0;
	if( ::GetAsyncKeyState(VK_LBUTTON)&0x8000 )
		STATE->Buttons |= 0x01;
	if( ::GetAsyncKeyState(VK_RBUTTON)&0x8000 )
		STATE->Buttons |= 0x02;
	if( ::GetAsyncKeyState(VK_MBUTTON)&0x8000 )
		STATE->Buttons |= 0x04;

	STATE->Xdelta =(mouse_x >>1) | (mouse_x &1);
	STATE->Ydelta =(mouse_y >>1) | (mouse_y &1);
	mouse_x = mouse_y = 0;
}

void	EXPAD_PS2_Mouse::SetMouseStatus( INT nX, INT nY, INT nKeys, INT nFlags )
{
	if (nFlags & 0xF0) {
		mouse_x = nX;
		mouse_y = nY;
	} else {
		;
	}
}

void EXPAD_PS2_Mouse::CPUCycle()
{ 
	STATE->port.cpuCycle();
	if (STATE->port.dataFromHost.size() >=1 && STATE->port.dataFromHost.front() ==0xEB) {
		STATE->port.dataFromHost.pop();
		STATE->port.dataToHost.push(0xFA);
		int Y;
		int X;
		int buttons;
		if (nes->pad->Mouse_type==0) {
			X = STATE->Ydelta;
			Y =-STATE->Xdelta;
			buttons =!!(STATE->Buttons &4)*2 +!!(STATE->Buttons &2)*1 +!!(STATE->Buttons &1)*4;
		} else {
			if(nes->pad->Mouse_type==3){
				X =-STATE->Ydelta;
				Y = STATE->Xdelta;
			}
			if(nes->pad->Mouse_type==4){
				X =-STATE->Xdelta;
				Y = STATE->Ydelta;
			}
			buttons =!!(STATE->Buttons &4)*4 +!!(STATE->Buttons &2)*1 +!!(STATE->Buttons &1)*2;
		}
		unsigned char byte1 =buttons &7 | 8 | (X <0? 0x10: 0) | (Y <0? 0x20: 0);		
		unsigned char byte2 =X;
		unsigned char byte3 =Y;
		STATE->port.dataToHost.push(byte1);
		STATE->port.dataToHost.push(byte2);
		STATE->port.dataToHost.push(byte3);
	} else
	if (STATE->port.dataFromHost.size() >=1 && STATE->port.dataFromHost.front() ==0xFF) {
		STATE->port.dataFromHost.pop();
		STATE->port.dataToHost.push(0xFA);
		STATE->port.dataToHost.push(0xAA);
		STATE->port.dataToHost.push(0x00);
	} else
	if (STATE->port.dataFromHost.size() >=1 && STATE->port.dataFromHost.front() ==0xF0) { // Set Remote Mode
		STATE->port.dataFromHost.pop();
		STATE->port.dataToHost.push(0xFA);
	} else
	if (STATE->port.dataFromHost.size()) {
		STATE->port.dataFromHost.pop();
	}
}

void	EXPAD_PS2_Mouse::Clock(INT nCycles)
{
	CPUCycle();
}
