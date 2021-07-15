//	$Id: lz77e.cpp,v 1.2 2000/01/15 00:31:41 cisc Exp $

#include "headers.h"
#include "lz77e.h"

// #define DBGLOG

LZ77Enc::LZ77Enc()
{
	prev = new const uint8* [dictsize];
	hash = new const uint8* [hashsize];
}

LZ77Enc::~LZ77Enc()
{
	delete[] prev;
	delete[] hash;
}

inline void LZ77Enc::AddHashTable(const uint8* at, uint hval)
{
	assert(hash[hval] != at);
	prev[(at-src) & (dictsize-1)] = hash[hval];
	hash[hval] = at;
}

inline uint LZ77Enc::GetMatchLen(const uint8* so, const uint8* d, int match)
{
	const uint8* s;
	const uint8* me = so + match;
	uint32 m;
	for (s=so; !(m = *(const uint32*)s ^ *(const uint32*) d) && s < me; s+=4, d+=4)
		;
	if (!m)
		return s+4-so;
	if (!(m & 0xffffff))
		return s+3-so;
	if (!(m & 0xffff))
		return s+2-so;
	if (!(m & 0xff))
		return s+1-so;
	return s-so;
}

inline uint LZ77Enc::GetMatchLen2(const uint8* so, const uint8* d, int minmatch, int maxmatch)
{
	const uint8* s;
	const uint8* me;
	
	for (s=so, me = so + (minmatch & ~3); s < me; s+=4, d+=4)
	{
		if (*(const uint32*)s ^ *(const uint32*) d)
			return 0;
	}

	uint32 m;
	for (me = so+maxmatch; !(m = *(const uint32*)s ^ *(const uint32*) d) && s < me; s+=4, d+=4)
		;
	
	if (!m)
		return s+4-so;
	if (!(m & 0xffffff))
		return s+3-so;
	if (!(m & 0xffff))
		return s+2-so;
	if (!(m & 0xff))
		return s+1-so;
	return s-so;
}

inline uint LZ77Enc::Hash(const uint8* s)
{
	return ((((s[0] << hashshift) ^ s[1]) << hashshift) ^ s[2]) & (hashsize-1);
}

inline void LZ77Enc::HashAdd(uint& h, uint v)
{
	h = ((h << hashshift) ^ v) & (hashsize-1);
}

inline void LZ77Enc::EmitBit(uint b)
{
	if (b)
		*key |= obit;
	if (!(obit >>= 1))
		Flush();
}

#ifdef DBGLOG
FILE* fp;
#endif

inline void LZ77Enc::EmitChar(uint c)
{
#ifdef DBGLOG
	fprintf(fp, "(0x%.2x)\n", c);
#endif
	EmitBit(1);
	*dest++ = c;
}

/*
	1 [xx]			�����k
	0 [xx] 0		near (3 bytes)
	0 [00]			end
	0 [xx] 1x1		200h
	0 [xx] 1x01		400h
	0 [xx] 1x00xxx	1000h
	
		- 1			3 bytes
		- 01		4
		  001		5
		  0001		6
		  000010	7
		  000011	8
		  000000 xxx 9 - 16
		  000001[xx] 272
	
	0 [xx] 1x1		 200h
	0 [xx] 1x01		 400h
	0 [xx] 1x00m1	 800h
	0 [xx] 1x00m0n1  1000h
	0 [xx] 1v00w0x0y 2000h

*/

void LZ77Enc::EmitLZ(uint len, uint pos)
{
#ifdef DBGLOG
	fprintf(fp, "%d, %d\n", pos, len);
#endif

	int r = -(int)pos;
	EmitBit(0);
	*dest++ = r & 0xff;

	if (len == 3 && pos <= 0xff)
	{
		EmitBit(0);
		return;
	}
	EmitBit(1);
	if (pos <= 0x200)						// 1z1
		EmitBit(r & 0x100), EmitBit(1);
	else if (pos <= 0x400)					// 1y01
		EmitBit(r & 0x100), EmitBit(0), EmitBit(1);
	else if (pos <= 0x800)					// 1y00z1
		EmitBit(r & 0x200), EmitBit(0), EmitBit(0), EmitBit(r & 0x100), EmitBit(1);
	else if (pos <= 0x1000)					// 1x00y0z1
		EmitBit(r & 0x400), EmitBit(0), EmitBit(0), EmitBit(r & 0x200), EmitBit(0), EmitBit(r & 0x100), EmitBit(1);
	else if (pos <= 0x2000)					// 1w00x0y0z
		EmitBit(r & 0x800), EmitBit(0), EmitBit(0), EmitBit(r & 0x400), EmitBit(0), EmitBit(r & 0x200), EmitBit(0), EmitBit(r & 0x100);
	else
	{
		printf("encode error: %d\n", pos);
	}
	
	for (uint i=3; i<=6; i++)
	{
		if (len == i) 
		{ 
			EmitBit(1); 
			return; 
		} 
		else 
		{ 
			EmitBit(0);
		}
	}
	if (len <= 8)
	{
		EmitBit(1); EmitBit(len-7);
		return;
	}
	EmitBit(0);
	if (len <= 16)
	{
		EmitBit(0); 
		EmitBit((len-9) & 4); EmitBit((len-9) & 2); EmitBit((len-9) & 1);
	}
	else
	{
		EmitBit(1);
		*dest++ = len - 17;
	}
}

void LZ77Enc::Flush()
{
	obit = 0x80, bptr = 1;
	if (dest - buf > bufsize - 0x20)
	{
		throw (void*) 0;
	}
	key = dest; *dest++ = 0;
}

int  LZ77Enc::Encode(const void* _s, int len, uint8* _d, uint dsize)
{
#ifdef DBGLOG
	static int index;
	char name[16];
	sprintf(name, "elz%.3d.txt", ++index);
	fp = fopen(name, "w");
#endif

	const int searchlen = 272;
		
	src = (const uint8*) _s;
	dest = buf = _d;
	bufsize = dsize;

	*(uint16*)dest = 0x8000 | len;
	dest += 2;
	
	key = dest; *dest++ = 0, obit = 0x80, bptr = 1;
	
	int i;
	const uint8* srctop = src + len;
	for (i=0; i<dictsize; i++)	prev[i] = src - dictsize;
	for (i=0; i<hashsize; i++)	hash[i] = src - dictsize;
	
	const uint8* lastmpos;
	int lastbest = threshold - 1;
	
	uint h = Hash(src);
	AddHashTable(src, h);
	
	const uint8* s = src;
	for (s = src+1; s < srctop-2; )
	{
		int best = threshold - 1;
		const uint8* matchpos;
		int maxmatch = (srctop - s < searchlen) ? srctop - s : searchlen;
		
		HashAdd(h, s[2]);
		assert(h == Hash(s));
		// get match len
		for (const uint8* p = hash[h]; s - p < dictsize; p = prev[(p-src) & (dictsize-1)])
		{
			if (p[best+1] == s[best+1])
			{
				int match = GetMatchLen2(s, p, best, maxmatch);
				if (match > best)
				{
					best = match, matchpos = p;
					if (best >= maxmatch)
					{
						best = maxmatch;
						break;
					}
				}
			}
		}
		AddHashTable(s, h);
		
		if (best < lastbest || lastbest == maxmatch)
		{
			EmitLZ(lastbest, s - lastmpos - 1);
			s++;
			for (int l = lastbest-1; l>0; l--)
			{
				HashAdd(h, s[2]);
				assert(h == Hash(s));
				
				AddHashTable(s, h);
				s++;
			}
			lastbest = threshold - 1;
		}
		else
		{
			EmitChar(s[-1]);
			lastbest = best, lastmpos = matchpos;
			s++;
		}
	}
	if (lastbest >= threshold)
	{
		EmitLZ(lastbest, s - lastmpos - 1);
		s += lastbest;
	}
	for (; s <= srctop; s++)
		EmitChar(s[-1]);

	EmitBit(0);
	*dest++ = 0;
	EmitBit(0);

#ifdef DBGLOG
	fclose(fp);
#endif
	return dest - buf;
}


#define GetBit()	(r=(bit>>bc)&1, bc--||(bit=*src++,bc=7), r)  

void LZ77Enc::Decode(uint8* dest, const uint8* src, const uint8* test)
{
#ifdef DBGLOG
	static int index = 0;
	char name[16];
	sprintf(name, "dlz%.3d.txt", ++index);
	FILE* fp = fopen(name, "w");
#endif
	
	uint bc=7, r;
	uint8* dorg = dest;
	uint bit = *src++;
	while (1)
	{
		if (GetBit())
		{
#ifdef DBGLOG
			fprintf(fp, "%.4x: (0x%.2x)\n", dest-dorg, *src);
#endif
			*dest++ = *src++;
			if (dest[-1] != *test++)
			{
				printf("decode error: 1[xx]\n");
				return;
			}
		}
		else
		{
			int x = ~0xff | *src++;
			int len;
			
			int y = ~0;
			int k=0, d;
#define GETBIT()	(d = GetBit(), k=k*2+d, d)
			
			if (!GETBIT())
			{
				if (x & 0xff)
					len = 3;
				else
					return;
			}
			else
			{

				y = (y << 1) | GETBIT();				// a
				if (!GETBIT())
				{
					y &= ~2;
					for (int i=0; i<3; i++)
					{
						if (GETBIT())
							break;
						y = (y << 1) | GETBIT();		// b
					}
				}
				x = (x & 0xff) | (y << 8);
				if (GetBit()) len = 3;
				else if (GetBit()) len = 4;
				else if (GetBit()) len = 5;
				else if (GetBit()) len = 6;
				else if (GetBit()) len = 7 + GetBit();
				else if (!GetBit())
				{
					len = GetBit();
					len = len * 2 + GetBit();
					len = len * 2 + GetBit() + 9;
				}
				else
					len = *src++ + 17;
			}
#ifdef DBGLOG
			fprintf(fp, "%.4x: %d, %d\n", dest - dorg, -x, len);
#endif
			int l0 = len;
			for (; len>0; len--)
			{
				dest[0] = dest[x];
				if (dest[0] != *test++)
				{
					printf("decode error: %d, %d (%.2x)\n", x, l0, k);
					return;
				}
				dest++;
			}
		}
	}
#ifdef DBGLOG
	fclose(fp);
#endif
}
