// ---------------------------------------------------------------------------
//	TransDisk 2.0
//	Copyright (C) cisc 2000.
// ---------------------------------------------------------------------------
//	$Id: main.cpp,v 1.11 2000/02/01 13:09:23 cisc Exp $

#include "headers.h"
#include <mbstring.h>
#include "types.h"
#include "xdisk.h"
#include "file.h"

void Usage();

bool verbose;
int version_l = 7;

static bool D88Seek(FileIO& fio, int index);

// ---------------------------------------------------------------------------
//	MAIN
//
int main(int ac, char** av)
{
	int drive = -1;
//	int retry = 10;
	char* filename = 0;
	char title[17] = "                ";
	bool writeprotect = false;
	bool fastrecv = true;
	verbose = false;
	int media = -1;
	uint baud = 19200;
	uint port = 1;
	int index = 1;

	printf("TransDisk 2.%.2d\n"
		   "Copyright (C) 2000 cisc.\n\n", version_l);

	if (ac < 2)
		Usage();
	
	int mode = tolower(av[1][0]);
	
	av += 2;
	for (int i=2; i<ac; i++, av++)
	{
		char* arg = *av;
		
		if (arg[0] == '-')
		{
			for (int j=1; arg[j]; j++)
			{
				switch (tolower(arg[j]))
				{
				case 'v':
					verbose = true;
					break;
				
				case 'w':
					writeprotect = true;
					break;

				case 's':
					fastrecv = false;
					break;
				
				case 'p':
					port = atoi(arg + j + 1);
					goto next;

				case 'm':
					media = atoi(arg + j + 1);
					if (media < 0 || media > 2)
						Usage();
					if (media == 2)
						media = 3;
					goto next;

				case 'd':
					drive = atoi(arg + j + 1);
					if (drive != 1 && drive != 2)
						Usage();
					goto next;

				case 't':
					if (++i < ac)
					{
						strncpy(title, *++av, 16);
						goto next;
					}
					Usage();

				case '1':
					baud = 19200;
					break;
				
				case '9':
					baud = 9600;
					break;
				
				default:
					Usage();
				}
			}
		}
		else
		{
			if (filename)
				Usage();
			filename = arg;
		}
next:	
		;
	}

	if (mode == 'r' || mode == 'w')
	{
		if (!filename || (drive != 1 && drive != 2))
		{
			Usage();
		}
	}

	printf("�ʐM�|�[�g %d, %d bps �Őڑ����܂�.\n\n", port, baud);

	if (mode == 'b')
	{
		SIO sio;
		if (SIO::OK != sio.Open(port, baud))
		{
			printf("�ʐM�f�o�C�X�̏������Ɏ��s���܂���.\n");
			return 1;
		}

		printf( "88 �Ƀv���O������]�����܂��B\n"
				"88 �� load \042COM:N81X\042 �Ƃ����R�}���h�����s���Ă��������B\n\n"
				"��낵���ł��� ?");

		getchar();
		printf("\n");

		FileIO fio("xdisk2.bas", FileIO::readonly);
		fio.Seek(0, FileIO::end);
		uint size = fio.Tellp();
		if (size)
		{
			fio.Seek(0, FileIO::begin);

			int fs = size;
			while (fs > 0)
			{
				printf("�]���� (%d%%).\r", (size-fs) * 100 / size);
				
				uint8 buf[512];
				uint ps = fs > sizeof(buf) ? sizeof(buf) : fs;
				fio.Read(buf, ps);
				if (SIO::OK != sio.Write(buf, ps))
				{
					printf("�]���ł��܂���ł���.\n");
					return 0;
				}

				fs -= ps;
			}
			printf("�������܂���.\n");
		}
		return 0;
	}

	TransDisk2 xdisk;

	int e = xdisk.Connect(port, baud, fastrecv);

	if (e != XComm2::s_ok)
	{
		printf("�ʐM�f�o�C�X�̏������Ɏ��s���܂���(%d).\n", e);
		return 1;
	}
	
	switch (mode)
	{
	case 'r':
		{
			FileIO file;
			if (!file.Open(filename, FileIO::openalways) 
				|| (file.GetFlags() & FileIO::readonly))
			{
				fprintf(stderr, "�t�@�C�������܂���.\n");
				return 1;
			}
			D88Seek(file, 99999);

			xdisk.ReceiveDisk(drive-1, media, file);
			file.Seek(0, FileIO::begin);
			file.Write(title, 16);
			file.Seek(26, FileIO::begin);
			uint8 wp = writeprotect ? 0x10 : 0;
			file.Write(&wp, 1);
		}
		break;
		
	case 'w':
		{
			FileIO file;
			
			_mbstok((uchar*) filename, (uchar*) ";");
			char* x = (char*) _mbstok(0, (uchar*) ";");
			int index = x ? atoi(x) : 1;

			if (!file.Open(filename, FileIO::readonly))
			{
				fprintf(stderr, "�t�@�C�����J���܂���.\n");
				return 1;
			}
			if (!D88Seek(file, index))
			{
				fprintf(stderr, "�w�肳�ꂽ�f�B�X�N������܂���.\n");
				return 1;
			}
			xdisk.SendDisk(drive-1, file);
		}
		break;

	case 's':
		xdisk.ReceiveROM();
		break;
	
	default:
		Usage();
	}
	printf("�I�����܂����B\n");
	return 0;
}

// ---------------------------------------------------------------------------

static bool D88Seek(FileIO& fio, int index)
{
	fio.Seek(0, FileIO::end);
	int dis = fio.Tellp();

	fio.SetLogicalOrigin(0);
	int pos = 0;
	
	for (int i=1; i<index && pos < dis; i++)
	{
		uint32 size = 0;
		fio.Seek(pos + 0x1c, FileIO::begin);
		fio.Read(&size, 4);
		if (size == 0xdeadd13c)
			break;
		if (size == 0 || size > 4 * 1024 * 1024)
			return false;
		pos += size;
	}
	if (verbose)
		printf("disk index: %d\n");
	fio.SetLogicalOrigin(pos);
	fio.Seek(0, FileIO::begin);
	return true;
}

// ---------------------------------------------------------------------------

void Usage()
{
	printf(
		   "usage:\n"
		   "  xdisk2 b [-p#]\n"
		   "          PC-8801 ���Ƀv���O������]������\n"
		   "  xdisk2 s [-p#] [-19s]\n"
		   "          PC-8801 �� ROM �f�[�^���z���o��\n"
		   "  xdisk2 r [-p#] [-d#] [-m#] [-19svw] [-t \"title\"] <disk.d88>\n"
		   "          �f�B�X�N����f�B�X�N�C���[�W���쐬����\n"
		   "  xdisk2 w [-p#] [-d#] [-19v] <disk.d88(;#)>\n"
		   "          �f�B�X�N�C���[�W���f�B�X�N�ɏ����o��\n"
		   "\n"
		   "option:\n"
           "    -p#   �ʐM�|�[�g�ԍ����w��\n"
           "    -1/-9 19200/9600 bps �ŒʐM\n"
		   "    -d#   �ǂݏ����Ɏg�p����h���C�u�ԍ����w��\n"
		   "    -m#   �ǂݏo���f�B�X�N�̃��f�B�A���w��(0:2D 1:2DD 2:2HD)\n"
		   "    -v    �ڍ׏o��\n"
		   "    -w    �ǂݍ��񂾃C���[�W�Ƀ��C�g�v���e�N�g��������\n"
		   "    -s    88 -> PC �Ԃō����]�����s��Ȃ� (19200 bps ���̂�)\n"
		   "    -t    �f�B�X�N�C���[�W�̃^�C�g�����w�肷��\n"
		  );

	exit(1);
}
