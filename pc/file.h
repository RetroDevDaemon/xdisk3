// ---------------------------------------------------------------------------
//	generic file io class
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: file.h,v 1.2 2000/01/13 13:39:58 cisc Exp $

#if !defined(win32_file_h)
#define win32_file_h

#include "types.h"
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

// ---------------------------------------------------------------------------

class FileIO
{
public:
	enum Flags
	{
		open		= 0x000001,
		readonly	= 0x000002,
		create		= 0x000004,
		openalways	= 0x000008,
	};

	enum SeekMethod
	{
		begin = 0, current = 1, end = 2,
	};

	enum Error
	{
		success = 0,
		file_not_found,
		sharing_violation,
		unknown = -1
	};

public:
	FileIO();
	FileIO(const char* filename, uint flg = 0);
	virtual ~FileIO();

	bool Open(const char* filename, uint flg = 0);
	bool CreateNew(const char* filename);
	bool Reopen(uint flg = 0);
	void Close();
	Error GetError() { return error; }

	int32 Read(void* dest, int32 len);
	int32 Write(const void* src, int32 len);
	bool Seek(int32 fpos, SeekMethod method);
	int32 Tellp();
	bool SetEndOfFile();

	uint GetFlags() { return flags; }
	void SetLogicalOrigin(int32 origin) { lorigin = origin; }

private:
	//HANDLE hfile;
	FILE* hfile;
	uint flags;
	uint32 lorigin;
	Error error;
	char path[MAX_PATH];
	
	FileIO(const FileIO&);
	const FileIO& operator=(const FileIO&);
};

#endif // win32_file_h
