// ---------------------------------------------------------------------------
//	generic file io class for Win32
//	Copyright (C) cisc 1998, 2000.
// ---------------------------------------------------------------------------
//	$Id: file.cpp,v 1.2 2000/01/13 13:39:58 cisc Exp $

#include "headers.h"
#include "file.h"

// ---------------------------------------------------------------------------
//	�\�z/����
//
FileIO::FileIO()
{
	flags = 0;
}

FileIO::FileIO(const char* filename, uint flg)
{
	flags = 0;
	Open(filename, flg);
}

FileIO::~FileIO()
{
	Close();
}

// ---------------------------------------------------------------------------
//	�t�@�C�����J��
//
// Ignoring flags, this simply opens a file pointer for reading.
bool FileIO::Open(const char* filename, uint flg)
{
	Close();

	strncpy(path, filename, MAX_PATH);

	DWORD access = (flg & readonly ? 0 : GENERIC_WRITE) | GENERIC_READ;
	DWORD share = (flg & readonly) ? FILE_SHARE_READ : 0;
	DWORD creation = flg & create ? CREATE_ALWAYS : (flg & openalways) ? OPEN_ALWAYS : OPEN_EXISTING;
	/*
	hfile = CreateFile(filename, 	// lpFileName
			access, 	// dwDesiredAccess
			share, 		// dwShareMode
			0, 		// lpSecurityAttributes
			creation, 	// dwCreationDisposition
			0, 		// dwFlagsAndAttributes
			0);		// hTemplateFile
	*/
	// opening files in read only mode is the only thing that work satm
	if ( flg & readonly )
	{
		hfile = fopen(filename, "rb");
	}
	else
	{
		hfile = fopen(filename, "wb");
	}
	flags = (flg & readonly) | (hfile == INVALID_HANDLE_VALUE ? 0 : open);
	if (!(flags & open))
	{
		//GetLastError is windows ish
		//switch (GetLastError())
		//{
		//	case ERROR_FILE_NOT_FOUND:		
		//		error = file_not_found; 
		//		break;
		//	case ERROR_SHARING_VIOLATION:	
		//		error = sharing_violation; 
		//		break;
		//	default: 
				error = unknown; 
		//		break;
		//}
	}
	
	// Open a new file using name 'filename'. Ignore flags for now.
	//hfile = fopen(filename, "rb");

	SetLogicalOrigin(0); // fseek(hfile, 0, SEEK_SET); i think

	return !!(flags & open);
	//return true;
}

// ---------------------------------------------------------------------------
//	�t�@�C�����Ȃ��ꍇ�͍쐬
//
// This is never used in the program.
bool FileIO::CreateNew(const char* filename)
{
	Close();

	strncpy(path, filename, MAX_PATH);

	//DWORD access = GENERIC_WRITE | GENERIC_READ;
	//DWORD share = 0;
	//DWORD creation = CREATE_NEW;

	//hfile = CreateFile(filename, access, share, 0, creation, 0, 0);
	hfile = fopen(filename, "wb");
	//flags = (hfile == INVALID_HANDLE_VALUE ? 0 : open);
	SetLogicalOrigin(0);

	return true;
	//return !!(flags & open);
}

// ---------------------------------------------------------------------------
//	�t�@�C������蒼��
//
/// I don't know what to do with this!
bool FileIO::Reopen(uint flg)
{
	//if (!(flags & open)) return false;
	//if ((flags & readonly) && (flg & create)) return false;

	//if (flags & readonly) flg |= readonly;

	//Close();

	//DWORD access = (flg & readonly ? 0 : GENERIC_WRITE) | GENERIC_READ;
	//DWORD share = flg & readonly ? FILE_SHARE_READ : 0;
	//DWORD creation = flg & create ? CREATE_ALWAYS : OPEN_EXISTING;

	//hfile = CreateFile(path, access, share, 0, creation, 0, 0);
	
	//flags = (flg & readonly) | (hfile == INVALID_HANDLE_VALUE ? 0 : open);
	SetLogicalOrigin(0);

	return true;
	//return !!(flags & open);
}

// ---------------------------------------------------------------------------
//	�t�@�C�������
//
void FileIO::Close()
{
	if (GetFlags() & open)
	{
		//CloseHandle(hfile);
		fclose(hfile);
		flags = 0;
	}
}
// ---------------------------------------------------------------------------
//	�t�@�C���k�̓ǂݏo��
//
int32 FileIO::Read(void* dest, int32 size)
{
	if (!(GetFlags() & open))
		return -1;
	
	size_t readsize; // I erased ReadFile oops
	readsize = fread(dest, 1, size, hfile);
	return readsize;
}

// ---------------------------------------------------------------------------
//	�t�@�C���ւ̏����o��
//
int32 FileIO::Write(const void* dest, int32 size)
{
	if (!(GetFlags() & open) || (GetFlags() & readonly))
		return -1;
	
	DWORD writtensize;
	//if (!WriteFile(hfile, dest, size, &writtensize, 0))
	//	return -1;
	writtensize = fwrite(dest, 1, size, hfile);
	return writtensize;
}

// ---------------------------------------------------------------------------
//	�t�@�C�����V�[�N
//
bool FileIO::Seek(int32 pos, SeekMethod method)
{
	if (!(GetFlags() & open))
		return false;
	
	DWORD wmethod;
	int s;
	switch (method)
	{
	case begin:	
		wmethod = SEEK_SET; 
		s = fseek(hfile, pos, SEEK_SET);
		pos += lorigin; 
		break;
	case current:
		s = fseek(hfile, pos, SEEK_CUR);	
		wmethod = SEEK_CUR; 
		break;
	case end:		
		s = fseek(hfile, pos, SEEK_END);
		wmethod = SEEK_END; 
		break;
	default:
		return false;
	}
	if (s == 0) return true;
	else return false;
	//return 0xffffffff != SetFilePointer(hfile, pos, 0, wmethod);
}

// ---------------------------------------------------------------------------
//	�t�@�C���̈ʒu�𓾂�
//
int32 FileIO::Tellp()
{
	if (!(GetFlags() & open))
		return 0;
	// returns the int32 offset into the file. 
	return ftell(hfile) - lorigin;
	//return SetFilePointer(hfile, 0, 0, FILE_CURRENT) - lorigin;
}

// ---------------------------------------------------------------------------
//	���݂̈ʒu���t�@�C���̏I�[�Ƃ���
//
bool FileIO::SetEndOfFile()
{
	if (!(GetFlags() & open))
		return false;
	//return ::SetEndOfFile(hfile) != 0;
	return fseek(hfile, 0, SEEK_END);
}
