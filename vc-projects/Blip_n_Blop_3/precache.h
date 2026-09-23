
#ifndef _Precache_
#define _Precache_

#include <stdio.h>
#include <array>

inline void Precache(const char * nf)
{
	FILE * f = fopen(nf, "rb");

	if (f == NULL) return;
	std::array<unsigned char, 8 * 1024> buffer;
	while (fread(buffer.data(), 1, buffer.size(), f) == buffer.size()) {}
	fclose(f);

}

#endif
