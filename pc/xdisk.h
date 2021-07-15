// ---------------------------------------------------------------------------
//	TransDisk 2.0
//	Copyright (c) 2000 cisc.
// ---------------------------------------------------------------------------
//	$Id: xdisk.h,v 1.2 2000/01/04 14:53:18 cisc Exp $

#ifndef XDISK_H
#define XDISK_H

#include "types.h"
#include "xcom.h"

// ---------------------------------------------------------------------------

class TransDisk2 : public XComm2
{
public:
	TransDisk2();
	~TransDisk2();
	
	uint Connect(int port, int baud, bool burst);
	bool ReceiveROM();
	bool ReceiveDisk(int drive, int media, FileIO& file);
	bool SendDisk(int drive, FileIO& file);
	bool SendProgram();

private:
	struct ImageHeader
	{
		char title[17];
		uint8 reserved[9];
		uint8 readonly;
		uint8 disktype;
		uint32 disksize;
		uint32 trackptr[164];
	};

	struct SectorHeader
	{
		uint8 c, h, r, n;
		uint16 sectors;
		uint8 density;
		uint8 deleted;
		uint8 status;
		uint8 reserved[5];
		uint16 length;
	};

	struct IDR
	{
		uint8 c, h, r, n;
		uint8 density;
		uint8 status;
		uint8 deleted;
		uint8* buffer;
		uint length;
	};

#pragma pack(push, 1)
	struct XD2IDR
	{
		uint8 r;
		uint16 len;
		uint16 off;
		uint8 c;
		uint8 h;
		uint8 n;
		uint8 density;
		uint8 st0;
		uint8 st1;
		uint8 st2;
	};
#pragma pack(pop)

	int ReceiveTrack(bool readnext, int track, FileIO& file);
	void IDDecompress(XD2IDR* idr, int nsecs, int track);
	bool CheckHeader(ImageHeader* ih);
	bool CalcMixLengthCondition(IDR* idr, int nsec, int* mle, int* mgap, int tracksize);
	void MakeMixLengthID(uint8*& dest, IDR* idr, int nsec, int le, int gap);
	void MakeNormalID(uint8*& dest, IDR* idr, int nsec, int le);
	int MakeWriteSeq(uint8* dest, IDR* idr, int nsec);

	int media;
	bool burst;
};

#endif // XDISK_H
