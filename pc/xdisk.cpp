// ---------------------------------------------------------------------------
//	TransDisk 2.0
//	Copyright (C) 2000 cisc.
// ---------------------------------------------------------------------------
//	$Id: xdisk.cpp,v 1.10 2000/02/01 13:09:24 cisc Exp $

#include "headers.h"
#include "types.h"
#include "xdisk.h"
#include "file.h"

extern bool verbose;

#define puttime		0

// ---------------------------------------------------------------------------
//	constructor/destructor
//
TransDisk2::TransDisk2()
{
	burst = true;
}

TransDisk2::~TransDisk2()
{
}


uint TransDisk2::Connect(int port, int baud, bool bm)
{
	uint r = XComm2::Connect(port, baud);
	
	burst = (GetSysInfo()->siotype && baud == 19200) ? bm : false;
	return r;
}

// ---------------------------------------------------------------------------
//	ディスクの内容を取り出す
//
bool TransDisk2::ReceiveDisk(int drive, int m, FileIO & file)
{
	uint8 buf[16];
	uint r;
	media = m;
	if (!GetSysInfo()->fddtype)
	{
		if (media == 1 || media == 3)
		{
			printf("この 88 は 2D 専用機です.\n");
			return true;
		}
		media = 0;
	}
	
	while (1)
	{
		// SET DRIVE
		buf[0] = 2;
		buf[1] = drive;	
		buf[2] = (0 <= media && media <= 3) ? media : -1;
		r = SendPacket(buf, 3, true);
		if (r != s_ok)
			return false;

		r = RecvPacket(buf, 1);
		if (r != s_ok)
			return false;

		media = buf[0];
		if (verbose)
			printf("media = %d\n", media);

		if (media == 0xff)
		{
			printf("ディスクの種類が特定できません.\n");
			return false;
		}
		if (media < 4)
		{
			break;
		}
		if (media != 0xfe)
		{
			printf("ディスク操作に関するエラーが生じました.\n");
			return false;
		}
		printf("PC88 のドライブ %d にディスクを入れてキーを押してください.\n", drive+1);
		getchar();
	}

	int ntracks = media & 1 ? 164 : 84;
	const char* type[] = { "2D", "2DD", "???", "2HD" };
	printf("ディスクの種類は %s です.\n", type[media]);

	ImageHeader image;
	memset(&image, 0, sizeof(image));
	strcpy(image.title, "made by xdisk2");
	image.disktype = media == 3 ? 0x20 : media == 1 ? 0x10 : 0;
	image.disksize = 0xdeadd13c;	// (未完成イメージを示す MAGIC)
	
	file.Seek(0, FileIO::begin);
	file.Write(&image, sizeof(image));

	buf[0] = 3;
	buf[1] = 0;		// track
	if (s_ok != SendPacket(buf, 2, true))
		return false;
	if (s_ok != RecvPacket(buf, 1))
		return false;
	
	int bt;
	if (puttime)
		bt = GetTickCount();
	int l;
	for (int tr=0; tr<ntracks-1; tr++)
	{
		image.trackptr[tr] = file.Tellp();

		l = ReceiveTrack(true, tr, file);
		if (l == -1)
			return false;
		if (!l)
			image.trackptr[tr] = 0;
	}
	image.trackptr[tr] = file.Tellp();
	l = ReceiveTrack(false, tr, file);
	if (l == -1)
		return false;
	if (!l)
		image.trackptr[tr] = 0;

	if (puttime)
		printf("%d ms.\n", GetTickCount() - bt);
	
	image.disksize = file.Tellp();
	file.Seek(0, FileIO::begin);
	file.Write(&image, sizeof(image));
	
	static const uint8 cmd[] = { 0x00 };
	SendPacket(cmd, 1, true);
	return true;
}

// ---------------------------------------------------------------------------
//	トラックの内容を取り出す
//
int TransDisk2::ReceiveTrack(bool readnext, int track, FileIO & file)
{
	uint8 buf[0x4000];
	SectorHeader sh;
	memset(&sh, 0, sizeof(sh));
	
	printf("Track %3d:", track);
	
	buf[0] = readnext ? 5 : 4;
	buf[1] = readnext ? track + 1 : burst;
	buf[2] = burst;

	if (s_ok != SendPacket(buf, 3, true))
		return -1;
	buf[0] = 0;
	if (s_ok != RecvPacket(buf, 0x4000, burst))
		return -1;

	int nsecs = buf[0];
	if (!nsecs)
	{
		printf(" unformatted.\n");
		return 0;
	}
	
	printf(" %2d sectors.\n", nsecs);
	
	XD2IDR* idr = (XD2IDR*) (buf + 1);
	IDDecompress(idr, nsecs, track);
	
	uint8* sect = (uint8*) &idr[nsecs];
	
	int i;
	for (i=0; i<nsecs; i++)
	{
		if (verbose)
			printf(	"%s %.2x %.2x %.2x %.2x  %.2x %.4x %.4x", 
					idr[i].density ? "MFM" : "FM",
					idr[i].c, idr[i].h, idr[i].r, idr[i].n, 
					idr[i].st0, idr[i].off, idr[i].len);
	
		sh.c = idr[i].c;
		sh.h = idr[i].h;
		sh.r = idr[i].r;
		sh.n = idr[i].n;
		sh.sectors = nsecs;
		sh.density = idr[i].density ^ 0x40;
		sh.deleted = idr[i].st2 & 0x40 ? 0x10 : 0;
		sh.status = 0;
		sh.length = idr[i].len; 
		if ((idr[i].st0 & 0xc0) == 0x40)
		{
			if (idr[i].st1 & 0x20)		// CRC
			{
				if (idr[i].st2 & 0x20)
				{
					if (verbose)
						printf(" err:DATA CRC");
					sh.status = 0xb0;	// DATA CRC
				}
				else
				{
					if (verbose)
						printf(" err:ID CRC");
					sh.status = 0xa0;	// ID CRC
					sh.length = 0;
				}
			}
			else
			{
				if (idr[i].st2 & 1)
				{
					sh.status = 0xf0;	// MAM
					sh.length = 0;
					if (verbose)
						printf(" err:MAM");
				}
			}
		}
		if (verbose)
			printf("\n");

		file.Write(&sh, 16);
		file.Write(sect + idr[i].off, idr[i].len);
	}
	if (verbose)
		printf("\n");

	return 1;
}

// ---------------------------------------------------------------------------

void TransDisk2::IDDecompress(XD2IDR* idr, int nsecs, int track)
{
	const uint8 dummyidr[] = { 0, 0, 0, 0, 0, 0, 0, 1, 0x40, 0, 0, 0 };
	XD2IDR* prev = (XD2IDR*) dummyidr;
	prev->c = track >> 1;
	prev->h = track & 1;

	for (int i=0; i<nsecs; i++)
	{
//		for (int j=0; j<12; j++) printf("%.2x ", ((uint8*) idr)[j]); printf("\n");
		// R = RP + 1 - @
		idr->r = prev->r + 1 - idr->r;

		// @ + p = c
		idr->c = prev->c + idr->c;
		idr->h = prev->h + idr->h;
		idr->n = prev->n + idr->n;
		idr->density = prev->density + idr->density;
		idr->st0 = prev->st0 + idr->st0;
		idr->st1 = prev->st1 + idr->st1;
		idr->st2 = prev->st2 + idr->st2;
		
		int ss = idr->n < 7 ? (0x80 << idr->n) : 0x100;

		idr->len = idr->len + ss;
		idr->off = i ? prev->off + ss - idr->off : -idr->off;

		prev = idr++;
	}
}

// ---------------------------------------------------------------------------
//	ROM データを取り出す
//
bool TransDisk2::ReceiveROM()
{
	struct ROMINFO
	{
		const char* name;
		int index;
		int blocks;
	};

	const static ROMINFO rominfo[] = 
	{
		{ "kanji1.rom",	0x00, 0x20 },	
		{ "kanji2.rom",	0x20, 0x20 },
		{ "n88.rom",	0x40, 0x08 },
		{ "n80.rom",	0x48, 0x08 },
		{ "n88_0.rom",	0x50, 0x02 },	
		{ "n88_1.rom",	0x52, 0x02 },	
		{ "n88_2.rom",	0x54, 0x02 },	
		{ "n88_3.rom",	0x56, 0x02 },
		{ "disk.rom",	0x58, 0x02 },
		{ "cdbios.rom",	0x60, 0x10 },
		{ "jisyo.rom",	0x70, 0x80 },
	};

	// 読み込むべき ROM を選ぶ
	uint bitmap = 0x1fc;
	uint nblks = 0x1a;
	uint8 romtype = GetSysInfo()->romtype;
	if (romtype & 1) bitmap |= 0x001, nblks += 0x20;
	if (romtype & 2) bitmap |= 0x002, nblks += 0x20;
	if (romtype & 4) bitmap |= 0x400, nblks += 0x10;
	if (romtype & 8) bitmap |= 0x200, nblks += 0x80;

	uint8 buf[0x1000];
	int n=0;
	for (int j=0; j<sizeof(rominfo)/sizeof(rominfo[0]); j++)
	{
		if (bitmap & (1 << j))
		{
			FileIO file;
			if (!file.Open(rominfo[j].name, FileIO::create))
				return false;

			for (int i=0; i<rominfo[j].blocks; i++)
			{
				printf("ROM データを受信中 (%d/%d)\r", ++n, nblks);
				
				uint8 cmd[3];
				cmd[0] = 0x07;
				cmd[1] = rominfo[j].index + i;
				cmd[2] = burst;

				uint r = SendPacket(cmd, 3, true);
				if (r != s_ok)
				{
					printf("\nError1: %d\n", r);
					return false;
				}

				r = RecvPacket(buf, 0x1000, burst);
				if (r != s_ok)
				{
					printf("\nError2: %d\n", r);
					return false;
				}
					
				// 2D 環境の DISK ROM は 800h bytes
				if (j == 8 && GetSysInfo()->fddtype == 0)
				{
					file.Write(buf, 0x800);
					break;
				}
				file.Write(buf, 0x1000);
			}
		}
	}
	printf("\n\n");
	
	static const uint8 cmd[] = { 0x00 };
	SendPacket(cmd, 1, true);
	return true;
}

// ---------------------------------------------------------------------------
//	ディスクの内容を取り出す
//
bool TransDisk2::SendDisk(int drive, FileIO& file)
{
	uint8 buffer[0x3800];
	uint r;

	const char* type[] = { "2D", "2DD", "???", "2HD" };
	
	ImageHeader ih;
	file.Read(&ih, sizeof(ImageHeader));
	
	if (!CheckHeader(&ih))
		return false;
	
	media = ih.disktype == 0x20 ? 3 : ih.disktype == 0x10 ? 1 : 0;

	printf("[%.16s] (%s) を書き戻します.\n", ih.title, type[media]);
	if (!GetSysInfo()->fddtype && media)
	{
		printf("この機種では %s のディスクイメージは書き戻しできません．\n", type[media]);
		return false;
	}
	
	while (1)
	{
		printf("PC88 のドライブ %d に新しいディスクを入れてキーを押してください.\n", drive+1, type[media]);
		getchar();
		
		// SET DRIVE
		buffer[0] = 2;
		buffer[1] = drive;	
		buffer[2] = media | 0x80;
		r = SendPacket(buffer, 3, true);
		if (r != s_ok)
			return false;

		r = RecvPacket(buffer, 1);
		if (r != s_ok)
			return false;

		if (verbose)
			printf("media = %d\n", buffer[0]);

		if (buffer[0] < 4)
			break;
	
		if (buffer[0] == 0xf9)
		{
			printf("ディスクはライトプロテクトされています.\n");
			return false;
		}

		if (buffer[0] != 0xfe)
		{
			printf("ディスク操作に関するエラーが生じました.\n");
			return false;
		}
	}

	int ntracks = media & 1 ? 164 : 84;

	IDR idr[64];

	for (int tr=0; tr<ntracks; tr++)
	{
		printf("Track %3d:", tr);
		int i=0;
		if (ih.trackptr[tr])
		{
			file.Seek(ih.trackptr[tr], FileIO::begin);
			uint8* buf = buffer;
			int buffre = 0x3000;

			SectorHeader sh;
			do
			{
				file.Read(&sh, sizeof(SectorHeader));
				int length = sh.length > buffre ? buffre : sh.length;
				
				file.Read(buf, length);
				if (sh.length - length)
					file.Seek(sh.length - length, FileIO::current);
				
				memset(&idr[i], 0, sizeof(IDR));
				idr[i].c = sh.c;
				idr[i].h = sh.h;
				idr[i].r = sh.r;
				idr[i].n = sh.n;
				idr[i].density = sh.density ^ 0x40;
				idr[i].buffer = buf;
				idr[i].length = length;
				idr[i].deleted = sh.deleted;
				idr[i].status = sh.status;
				
				buf += length;
				buffre -= length;
			} while (++i<sh.sectors);
			printf(" %2d sectors", i);
		}
		else
		{
			printf(" unformatted");
		}

		uint8 seq[0x3800];
		int l = MakeWriteSeq(seq, idr, i);
		if (l < 0)
			return false;

		uint8 cmd[4];
		cmd[0] = 9;
		cmd[1] = tr;	
		cmd[2] = l & 0xff;
		cmd[3] = l >> 8;
		
		if (SendPacket(cmd, 4, true) != s_ok)
			return false;
		if (SendPacket(seq, l, false, true) != s_ok)
			return false;
		if (RecvPacket(cmd, 1) != s_ok)
			return false;
		printf(".\n");
	}

	static const uint8 cmd[] = { 0x00 };
	SendPacket(cmd, 1, true);
	return true;
}

// ---------------------------------------------------------------------------
//	トラック書き込み
//
int TransDisk2::MakeWriteSeq(uint8* dest, IDR* idr, int nsec)
{
	int i;

	// ID
	if (!nsec)
	{
		dest[0] = 0x41;					// WRITE ID
		dest[1] = media & 2 ? 7 : 6;	// LE
		dest[2] = 1;					// NS
		dest[3] = 6;					// GAP
		dest[4] = 0x4e;					// D
		dest[5] = 0;
		return 6;
	}

	uint8* dorg = dest;
	
	int le = -1;
	bool mlen = false;
	for (i=0; i<nsec; i++)
	{
		if (!(idr[i].status))
		{
			if (le != -1 && le != idr[i].n) 
				mlen = true;
			else
				le = idr[i].n;
		}
	}

	if (mlen)
	{
		int gap3;
		int tracksize = (media & 2 ? 10416 : 6250) - 50;
		if (idr[0].density == 0) tracksize >>= 1;
		if (!CalcMixLengthCondition(idr, nsec, &le, &gap3, tracksize))
		{
			printf("[TOO DIFFICULT!]");
		}
		MakeMixLengthID(dest, idr, nsec, le, gap3);
	}
	else
	{
		MakeNormalID(dest, idr, nsec, le);
	}
	
	// DATA
	for (i=0; i<nsec; i++)
	{
		if (idr[i].status)
			continue;

		*dest++ = 2 | idr[i].density | (idr[i].deleted ? 0x80 : 0);
		*dest++ = idr[i].c;
		*dest++ = idr[i].h;
		*dest++ = idr[i].r;
		*dest++ = idr[i].n;
		*dest++ = idr[i].length & 0xff;
		*dest++ = (idr[i].length >> 8) & 0xff;
		memcpy(dest, idr[i].buffer, idr[i].length);
		dest += idr[i].length;
	}
	*dest++ = 0;

	return dest - dorg;
}

// ----------------------------------------------------------------------------
//	ノーマルトラックフォーマット
//
void TransDisk2::MakeNormalID(uint8*& dest, IDR* idr, int nsec, int le)
{
	le = le > 8 ? 8 : le;
	int slen = (0x80 << le);
	int gap3;
	int tracksize = media & 2 ? 10416 : 6250;
	
	if (!idr->density)
	{
		tracksize >>= 1;
		gap3 = (tracksize - 137 - nsec * ((0x80 << le) + 32)) / nsec;
		if (gap3 < 12)
			gap3 = (tracksize - 73 - nsec * ((0x80 << le) + 32)) / nsec;
		if (gap3 < 8)
			gap3 = (tracksize - 32 - nsec * ((0x80 << le) + 32)) / nsec;
	}
	else
	{
		gap3 = (tracksize - 274 - nsec * ((0x80 << le) + 62)) / nsec;
		if (gap3 < 24)
			gap3 = (tracksize - 146 - nsec * ((0x80 << le) + 62)) / nsec;
		if (gap3 < 16)
			gap3 = (tracksize - 64 - nsec * ((0x80 << le) + 62)) / nsec;
	}
	
	*dest++ = 1 | idr->density;	// WRITE ID
	*dest++ = le;	// LE
	*dest++ = nsec;	// NS
	*dest++ = gap3; // GAP
	*dest++ = 0;	// D
	for (int i=0; i<nsec; i++)
	{
		*dest++ = idr[i].c;
		*dest++ = idr[i].h;
		*dest++ = idr[i].r;
		*dest++ = idr[i].n;
	}
	printf(" (N:%d Gap3:%.2xh)", le, gap3);
}

// ----------------------------------------------------------------------------
//	ミックスレンクトラックフォーマット
//
void TransDisk2::MakeMixLengthID(uint8*& dest, IDR* idr, int nsec, int le, int gap)
{
	*dest++ = 1 | idr->density;	// WRITE ID
	*dest++ = le;	// LE
	uint8* psec = dest++;
	*dest++ = gap; // GAP
	*dest++ = 0x4e;	// D
	printf(" (N:%d Gap3:%.2xh[Mixed])", le, gap);

	int misc = idr->density ? 64 : 34;
	int sos = (0x80 << le) + 0x3e + gap;
	int ws = 0;
	for (int s=0; s<nsec; s++)
	{
		if (!idr->status)
		{
			int size = sos - ((0x80 << idr->n) + misc);
			
//			printf("%.2x %.2x %.2x %.2x\n", idr->c, idr->h, idr->r, idr->n);
			*dest++ = idr->c;
			*dest++ = idr->h;
			*dest++ = idr->r;
			*dest++ = idr->n;
			idr++, ws++;
			
			if (s < nsec - 1 && size < 0)
			{
				while (size<0)
				{
//					printf("-- -- -- --\n");
					dest += 4, size += sos, ws++;
				}
			}
		}
	}
	*psec = ws;
}

// ----------------------------------------------------------------------------
//	ミックスセクタトラックフォーマット
//	最適なフォーマットパターンの探索
//
bool TransDisk2::CalcMixLengthCondition(IDR* idr, int nsec, int* mle, int* mgap, int tracksize)
{
	int le;
	int gap3;
	int max = 0;
	int misc = idr->density ? 62 : 33;
	for (le=0; le<=3; le++)
	{
		for (gap3=2; gap3<256; gap3++)
		{
			int sgap = 9999;
			int sos = (0x80 << le) + misc + gap3;
			int bytes = 0;
			IDR* i = idr;
			int ns = 0;
			for (int s=0; s<nsec; s++, i++)
			{
				if (!i->status)
				{
					int size = -((0x80 << i->n) + misc + 2);
					while (size<0)
						size += sos, bytes += sos, ns++;
					if (sgap >= size)
						sgap = size;
				}
			}
			if (bytes < tracksize && ns < 64)
			{
				if (max < sgap)
				{
//					printf("sgap: %d  bytes: %d\n", sgap, bytes);
					max = sgap, *mle = le, *mgap = gap3;
				}
			}
		}
	}
	return max > 0;
}

// ---------------------------------------------------------------------------
//	ヘッダー中のごみ情報の削除
//
bool TransDisk2::CheckHeader(ImageHeader* ih)
{
	uint trackstart = sizeof(ImageHeader);
	for (int t=0; t<84; t++)
	{
		if (ih->trackptr[t] && ih->trackptr[t] < trackstart)
			trackstart = (uint) ih->trackptr[t];
	}
	if (trackstart < sizeof(ImageHeader))
		memset(((char*) ih) + trackstart, 0, sizeof(ImageHeader)-trackstart);
	return true;
}

