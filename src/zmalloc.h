#ifndef ZMALLOC_H
#define ZMALLOC_H
#include <stdint.h>

// given an unsigned integer, size
// returns a pointer to a place in memory with at lease size bytes
// will be 8-byte (double-word) aligned
// will return NULL if memory cannot be allocated or 0 is passed in
void* zmalloc(uint32_t size);

// deallocates memory allocated by zmalloc
void zfree(void* ptr);

// to be implemented
// void* zrealloc(uint32_t size);
// void* zcalloc((uint32_t num, uint32_t size);

#ifdef DEBUG
// prints the memory usage and mapping to stdout
void zprintMemory(void);
#endif // DEBUG

// since everything needs to be double-word aligned the ZRegion
// struct (bookkeeping information for each block) should be 8 bytes
typedef struct ZRegion {
	uint32_t free;
	uint32_t size;
} ZRegion;


#endif // ZMALLOC_H

