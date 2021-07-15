## RS232c over USB!?

 Knowing how to convert Windows file I/O, and by extension terminal and RS-232/c protocol to UNIX/Linux, is a valuable skill to have. Many systems use RS232 as their standard serial transfer protocol (including the Megadrive/Genesis!). What you learn here can be utilized elsewhere. 

 A simple USB -> RS-232/c cable is quite cheap nowadays. Combined with a 9-to-25 pin connection adapter, you have a quick and cheap way to access the PC-88 remotely. 

Here are two that should work (note the PL2303 chipset):

UGREEN USB 2.0 to RS232 DB9 Serial Cable Male A Converter Adapter with PL2303... https://www.amazon.com/dp/B00QUZY4WO/ref=cm_sw_r_tw_dp_0Z57FC0TDZ9WXKBM6D7E?_encoding=UTF8&psc=1 

StarTech.com 10 ft Cross Wired DB9 to DB25 Serial Null Modem Cable - F/M - Nu... https://www.amazon.com/dp/B00066HL50/ref=cm_sw_r_tw_dp_K69NMDKCXB9BBT7X9NH5 

Combined, it should be around $15 USD.

When the USB cable is plugged into your Raspbian (or other Linux) machine, it will show up as a device in the device path (`/dev`). Typically, it will take the first available USB port number. For me, it's always 0 on my Pi 400. The full path of the USB serial device becomes:
`/dev/ttyUSB0`<br><br>

## Linux serial communication

Everything on Linux is a file - even a USB serial device. Now that we know the file alias for the device in Linux, we can try probing it. It took a bit of searching to find information on typical serial I/O on *nix, but eventually I found that it uses terminal protocol, and requires <termios.h>. 

The typical way you set up a serial transfer in Linux is: you query the device, modify the connection parameters, then use the normal read() and write() functions to read or write n bytes at a time.

A small program to set connection parameters and write a byte to the USB serial port looks something like this:
```
#include <unistd.h>     // unix
#include <fcntl.h>      // file control
#include <termios.h>    // posix terminal stuff

struct termios options;
char c = "!";

void main()
{
	int serialport = open("/dev/ttyUSB0", O_RDWR | O_NDELAY);
	
	// Get current settings
	tcgetattr(serialport, &options);
	
	// Set desired options  
	/*  (set options here) */
	tcflush(serialport, TCIFLUSH);
	tcsetattr(serialport, TCSANOW, &options);
	fcntl(serialport, F_SETFL, 0);
	
	// Write one byte 
	write(serialport, &c, 1);
	
	// Close the port
	close(serialport);
}
```
To open the port, you simply use open(). Flags needed are O_RDWR to open the port for both reading and writing, and O_NDELAY to disable blocking read. If O_NDELAY is not set, your program will freeze while waiting for input. 

tcgetattr() gets the current serial port settings and stores it in the termios struct pointed to by options. tcsetattr() will apply the settings. The TCSANOW flag forces it to happen immediately. 

All options are generally stored in few words: c_cc, c_cflag, c_iflag, c_lflag, and c_oflag. 

tcflush() and fctnl(..., 0) are simply safe programming practices in case there is anything remaining in the buffer or the serial port is in an odd state.

Normal file/alias read and write control in unix is in unistd.h. write() and close() are easy to use and understand.<br><br>

## TransDisk, here we come

Now that we know the basics, we can take a look at Xdisk2, line-by-line, and try to convert anything that looks Windows-like to Linux standard. (To make this easy, I diffed everything and will list it here). 

Starting with `main.cpp`, we have to include a bunch of unix headers, define the Windows MAX_PATH constant, and remove <mbstring.h> (because it's not supported on my variety of linux, and doesn't do anything strictly required). 
```
#define MAX_PATH 260
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>     // unix
#include <fcntl.h>      // file control
#include <errno.h>      // error number definitions
#include <termios.h>    // posix terminal stuff
#include <sys/ioctl.h>

#include "headers.h"
//#include <mbstring.h> 
#include "types.h
#include "xdisk.h"
#include "file.h"
```
Also copy this batch of *nix headers into "headers.h" so other files don't complain about them missing.

Unfortunately removing mbstring means the tokenizing for multiple disk images in the command line won't work, so around line 205 I did the following:
```
	//_mbstok((uchar*) filename, (uchar*) ";");
	//char* x = (char*) _mbstok(0, (uchar*) ";");
	int index = 1;//x ? atoi(x) : 1;
```
This simply means that only 1 D88 file at a time will be supported in write mode - not the end of the world.<br><br>

## Disk-based file I/O

Next is `file.cpp/.h`. This file has to do with actual file I/O on the host machine (i.e. opening and writing D88 images).
In `file.h`, in order to get a successful compile, I defined false values for Windows read/write flags, MAX_PATH and DWORD:
```
// These are fake!
#define GENERIC_WRITE 0x1
#define GENERIC_READ 0x2
#define FILE_SHARE_READ 0x4
#define CREATE_ALWAYS 0x8
#define OPEN_ALWAYS 0x10
#define OPEN_EXISTING 0x20
#define INVALID_HANDLE_VALUE (FILE*)0x40

#define MAX_PATH 260
#define DWORD uint32 
```
In `file.cpp`, `#include "File.h"` needs to be changed to `"file.h"` (because linux is case-sensitive). 
Around line 40, we change majority of the `::Open` method. `CreateFile()` actually creates a Windows file alias, and does not make a new file as the name implies. The equivalent would be Linux's `fopen()` in read or write mode:
```
	//hfile = CreateFile(filename, access, share, 0, creation, 0, 0);
	if (flg & readonly) hfile = fopen(filename, "rb");
	else hfile = fopen(filename, "wb");
	flags = (flg & readonly) | (hfile == INVALID_HANDLE_VALUE ? 0 : open);

 	if (!(flags & open)) {} // No error handling for windows type
 	
 	SetLogicalOrigin(0);
 	
 	return !!(flags & open);
 }
```
This is my quick and dirty way of implementing file open and close without great error handling. GetLastError() is a Windows method, and pursuing this seemed like it would be more time than it was worth, so I got it working and moved on. 

Directly below, the `::CreateNew` method I do something similar to get it "just working". Of note, this method is never actually called in the program, so feel free to comment it all out. 
```
 bool FileIO::CreateNew(const char* filename)

 {

 	Close();

 	strncpy(path, filename, MAX_PATH);

 	hfile = fopen(filename, "wb");
 	SetLogicalOrigin(0);
 	return true;
 }
```
As for `::Reopen()`, I had no idea what to do, so I deleted most of it:
```
 bool FileIO::Reopen(uint flg)

 {
 	SetLogicalOrigin(0);
 	return true;
 }
```
Now, `::Close()` is much simpler. `fclose()` is what we use instead of `CloseHandle()`:
```
 void FileIO::Close()
 {
  	if (GetFlags() & open)
  	{
  			fclose(hfile);
  			flags = 0;
   	}
 }
```
`::Read` is also fairly straightforward. The Linux equivalent of `ReadFile()` is `fread()`. size_t is an unsigned 32-bit integer (if the standard is adhered to), so it can be cast back without issue.
```
int32 FileIO::Read(void* dest, int32 size)
{
	if (!(GetFlags() & open))
		return -1;

	size_t readsize; 
	readsize = fread(dest, 1, size, hfile);
	return readsize; 
}
```
Continuing to move down, we have `::Write()`. This is nearly identical to the rewrite of `:Read()`, save we obviously use `fwrite` instead of `fread`:
```
 int32 FileIO::Write(const void* dest, int32 size)
 {
 	DWORD writtensize;
	//if (!WriteFile(hfile, dest, size, &writtensize, 0))
	//	return -1;
	writtensize = fwrite(dest, 1, size, hfile);
 	return writtensize;
 }
 ```
`::Seek()` required some thought. Observe the new method:
```
bool FileIO::Seek(int32 pos, SeekMethod method)
 {
 	
 	DWORD wmethod;
+	int s;
 	switch (method)
 	{
 	case begin:	
		//wmethod = FILE_BEGIN; pos += lorigin; 
+		wmethod = SEEK_SET; 
+		s = fseek(hfile, pos, SEEK_SET);
+		pos += lorigin; 
 		break;
	case current:	
		//wmethod = FILE_CURRENT; 
+		s = fseek(hfile, pos, SEEK_CUR);	
+		wmethod = SEEK_CUR; 
 		break;
 	case end:		
		//wmethod = FILE_END; 
+		s = fseek(hfile, pos, SEEK_END);
+		wmethod = SEEK_END; 
 		break;
 	default:
 		return false;
 	}

+	if (s == 0) return true;
+	else return false;
	//return 0xffffffff != SetFilePointer(hfile, pos, 0, wmethod);
 }
 ```
 The Linux equivalent of `SetFilePointer()` is `fseek()`. Instead of the constants `FILE_BEGIN, FILE_CURRENT, FILE_END`, we use `SEEK_SET, SEEK_CUR, SEEK_END`. 
 
 I also simplified the return value to make it more readable. (wmethod is no longer used, so it can be deleted). 

I needed help with SetFilePointer's return value. fseek returns 0 when successful, so we need the FILE* offset some other way. Thanks to @spaceotter, I was saved precious time searching for the Linux equivalent for the return value - `ftell()`. The name of the function `Tellp()` should have been a dead giveaway!
 ```
  int32 FileIO::Tellp()
 {
 	if (!(GetFlags() & open))
 		return 0;
+	return ftell(hfile) - lorigin;
 	//return SetFilePointer(hfile, 0, 0, FILE_CURRENT) - lorigin;
 }
 ```
 This is actually easier than Windows. Instead of re-setting the file pointer to its current position and using that return value, you can simply use ftell() to get the pointer offset.

Last one! `::SetEndOfFile()` is very easy to convert using `fseek()`:
```
 bool FileIO::SetEndOfFile()
 {
 	if (!(GetFlags() & open))
 		return false;
	//return ::SetEndOfFile(hfile) != 0;
+	return fseek(hfile, 0, SEEK_END);
 }
 ```
 <br><br>
 ### Remove Windows headers
 in `headers.h` you'll find that you need to comment out the `#include` lines for `"windows.h"` and `"process.h"`. The rest should be fine.
<br><br>

### lz77e.cpp typo
```
const uint8* s = src;
for (s = src+1; s < srctop-2; )
```
The 'ss' is a typo, near as I can tell. This change in line `229` will allow the app to compile.<br><br>

## sio.h
The sio files are where the biggest changes will be. In SIO.h, I created a new class DCB as a placeholder container for our serial port settings. Linux doesn't use anything similar to Windows DCB - this was just an attempt to keep nomenclature the same. The termios config is where the magic happens.
```
#include "headers.h" 	// our new nix headers are here
#define TRUE 1
#define FALSE 0
class DCB 
{
public: 
	uint32 BaudRate;	//	= baud;
	struct termios config;	

public:
	DCB();
	~DCB();
};
```
And under the SIO class' public properties, we need the serial port alias:
```
char fpa[13] = "/dev/ttyUSB ";
```
Note the empty space at the end.
I also added a new method, `DCBToNix()`, as a way to keep the terminal option settings separate. Line 49+ becomes:
```
	void DCBToNix();
private:
	//HANDLE hfile;
	int serialport;  // File handles in *nix are actually int type
	DCB dcb;
	int timeouts;
};
```
## sio.cpp
First, we need two simple defines to get compile going:
```
#define INVALID_HANDLE_VALUE 0
#define DWORD uint32 
```
Then, a slight tweak to the constructors and destructors:
```
SIO::SIO()
: serialport(INVALID_HANDLE_VALUE)
//: hfile(INVALID_HANDLE_VALUE)
 {}
 
 SIO::~SIO()
 {
	//if (hfile)
	if (serialport != INVALID_HANDLE_VALUE)
 		Close();
 }

DCB::DCB()
{}
 
DCB::~DCB()
{}
```
Since we aren't using the FILE* type for our serial port alias, I thought it better to rename it to something more readable anyway.

## Serial IO!
Now we're down to the good stuff: the `::Write()` function. `write()` is a perfect replacement for `WriteFile()`. We simply use the normal `write()` instead of `fwrite()` this time. 
```
 SIO::Result SIO::Write(const void* src, int len)
 {
 	DWORD writtensize;

	//if (WriteFile(hfile, src, len, &writtensize, 0))
	//{
	//	if (int(writtensize) == len)
	//		return OK;
	//}

	writtensize = write(serialport, src, len);
	if(int(writtensize) == len) 
	{
		return OK;
 	}

 	if (verbose)
 		printf("sio::write error\n");
 	return ERR;
 }
 ```
 `::Read()` is very much the same. We check the returned size from `read()` in the same way Windows' `ReadFile()` works:
 ``` 
 SIO::Result SIO::Read(void* dest, int len)
 {
	//DWORD readsize;
	//if (ReadFile(hfile, dest, len, &readsize, 0))
	//{
	//	if (int(readsize) == len)
	//		return OK;
	//}

	size_t readsize;
	for(int f = 0; f < 3; f++)
	{
		readsize = read(serialport, dest, len);
		if(readsize == len) f = 4; // break loop
	}
	if(int(readsize) == len) 
		return OK;

 	if (verbose)
 		printf("sio::read error\n");
 }
 ```
You'll note that I put `read()` in a loop - this lets it timeout 3 times before actually failing. I got the idea from `xcomm.cpp`'s SendPacket(). (This may be able to be removed now that I've debugged most of it).

Now for `::Open()`. This is what actually opens the connection to the PC88, so this part is very key. The first part, we comment out the Windows error handling, and instead set the final character of the `fpa` array to the ASCII character value of the `port` passed when running the program and print the string value of the port. 
```
SIO::Result SIO::Open(int port, int baud)
 {
	//if (hfile != INVALID_HANDLE_VALUE)
	//	Close();
	//if (port < 1)
	//	return ERRINVALID;
	//char buf[16];
	//wsprintf(buf, "COM%d", port);
 
	timeouts = 2000;
	fpa[11] = port | 0x30;
	printf(fpa);
	printf("\n");
```	
Next we actually open the serial port. Instead of `CreateFile()` like above, we use `open()` - only this time, we are opening the serial port string, and we want to pass two parameters - O_RDWR, and O_NDELAY. O_RDWR is equivalent to GENERIC_READ | GENERIC_WRITE, and O_NDELAY is required on Linux to disable blocking read. 
```
	serialport = open(fpa, O_RDWR | O_NDELAY);
	
 	//hfile = CreateFile(	buf,
	//			GENERIC_READ | GENERIC_WRITE,
	//			0,
	//			NULL,
	//			OPEN_EXISTING,
	//			0,
	//			NULL );
 	//if (hfile == INVALID_HANDLE_VALUE)
 	//{
 	//	return ERROPEN;
 	//}
	// we dont need to zero this out since we aren't using it
	//memset(&dcb, 0, sizeof(DCB));
	//dcb.DCBlength		= sizeof(DCB);
	
	// Get the current port status, set baud, and reset attr
	//GetCommState(hfile, &dcb);
	tcgetattr(serialport, &dcb.config);
 	if (!SetMode(baud))
 	{
 		Close();
 		return ERRCONFIG;
 	}
	tcflush(serialport, TCIFLUSH);
	//PurgeComm(hfile, 
	//	PURGE_TXABORT| // terminate write 
	//	PURGE_TXCLEAR| // clear output buffer
	//	PURGE_RXABORT| // terminate read
	//	PURGE_RXCLEAR); // clear input buffer
 	return OK;
 }
 ```
 Instead of `GetCommState()`, we use termios' `tcgetattr()`. Obviously the values do not match up 1:1, so we'll have to make sure it's set correctly later. 

 `tcflush()` is our equivalent to Windows' `PurgeComm()`. The `TCIFLUSH` parameter, or `immediate flush`, gets rid of everything in the buffer ASAP. 

 Similar to `FileIO::Close()`, we change `SIO::Close()` to use `serialport` instead of `hfile`, `tcflush` instead of `PurgeComm`, and `close` instead of `CloseHandle`.
 ```
 SIO::Result SIO::Close()
 {
	if (serialport != INVALID_HANDLE_VALUE)
 	{
		//PurgeComm(hfile, 
		//	PURGE_TXABORT|PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR);
		//CloseHandle(hfile);
		tcflush(serialport, TCIFLUSH);
		close(serialport);
		serialport = INVALID_HANDLE_VALUE;
 	}
 	return OK;
 }
 ```
 Next is the new function to set the Linux style termios settings for the RS232 in an analgous way to how TransDisk 2 sets them in Windows. This is what I got:
 ``` 
void SIO::DCBToNix()
{
	dcb.config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |\
                    INLCR | PARMRK | INPCK | ISTRIP | IXON);
	dcb.config.c_oflag = 0;
	dcb.config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);

	switch(dcb.BaudRate)
	{
		case(9600):
			cfsetispeed(&dcb.config, (speed_t)B9600);
			cfsetospeed(&dcb.config, (speed_t)B9600);
			break;
		case(19200):
			cfsetispeed(&dcb.config, (speed_t)B19200);
			cfsetospeed(&dcb.config, (speed_t)B19200);
			break;
		default:
			break;
	}
	dcb.config.c_iflag &= ~(INPCK|IXOFF); 	// disable input parity checking
	dcb.config.c_iflag |= IXON; 		// foutx, finx
	dcb.config.c_cflag &= ~(CSIZE | PARENB);
	dcb.config.c_cflag |= CS8; 		// set byte size to 8, no parity
	dcb.config.c_cflag &= ~(CSTOPB); 	// 1 stop bit
	dcb.config.c_cc[VINTR] = 10;
	dcb.config.c_lflag &= ~(ICANON);	// Canonical off
	dcb.config.c_cc[VMIN] = 1;		// Need at least one byte!
	dcb.config.c_cc[VTIME] = 3; 		
}
```
The first few lines clear all flags. 
Then, we set the bitrate using `cfsetispeed()` and `cfsetospeed()`. 

The following settings are taken from TransDisk: Parity check off, fInX on, fOutX off, No parity, 8-bit bytes, 1 stop bit, event byte 0xa (10). 

Canonical mode, VMIN and VTIME required experimentation to get right. If canonical mode is on, and VMIN is not 0, you will get blocking reads - just like if O_NDELAY flag was not set. We set VMIN to 1 because we always only read one byte at a time from the serial 88, and VTIME to 0.3 seconds because *it works* for 9600 baud. We'll get back to this magic number in a bit.

All this stores the config in RAM. We still need to apply it via `::SetMode()`:
```
 bool SIO::SetMode(int baud)
 {
	//PurgeComm(hfile, PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR);
	tcflush(serialport, TCIFLUSH);
	
	DCBToNix();
	int r = tcsetattr(serialport, TCSANOW, &dcb.config);
	fcntl(serialport, F_SETFL, 0);
	
	if(r == 0) r = 1;
	else r = 0;		// flip r
	return r;		// fail = 0, pass = nonzero
 }
 ```
 I removed most of it. As we know, PurgeComm == tcflush(). Then we need to call our new DCBToNix function to populate the config element in RAM. To apply the config, we call `tcsetattr()` on the serial port. (fcntl() is just some safe practice, but likely does nothing). Finally, SetMode()'s return value is the opposite of tcsetattr()'s return value, so I flip it with **readable code** before returning.
 
 The last function, `::SetTimeouts()`, goes back to the `[VTIME]` magic number. If VTIME is too large, the data transfer hangs. If VTIME is too small, the data transfer gets entirely out of sync. 

 I am unaware of a way to have different VTIMEs for read and write on Linux. According to the formula in the original `SetTimeouts` function:
 ```
	cto.ReadTotalTimeoutConstant = timeouts = rto;
	cto.ReadTotalTimeoutMultiplier = 20000 / dcb.BaudRate + 1;
	cto.WriteTotalTimeoutConstant = 2000; 
	cto.WriteTotalTimeoutMultiplier = 20000 / dcb.BaudRate + 1;
 ```
If you follow the source code, this seems to imply that read timeout on Connect() is temporarily set to `1000 * (20000 / (9600)) + 1`, or 3000<br>
And otherwise, is set to `10,000 * (20000/9600) + 1`, or 30,000<br>
(Or, 2000 and 20,000 for 19200 bps). <br><br>
Whatever these values represent, the appropriate divisor for the Linux equivalent seems to be 1000 (or 10000), such that the correct VTIME at 9600 baud is 3; and at 19200 baud is 2:
```
void SIO::SetTimeouts(int rto)
{
	tcflush(serialport, TCIFLUSH);
	if(dcb.BaudRate == 9600)
		dcb.config.c_cc[VTIME] = 3;
	else 
		dcb.config.c_cc[VTIME] = 2;
	tcsetattr(serialport, TCSANOW, &dcb.config);
}
 ```
Phew!
<br><br>
## Small final changes to xdisk.cpp
Functionally speaking, we're there, but we need to do a few small changes to xdisk's back end to make sure the app runs correctly. 

First, I have no idea how burst mode works or how to configure it, so I disabled it:
```
 uint TransDisk2::Connect(int port, int baud, bool bm)
 {
 	uint r = XComm2::Connect(port, baud);
	//burst = (GetSysInfo()->siotype && baud == 19200) ? bm : false;
	burst = false;
```
And lower, on line 109, we need to get rid of `GetTickCount()` (a Windows function) and then declare `int tr` outside of the track loop. Once we do that, we're golden!!
```
 	int bt;
	//if (puttime)
	//	bt = GetTickCount();
 	int l;
	int tr;
	for (tr=0; tr<ntracks-1; tr++)
	//for (int tr=0; tr<ntracks-1; tr++)
 	{
```
Using this provided Makefile, xdisk should compile as-is and you should be able to write disks over serial with no issue!
```
# Makefile for xdisk3
OBJ:=main file lz77e pch sio xcom xdisk 

default: clean $(OBJ)
	g++ -g -o xdisk3 ./*.o

$(OBJ): %: %.cpp 
	g++ -g -c $< -o $@.o

clean:
	rm -rf ./*.o

```
As far as sending the BASIC program, reading disks and ROMs, I'm not quite sure yet. Additional testing is appreciated.

Source:
http://github.com/bferguson3/xdisk3 