#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "./zmalloc.h"

#ifdef DEBUG
#include <stdio.h>
#endif // DEBUG

// set in zmalloc_init()
static ZRegion *z_head_region = NULL; // where the block of memory starts
static ZRegion *z_tail_region = NULL; // where the block of memory ends

// utility functions
static size_t zgetSize(size_t size); // add sizeof(ZRegion) and round up to nearest power of 2
static ZRegion *zfindAvailable(size_t size); // find a place in memory
static bool zmergeFree(); // merge adjacent free blocks
static ZRegion *znextRegion(ZRegion *region); // go to the next ZRegion
static ZRegion *zdivideRegion(ZRegion *region, size_t size); // divide up a region to fit size bytes

// BEGIN things that you should change to use zmalloc with
// a different device/operating system

// the total size of memory that can be allocated
// must be a power of 2
// must be larger than z_min_chunk_size
static const size_t z_memory_size = 0x100000;

// the minimum size of a chunk in bytes
// must be a power of 2 and greater than or equal to 16
// including the ZRegion struct (8  bytes)
static const size_t z_min_chunk_size = 0x20;

// initializes the z_head_region and z_tail_region
void zmalloc_init(void)
{
  z_head_region = (ZRegion *) malloc(z_memory_size);
  z_head_region->free = true;
  z_head_region->size = z_memory_size;

  z_tail_region = znextRegion(z_head_region);
  return;
}

// clean up all memory allocated above
void zmalloc_cleanup(void)
{
  free(z_head_region);
  z_head_region = NULL;
  return;
}

// END things that you should change

// given an unsigned int returns a block of memory with at least as many bytes
// modifies the global chunk of memory by dividing it into regions or merging adjacent free regions
void *zmalloc(size_t size)
{

  if (z_head_region == NULL) zmalloc_init();
  if (size == 0) return NULL;

  size_t total_size = zgetSize(size);
  ZRegion *available = zfindAvailable(total_size);

  if (available == NULL) {
    // if there is no space left then merge free blocks until all free blocks are merged
    while (zmergeFree());
    available = zfindAvailable(total_size);
  }

  if (available == NULL) return NULL;

  available->free = false;
  return (void *) (available + 1);
}

// allocates num * size bytes and initializes them to 0
// calls zmalloc then initializing every byte to 0
void *zcalloc(size_t num, size_t size)
{
  size_t num_bytes = num * size;
  void *block = zmalloc(num_bytes);

  if (block == NULL) return (void *) block;

  memset(block, 0, num_bytes);

  return block;
}

// given a pointer, frees the memory if the pointer had previously been allocated with zmalloc
void zfree(void *ptr)
{

  ZRegion *free_region = ((ZRegion *) ptr) - 1;

  if (free_region < z_head_region || free_region > (z_tail_region - 1)) {
    return;
  }

  free_region->free = true;

  return;
}

// moves the block pointed to by ptr to somewhere with at leaste size bytes
void *zrealloc(void *ptr, size_t size)
{
  ZRegion *region = ((ZRegion *) ptr) - 1;

  if (region < z_head_region || region > (z_tail_region - 1)) {
    return NULL;
  }

  if (size == 0) {
    zfree(ptr);
    return NULL;
  }


  size_t total_size = zgetSize(size);
  if (total_size <= region->size) return ptr;

  void *new_block = zmalloc(size);

  if (new_block == NULL) return NULL;

  // destination source numbytes
  memcpy(new_block, ptr, region->size - sizeof(ZRegion));
  zfree(ptr);

  return new_block;
}

// given an integer, adds the size of a region and finds the minimum power of 2 it can fit in
static size_t zgetSize(size_t size)
{
  size += sizeof(ZRegion);
  size_t total_size = z_min_chunk_size;

  while (size > total_size) total_size *= 2;

  return total_size;
}

// loops through the memory looking for a free block that matches the size passed in
// if it finds no free blocks of that size, but there is a larger free block,
// then it calls zdivideRegion which splits of the larger free block into smaller ones
// if there are no larger free blocks, returns NULL
// merges adjacent free blocks as it goes through the list
static ZRegion *zfindAvailable(size_t size)
{
  ZRegion *region = z_head_region;
  ZRegion *buddy = znextRegion(region);
  ZRegion *closest = NULL;

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
static bool zmergeFree()
{
  ZRegion *region = z_head_region;
  ZRegion *buddy = znextRegion(region);
  bool modified = false;

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

static ZRegion *znextRegion(ZRegion *region)
{
  return (ZRegion *) (((uint8_t *) region) + region->size);
}

// given a region of free memory and a size, splits it in half repeatedly until the desired size is reached
// then returns a pointer to that new free region
static ZRegion *zdivideRegion(ZRegion *region, size_t size)
{
  while (region->size > size) {
    size_t rsize = region->size / 2;
    region->size = rsize;
    region = znextRegion(region);

    region->size = rsize;
    region->free = true;
  }

  return region;
}

#ifdef DEBUG
// loops through the allocated memory and prints each block
void zprint_memory()
{
  if (z_head_region == NULL) {
    printf("No memory allocated\n");
  } else {
    ZRegion *region = z_head_region;
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
