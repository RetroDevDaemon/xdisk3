// ---------------------------------------------------------------------------
//	TransDisk/88 �Ƃ̌�M�N���X
//	Copyright (C) 2000 cisc.
// ---------------------------------------------------------------------------
//	$Id: xcom.cpp,v 1.7 2000/01/21 01:57:47 cisc Exp $

#include "headers.h"
#include "xcom.h"

uint16 XComm2::crctable[0x100] = { 123 };

extern bool verbose;

extern int version_l;

// ---------------------------------------------------------------------------
//	�\�z�E�j��
//
XComm2::XComm2()
: connect(false)
{
	if (crctable[0] == 123)
		BuildCRCTable();
}

XComm2::~XComm2()
{
	Disconnect();
}

// ---------------------------------------------------------------------------
//	�ڑ�
//
uint XComm2::Connect(int port, int _baud)
{
	Disconnect();
	baud = _baud;
	
	if (sio.Open(port, baud) != SIO::OK)
		return e_port;
	
	sio.SetTimeouts(1000);
	
	uint e;
	
	static const uint8 cmd[] = { 0x01 };
	
	if ((e = SendPacket(cmd, 1, true)) != s_ok)
	{
		//printf("sendpacket fail...\n");	
		sio.Close();
		return e;
	}
	//printf("sendpacket ok?\n");
	
	if ((e = RecvPacket((uint8*) &si, sizeof(si))) != s_ok)
	{
		sio.Close();
		return e;
	}

	if (si.verh != 2 || si.verl < version_l)
	{
		fprintf(stderr, "Version on 88 does not match.\n");
		sio.Close();
		return e_version;
	}
	if (verbose)
	{
		printf("xdisk88: version is %d.%.2d\n", si.verh, si.verl);
		printf("pc88 has %s drive(s).\n", si.fddtype ? "2HD" : "2D");
		printf("rom bitmap is %.2x (Kanji 1:%d, Kanji 2:%d, Jisho:%d, CD:%d)\n", si.romtype,
			!!(si.romtype & 1),
			!!(si.romtype & 2),
			!!(si.romtype & 4),
			!!(si.romtype & 8)
			);
		printf("pc88 has %s-rate serial port.\n", si.siotype ? "variable" : "fixed");
	}
	
	unc = -1;
	connect = true;
	sio.SetTimeouts(10000);
	return s_ok;
}

// ---------------------------------------------------------------------------
//	�ڑ�����
//
uint XComm2::Disconnect()
{
	if (connect)
	{
		sio.Close();
		connect = false;
	}
	return s_ok;
}

// ---------------------------------------------------------------------------
//	�p�P�b�g�𑗂�
//
uint XComm2::SendPacket(const uint8* buffer, int length, bool cmd, bool data)
{
	uint8 buf[0x4000];
	if (data)
	{
		if (verbose)
			printf(" (%4d", length);
		int t;
		crc = -1;
		for (t=0; t<length; t++) 
			CalcCRC(buffer[t]);
		int cl = enc.Encode(buffer, length, buf, sizeof(buf));
		buf[cl]   = crc & 0xff;
		buf[cl+1] = (crc >> 8) & 0xff;
		cl += 2;

#if 0
		uint8 decbuf[0x10000];
		enc.Decode(decbuf, buf+2, buffer);
#endif
//		for (int x=16; x>0; x--)
//			printf("%.2x ", decbuf[length-x]);

//		printf("CRC[%.4x]", crc);
		if (verbose)
			printf(" -> %4d, %3d%%)", cl, cl * 100 / length);
	
		if (cl < length)
			buffer = buf, length = cl;
		else
			data = false;
	}
	
	int i;
	uint err = e_connect;
	for (i=3; i>0; i--)
	{
		//printf("errored?\n");
	
		try
		{
			// �w�b�_����
			uint8 hdr = cmd ? '!' : data ? '$' : '#';
			if (SIO::OK != sio.Write(&hdr, 1))
				Throw(e_connect);
			
			InitPack();
			if (cmd) index++;
		//printf("SEND[%.4x][%.4x]\n", index, length);
			SendWord(index);
			SendWord(length);
			
			for (int l=0; l<length; l++)
				SendByte(buffer[l]);

			SendWord(crc, false);
			SendFlush();

			
			if (!cmd)
				return s_ok;
			
			uint8 buf;
			if (RecvPacket(&buf, 1) == s_ok)
			{
				if (buf == 0)
					Throw(e_protocol);
				return s_ok;
			}
			
	
		}
		catch (XERR xe)
		{
			err = xe.err;
		}
	}
	return err;
}

// ---------------------------------------------------------------------------
//	�p�P�b�g���󂯎��
//	TODO:	���k�f�[�^�Ή�
//			�đ�
//
uint XComm2::RecvPacket(uint8* buffer, int length, bool burst)
{
	uint count = 3;
	uint8 recvbuf[0x4000];
	uint8 c;
	//if (burst) 
	//	unc = -1, sio.SetMode(baud * 2);
	//unc = 1;
	do
	{
		if (unc != -1)
		{
			c = unc, unc = -1;
		}
		else if (SIO::OK != sio.Read(&c, 1))
		{
			if (!--count)
				return e_connect;
			continue;
		}
	//	printf(".\n");		
		//printf("<%.2x>\n", c);
	} while (c != '%' && c != '#' && c != '$');
	//printf("loop over\n");
	try
	{
		InitPack();
		uint i = RecvWord();
		if (i != index){
			//printf("protocol fail\n");
			Throw(e_protocol);
		}

		uint l = RecvWord() & 0x3fff;
		//printf("RECV[%.4x][%.4x(%.4x)]\n", index, l, length);
		
		if (c != '$')
		{
			for (; length > 0 && l > 0; length--, l--)
				*buffer++ = RecvByte();
			for (; l > 0; l--)
				RecvByte();
		}
		else
		{
			for (uint j=0; j<l; j++)
				recvbuf[j] = RecvByte();
		}

		if (crc != RecvWord(false))
			Throw(e_crc);

		if (burst)
			sio.SetMode(baud);

		if (c == '$')
		{
			if (!DecompressPacket(buffer, length, recvbuf, l))
				return e_compression;
#if 0
			char buf[100];
			static int c = 0;
			sprintf(buf, "packet%.3d.bin", c++);
			FileIO fio(buf, FileIO::create);
			uint orgsize = (recvbuf[0] + recvbuf[1] * 256) & 0x3fff;
			fio.Write(buffer, orgsize);
#endif
		}
		return s_ok;
	}
	catch (XERR xe)
	{
		if (burst)
			sio.SetMode(baud);
		return xe.err;
	}
}

// ---------------------------------------------------------------------------
//	CRC �e�[�u���̍쐬
//
void XComm2::BuildCRCTable()
{
	for (int i=0; i<0x100; i++)
	{
		int a = i << 8;
		for (int j=0; j<8; j++)
			a = (a << 1) ^ (a & 0x8000 ? 0x1021 : 0);
		crctable[i] = a;
	}
}

// ---------------------------------------------------------------------------
//	7bit format �ɂ��o�C�i���̓ǂݍ���
//
int XComm2::RecvByte(bool cc)
{
	if (e == 0)
		d = RecvC() << 1, e = 7;
	int c = RecvC();
	int a = d | (c >> (e-1));
	d = (c << (9-e)) & 0xff;
	e--;
	if (cc)
		CalcCRC(a);
	return a;
}

// ---------------------------------------------------------------------------
//	7bit format �ɂ��o�C�i���̑��M
//
void XComm2::SendByte(int a, bool cc)
{
	SendC(d | (a >> (e+1)));
	if (e < 6)
		d = a << (6-e++);
	else
		SendC(a), d = e = 0;
	if (cc)
		CalcCRC(a);
}

void XComm2::SendFlush()
{
	if (e)
		SendC(d);
}

// ---------------------------------------------------------------------------

void XComm2::SendC(uint8 c)
{
	c = (c & 0x7f) + 0x40;

//	printf("(%.2X)", c);
	
	if (SIO::OK != sio.Write(&c, 1))
		Throw(e_connect);
}

uint XComm2::RecvC()
{
	uint8 c;
	if (SIO::OK != sio.Read(&c, 1))
		Throw(e_connect);

	if ((c - 0x40) & 0x80)
	{
		unc = c;
		Throw(e_protocol);
	}
	return c - 0x40;
}

// ---------------------------------------------------------------------------

bool XComm2::DecompressPacket(uint8* buf, uint bufsize, const uint8* src, uint pksize)
{
	const uint8* st = src;
	const uint8* dt = buf;

	uint orgsize = (src[0] + src[1] * 256) & 0x3fff;
	uint method = (src[1] >> 6) & 3;
//	printf("Compression: %.4x\n", src[0] + src[1] * 256);
	uint exsize = orgsize > bufsize ? bufsize : orgsize;
	src += 2;

	switch (method)
	{
	case 0:		// �����k
		memcpy(buf, src, exsize);
		return true;

	case 1:		// RLE
		while (exsize > 0)
		{
			uint a = *src++;
			if (a & 0x80)
			{
				uint l = (a & 0x7f) + 3;
				uint b = *src++;
				l = l > exsize ? exsize : l;
				exsize -= l;
				for (; l > 0; l--)
					*buf++ = b;
			}
			else
			{
				uint l = (a & 0x7f) + 1;
				l = l > exsize ? exsize : l;
				exsize -= l;
				for (; l > 0; l--)
					*buf++ = *src++;
			}
		}
		if (0)
		{
			crc = -1;
			exsize = orgsize > bufsize ? bufsize : orgsize;
			uint i;
			for (i=0; i<exsize-2; i++)
				CalcCRC(dt[i]);
			uint dcrc = *(const uint16*)(dt + exsize - 2);
			if (verbose) 
				printf("(%.4x-%.4x)", dcrc & 0xffff, src - st);
			if (((dcrc ^ crc) & 0xffff) || pksize - (src - st))
				printf("xcom: ���k�f�[�^�̓W�J�Ɏ��s\n");
		}

		return true;

	default:
		printf("unsupported-compression method\n");
		return false;
	}
}

