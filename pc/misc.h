//	$Id: misc.h,v 1.2 2000/01/04 14:53:17 cisc Exp $

#ifndef MISC_H
#define MISC_H

inline int Max(int x, int y) { return (x > y) ? x : y; }
inline int Min(int x, int y) { return (x < y) ? x : y; }
inline int Limit(int v, int max, int min) { return Max(min, Min(max, v)); }

#endif // MISC_H

