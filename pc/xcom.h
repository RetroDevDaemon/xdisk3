// ---------------------------------------------------------------------------
//	TransDisk/88 ‚Æ‚ÌŒðMƒNƒ‰ƒX
//	Copyright (C) 2000 cisc.
// ---------------------------------------------------------------------------
//	$Id: xcom.h,v 1.2 2000/01/04 14:53:17 cisc Exp $

#ifndef XCOM_H
#define XCOM_H

#include "sio.h"
#include "file.h"
#include "lz77e.h"

// ---------------------------------------------------------------------------
//
//
class XComm2
{
public:
	enum Error
	{
		s_ok = 0,
		e_port = -1, e_connect = -2, e_version = -3, e_protocol = -4,
		e_crc = -5, e_compression = -6,
	};
	struct SystemInfo
	{
		uint8 verh;
		uint8 verl;
		uint8 fddtype;
		uint8 romtype;
		uint8 siotype;
	};

public:
	XComm2();
	~XComm2();

	uint Connect(int port, int baud);
	uint Disconnect();

	uint SendPacket(const uint8* buffer, int length, bool cmd = false, bool dat = false);
	uint SendDataPacket(const uint8* buffer, int length);
	uint RecvPacket(uint8* buffer, int buffersize, bool burst = false);
	const SystemInfo* GetSysInfo() { return &si; }

private:
	struct XERR
	{
		uint err;
	};

	void SendWord(int a, bool cc=true);
	void SendByte(int a, bool cc=true);
	void SendFlush();
	int  RecvWord(bool cc=true);
	int  RecvByte(bool cc=true);
	void InitPack();
	void BuildCRCTable();
	void SendC(uint8 c);
	uint RecvC();
	void CalcCRC(int data);
	void Throw(Error e);

	bool DecompressPacket(uint8* buf, uint bufsize, const uint8* src, uint pksize);

	SIO sio;
	SystemInfo si;
	uint16 index;
	int d, e;
	uint16 crc;
	bool connect;
	int unc;
	int baud;
	LZ77Enc enc;
	
	static uint16 crctable[256];
};

inline void XComm2::SendWord(int a, bool cc)
{
	SendByte(a & 0xff, cc);
	SendByte((a >> 8) & 0xff, cc);
}

inline int XComm2::RecvWord(bool c)
{
	int r = RecvByte(c);
	return r + RecvByte(c) * 256;
}

inline void XComm2::InitPack()
{
	d = e = 0;
	crc = ~0;
}

inline void XComm2::CalcCRC(int data)
{
	crc = (crc << 8) ^ crctable[(data ^ (crc >> 8)) & 0xff];
}

inline void XComm2::Throw(Error e)
{
	XERR xe; xe.err = e; throw xe;
}

#endif // XCOM_H
