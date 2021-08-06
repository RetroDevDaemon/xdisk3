// ---------------------------------------------------------------------------
//	TransDisk 3.0
//	Copyright (C) cisc 2000.
// ---------------------------------------------------------------------------
//	$Id: main.cpp,v 1.11 2000/02/01 13:09:23 cisc Exp $


#include "headers.h"
//#include <mbstring.h> 
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

	printf("TransDisk 3.%.2d\n"
		   "Copyright (C) 2000 cisc.\n"
		   "Unix port (C) 2021 b.f. \n\n", version_l);

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

	//printf("通信ポート %d, %d bps で接続します.\n\n", port, baud);
	printf("Connected to port %d at %d bps.\n\n", port, baud);
	if (mode == 'b')
	{
		SIO sio;
		if (SIO::OK != sio.Open(port, baud))
		{
			//printf("通信デバイスの初期化に失敗しました.\n");
			printf("Failed to initialize serial device.");
			//printf("basic fail");
			return 1;
		}

		//printf( "88 にプログラムを転送します。\n"
		//	"88 で load \042COM:N81X\042 というコマンドを実行してください。\n\n"
		//	"よろしいですか ?");
		printf("Sending program to the 88.\n"
			"Please run the following command on your 88: load\"COM:N81X\"\n\n"
			"Ready?");

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
				//printf("転送中 (%d%%).\r", (size-fs) * 100 / size);
				printf("Transferring (%d%%).\r\n", (size-fs) * 100 / size);
				uint8 buf[512];
				uint ps = fs > sizeof(buf) ? sizeof(buf) : fs;
				fio.Read(buf, ps);
				if (SIO::OK != sio.Write(buf, ps))
				{
					//printf("転送できませんでした.\n");
					printf("Transfer failed.\n");
					return 0;
				}

				fs -= ps;
			}
			printf("Completed.\n");
			//printf("完了しました.\n");
		}
		return 0;
	}

	TransDisk2 xdisk;

	int e = xdisk.Connect(port, baud, fastrecv);
			
	if (e != XComm2::s_ok)
	{
		//printf("通信デバイスの初期化に失敗しました(%d).\n", e);
		printf("Failed to initialize serial device(%d).\n", e);
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
				//fprintf(stderr, "ファイルを作れません.\n");
				fprintf(stderr, "Could not create file.\n");
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
			
			//_mbstok((uchar*) filename, (uchar*) ";");
			//char* x = (char*) _mbstok(0, (uchar*) ";");
			int index = 1;//x ? atoi(x) : 1;
			if (!file.Open(filename, FileIO::readonly))
			{
				//fprintf(stderr, "ファイルを開けません.\n");
				fprintf(stderr, "Could not open file.\n");
				return 1;
			}
			if (!D88Seek(file, index))
			{
				//fprintf(stderr, "指定されたディスクがありません.\n");
				fprintf(stderr, "Given disk does not exist.\n");
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
	//printf("終了しました。\n");
	printf("Completed.\n");
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
	/*printf(
		   "usage:\n"
		   "  xdisk2 b [-p#]\n"
		   "          PC-8801 側にプログラムを転送する\n"
		   "  xdisk2 s [-p#] [-19s]\n"
		   "          PC-8801 の ROM データを吸い出す\n"
		   "  xdisk2 r [-p#] [-d#] [-m#] [-19svw] [-t \"title\"] <disk.d88>\n"
		   "          ディスクからディスクイメージを作成する\n"
		   "  xdisk2 w [-p#] [-d#] [-19v] <disk.d88(;#)>\n"
		   "          ディスクイメージをディスクに書き出す\n"
		   "\n"
		   "option:\n"
           "    -p#   通信ポート番号を指定\n"
           "    -1/-9 19200/9600 bps で通信\n"
		   "    -d#   読み書きに使用するドライブ番号を指定\n"
		   "    -m#   読み出すディスクのメディアを指定(0:2D 1:2DD 2:2HD)\n"
		   "    -v    詳細出力\n"
		   "    -w    読み込んだイメージにライトプロテクトをかける\n"
		   "    -s    88 -> PC 間で高速転送を行わない (19200 bps 時のみ)\n"
		   "    -t    ディスクイメージのタイトルを指定する\n"
		  );
	*/
	printf(
		"usage:\n"
		"  xdisk3 b [-p#]\n"
		"       Send the xdisk BASIC program to the PC-8801\n"
		"  xdisk3 s [-p#] [-19s]\n"
		"	Extract ROM data from the PC-8801\n"
		"  xdisk3 r [-p#] [-d#] [-m#] [-19svw] [-t \"title\"] <disk.d88>\n"
		"       Create disk image from physical disk\n"
		"  xdisk3 w [-p#] [-d#] [-19v] <disk.d88>\n"
		"	Write local disk image to physical disk\n"
		"\n"
		"options:\n"
		"    -p#   Port number\n"
		"    -1/-9 Set 19200/9600 bps\n"
		"    -d#   Designate target disk drive\n"
		"    -m#   Set disk media type (0:2d 1:2dd 2:2HD)\n"
		"    -v    Detailed output\n"
		"    -w    Set write protect on output disk image\n"
		"    -s    Don't use hi-speed transfer from 88->PC (19200 bps mode only)\n"
		"\n"
		"Note: Use \'ls /dev/ttyUSB*\' to show connected USB devices.\n"
		" (You may need to modify source if your USB serial device has\n"
		"  a different prefix.)\n"
		"\n"
	);
	exit(1);
}
