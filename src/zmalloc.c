#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "./zmalloc.h"

#ifdef DEBUG
#include <stdio.h>
#endif // DEBUG

// set in zmallocInit()
static ZRegion* z_head_region = NULL; // where the block of memory starts
static ZRegion* z_tail_region = NULL; // where the block of memory ends

// utility functions
static uint32_t zgetSize(uint32_t size); // add sizeof(ZRegion) and round up to nearest power of 2
static ZRegion* zfindAvailable(uint32_t size); // find a place in memory
static int zmergeFree(); // merge adjacent free blocks
static ZRegion* znextRegion(ZRegion* region); // go to the next ZRegion
static ZRegion* zdivideRegion(ZRegion* region, uint32_t size); // divide up a region to fit size bytes

// BEGIN things that you should change to use zmalloc with
// a different device/operating system

// the total size of memory that can be allocated
// must be a power of 2
// must be larger than z_min_chunk_size
static const int z_memory_size = 0x100000;

// the minimum size of a chunk in bytes
// must be a power of 2 and greater than or equal to 16
// including the ZRegion struct (8  bytes)
static const int z_min_chunk_size = 0x20;

// function that initializes the memory
// right now it just uses system malloc
// to allocate a block of z_min_chunk_sizen bytes
static void zmallocInit();

// initializes the z_head_region and z_tail_region
static void zmallocInit()
{
	z_head_region = (ZRegion*) malloc(z_memory_size);
	z_head_region->free = true;
	z_head_region->size = z_memory_size;

	z_tail_region = znextRegion(z_head_region);
	return;
}

// END things that you should change

// given an unsigned int returns a block of memory with at least as many bytes
// modifies the global chunk of memory by dividing it into regions or merging adjacent free regions
void* zmalloc(uint32_t size)
{

	if (z_head_region == NULL) {
		zmallocInit();
	}

	if (size == 0) {
		return NULL;
	}

	int total_size = zgetSize(size);
	ZRegion* available = zfindAvailable(total_size);

	if (available == NULL) {
		// if there is no space left then merge free blocks until all free blocks are merged
		while (zmergeFree()) { }
		available = zfindAvailable(total_size);
	}

	if (available == NULL) {
		return NULL;
	}

	available->free = false;
	return (void*) (available + 1);
}

// allocates num * size bytes and initializes them to 0
// calls zmalloc then loops through the allocated memory, initializing everything to 0
void* zcalloc(uint32_t num, uint32_t size)
{
	uint32_t num_bytes = num * size;
	void* block = zmalloc(num_bytes);

	if (block == NULL) {
		return (void*) block;
	}

	memset(block, 0, num_bytes);

	return block;
}

// given a pointer, frees the memory if the pointer had previously been allocated with zmalloc
void zfree(void* ptr)
{

	ZRegion* free_region = ((ZRegion*) ptr) - 1;

	if (free_region < z_head_region || free_region > (z_tail_region - 1)) {
		return;
	}

	free_region->free = true;

	return;
}

// given an integer, adds the size of a region and finds the minimum power of 2 it can fit in
static uint32_t zgetSize(uint32_t size)
{
	size += sizeof(ZRegion);
	uint32_t total_size = z_min_chunk_size;

	while (size > total_size) {
		total_size *= 2;
	}
	return total_size;
}

// loops through the memory looking for a free block that matches the size passed in
// if it finds no free blocks of that size, but there is a larger free block,
// then it calls zdivideRegion which splits of the larger free block into smaller ones
// if there are no larger free blocks, returns NULL
// merges adjacent free blocks as it goes through the list
static ZRegion* zfindAvailable(uint32_t size)
{
	ZRegion* region = z_head_region;
	ZRegion* buddy = znextRegion(region);
	ZRegion* closest = NULL;

//	while (region < z_tail_region && closest == NULL) {
//		if (region->free && size <= region->size && (closest == NULL || region->size <= closest->size)) {
//			closest = region;
//		}
//		region = znextRegion(region);
//	}

	// if there is only one block of memory divide it up
	if (buddy == z_tail_region && region->free) {
		return zdivideRegion(region, size);
	}

	// find the minimum-sized match within the area of memory available
	// merge along the way
	while (region < z_tail_region && buddy < z_tail_region) {

		if (region->free && buddy->free && region->size == buddy->size) { // both region and buddy are free
			region->size *= 2;

			if (region->free && size <= region->size && (closest == NULL || region->size <= closest->size)) {
				closest = region;
			}

			region = znextRegion(buddy);
			if (region < z_tail_region) {
				buddy = znextRegion(region);
			}

		} else {

			if (region->free && size <= region->size && (closest == NULL || region->size <= closest->size)) {
				closest = region;
			}
			if (buddy->free && size <= buddy->size && (closest == NULL || buddy->size <= closest->size)) {
				closest = buddy;
			}

			if (region->size > buddy->size) { // case where buddy has been split up into smaller chunks
				region = buddy;
				buddy = znextRegion(buddy);
			} else { // otherwise jump ahead two regions
				region = znextRegion(buddy);
				if (region < z_tail_region) {
					buddy = znextRegion(region);
				}
			}
		}
	}

	if (closest == NULL || closest->size == size) {
		return closest;
	}

	return zdivideRegion(closest, size); // divide up closest
}

// does a single level merge of adjacent free memory blocks
static int zmergeFree()
{
	ZRegion* region = z_head_region;
	ZRegion* buddy = znextRegion(region);
	int modified = false;

	while (region < z_tail_region && buddy < z_tail_region) {
		if (region->free && buddy->free && region->size == buddy->size) { // both region and buddy are free
			region->size *= 2;
			region = znextRegion(buddy);
			if (region < z_tail_region) {
				buddy = znextRegion(region);
			}
			modified = true;
		} else if (region->size > buddy->size) { // case where buddy has been split up into smaller chunks
			region = buddy;
			buddy = znextRegion(buddy);
		} else { // otherwise jump ahead two regions
			region = znextRegion(buddy);
			if (region < z_tail_region) {
				buddy = znextRegion(region);
			}
		}
	}

	return modified;
}

static ZRegion* znextRegion(ZRegion* region)
{
	return (ZRegion*) (((uint8_t*) region) + region->size);
}

// given a region of free memory and a size, splits it in half repeatedly until the desired size is reached
// then returns a pointer to that new free region
static ZRegion* zdivideRegion(ZRegion* region, uint32_t size)
{
	while (region->size > size) {
		int rsize = region->size / 2;
		region->size = rsize;
		region = znextRegion(region);

		region->size = rsize;
		region->free = true;
	}

	return region;
}

#ifdef DEBUG
// loops through the allocated memory and prints each block
void zprintMemory()
{
	if (z_head_region == NULL) {
		printf("No memory allocated\n");
	} else {

		ZRegion* region = z_head_region;
		while (region < z_tail_region) {
			if (region->free) {
				printf("Free (%p) [ size: 0x%08x ]\n", region, region->size);
			} else {
				printf("Used (%p) [ size: 0x%08x ]\n", region, region->size);
			}
			region = znextRegion(region);
		}
	}

	return;
}
#endif // DEBUG
